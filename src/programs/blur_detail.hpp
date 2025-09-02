#pragma once

#include "bleControl.h"

namespace blur {

	bool blurInstance = false;
    XYMap* myXYmapPtr;
	XYMap* xyRectPtr;
    
    #define BLUR_AMOUNT 172

	void initBlur(XYMap& myXYmapRef, XYMap& xyRectRef) {
		blurInstance = true;

        // Store XYMap references
		myXYmapPtr = &myXYmapRef;
		xyRectPtr = &xyRectRef;

        uint8_t pos = 0;
        bool toggle = false;
	}

	void runBlur() {
        static int x = random(WIDTH);
        static int y = random(HEIGHT);
        static CRGB c = CRGB(0, 0, 0);
        blur2d(leds, WIDTH, HEIGHT, BLUR_AMOUNT, *myXYmapPtr);
        EVERY_N_MILLISECONDS(1000) {
            x = random(WIDTH);
            y = random(HEIGHT);
            uint8_t r = random(255);
            uint8_t g = random(255);
            uint8_t b = random(255);
            c = CRGB(r, g, b);
        }
        leds[(*myXYmapPtr)(x, y)] = c;
    }
}
