#include <Arduino.h>
#include <FastLED.h>

#include "driver/rtc_io.h"

#include "palettes.h"

#include <FS.h>
#include "LittleFS.h"
#define FORMAT_LITTLEFS_IF_FAILED true 

#include <Preferences.h>  
Preferences preferences;

#define DATA_PIN_1 D2 // D8 for Charm; D0 for Pebble 

#define BUTTON_PIN_BITMASK 0x10 // On/off GPIO 4
#define wakeupPin 4
const uint16_t shutdownCheckInterval = 200; 

#include "matrixMap_6x10.h"
#define WIDTH 10
#define HEIGHT 6 
#define NUM_LEDS ( WIDTH * HEIGHT )

const uint16_t MIN_DIMENSION = MIN(WIDTH, HEIGHT);
const uint16_t MAX_DIMENSION = MAX(WIDTH, HEIGHT);

CRGB leds[NUM_LEDS];
uint16_t ledNum = 0;

using namespace fl;

//bleControl variables ***********************************************************************
//elements that must be set before #include "bleControl.h" 

extern const TProgmemRGBGradientPaletteRef gGradientPalettes[]; 
extern const uint8_t gGradientPaletteCount;
uint8_t gCurrentPaletteNumber;
uint8_t gTargetPaletteNumber;
CRGBPalette16 gCurrentPalette;
CRGBPalette16 gTargetPalette;

uint8_t PROGRAM;
uint8_t MODE;
uint8_t SPEED;
uint8_t BRIGHTNESS;

uint8_t mapping = 1;

#include "bleControl.h"

#include "rainbow.hpp"
#include "waves.hpp"

// Misc global variables ********************************************************************

uint8_t savedSpeed;
uint8_t savedBrightness;
uint8_t savedProgram;
uint8_t savedMode;

// MAPPINGS **********************************************************************************

extern const uint16_t progTopDown[NUM_LEDS] PROGMEM;
extern const uint16_t progBottomUp[NUM_LEDS] PROGMEM;
extern const uint16_t serpTopDown[NUM_LEDS] PROGMEM;
extern const uint16_t serpBottomUp[NUM_LEDS] PROGMEM;
extern const uint16_t progLeftRight[NUM_LEDS] PROGMEM;

enum Mapping {
	TopDownProgressive = 1,
	TopDownSerpentine,
	BottomUpProgressive,
	BottomUpSerpentine,
	LeftRightProgressive
}; 

// General (non-FL::XYMap) mapping 
	uint16_t myXY(uint8_t x, uint8_t y) {
			if (x >= WIDTH || y >= HEIGHT) return 0;
			uint16_t i = ( y * WIDTH ) + x;
			switch(mapping){
				case 1:	 ledNum = progTopDown[i]; break;
				case 2:	 ledNum = progBottomUp[i]; break;
				case 3:	 ledNum = serpTopDown[i]; break;
				case 4:	 ledNum = serpBottomUp[i]; break;
				case 5:	 ledNum = progLeftRight[i]; break;
			}
			return ledNum;
	}

// Used only for FL::XYMap purposes
	/*
	uint16_t myXYFunction(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
			width = WIDTH;
			height = HEIGHT;
			if (x >= width || y >= height) return 0;
			uint16_t i = ( y * width ) + x;

			switch(mapping){
				case 1:	 ledNum = progTopDown[i]; break;
				case 2:	 ledNum = progBottomUp[i]; break;
				case 3:	 ledNum = serpTopDown[i]; break;
				case 4:	 ledNum = serpBottomUp[i]; break;
			}
			
			return ledNum;
	}*/

	//uint16_t myXYFunction(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

	//XYMap myXYmap = XYMap::constructWithUserFunction(WIDTH, HEIGHT, myXYFunction);
	XYMap myXYmap = XYMap::constructWithLookUpTable(WIDTH, HEIGHT, progBottomUp);
	XYMap xyRect = XYMap::constructRectangularGrid(WIDTH, HEIGHT);

//******************************************************************************************************************************
 

//******************************************************************************************************************************

void setup() {
		
		pinMode(wakeupPin, INPUT);	

		preferences.begin("settings", true); // true == read only mode
			savedBrightness  = preferences.getUChar("brightness");
			savedSpeed  = preferences.getUChar("speed");
			savedProgram  = preferences.getUChar("program");
			savedMode  = preferences.getUChar("mode");
		preferences.end();	

		BRIGHTNESS = 50;
		SPEED = 5;
		PROGRAM = 1;
		MODE = 0;
		//BRIGHTNESS = savedBrightness;
		//SPEED = savedSpeed;
		//PROGRAM = savedProgram;
		//MODE = savedMode;

		FastLED.addLeds<WS2812B, DATA_PIN_1, GRB>(leds, NUM_LEDS)
				.setCorrection(TypicalLEDStrip);
				//.setDither(BRIGHTNESS < 255);

		FastLED.setBrightness(BRIGHTNESS);

		FastLED.clear();
		FastLED.show();

		if (debug) {
			Serial.begin(115200);
			delay(500);
			Serial.print("Initial program: ");
			Serial.println(PROGRAM);
			Serial.print("Initial brightness: ");
			Serial.println(BRIGHTNESS);
			Serial.print("Initial speed: ");
			Serial.println(SPEED);
		}

		bleSetup();

		if (!LittleFS.begin(true)) {
        	Serial.println("LittleFS mount failed!");
        	return;
		}
		Serial.println("LittleFS mounted successfully.");   

}

//*****************************************************************************************

void shutdownCheck() {
 if( digitalRead(wakeupPin) == HIGH ) {
   esp_sleep_enable_ext1_wakeup_io(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
   rtc_gpio_pulldown_en((gpio_num_t)wakeupPin);  // Tie to GND in order to wake up in HIGH
   rtc_gpio_pullup_dis((gpio_num_t)wakeupPin);   // Disable PULL_UP in order to allow it to wakeup on HIGH
   Serial.println("Going to sleep now");
   FastLED.clear();
   FastLED.show();
   delay(2000);
   esp_deep_sleep_start();
 }
}

//*****************************************************************************************

void updateSettings_brightness(uint8_t newBrightness){
 preferences.begin("settings",false);  // false == read write mode
	 preferences.putUChar("brightness", newBrightness);
 preferences.end();
 savedBrightness = newBrightness;
 if (debug) {Serial.println("Brightness setting updated");}
}

//*******************************************************************************************

void updateSettings_speed(uint8_t newSpeed){
 preferences.begin("settings",false);  // false == read write mode
	 preferences.putUChar("speed", newSpeed);
 preferences.end();
 savedSpeed = newSpeed;
 if (debug) {Serial.println("Speed setting updated");}
}

//*****************************************************************************************

void updateSettings_program(uint8_t newProgram){
 preferences.begin("settings",false);  // false == read write mode
	 preferences.putUChar("program", newProgram);
 preferences.end();
 savedProgram = newProgram;
 if (debug) {Serial.println("Program setting updated");}
}

//*****************************************************************************************

void updateSettings_mode(uint8_t newMode){
 preferences.begin("settings",false);  // false == read write mode
	 preferences.putUChar("mode", newMode);
 preferences.end();
 savedMode = newMode;
 if (debug) {Serial.println("Mode setting updated");}
}

//*****************************************************************************************

void loop() {

		//EVERY_N_MILLISECONDS(shutdownCheckInterval) { shutdownCheck(); }

		EVERY_N_SECONDS(30) {
			if ( BRIGHTNESS != savedBrightness ) updateSettings_brightness(BRIGHTNESS);
			if ( SPEED != savedSpeed ) updateSettings_speed(SPEED);
			if ( PROGRAM != savedProgram ) updateSettings_program(PROGRAM);
			if ( MODE != savedMode ) updateSettings_mode(MODE);
		}
 
		if (!displayOn){
			FastLED.clear();
		}
		
		else {
			
			//FastLED.setBrightness(BRIGHTNESS);

			switch(PROGRAM){

				case 0:  
					mapping = Mapping::TopDownProgressive;
					if (!rainbow::rainbowInstance) {
						rainbow::initRainbow(myXY);
					}
					rainbow::runRainbow();
					//nscale8(leds,NUM_LEDS,BRIGHTNESS);
					break; 

				case 1:
					// 1D; mapping not needed
					if (!waves::wavesInstance) {
						waves::initWaves();
					}
					waves::runWaves(); 
					break;
 
				/*case 2:  
					mapping = Mapping::TopDownSerpentine;
					break;  

				case 3:    
					mapping = Mapping::TopDownProgressive;
					break;
				*/
			}
		}
				
		FastLED.show();
	
		// upon BLE disconnect
		if (!deviceConnected && wasConnected) {
			if (debug) {Serial.println("Device disconnected.");}
			delay(500); // give the bluetooth stack the chance to get things ready
			pServer->startAdvertising();
			if (debug) {Serial.println("Start advertising");}
			wasConnected = false;
		}

} // loop()