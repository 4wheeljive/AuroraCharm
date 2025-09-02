#pragma once

#include "bleControl.h"

namespace fade {

	bool fadeInstance = false;

	void initFade() {
		fadeInstance = true;
		CRGB leds[NUM_LEDS];
		CRGB leds2[NUM_LEDS];
		CRGB leds3[NUM_LEDS];
	}

	void animationA() {
	// running red stripes 
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		uint8_t red = (millis() / 3) + (i * 5);
		if (red > 128) red = 0;
		leds2[i] = CRGB(red, 0, 0);
	}
	}

	void animationB() {
	// the moving rainbow
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		leds3[i] = CHSV((millis() / 4) - (i * 3), 255, 255);
	}
	}

	void runFade() {
	
		// render the first animation into leds2 
		animationA();

		// render the second animation into leds3
		animationB();

		// set the blend ratio for the video cross fade
		// (set ratio to 127 for a constant 50% / 50% blend)
		uint8_t ratio = beatsin8(5);

		// mix the 2 arrays together
		for (int i = 0; i < NUM_LEDS; i++) {
		leds[i] = blend( leds2[i], leds3[i], ratio );
		}

		FastLED.show();

	} // runFade()

} // namespace fade








