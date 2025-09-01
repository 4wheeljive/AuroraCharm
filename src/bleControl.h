#pragma once

#include "FastLED.h"
#include <ArduinoJson.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>

#include <FS.h>
#include "LittleFS.h"
#define FORMAT_LITTLEFS_IF_FAILED true 

bool displayOn = true;
bool debug = true;
bool pauseAnimation = false;

uint8_t dummy = 1;

extern uint8_t PROGRAM;
extern uint8_t MODE;

 // PROGRAM/MODE FRAMEWORK ****************************************

  enum Program : uint8_t {
      RAINBOW = 0,
      WAVES = 1,
      PROGRAM_COUNT
  };

  // Program names in PROGMEM
  const char rainbow_str[] PROGMEM = "rainbow";
  const char waves_str[] PROGMEM = "waves";
  
  const char* const PROGRAM_NAMES[] PROGMEM = {
      rainbow_str, waves_str 
  };

  // Mode names in PROGMEM
  const char palette_str[] PROGMEM = "palette";
  const char pride_str[] PROGMEM = "pride";

  const char* const WAVES_MODES[] PROGMEM = {
      palette_str, pride_str
   };

  const uint8_t MODE_COUNTS[] = {0, 2};

   // Visualizer parameter mappings - PROGMEM arrays for memory efficiency
   // Individual parameter arrays for each visualizer
   const char* const RAINBOW_PARAMS[] PROGMEM = {};
   const char* const WAVES_PALETTE_PARAMS[] PROGMEM = {"speed", "hueIncMax", "blendFract", "brightTheta"};
   const char* const WAVES_PRIDE_PARAMS[] PROGMEM = {"speed", "hueIncMax", "blendFract", "brightTheta"};
   
   // Struct to hold visualizer name and parameter array reference
   struct VisualizerParamEntry {
      const char* visualizerName;
      const char* const* params;
      uint8_t count;
   };

   // String-based lookup table - mirrors JavaScript VISUALIZER_PARAMS
   // Can number values be replace by an array element count?
   const VisualizerParamEntry VISUALIZER_PARAM_LOOKUP[] PROGMEM = {
      {"rainbow", RAINBOW_PARAMS, 4},
      {"waves-palette", WAVES_PALETTE_PARAMS, 4},
      {"waves-pride", WAVES_PRIDE_PARAMS, 4}
   };

  class VisualizerManager {
  public:
      static String getVisualizerName(int programNum, int mode = -1) {
          if (programNum < 0 || programNum > PROGRAM_COUNT-1) return "";

          // Get program name from flash memory
          char progName[16];
          strcpy_P(progName,(char*)pgm_read_ptr(&PROGRAM_NAMES[programNum]));

          if (mode < 0 || MODE_COUNTS[programNum] == 0) {
              return String(progName);
          }

          // Get mode name
          const char* const* modeArray = nullptr;
          switch (programNum) {
              case WAVES: modeArray = WAVES_MODES; break;
              default: return String(progName);
          }

          if (mode >= MODE_COUNTS[programNum]) return String(progName);

          char modeName[20];
          strcpy_P(modeName,(char*)pgm_read_ptr(&modeArray[mode]));

         //return String(progName) + "-" + String(modeName);
         String result = "";
         result += String(progName);
         result += "-";
         result += String(modeName);
         return result;
      }
      
      // Get parameter list based on visualizer name
      static const VisualizerParamEntry* getVisualizerParams(const String& visualizerName) {
          const int LOOKUP_SIZE = sizeof(VISUALIZER_PARAM_LOOKUP) / sizeof(VisualizerParamEntry);
          
          for (int i = 0; i < LOOKUP_SIZE; i++) {
              char entryName[32];
              strcpy_P(entryName, (char*)pgm_read_ptr(&VISUALIZER_PARAM_LOOKUP[i].visualizerName));
              
              if (visualizerName.equals(entryName)) {
                  return &VISUALIZER_PARAM_LOOKUP[i];
              }
          }
          return nullptr;
      }
  };  // class VisualizerManager


// Parameter control *************************************************************************************

using namespace ArduinoJson;

bool rotateWaves = true; 
uint8_t cBright = 75;
uint8_t cColOrd = 0;                  

float cSpeed = 1.f;
float cZoom = 1.f;
float cScale = 1.f; 
float cAngle = 1.f; 
float cTwist = 1.f;
float cRadius = 1.0f; 
float cEdge = 1.0f;
float cZ = 1.f; 
float cRatBase = 0.0f; 
float cRatDiff= 1.f; 
float cOffBase = 1.f; 
float cOffDiff = 1.f; 
float cRed = 1.f; 
float cGreen = 1.f; 
float cBlue = 1.f;

//String cVisualizer;
uint8_t cSpeedInt = 1;

//Waves
float cHueIncMax = 300;
uint8_t cBlendFract = 128;
float cBrightTheta = 1;

EaseType getEaseType(uint8_t value) {
    switch (value) {
        case 0: return EASE_NONE;
        case 1: return EASE_IN_QUAD;
        case 2: return EASE_OUT_QUAD;
        case 3: return EASE_IN_OUT_QUAD;
        case 4: return EASE_IN_CUBIC;
        case 5: return EASE_OUT_CUBIC;
        case 6: return EASE_IN_OUT_CUBIC;
        case 7: return EASE_IN_SINE;
        case 8: return EASE_OUT_SINE;
        case 9: return EASE_IN_OUT_SINE;
    }
    FL_ASSERT(false, "Invalid ease type");
    return EASE_NONE;
}

uint8_t cEaseSat = 0;
uint8_t cEaseLum = 0;

bool Layer1 = true;
bool Layer2 = true;
bool Layer3 = true;
bool Layer4 = true;
bool Layer5 = true;

ArduinoJson::JsonDocument sendDoc;
ArduinoJson::JsonDocument receivedJSON;

//*******************************************************************************
//BLE CONFIGURATION *************************************************************

BLEServer* pServer = NULL;
BLECharacteristic* pButtonCharacteristic = NULL;
BLECharacteristic* pCheckboxCharacteristic = NULL;
BLECharacteristic* pNumberCharacteristic = NULL;
BLECharacteristic* pStringCharacteristic = NULL;

bool deviceConnected = false;
bool wasConnected = false;

#define SERVICE_UUID                  	"19b10000-e8f2-537e-4f6c-d104768a1214"
#define BUTTON_CHARACTERISTIC_UUID     "19b10001-e8f2-537e-4f6c-d104768a1214"
#define CHECKBOX_CHARACTERISTIC_UUID   "19b10002-e8f2-537e-4f6c-d104768a1214"
#define NUMBER_CHARACTERISTIC_UUID     "19b10003-e8f2-537e-4f6c-d104768a1214"
#define STRING_CHARACTERISTIC_UUID     "19b10004-e8f2-537e-4f6c-d104768a1214"

BLEDescriptor pButtonDescriptor(BLEUUID((uint16_t)0x2902));
BLEDescriptor pCheckboxDescriptor(BLEUUID((uint16_t)0x2902));
BLEDescriptor pNumberDescriptor(BLEUUID((uint16_t)0x2902));
BLEDescriptor pStringDescriptor(BLEUUID((uint16_t)0x2902));


//*******************************************************************************
// CONTROL FUNCTIONS ************************************************************

void startingPalette() {
   gCurrentPaletteNumber = random(0,gGradientPaletteCount-1);
   CRGBPalette16 gCurrentPalette( gGradientPalettes[gCurrentPaletteNumber] );
   gTargetPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
   gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
}

// UI update functions ***********************************************

void sendReceiptButton(uint8_t receivedValue) {
   pButtonCharacteristic->setValue(String(receivedValue).c_str());
   pButtonCharacteristic->notify();
   if (debug) {
      Serial.print("Button value received: ");
      Serial.println(receivedValue);
   }
}

void sendReceiptCheckbox(String receivedID, bool receivedValue) {
  
   sendDoc.clear();
   sendDoc["id"] = receivedID;
   sendDoc["val"] = receivedValue;

   String jsonString;
   serializeJson(sendDoc, jsonString);

   pCheckboxCharacteristic->setValue(jsonString);
   
   pCheckboxCharacteristic->notify();
   
   if (debug) {
      Serial.print("Sent receipt for ");
      Serial.print(receivedID);
      Serial.print(": ");
      Serial.println(receivedValue);
   }
}

void sendReceiptNumber(String receivedID, float receivedValue) {

   sendDoc.clear();
   sendDoc["id"] = receivedID;
   sendDoc["val"] = receivedValue;

   String jsonString;
   serializeJson(sendDoc, jsonString);

   pNumberCharacteristic->setValue(jsonString);
   
   pNumberCharacteristic->notify();
   
   if (debug) {
      Serial.print("Sent receipt for ");
      Serial.print(receivedID);
      Serial.print(": ");
      Serial.println(receivedValue);
   }
}

void sendReceiptString(String receivedID, String receivedValue) {

   sendDoc.clear();
   sendDoc["id"] = receivedID;
   sendDoc["val"] = receivedValue;

   String jsonString;
   serializeJson(sendDoc, jsonString);

   pStringCharacteristic->setValue(jsonString);

   pStringCharacteristic->notify();
   
   if (debug) {
      Serial.print("Sent receipt for ");
      Serial.print(receivedID);
      Serial.print(": ");
      Serial.println(receivedValue);
   }
}

//***********************************************************************
// PARAMETER/PRESET MANAGEMENT SYSTEM ("PPMS")
// X-Macro table 
#define PARAMETER_TABLE \
   X(uint8_t, ColOrd, 1.0f) \
   X(float, Speed, 1.0f) \
   X(float, Zoom, 1.0f) \
   X(float, Scale, 1.0f) \
   X(float, Angle, 1.0f) \
   X(float, Twist, 1.0f) \
   X(float, Radius, 1.0f) \
   X(float, Edge, 1.0f) \
   X(float, Z, 1.0f) \
   X(float, RatBase, 1.0f) \
   X(float, RatDiff, 1.0f) \
   X(float, OffBase, 1.0f) \
   X(float, OffDiff, 1.0f) \
   X(float, Red, 1.0f) \
   X(float, Green, 1.0f) \
   X(float, Blue, 1.0f) \
   X(uint8_t, SpeedInt, 1) \
   X(float, HueIncMax, 300.0f) \
   X(uint8_t, BlendFract, 128) \
   X(float, BrightTheta, 1.0f) \
   X(uint8_t, EaseSat, 0) \
   X(uint8_t, EaseLum, 0) 



// Auto-generated helper functions using X-macros
void captureCurrentParameters(ArduinoJson::JsonObject& params) {
    #define X(type, parameter, def) params[#parameter] = c##parameter;
    PARAMETER_TABLE
    #undef X
}

void applyCurrentParameters(const ArduinoJson::JsonObjectConst& params) {
    #define X(type, parameter, def) \
        if (!params[#parameter].isNull()) { \
            auto newValue = params[#parameter].as<type>(); \
            if (c##parameter != newValue) { \
                c##parameter = newValue; \
                sendReceiptNumber("in" #parameter, c##parameter); \
            } \
        }
    PARAMETER_TABLE
    #undef X
}


// Preset file persistence functions with JSON structure
bool savePreset(int presetNumber) {
    String filename = "/preset_";
    filename += presetNumber;
    filename += ".json";
    
    ArduinoJson::JsonDocument preset;
    preset["programNum"] = PROGRAM;
    if (MODE_COUNTS[PROGRAM] > 0) { 
      preset["modeNum"] = MODE;
    }    
    ArduinoJson::JsonObject params = preset["parameters"].to<ArduinoJson::JsonObject>();
    captureCurrentParameters(params);
    
    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.print("Failed to save preset: ");
        Serial.println(filename);
        return false;
    }
    
    serializeJson(preset, file);
    file.close();
    
    Serial.print("Preset saved: ");
    Serial.println(filename);
    return true;
}

bool loadPreset(int presetNumber) {
    String filename = "/preset_";
    filename += presetNumber;
    filename += ".json";
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.print("Failed to load preset: ");
        Serial.println(filename);
        return false;
    }
    
    ArduinoJson::JsonDocument preset;
    deserializeJson(preset, file);
    file.close();
    
    if (preset["programNum"].isNull() || preset["parameters"].isNull()) {
        Serial.print("Invalid preset format: ");
        Serial.println(filename);
        return false;
    }

    PROGRAM = (uint8_t)preset["programNum"];
    if (preset["modeNum"]) {
      MODE = (uint8_t)preset["modeNum"];
    }
    pauseAnimation = true;
    applyCurrentParameters(preset["parameters"]);
    pauseAnimation = false;
    
    Serial.print("Preset loaded: ");
    Serial.println(filename);
    return true;
}

//***********************************************************************

void sendDeviceState() { 
   if (debug) {
      Serial.println("Sending device state...");
   }
   
   ArduinoJson::JsonDocument stateDoc;
   stateDoc["program"] = PROGRAM;
   stateDoc["mode"] = MODE;
   
   String currentVisualizer = VisualizerManager::getVisualizerName(PROGRAM, MODE); 
   
   // Get parameter list for current visualizer
   const VisualizerParamEntry* visualizerParams = VisualizerManager::getVisualizerParams(currentVisualizer);

   ArduinoJson::JsonObject params = stateDoc["parameters"].to<ArduinoJson::JsonObject>();

   if (debug) {
       String currentVisualizer = VisualizerManager::getVisualizerName(PROGRAM, MODE);
       Serial.print("Current visualizer: ");
       Serial.println(currentVisualizer);
       Serial.print("Found params: ");
       Serial.println(visualizerParams != nullptr ? "YES" : "NO");
       if (visualizerParams != nullptr) {
           Serial.print("Param count: ");
           Serial.println(visualizerParams->count);
       }
   }
   
   if (visualizerParams != nullptr) {
       // Loop through parameters for current visualizer
       for (uint8_t i = 0; i < visualizerParams->count; i++) {
           char paramName[32];
           strcpy_P(paramName, (char*)pgm_read_ptr(&visualizerParams->params[i]));
           
           if (debug) {
               Serial.print("Processing parameter: ");
               Serial.println(paramName);
           }
       }
   }

   // Add parameter values to JSON based on visualizer params
   for (uint8_t i = 0; i < visualizerParams->count; i++) {
       char paramName[32];
       strcpy_P(paramName, (char*)pgm_read_ptr(&visualizerParams->params[i]));
       
       bool paramFound = false;
       // Use X-macro to match parameter names and add values
       // Handle case-insensitive comparison for parameter names
       #define X(type, parameter, def) \
           if (strcasecmp(paramName, #parameter) == 0) { \
               params[paramName] = c##parameter; \
               if (debug) { \
                   Serial.print("Added parameter "); \
                   Serial.print(paramName); \
                   Serial.print(": "); \
                   Serial.println(c##parameter); \
               } \
               paramFound = true; \
           }
       PARAMETER_TABLE
       #undef X
       
       if (!paramFound && debug) {
           Serial.print("Warning: Parameter not found in X-macro table: ");
           Serial.println(paramName);
       }
   }

   
   String stateJson;
   serializeJson(stateDoc, stateJson);
   sendReceiptString("deviceState", stateJson);
}


// Handle UI request functions ***********************************************

std::string convertToStdString(const String& flStr) {
   return std::string(flStr.c_str());
}

void processButton(uint8_t receivedValue) {

   sendReceiptButton(receivedValue);
      
   if (receivedValue < 20) { // Program selection
      PROGRAM = receivedValue;
      MODE = 0;
      displayOn = true;
   }
   
   if (receivedValue >= 20 && receivedValue < 40) { // Mode selection
      MODE = receivedValue - 20;
      //cFxIndex = MODE;
      displayOn = true;
   }

   if (debug) {
      Serial.print("Current visualizer: ");
      Serial.println(VisualizerManager::getVisualizerName(PROGRAM, MODE));
   }

   //if (receivedValue == 91) { updateUI(); }
   if (receivedValue == 92) { sendDeviceState(); }
   //if (receivedValue == 94) { fancyTrigger = true; }
   //if (receivedValue == 95) { resetAll(); }
   
   if (receivedValue == 98) { displayOn = true; }
   if (receivedValue == 99) { displayOn = false; }

   if (receivedValue >= 101 && receivedValue <= 150) { 
      uint8_t savedPreset = receivedValue - 100;  
      savePreset(savedPreset); 
   }

   if (receivedValue >= 151 && receivedValue <= 200) { 
       uint8_t presetToLoad = receivedValue - 150;
       if (loadPreset(presetToLoad)) {
           loadPreset(presetToLoad);
           Serial.print("Loaded preset: ");
           Serial.println(presetToLoad);
       }
   }
}

//*****************************************************************************

void processNumber(String receivedID, float receivedValue ) {

   sendReceiptNumber(receivedID, receivedValue);
  
   if (receivedID == "inBright") {
      cBright = receivedValue;
      BRIGHTNESS = cBright;
      FastLED.setBrightness(BRIGHTNESS);
   };


   if (receivedID == "inPalNum") {
      uint8_t newPalNum = receivedValue;
      gTargetPalette = gGradientPalettes[ newPalNum ];
      if(debug) {
         Serial.print("newPalNum: ");
         Serial.println(newPalNum);
      }
   };
  
   // Auto-generated custom parameter handling using X-macros
   #define X(type, parameter, def) \
       if (receivedID == "in" #parameter) { c##parameter = receivedValue; return; }
   PARAMETER_TABLE
   #undef X

}

void processCheckbox(String receivedID, bool receivedValue ) {
   
   sendReceiptCheckbox(receivedID, receivedValue);
   
   if (receivedID == "cx10") {rotateWaves = receivedValue;};
   if (receivedID == "cxLayer1") {Layer1 = receivedValue;};
   if (receivedID == "cxLayer2") {Layer2 = receivedValue;};
   if (receivedID == "cxLayer3") {Layer3 = receivedValue;};
   if (receivedID == "cxLayer4") {Layer4 = receivedValue;};
   if (receivedID == "cxLayer5") {Layer5 = receivedValue;};

}

void processString(String receivedID, String receivedValue ) {
   sendReceiptString(receivedID, receivedValue);
}

//*******************************************************************************
// CALLBACKS ********************************************************************

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    wasConnected = true;
    if (debug) {Serial.println("Device Connected");}
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    wasConnected = true;
  }
};

class ButtonCharacteristicCallbacks : public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *characteristic) {

      String value = characteristic->getValue();
      if (value.length() > 0) {
         
         uint8_t receivedValue = value[0];
         
         if (debug) {
            Serial.print("Button value received: ");
            Serial.println(receivedValue);
         }
         
         processButton(receivedValue);
        
      }
   }
};

class CheckboxCharacteristicCallbacks : public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *characteristic) {
  
      String receivedBuffer = characteristic->getValue();
  
      if (receivedBuffer.length() > 0) {
                  
         if (debug) {
            Serial.print("Received buffer: ");
            Serial.println(receivedBuffer);
         }
      
         ArduinoJson::deserializeJson(receivedJSON, receivedBuffer);
         String receivedID = receivedJSON["id"] ;
         bool receivedValue = receivedJSON["val"];
      
         if (debug) {
            Serial.print(receivedID);
            Serial.print(": ");
            Serial.println(receivedValue);
         }
      
         processCheckbox(receivedID, receivedValue);
      
      }
   }
};

class NumberCharacteristicCallbacks : public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *characteristic) {
      
      String receivedBuffer = characteristic->getValue();
      
      if (receivedBuffer.length() > 0) {
      
         if (debug) {
            Serial.print("Received buffer: ");
            Serial.println(receivedBuffer);
         }
      
         ArduinoJson::deserializeJson(receivedJSON, receivedBuffer);
         String receivedID = receivedJSON["id"] ;
         float receivedValue = receivedJSON["val"];
      
         if (debug) {
            Serial.print(receivedID);
            Serial.print(": ");
            Serial.println(receivedValue);
         }
      
         processNumber(receivedID, receivedValue);
      }
   }
};

class StringCharacteristicCallbacks : public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *characteristic) {
      
      String receivedBuffer = characteristic->getValue();
      
      if (receivedBuffer.length() > 0) {
      
         if (debug) {
            Serial.print("Received buffer: ");
            Serial.println(receivedBuffer);
         }
      
         ArduinoJson::deserializeJson(receivedJSON, receivedBuffer);
         String receivedID = receivedJSON["id"] ;
         String receivedValue = receivedJSON["val"];
      
         if (debug) {
            Serial.print(receivedID);
            Serial.print(": ");
            Serial.println(receivedValue);
         }
      
         processString(receivedID, receivedValue);
      }
   }
};

//*******************************************************************************
// BLE SETUP FUNCTION ***********************************************************

void bleSetup() {

   BLEDevice::init("Aurora Charm");

   pServer = BLEDevice::createServer();
   pServer->setCallbacks(new MyServerCallbacks());

   BLEService *pService = pServer->createService(SERVICE_UUID);

   pButtonCharacteristic = pService->createCharacteristic(
                     BUTTON_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                  );
   pButtonCharacteristic->setCallbacks(new ButtonCharacteristicCallbacks());
   pButtonCharacteristic->setValue(String(dummy).c_str());
   pButtonCharacteristic->addDescriptor(new BLE2902());

   pCheckboxCharacteristic = pService->createCharacteristic(
                     CHECKBOX_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                  );
   pCheckboxCharacteristic->setCallbacks(new CheckboxCharacteristicCallbacks());
   pCheckboxCharacteristic->setValue(String(dummy).c_str());
   pCheckboxCharacteristic->addDescriptor(new BLE2902());
   
   pNumberCharacteristic = pService->createCharacteristic(
                     NUMBER_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                  );
   pNumberCharacteristic->setCallbacks(new NumberCharacteristicCallbacks());
   pNumberCharacteristic->setValue(String(dummy).c_str());
   pNumberCharacteristic->addDescriptor(new BLE2902());

   pStringCharacteristic = pService->createCharacteristic(
                     STRING_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                  );
   pStringCharacteristic->setCallbacks(new StringCharacteristicCallbacks());
   pStringCharacteristic->setValue(String(dummy).c_str());
   pStringCharacteristic->addDescriptor(new BLE2902());
   

   //**********************************************************

   pService->start();

   BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
   pAdvertising->addServiceUUID(SERVICE_UUID);
   pAdvertising->setScanResponse(false);
   pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
   BLEDevice::startAdvertising();
   if (debug) {Serial.println("Waiting a client connection to notify...");}

}
