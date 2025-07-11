// Basic framework *************************************************
#include <Arduino.h>
#include <FastLED.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define WIDTH 10
#define HEIGHT 6
#define NUM_LEDS  WIDTH * HEIGHT

#include <matrixMap_mini6x10.h>
#include <palettes.h>

CRGB leds[NUM_LEDS];
uint16_t ledNum;

using namespace fl;

#include <Preferences.h>
Preferences preferences;

//bleControl variables **************************************

uint8_t program = 1;
uint8_t pattern;
bool displayOn = true;
bool runAurora = true;
bool runWaves = false;
bool rotateWaves = true; 

extern const TProgmemRGBGradientPaletteRef gGradientPalettes[]; 
extern const uint8_t gGradientPaletteCount;

uint8_t gCurrentPaletteNumber;
CRGBPalette16 gCurrentPalette;
CRGBPalette16 gTargetPalette;

uint8_t SPEED;
float speedfactor;
uint8_t BRIGHTNESS;
const uint8_t brightnessInc = 15;
bool brightnessChanged = false;

#include "bleControl.h"

// Physical configuration ************************************

#define DATA_PIN_1 D8

// Misc global  ***********************************************************************

uint8_t blendFract = 64;
uint16_t hueIncMax = 1500;
CRGB newcolor = CRGB::Black;

uint8_t savedSpeed;
uint8_t savedBrightness;

const uint16_t brightnessCheckInterval = 200;
const uint16_t shutdownCheckInterval = 200; 

#define SECONDS_PER_PALETTE 20

//*******************************************************************************************

void setup() {

 delay(1000);
 
 preferences.begin("settings", true); // true == read only mode
   savedBrightness  = preferences.getUChar("brightness");
   savedSpeed  = preferences.getUChar("speed");
 preferences.end();

BRIGHTNESS = 50;
//SPEED = 5;
//BRIGHTNESS = savedBrightness;
SPEED = savedSpeed;

 FastLED.addLeds<WS2812B, DATA_PIN_1, GRB>(leds, NUM_LEDS)
   .setCorrection(TypicalLEDStrip);
  // .setDither(BRIGHTNESS < 255);

 FastLED.setBrightness(BRIGHTNESS);

 FastLED.clear();
 FastLED.show();

 Serial.begin(115200);
 delay(500);
 Serial.print("Initial brightness: ");
 Serial.println(BRIGHTNESS);
 Serial.print("Initial speed: ");
 Serial.println(SPEED);

 bleSetup();


}

//*******************************************************************************************

void updateSettings_brightness(uint8_t newBrightness){
 preferences.begin("settings",false);  // false == read write mode
   preferences.putUChar("brightness", newBrightness);
 preferences.end();
 savedBrightness = newBrightness;
 Serial.println("Brightness setting updated");
}

//*******************************************************************************************

void updateSettings_speed(uint8_t newSpeed){
 preferences.begin("settings",false);  // false == read write mode
   preferences.putUChar("speed", newSpeed);
 preferences.end();
 savedSpeed = newSpeed;
 Serial.println("Speed setting updated");
}

//*******************************************************************************************


// AURORA / WAVES**************************************************************************
// Matrix format: Serpentine

void display(uint8_t pattern) {

  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 240, 255); 
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60); 
 
  uint16_t hue16 = sHue16; 
  uint16_t hueinc16 = beatsin88(113, 1, hueIncMax );    
  uint16_t ms = millis();  
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;     
  sPseudotime += deltams * msmultiplier*speedfactor;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;

  for( uint16_t i = 0 ; i < NUM_LEDS; i++ ) {
   hue16 += hueinc16;
   uint8_t hue8 = hue16 / 256;

   if (runWaves) {
     uint16_t h16_128 = hue16 >> 7;
     if( h16_128 & 0x100) {
       hue8 = 255 - (h16_128 >> 1);
     } else {
       hue8 = h16_128 >> 1;
     }
   }

   brightnesstheta16  += brightnessthetainc16;
   uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

   uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
   uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
   bri8 += (255 - brightdepth);
     
   switch (pattern) {
     case 1:
       newcolor = CHSV( hue8, sat8, bri8);
       blendFract = 64;
       break;
     case 2:
       uint8_t index = hue8;
       //index = triwave8( index);
       index = scale8( index, 240);
       newcolor = ColorFromPalette( gCurrentPalette, index, bri8);
       blendFract = 128;
       break;
   }

   uint16_t pixelnumber = i;
   //pixelnumber = (NUM_LEDS-1) - pixelnumber;    // commenting this line reverses apparent direction of LED progression   
   uint16_t ledNum = loc2indSerp[i];
   nblend( leds[ledNum], newcolor, blendFract);
   }

}

// RAINBOW MATRIX******************************************************************************
// Matrix format: Progressive by rows

void DrawOneFrame( uint8_t startHue8, int8_t yHueDelta8, int8_t xHueDelta8)
{
  uint8_t lineStartHue = startHue8;
  for( uint8_t y = 0; y < HEIGHT; y++) {
    lineStartHue += yHueDelta8;
    uint8_t pixelHue = lineStartHue;      
    for( uint8_t x = 0; x < WIDTH; x++) {
      pixelHue += xHueDelta8;
      leds[loc2indProgbyRow[y][x]] = CHSV(pixelHue, 255, 255); 
    }
  }
}

void rainbowMatrix () 
{
    uint32_t ms = millis();
    int32_t yHueDelta32 = ((int32_t)cos16( ms * (27/1) ) * (350 / WIDTH));
    int32_t xHueDelta32 = ((int32_t)cos16( ms * (39/1) ) * (310 / HEIGHT));
    DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
 }

// CHAMELEON ******************************************************************************


void loop() {

 EVERY_N_SECONDS(30) {
   if ( BRIGHTNESS != savedBrightness ) updateSettings_brightness(BRIGHTNESS);
   if ( SPEED != savedSpeed ) updateSettings_speed(SPEED);
 }

 if (!displayOn){
   FastLED.clear();
   FastLED.show();
 }
 else {
   
   switch(program){

     case 0:  
       rainbowMatrix ();
       nscale8(leds,NUM_LEDS,BRIGHTNESS);
       break; 

      case 1:
         if (runAurora) { 
           hueIncMax = 3000;
           display(1); 
         }
   
         if (runWaves) { 
           hueIncMax = 1500;
           if (rotateWaves) {
             EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
               gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
               gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
               pPaletteCharacteristic->setValue(String(gCurrentPaletteNumber).c_str());
               pPaletteCharacteristic->notify();
               Serial.print("Color palette: ");
               Serial.println(gCurrentPaletteNumber);
             }
           }
           EVERY_N_MILLISECONDS(40) {
               nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 16); 
           }
           display(2); 
         }
         break;

   }

   FastLED.show();
 }

 // while connected
 if (deviceConnected) {
   if (brightnessChanged) { 
     pBrightnessCharacteristic->notify();
     brightnessChanged = false;
   }
 }

 // upon disconnect
 if (!deviceConnected && wasConnected) {
   Serial.println("Device disconnected.");
   delay(500); // give the bluetooth stack the chance to get things ready
   pServer->startAdvertising();
   Serial.println("Start advertising");
   wasConnected = false;
 }

}

