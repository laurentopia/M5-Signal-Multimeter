// 4/3/2018 FFT
// 4/9/2018 Oscillo
// 4/9/2018 Hz
// 4/11/2018 no more core 1, moved oscillo, fft to the main loop, back to drawline

// TODO: fix the weird retults, FFT access wrong?
// TODO: fix the oscillo draw which is a bit on the sloooooooowwwwwwwww siiiiiiiiide

#include <M5Stack.h>
#include <arduinoFFT.h>
arduinoFFT FFT = arduinoFFT();

/////////////////////////////////////////////////////////////////////////
#define SAMPLES 512              // Must be a power of 2
#define SAMPLING_FREQUENCY 500 // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define amplitude 200            // Depending on your audio source level, you may need to increase this value
unsigned int sampling_period_us;
unsigned long microseconds;
//byte peak[8] = { 0 };
double vSample[SAMPLES];
double oscilloBuffer[SAMPLES];
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

void DrawOscillo()
{
	//copy ADC samples to oscillo buffer
	for (int i = 0; i < SAMPLES; i++)
		oscilloBuffer[i] = vSample[i];

	for (int i = 0; i < SAMPLES; i++)
	{
		if (i == 0)
		{
			M5.Lcd.drawPixel(i, OSCILLO_Y - oldOscilloBuffer[i] * OSCILLO_YSCALE, BLACK);  //clear
			M5.Lcd.drawPixel(i, OSCILLO_Y - oscilloBuffer[i] * OSCILLO_YSCALE, RED);
		}
		else
		{
			M5.Lcd.drawLine(i - 1, OSCILLO_Y - oldOscilloBuffer[i - 1] * OSCILLO_YSCALE, i, OSCILLO_Y - oldOscilloBuffer[i] * OSCILLO_YSCALE, BLACK); //clear
			M5.Lcd.drawLine(i - 1, OSCILLO_Y - oscilloBuffer[i - 1] * OSCILLO_YSCALE, i, OSCILLO_Y - oscilloBuffer[i] * OSCILLO_YSCALE, RED);
			//M5.Lcd.drawFastVLine (i, OSCILLO_Y - min(oldOscilloBuffer[i - 1], oldOscilloBuffer[i]) * OSCILLO_YSCALE, max(1,abs(oldOscilloBuffer[i]- oldOscilloBuffer[i-1])) * OSCILLO_YSCALE, BLACK);
			//M5.Lcd.drawFastVLine(i, OSCILLO_Y - min(oscilloBuffer[i - 1], oscilloBuffer[i]) * OSCILLO_YSCALE, max(1,abs(oscilloBuffer[i] - oscilloBuffer[i - 1])) * OSCILLO_YSCALE, RED);
		}
	}

	//copy to back buffer
	for (int i = 0; i < SAMPLES; i++)
		oldOscilloBuffer[i] = oscilloBuffer[i];
}

void CaptureOscilloFFT()
{
	//sampling & oscilloscope
	for (int i = 0; i < SAMPLES; i++)
	{
		newTime = micros() - oldTime;
		oldTime = newTime;
		vSample[i] = analogRead(35); // A conversion takes about 1uS on an ESP32
		vReal[i] = vSample[i];
		vImag[i] = 0;
		while (micros() < (newTime + sampling_period_us)) 
		{
			//wait
		}
	}

	//FFT
	FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_BLACKMAN, FFT_FORWARD);
	FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
	FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

	//display spectral bands
	for (int i = 1; i < SAMPLES / 2; i++)
	{
		//displayValues[i] = vReal[i];
		if (i == SAMPLES / 2)
			displayBand(i, (int)vReal[i] * FFTDisplayScale / amplitude, String(i*SAMPLING_FREQUENCY / SAMPLES));
		else
			displayBand(i, (int)vReal[i] * FFTDisplayScale / amplitude, "");
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
		M5.Lcd.fillRect(10, MAX_BAND_HEIGHT + 10, 70, MAX_BAND_HEIGHT+20, BLACK);
		M5.Lcd.drawString(majorPeakFrequency + " Hz", 10, MAX_BAND_HEIGHT+10);
		M5.Lcd.fillRect(100, MAX_BAND_HEIGHT+10, 170, MAX_BAND_HEIGHT+20, BLACK);
		M5.Lcd.drawString("THD " + THD + "%", 100, MAX_BAND_HEIGHT+10);
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

void task1(void * pvParameters)
{
	for (;;)
	{
		CaptureOscilloFFT();
	}
}

void task2(void * pvParameters)
{
	for (;;)
	{
		DrawOscillo();
	}
}

void setup()
{
	M5.begin();

	//speaker shhhhh
	dacWrite(25, 0);

	sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
	vSample[SAMPLES] = { 0 };
	vSample[SAMPLES] = { 0 };
	oscilloBuffer[SAMPLES] = { 0 };
	oldOscilloBuffer[SAMPLES] = { 0 };
	vReal[SAMPLES] = { 0 };
	vOldSample[SAMPLES] = { 0 };
	vImag[SAMPLES] = { 0 };
	FFTDisplayScale = 0.3;

	//launch tasks
	// (task function, name, stack size (W), task parameters, priority, task handle, core#) 
	xTaskCreatePinnedToCore(task1,"AD sampling and FFT",8192,NULL,1,NULL,1);
	//xTaskCreatePinnedToCore(task2,"draw oscillo", 8192,NULL,2,NULL,0);
}

void loop()
{
	//CaptureOscilloFFT();

	//Input();
	
	DrawOscillo();

	// necessary when nothing is in loop and one core has a task running
	//delay(500);
	//M5.update();
}

void displayBand(int band, int dsize, const String& string)
{
	if (string != "")
		M5.Lcd.drawString(string, 18 * band, 70);
	dsize = min(dsize, MAX_BAND_HEIGHT);
	M5.Lcd.drawFastVLine(band, 0, MAX_BAND_HEIGHT, BLACK);
	M5.Lcd.drawFastVLine(band, MAX_BAND_HEIGHT - dsize, dsize, GREEN);
}
