// 4/3/2018 FFT
// 4/9/2018 Oscillo
// 4/9/2018 Hz
// 4/11/2018 moved oscillo, fft to the main loop

// TODO: fix the weird retults, FFT access wrong?

#include <M5Stack.h>
#include <arduinoFFT.h>
//#include <TFT_eSPI.h>
// Use hardware SPI
//TFT_eSPI tft = TFT_eSPI();
arduinoFFT FFT = arduinoFFT();

/////////////////////////////////////////////////////////////////////////
#define SAMPLES 512              // Must be a power of 2
#define SAMPLING_FREQUENCY 480 // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define amplitude 200            // Depending on your audio source level, you may need to increase this value
unsigned int sampling_period_us;
unsigned long microseconds;
//byte peak[8] = { 0 };
double vSample[SAMPLES];
double vReal[SAMPLES];
double vOldReal[SAMPLES];
double vImag[SAMPLES];
double displayValues[SAMPLES];
unsigned long newTime, oldTime;
/////////////////////////////////////////////////////////////////////////

float FFTDisplayScale;

String majorPeakFrequency = "";
String THD = "";
unsigned long majorPeakTime;


void task1(void * pvParameters)
{
	for (;;)
	{
		//sampling & oscilloscope
		for (int i = 0; i < SAMPLES; i++)
		{
			newTime = micros() - oldTime;
			oldTime = newTime;
			vSample[i] = analogRead(35); // A conversion takes about 1uS on an ESP32
			vReal[i] = vSample[i];
			vImag[i] = 0;
			M5.Lcd.drawPixel(i, 128 - vOldReal[i] * .01, BLACK);
			M5.Lcd.drawPixel(i, 128 - vReal[i] * .01, RED);
			//M5.Lcd.drawPixel(i, 128 - vOldReal[i] * .01, BLACK);
			//M5.Lcd.drawPixel(i, 128 - vReal[i] * .01, RED);
			//if (i == 0)
			//{
			//	M5.Lcd.drawPixel(i, 128 - vOldReal[i] * .01, BLACK);
			//	M5.Lcd.drawPixel(i, 128 - vReal[i] * .01, RED);
			//}
			//else
			//{
			//	//M5.Lcd.drawLine(i - 1, 128 - vOldReal[i - 1] * .01, i, 128 - vOldReal[i] * .01, BLACK);
			//	//M5.Lcd.drawLine(i-1, 128 - vReal[i-1] * .01, i, 128 - vReal[i] * .01, RED);
			//	M5.Lcd.drawFastVLine (i, 128 - min(vOldReal[i - 1], vOldReal[i]) * .01, max(1,abs(vOldReal[i]- vOldReal[i-1])) * .01, BLACK);
			//	M5.Lcd.drawFastVLine(i, 128 - min(vReal[i - 1], vReal[i]) * .01, max(1,abs(vReal[i] - vReal[i - 1])) * .01, RED);
			//}
			while (micros() < (newTime + sampling_period_us)) { /* do nothing to wait */ }
		}
		//back buffer
		for (int i = 0; i < SAMPLES; i++)
			vOldReal[i] = vReal[i];


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
			M5.Lcd.fillRect(10, 150, 70, 160, BLACK);
			M5.Lcd.drawString(majorPeakFrequency + " Hz", 10, 150);
			M5.Lcd.fillRect(100, 150, 170, 160, BLACK);
			M5.Lcd.drawString("THD " + THD + "%", 100, 150);
		}
	}
}

void setup()
{
	FFTDisplayScale = 0.3;
	M5.begin();
	Serial.begin(115200);
	dacWrite(25, 0);
	//M5.Speaker.setVolume(0);
	//M5.Speaker.update();
	sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
	displayValues[SAMPLES] = { 0 };
//	xTaskCreatePinnedToCore(
//		task1,     /* Function to implement the task */
//		"AD sampling and FFT",   /* Name of the task */
//		8192,      /* Stack size in words */
//		NULL,      /* Task input parameter */
//		1,         /* Priority of the task */
//		NULL,      /* Task handle. */
//		1);        /* Core where the task should run */
}

String oldFFTDisplayScale;
void loop()
{
  //sampling & oscilloscope
    for (int i = 0; i < SAMPLES; i++)
    {
      newTime = micros() - oldTime;
      oldTime = newTime;
      vSample[i] = analogRead(35); // A conversion takes about 1uS on an ESP32
      vReal[i] = vSample[i];
      vImag[i] = 0;
      //M5.Lcd.drawPixel(i, 128 - vOldReal[i] * .01, BLACK);
      //M5.Lcd.drawPixel(i, 128 - vReal[i] * .01, RED);
      M5.Lcd.drawPixel(i, 128 - vOldReal[i] * .01, BLACK);
      M5.Lcd.drawPixel(i, 128 - vReal[i] * .01, RED);
      if (i == 0)
      {
        M5.Lcd.drawPixel(i, 128 - vOldReal[i] * .01, BLACK);
        M5.Lcd.drawPixel(i, 128 - vReal[i] * .01, RED);
      }
      else
      {
        M5.Lcd.drawLine(i - 1, 128 - vOldReal[i - 1] * .01, i, 128 - vOldReal[i] * .01, BLACK);
        M5.Lcd.drawLine(i-1, 128 - vReal[i-1] * .01, i, 128 - vReal[i] * .01, RED);
        //M5.Lcd.drawFastVLine (i, 128 - min(vOldReal[i - 1], vOldReal[i]) * .01, max(1,abs(vOldReal[i]- vOldReal[i-1])) * .01, BLACK);
        //M5.Lcd.drawFastVLine(i, 128 - min(vReal[i - 1], vReal[i]) * .01, max(1,abs(vReal[i] - vReal[i - 1])) * .01, RED);
      }
      while (micros() < (newTime + sampling_period_us)) { /* do nothing to wait */ }
    }
    //back buffer
    for (int i = 0; i < SAMPLES; i++)
      vOldReal[i] = vReal[i];


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

	  M5.Lcd.fillRect(10, 150, 70, 160, BLACK);
	  M5.Lcd.drawString(majorPeakFrequency + " Hz", 10, 150);
	  M5.Lcd.fillRect(100, 150, 170, 160, BLACK);
	  M5.Lcd.drawString("THD " + THD + "%", 100, 150);
	}

	//input
	//if (M5.BtnA.wasPressed() == true)
	//{
	//	FFTDisplayScale -= .3;
	//	FFTDisplayScale = max(FFTDisplayScale, 0.1);
	//}
	//if (M5.BtnC.wasPressed() == true)
	//{
	//	FFTDisplayScale += .3;
	//	FFTDisplayScale = min(FFTDisplayScale, 3);
	//}
	//M5.Lcd.drawString(oldFFTDisplayScale, 160, 230, BLACK);
	//oldFFTDisplayScale = String(FFTDisplayScale);
	//M5.Lcd.drawString(oldFFTDisplayScale, 160, 230, WHITE);
	//delay(500);
	//M5.update();
}

#define MAX_BAND_HEIGHT 64
void displayBand(int band, int dsize, const String& string)
{
	if (string != "")
		M5.Lcd.drawString(string, 18 * band, 70);
	dsize = min(dsize, MAX_BAND_HEIGHT);
	M5.Lcd.drawFastVLine(band, 64 - MAX_BAND_HEIGHT, MAX_BAND_HEIGHT, BLACK);
	M5.Lcd.drawFastVLine(band, 64 - dsize, dsize, GREEN);
}
