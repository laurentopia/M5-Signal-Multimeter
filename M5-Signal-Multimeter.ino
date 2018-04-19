// 4/3/2018 FFT
// 4/9/2018 Oscillo
// 4/9/2018 Hz
// 4/11/2018 no more core 1, moved oscillo, fft to the main loop, back to drawline
// 4/12/2018 moved everything in the main loop
// 4/15/2018 split draw and capture, added semaphores (doesn't work)
// 4/19/2018 back to main loop, split FFT() and drawFFT(), increased sampling to 20kHz for sound, 

// TODO: fix the weird retults, FFT access wrong?
// TODO: fix the oscillo draw which is a bit on the sloooooooowwwwwwwww siiiiiiiiide

#include <M5Stack.h>
#include <arduinoFFT.h>
arduinoFFT FFT = arduinoFFT();

/////////////////////////////////////////////////////////////////////////
#define SAMPLES 512			   // Must be a power of 2
#define SAMPLING_FREQUENCY 20000 // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define SIGNAL_AMPLITUDE 200   // Depending on your audio source level, you may need to increase this value
unsigned int sampling_period_us;
unsigned long microseconds;
//byte peak[8] = { 0 };
double vSample[SAMPLES] = { 0 };
double buffer[SAMPLES] = { 0 };
double oldOscilloBuffer[SAMPLES] = { 0 };
double vOldSample[SAMPLES] = { 0 };
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
	{
		M5.Lcd.setTextColor(GREEN);
		M5.Lcd.drawString(string, band, MAX_BAND_HEIGHT);
	}
	dsize = min(dsize, MAX_BAND_HEIGHT);
	M5.Lcd.drawFastVLine(band, 0, MAX_BAND_HEIGHT, BLACK);
	M5.Lcd.drawFastVLine(band, MAX_BAND_HEIGHT - dsize, dsize, GREEN);
}

SemaphoreHandle_t battonEndCapture, battonEndDraw, battonEndDrawOscillo;

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
			//wait for next sample time
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
		//if (i == 0)
		//{
			M5.Lcd.drawPixel(i, OSCILLO_Y - oldOscilloBuffer[i] * OSCILLO_YSCALE, BLACK); //clear
			M5.Lcd.drawPixel(i, OSCILLO_Y - buffer[i] * OSCILLO_YSCALE, RED);
		//}
		//else
		//{
		//	M5.Lcd.drawLine(i - 1, OSCILLO_Y - oldOscilloBuffer[i - 1] * OSCILLO_YSCALE, i, OSCILLO_Y - oldOscilloBuffer[i] * OSCILLO_YSCALE, BLACK); //clear
		//	M5.Lcd.drawLine(i - 1, OSCILLO_Y - buffer[i - 1] * OSCILLO_YSCALE, i, OSCILLO_Y - buffer[i] * OSCILLO_YSCALE, RED);
			//M5.Lcd.drawFastVLine (i, OSCILLO_Y - min(oldOscilloBuffer[i - 1], oldOscilloBuffer[i]) * OSCILLO_YSCALE, max(1,abs(oldOscilloBuffer[i]- oldOscilloBuffer[i-1])) * OSCILLO_YSCALE, BLACK);
			//M5.Lcd.drawFastVLine(i, OSCILLO_Y - min(buffer[i - 1], buffer[i]) * OSCILLO_YSCALE, max(1,abs(buffer[i] - buffer[i - 1])) * OSCILLO_YSCALE, RED);
		//}
	}

	//copy to back buffer
	for (int i = 0; i < SAMPLES; i++)
		oldOscilloBuffer[i] = buffer[i];
}

double FFTreal[SAMPLES] = { 0 };
double FFTimaginary[SAMPLES] = { 0 };
double FFTmagnitude[SAMPLES/2] = { 0 };
void ComputeFFT()
{
	//copy to fft buffer
	for (int i = 0; i < SAMPLES; i++)
	{
		FFTreal[i] = vSample[i];
		FFTimaginary[i] = 0;
	}

	//FFT
	FFT.Windowing(FFTreal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
	FFT.Compute(FFTreal, FFTimaginary, SAMPLES, FFT_FORWARD);
	FFT.ComplexToMagnitude(FFTreal, FFTimaginary, SAMPLES);

	//copy to fft magnitude
	for (int i = 0; i < SAMPLES / 2; i++)
	{
		FFTmagnitude[i] = FFTreal[i];
	}
}

double FFTDisplayBuffer[SAMPLES] = { 0 };
void DrawFFT()
{
	// copy to display buffer
	//copy to fft magnitude
	for (int i = 0; i < SAMPLES; i++)
	{
		FFTDisplayBuffer[i] = FFTmagnitude[i];
	}


	//display spectral bands
	for (int i = 1; i < SAMPLES/2; i++)
	{
		int amplitude = (int)FFTDisplayBuffer[i] * FFTDisplayScale / SIGNAL_AMPLITUDE;
		//displayValues[i] = vReal[i];
		if (i == SAMPLES / 4)
			displayBand(i, amplitude, String(i * 2 * SAMPLING_FREQUENCY / SAMPLES));
		else
			displayBand(i, amplitude, "");
	}

	//calculate peak and THD
	if (millis() > majorPeakTime)
	{
		majorPeakTime = millis() + 500;
		majorPeakFrequency = String(FFT.MajorPeak(FFTDisplayBuffer, SAMPLES, SAMPLING_FREQUENCY), 1);
		//THD
		double totalTHD = 0;
		double maxPowerForTHD = 0;
		for (int i = 1; i < SAMPLES / 2; i++)
		{
			totalTHD += sq(FFTreal[i]);
			if (FFTreal[i] > maxPowerForTHD)
				maxPowerForTHD = FFTDisplayBuffer[i];
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
		xSemaphoreTake(battonEndDraw, portMAX_DELAY);
		xSemaphoreTake(battonEndDrawOscillo, portMAX_DELAY);
		Capture();
		xSemaphoreGive(battonEndCapture);

	}
}

void TaskDraw(void *pvParameters)
{
	for (;;)
	{
		xSemaphoreTake(battonEndCapture, portMAX_DELAY);
		DrawFFT();
		DrawOscillo();
		xSemaphoreGive(battonEndDraw);
	}
}

File file;
#define PATH "/hello.txt"

void setup()
{
	M5.begin();
	Serial.begin(115200);

	//speaker shhhhh
	dacWrite(25, 0);

	sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
	FFTDisplayScale = 0.3;

	file = SD.open(PATH, FILE_WRITE);

	//launch tasks parameters:  (task function, name, stack size (W), task parameters, priority, task handle, core#)
	//xTaskCreatePinnedToCore(taskCapture, "AD sampling", 8192, NULL, 1, NULL, 0);
	//xTaskCreatePinnedToCore(TaskDraw, "draw FFT", 8192, NULL, 2, NULL, 0);
}

long int oldMillis;
void loop()
{
	//Input();
	oldMillis = millis();
	Capture();
	ComputeFFT();
	DrawFFT();
	DrawOscillo();
	Serial.println(millis() - oldMillis);

	// necessary when nothing is in loop and one core has a task running
	//delay(500);
	//M5.update();
}
