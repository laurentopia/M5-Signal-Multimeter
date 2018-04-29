// 4/3/2018 FFT
// 4/9/2018 Oscillo
// 4/9/2018 Hz
// 4/11/2018 no more core 1, moved oscillo, fft to the main loop, back to drawline
// 4/12/2018 moved everything in the main loop
// 4/15/2018 split draw and capture, added semaphores (doesn't work)
// 4/19/2018 back to main loop, split FFT() and drawFFT(), increased sampling to 20kHz for sound,
//4/2/2018 waterfall, sprite, using TFT_eSPI, no more m5 but... capture is anemic.... why??


// TODO: fix the weird retults, FFT access wrong?
// TODO: fix the oscillo draw which is a bit on the sloooooooowwwwwwwww siiiiiiiiide

#include <dummy.h>
//#include <M5Stack.h>
#include <arduinoFFT.h>
arduinoFFT FFT = arduinoFFT();

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spectrum = TFT_eSprite(&tft); // Sprite object spectrograph
TFT_eSprite waterfall = TFT_eSprite(&tft); // Sprite object waterfall
TFT_eSprite oscillo = TFT_eSprite(&tft); // Sprite object waterfall
const unsigned int GRAPHWIDTH = 256;
const unsigned int SPECTRUMHEIGHT = 40;
const unsigned int WATERFALLHEIGHT = 100;
const unsigned int OSCILLOHEIGHT = 80;


/////////////////////////////////////////////////////////////////////////
const unsigned int SAMPLES = 512;				  // Must be a power of 2;
const unsigned long SAMPLING_FREQUENCY = 40000UL; // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
const unsigned int SIGNAL_AMPLITUDE = 200;		  // Depending on your audio source level, you may need to increase this value;
unsigned int sampling_period_ms = 0;
unsigned long long microseconds = 0;
//byte peak[8] = { 0 };
double vSample[SAMPLES] = { 0 };
double buffer[SAMPLES] = { 0 };
double oldOscilloBuffer[SAMPLES] = { 0 };
double vOldSample[SAMPLES] = { 0 };
unsigned long long newTime = 0, oldTime = 0;
/////////////////////////////////////////////////////////////////////////
const unsigned int OSCILLO_Y = 160;
float OSCILLO_YSCALE = 0.015f;
float FFTDisplayScale = 0;

String majorPeakFrequency = "";
String THD = "";
unsigned long majorPeakTime = 0;
String oldFFTDisplayScale = "";

bool getHeatMapColor(float value, uint8_t *red, uint8_t *green, uint8_t *blue)
{
	const int NUM_COLORS = 3;
	static float color[NUM_COLORS][3] = { { 50,0,100 },{ 255,0,0 } ,{ 255,255,0 } };

	int idx1;        // |-- Our desired color will be between these two indexes in "color".
	int idx2;        // |
	float fractBetween = 0;  // Fraction between "idx1" and "idx2" where our value is.

	if (value <= 0) { idx1 = idx2 = 0; }    // accounts for an input <=0
	else if (value >= 1) { idx1 = idx2 = NUM_COLORS - 1; }    // accounts for an input >=0
	else
	{
		value = value * (NUM_COLORS - 1);        // Will multiply value by 3.
		idx1 = floor(value);                  // Our desired color will be after this index.
		idx2 = idx1 + 1;                        // ... and before this index (inclusive).
		fractBetween = value - float(idx1);    // Distance between the two indexes (0-1).
	}

	*red = (color[idx2][0] - color[idx1][0])*fractBetween + color[idx1][0];
	*green = (color[idx2][1] - color[idx1][1])*fractBetween + color[idx1][1];
	*blue = (color[idx2][2] - color[idx1][2])*fractBetween + color[idx1][2];
}

#include <driver/adc.h>
void Capture()
{
	//capturee
	for (int i = 0; i < SAMPLES; i++)
	{
		newTime = micros() - oldTime;
		oldTime = newTime;
		vSample[i] = analogRead(35); // A conversion takes about 1uS on an ESP32
		//vSample[i] = adc1_get_raw(ADC1_CHANNEL_7);
		vTaskDelay(sampling_period_ms / portTICK_PERIOD_MS);
	}
}

void DrawOscillo()
{
	//copy ADC samples to buffer
	for (int i = 0; i < SAMPLES; i++)
		buffer[i] = vSample[i];

	oscillo.fillSprite(TFT_BLACK);
	for (int i = 0; i < min(GRAPHWIDTH,SAMPLES); i++)
	{
		//if (i == 0)
		//{
		oscillo.drawPixel(i, buffer[i] * OSCILLO_YSCALE, TFT_ORANGE);

		//spectrum.drawLine(i - 1, buffer[i - 1] * OSCILLO_YSCALE, i, buffer[i] * OSCILLO_YSCALE, TFT_RED);
		//}
		//else
		//{
			//M5.Lcd.drawLine(i - 1, OSCILLO_Y - oldOscilloBuffer[i - 1] * OSCILLO_YSCALE, i, OSCILLO_Y - oldOscilloBuffer[i] * OSCILLO_YSCALE, BLACK); //clear
			//M5.Lcd.drawLine(i - 1, OSCILLO_Y - buffer[i - 1] * OSCILLO_YSCALE, i, OSCILLO_Y - buffer[i] * OSCILLO_YSCALE, RED);
		//M5.Lcd.drawFastVLine (i, OSCILLO_Y - min(oldOscilloBuffer[i - 1], oldOscilloBuffer[i]) * OSCILLO_YSCALE, max(1,abs(oldOscilloBuffer[i]- oldOscilloBuffer[i-1])) * OSCILLO_YSCALE, BLACK);
		//M5.Lcd.drawFastVLine(i, OSCILLO_Y - min(buffer[i - 1], buffer[i]) * OSCILLO_YSCALE, max(1,abs(buffer[i] - buffer[i - 1])) * OSCILLO_YSCALE, RED);
		//}
	}

	oscillo.pushSprite(0,OSCILLO_Y);


	//copy to back buffer
	for (int i = 0; i < SAMPLES; i++)
		oldOscilloBuffer[i] = buffer[i];
}

double FFTreal[SAMPLES] = { 0 };
double FFTimaginary[SAMPLES] = { 0 };
double FFTmagnitude[SAMPLES] = { 0 };
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
	for (int i = 0; i < SAMPLES; i++)
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
	spectrum.fillSprite(TFT_BLACK);
	for (int i = 2; i < min(SAMPLES>>1 , GRAPHWIDTH) ; i=i+1)
	{
		int amplitude = (int)FFTDisplayBuffer[i] * FFTDisplayScale / SIGNAL_AMPLITUDE;
		amplitude = min(amplitude, SPECTRUMHEIGHT);
		uint8_t r, g, b;
		getHeatMapColor((float)amplitude / (float)SPECTRUMHEIGHT, &r, &g, &b);
		uint16_t color = tft.color565(r, g, b);
		spectrum.drawFastVLine(i, SPECTRUMHEIGHT - amplitude, amplitude, color);
		waterfall.drawPixel(i, 0, color);
	}

	spectrum.pushSprite(0, 0);
	waterfall.pushSprite(0, SPECTRUMHEIGHT);
	waterfall.scroll(0, 1);

	//calculate peak and THD
	if (millis() > majorPeakTime)
	{
		majorPeakTime = millis() + 500;
		majorPeakFrequency = String(FFT.MajorPeak(FFTDisplayBuffer, SAMPLES, SAMPLING_FREQUENCY), 1);
		//THD
		double totalTHD = 0;
		double maxPowerForTHD = 0;
		for (int i = 2; i < SAMPLES / 2; i++)
		{
			totalTHD += sq(FFTreal[i]);
			if (FFTreal[i] > maxPowerForTHD)
				maxPowerForTHD = FFTDisplayBuffer[i];
		}
		totalTHD -= sq(maxPowerForTHD);
		totalTHD = sqrt(totalTHD) / maxPowerForTHD;
		totalTHD *= 100;

		THD = String(totalTHD, 0);
		
		TextBox(String(majorPeakFrequency + " Hz"), GRAPHWIDTH, 10);
		TextBox("THD " + THD + " %", GRAPHWIDTH, 40);
	}
}

TFT_eSprite img = TFT_eSprite(&tft);  // Create Sprite object "img" with pointer to "tft" object
void TextBox(const String& txt, int x, int y)
{

	// Size of sprite
#define IWIDTH  60
#define IHEIGHT 16

	// Create a 8 bit sprite 80 pixels wide, 35 high (2800 bytes of RAM needed)
	img.setColorDepth(8);
	img.createSprite(IWIDTH, IHEIGHT);

	// Fill it with black (this will be the transparent colour this time)
	img.fillSprite(TFT_BLACK);

	// Draw a background for the numbers
	img.fillRoundRect(0, 0, IWIDTH, IHEIGHT-1, 6, TFT_DARKGREY);
	//img.drawRoundRect(0, 0, IWIDTH, IHEIGHT-1, 6, TFT_WHITE);

	// Set the font parameters
	img.setTextSize(1);           // Font size scaling is x1
	img.setTextColor(TFT_WHITE);  // White text, no background colour

								  // Set text coordinate datum to middle right
	img.setTextDatum(MR_DATUM);

	// Draw the number to 3 decimal places at 70,20 in font 4
	img.drawString(txt, IWIDTH,8, 1);

	// Push sprite to TFT screen CGRAM at coordinate x,y (top left corner)
	// All black pixels will not be drawn hence will show as "transparent"
	img.pushSprite(x, y, TFT_BLACK);

	// Delete sprite to free up the RAM
	img.deleteSprite();
}

void Synusoide_Task(void *parameter)
{
	for (;;)
	{
		for (int i = 0; i < 360; i++)
		{
			dacWrite(26, (120 + (sin((i * 3.14) / 180) * (120 - (i / 100)))));
		}
	}
	vTaskDelete(NULL);
}
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255)
{
	uint32_t duty = (8191 / valueMax) * min(value, valueMax);
	ledcWrite(channel, duty);
}
// Make a PWM generator task on core 0 Signal generator pin 2
void LedC_Task(void *parameter)
{
	int phase = 128;
	int phaseStep = 5;
	ledcSetup(0, 600, 13);
	ledcAttachPin(2, 0);

	for (;;)
	{
		ledcAnalogWrite(0, phase);
		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}
void taskCapture(void *pvParameters)
{
	for (;;)
	{
		Capture();
	}
}
void TaskDraw(void *pvParameters)
{
	for (;;)
	{
		DrawFFT();
		DrawOscillo();
	}
}
void TaskDebug(void *pvParameters)
{
	for (;;)
	{
		Serial.println(esp_get_minimum_free_heap_size());
		delay(5000);
	}
}

void setup()
{
	tft.init();
	tft.fillScreen(TFT_BLACK);
	tft.setRotation(1);

	// Create a sprite for the graph
	spectrum.setColorDepth(16);
	spectrum.createSprite(GRAPHWIDTH, SPECTRUMHEIGHT);
	spectrum.fillSprite(TFT_BLACK); // Note: Sprite is filled with black when created

	waterfall.setColorDepth(16);
	waterfall.createSprite(GRAPHWIDTH, WATERFALLHEIGHT);
	waterfall.fillSprite(TFT_BLACK); // Note: Sprite is filled with black when created

	oscillo.setColorDepth(16);
	oscillo.createSprite(GRAPHWIDTH, OSCILLOHEIGHT);
	oscillo.fillSprite(TFT_BLACK); // Note: Sprite is filled with black when created

	Serial.begin(115200);

	//speaker shhhhh
	dacWrite(25, 0);

	//adc1_config_width(ADC_WIDTH_BIT_12);   //Range 0-1023 
	//adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_2_5);  //ADC_ATTEN_DB_11 = 0-3,6V
	analogReadResolution(12); // Default of 12 is not very linear. Recommended to use 10 or 11 depending on needed resolution.
	analogSetAttenuation(ADC_0db); // Default is 11db which is very noisy. Recommended to use 2.5 or 6.

	sampling_period_ms = round(1000 * (1.0 / SAMPLING_FREQUENCY));
	FFTDisplayScale = 0.3;

	//launch tasks parameters:  (task function, name, stack size (W), task parameters, priority, task handle, core#)
	//xTaskCreatePinnedToCore(taskCapture, "AD sampling", 8192, NULL, 1, NULL, 0);
	//xTaskCreatePinnedToCore(TaskDraw, "draw FFT", 8192, NULL, 2, NULL, 0);
	//xTaskCreatePinnedToCore(TaskDebug, "debug", 8192, NULL, 2, NULL, 0);
	//xTaskCreatePinnedToCore(Synusoide_Task, "sin output on 26", 8192, NULL, 2, NULL, 0);
	xTaskCreatePinnedToCore(LedC_Task, "square wave on 26", 8192, NULL, 2, NULL, 0);

}

long int oldMillis = 0;
void loop()
{
	uint32_t dt = millis();

	//Input();
	//oldMillis = millis();
	Capture(); //10ms
	//Serial.println(millis() - oldMillis);
	ComputeFFT(); //14ms
	DrawOscillo(); // 14ms
	DrawFFT(); //28ms

	// necessary when nothing is in loop and one core has a task running
	//delay(500);
	//M5.update();
	TextBox(String( millis() - dt) + " ms",GRAPHWIDTH,200);
}
