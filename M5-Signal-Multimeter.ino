// 4/3/2018 FFT
// 4/9/2018 Oscillo
// 4/9/2018 Hz
// 4/11/2018 no more core 1, moved oscillo, fft to the main loop, back to drawline
// 4/12/2018 moved everything in the main loop
// 4/15/2018 split draw and capture, added semaphores (doesn't work)

// TODO: fix the weird retults, FFT access wrong?
// TODO: fix the oscillo draw which is a bit on the sloooooooowwwwwwwww siiiiiiiiide

#include <M5Stack.h>
#include <arduinoFFT.h>
arduinoFFT FFT = arduinoFFT();

/////////////////////////////////////////////////////////////////////////
#define SAMPLES 512			   // Must be a power of 2
#define SAMPLING_FREQUENCY 500 // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define SIGNAL_AMPLITUDE 200   // Depending on your audio source level, you may need to increase this value
unsigned int sampling_period_us;
unsigned long microseconds;
//byte peak[8] = { 0 };
double vSample[SAMPLES];
double buffer[SAMPLES];
double oldOscilloBuffer[SAMPLES];
double vReal[SAMPLES];
double vOldSample[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime, oldTime;
/////////////////////////////////////////////////////////////////////////
#define MAX_BAND_HEIGHT 100
#define OSCILLO_Y 200
#define OSCILLO_YSCALE 0.015

float FFTDisplayScale;

String majorPeakFrequency = "";
String THD = "";
unsigned long majorPeakTime;
String oldFFTDisplayScale;

void displayBand(int band, int dsize, const String &string)
{
	if (string != "")
		M5.Lcd.drawString(string, 18 * band, 70);
	dsize = min(dsize, MAX_BAND_HEIGHT);
	M5.Lcd.drawFastVLine(band, 0, MAX_BAND_HEIGHT, BLACK);
	M5.Lcd.drawFastVLine(band, MAX_BAND_HEIGHT - dsize, dsize, GREEN);
}

SemaphoreHandle_t battonEndCapture, battonEndDrawFFT, battonEndDrawOscillo;

void Capture()
{
	//capturee
	for (int i = 0; i < SAMPLES; i++)
	{
		newTime = micros() - oldTime;
		oldTime = newTime;
		vSample[i] = analogRead(35); // A conversion takes about 1uS on an ESP32
		while (micros() < (newTime + sampling_period_us))
		{
			//wait
		}
	}
}

void DrawOscillo()
{
	//copy ADC samples to buffer
	for (int i = 0; i < SAMPLES; i++)
		buffer[i] = vSample[i];

	for (int i = 0; i < SAMPLES; i++)
	{
		if (i == 0)
		{
			M5.Lcd.drawPixel(i, OSCILLO_Y - oldOscilloBuffer[i] * OSCILLO_YSCALE, BLACK); //clear
			M5.Lcd.drawPixel(i, OSCILLO_Y - buffer[i] * OSCILLO_YSCALE, RED);
		}
		else
		{
			M5.Lcd.drawLine(i - 1, OSCILLO_Y - oldOscilloBuffer[i - 1] * OSCILLO_YSCALE, i, OSCILLO_Y - oldOscilloBuffer[i] * OSCILLO_YSCALE, BLACK); //clear
			M5.Lcd.drawLine(i - 1, OSCILLO_Y - buffer[i - 1] * OSCILLO_YSCALE, i, OSCILLO_Y - buffer[i] * OSCILLO_YSCALE, RED);
			//M5.Lcd.drawFastVLine (i, OSCILLO_Y - min(oldOscilloBuffer[i - 1], oldOscilloBuffer[i]) * OSCILLO_YSCALE, max(1,abs(oldOscilloBuffer[i]- oldOscilloBuffer[i-1])) * OSCILLO_YSCALE, BLACK);
			//M5.Lcd.drawFastVLine(i, OSCILLO_Y - min(buffer[i - 1], buffer[i]) * OSCILLO_YSCALE, max(1,abs(buffer[i] - buffer[i - 1])) * OSCILLO_YSCALE, RED);
		}
	}

	//copy to back buffer
	for (int i = 0; i < SAMPLES; i++)
		oldOscilloBuffer[i] = buffer[i];
}

void DrawFFT()
{
	//copy to fft buffer
	for (int i = 0; i < SAMPLES; i++)
	{
		vReal[i] = vSample[i];
		vImag[i] = 0;
	}

	//FFT
	FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_BLACKMAN, FFT_FORWARD);
	FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
	FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

	//display spectral bands
	for (int i = 1; i < SAMPLES / 2; i++)
	{
		int amplitude = (int)vReal[i] * FFTDisplayScale / SIGNAL_AMPLITUDE;
		//displayValues[i] = vReal[i];
		if (i == SAMPLES / 2)
			displayBand(i, amplitude, String(i * SAMPLING_FREQUENCY / SAMPLES));
		else
			displayBand(i, amplitude, "");
	}

	//calculate peak and THD
	if (millis() > majorPeakTime)
	{
		majorPeakTime = millis() + 500;
		majorPeakFrequency = String(FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY), 1);
		//THD
		double totalTHD = 0;
		double maxPowerForTHD = 0;
		for (int i = 1; i < SAMPLES / 2; i++)
		{
			totalTHD += sq(vReal[i]);
			if (vReal[i] > maxPowerForTHD)
				maxPowerForTHD = vReal[i];
		}
		totalTHD -= sq(maxPowerForTHD);
		totalTHD = sqrt(totalTHD) / maxPowerForTHD;
		totalTHD *= 100;
		THD = String(totalTHD, 1);
		M5.Lcd.fillRect(10, MAX_BAND_HEIGHT + 10, 70, MAX_BAND_HEIGHT + 20, BLACK);
		M5.Lcd.drawString(majorPeakFrequency + " Hz", 10, MAX_BAND_HEIGHT + 10);
		M5.Lcd.fillRect(100, MAX_BAND_HEIGHT + 10, 170, MAX_BAND_HEIGHT + 20, BLACK);
		M5.Lcd.drawString("THD " + THD + "%", 100, MAX_BAND_HEIGHT + 10);
	}
}

/// M5 button input change scale etc...
void Input()
{
	if (M5.BtnA.wasPressed() == true)
	{
		FFTDisplayScale -= .3;
		FFTDisplayScale = max(FFTDisplayScale, 0.1);
	}
	if (M5.BtnC.wasPressed() == true)
	{
		FFTDisplayScale += .3;
		FFTDisplayScale = min(FFTDisplayScale, 3);
	}
	M5.Lcd.drawString(oldFFTDisplayScale, 160, 230, BLACK);
	oldFFTDisplayScale = String(FFTDisplayScale);
	M5.Lcd.drawString(oldFFTDisplayScale, 160, 230, WHITE);
}

void taskCapture(void *pvParameters)
{
	for (;;)
	{
		xSemaphoreTake(battonEndDrawFFT, portMAX_DELAY);
		xSemaphoreTake(battonEndDrawOscillo, portMAX_DELAY);
		Capture();
		xSemaphoreGive(battonEndCapture);

	}
}

void taskDrawFFT(void *pvParameters)
{
	for (;;)
	{
		xSemaphoreTake(battonEndCapture, portMAX_DELAY);
		DrawFFT();
		xSemaphoreGive(battonEndDrawFFT);
	}
}

void taskDrawOscillo(void *pvParameters)
{
	for (;;)
	{
		xSemaphoreTake(battonEndCapture, portMAX_DELAY);
		DrawOscillo();
		xSemaphoreGive(battonEndDrawOscillo);
	}
}



File file;
#define PATH "/hello.txt"

void setup()
{
	M5.begin();

	//speaker shhhhh
	dacWrite(25, 0);

	sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
	vSample[SAMPLES] = { 0 };
	vSample[SAMPLES] = { 0 };
	buffer[SAMPLES] = { 0 };
	oldOscilloBuffer[SAMPLES] = { 0 };
	vReal[SAMPLES] = { 0 };
	vOldSample[SAMPLES] = { 0 };
	vImag[SAMPLES] = { 0 };
	FFTDisplayScale = 0.3;

	file = SD.open(PATH, FILE_WRITE);

	//launch tasks parameters:  (task function, name, stack size (W), task parameters, priority, task handle, core#)
	//xTaskCreatePinnedToCore(taskCapture, "AD sampling", 8192, NULL, 1, NULL, 0);
	//xTaskCreatePinnedToCore(taskDrawFFT, "draw FFT", 8192, NULL, 2, NULL, 0);
	//xTaskCreatePinnedToCore(taskDrawOscillo,"draw Oscillo", 8192,NULL,2,NULL,1);
}

void loop()
{
	//nput();
	Capture();
	DrawFFT();
	DrawOscillo();

	// necessary when nothing is in loop and one core has a task running
	//delay(500);
	M5.update();
}