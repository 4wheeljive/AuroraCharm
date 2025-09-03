#pragma once

#include "bleControl.h"
#include "fx/time.h"  

namespace fire {

	bool fireInstance = false;
	uint16_t (*xyFunc)(uint8_t x, uint8_t y);

	void initFire(uint16_t (*xy_func)(uint8_t, uint8_t)) {
		fireInstance = true;
		xyFunc = xy_func;
	}

	DEFINE_GRADIENT_PALETTE(hot_gp) {
		27, 0, 0, 0,                     // black
		28, 140, 40, 0,                 // red
		30, 205, 80, 0,              // orange
		155, 255, 100, 0,    
		210, 255, 200, 0,             // yellow
		255, 255, 255, 255             // white
	};
	CRGBPalette32 hotPalette = hot_gp;

	#define CentreX  (WIDTH / 2) - 1
	#define CentreY (HEIGHT / 2) - 1

	// Fire properties
	#define FIRESPEED 5
	#define FLAMEHEIGHT 2 // 3.8 // the higher the value, the higher the flame
	#define FIRENOISESCALE 100 // 50 // small values, softer fire. Big values, blink fire. 0-255

	// Smoke screen properties
	// The smoke screen works best for big fire effects. It effectively cuts of a part of the flames
	// from the rest, sometimes; which looks very much fire-like. For small fire effects with low
	// LED count in the height, it doesn't help
	// speed must be a little different and faster from Firespeed, to be visible. 
	// Dimmer should be somewhere in the middle for big fires, and low for small fires.
	#define SMOKESPEED 15 // 25 // how fast the perlin noise is parsed for the smoke
	#define SMOKENOISE_DIMMER 50 // thickness of smoke: the lower the value, the brighter the flames. 0-255
	#define SMOKENOISESCALE 25 // 125 // small values, softer smoke. Big values, blink smoke. 0-255

	// parameters and buffer for the noise array
	#define NUM_LAYERS 2
	// two layers of perlin noise make the fire effect
	#define FIRENOISE 0
	#define SMOKENOISE 1
	uint32_t x[NUM_LAYERS];
	uint32_t y[NUM_LAYERS];
	uint32_t z[NUM_LAYERS];
	uint32_t scale_x[NUM_LAYERS];
	uint32_t scale_y[NUM_LAYERS];

	uint8_t noise[NUM_LAYERS][WIDTH][HEIGHT];
	uint8_t noise2[NUM_LAYERS][WIDTH][HEIGHT];

	uint16_t heat[NUM_LEDS];

	void Fire2023(uint32_t now);


	/*
	DEFINE_GRADIENT_PALETTE(firepal){
		// Traditional fire palette - transitions from black to red to yellow to white
		0,   0,   0,   0,  // black (bottom of fire)
		32,  255, 0,   0,  // red (base of flames)
		190, 255, 255, 0,  // yellow (middle of flames)
		255, 255, 255, 255 // white (hottest part/tips of flames)
	};

	DEFINE_GRADIENT_PALETTE(electricGreenFirePal){
		// Green fire palette - for a toxic/alien look
		0,   0,   0,   0,  // black (bottom)
		32,  0,   70,  0,  // dark green (base)
		190, 57,  255, 20, // electric neon green (middle)
		255, 255, 255, 255 // white (hottest part)
	};

	DEFINE_GRADIENT_PALETTE(electricBlueFirePal){
		// Blue fire palette - for a cold/ice fire look
		0,   0,   0,   0,   // Black (bottom)
		32,  0,   0,   70,  // Dark blue (base)
		128, 20,  57,  255, // Electric blue (middle)
		255, 255, 255, 255  // White (hottest part)
	};

	DEFINE_GRADIENT_PALETTE(JuJuPink){
		// Julia's pink fire palette
		0,  0,   0,   0,   
		32,  20,   0,   10,  
		128, 75,  0,  35, 
		255, 255, 255, 255  
	};

	TimeWarp timeScale(0, 1.0f);  // Initialize with 0 starting time and 1.0 speed multiplier

	uint16_t scaleXY = 8;
	float speedY = 1.3f;
	float scaleX = .3f;
	uint16_t invSpeedZ = 20;
	uint8_t palette = 0;

	CRGBPalette16 getPalette() {
		// This function returns the appropriate color palette based on the UI selection
		switch (palette) {
		case 0:
			return firepal;               // Traditional orange/red fire
		case 1:
			return electricGreenFirePal;  // Green "toxic" fire
		case 2:
			return electricBlueFirePal;   // Blue "cold" fire
		case 3:
			return JuJuPink;              //  Julia's pink fire
		default:
			return firepal;               // Default to traditional fire if invalid value
		}
	}

	uint8_t getPaletteIndex(uint32_t millis32, int width, int max_width, int height, int max_height,
							uint32_t y_speed) {
		// This function calculates which color to use from our palette for each LED
		
		// Get the scale factor from the UI slider
		uint16_t scale = scaleXY;
		
		// Convert width position to an angle (0-255 represents 0-360 degrees)
		// This maps our flat coordinate to a position on a cylinder
		float xf = (float)width / (float)max_width;  // Normalized position (0.0 to 1.0)
		uint8_t x = (uint8_t)(xf * 255);            // Convert to 0-255 range for trig functions
		
		// Calculate the sine and cosine of this angle to get 3D coordinates on the cylinder
		uint32_t cosx = cos8(x);  // cos8 returns a value 0-255 representing cosine
		uint32_t sinx = sin8(x);  // sin8 returns a value 0-255 representing sine
		
		// Apply scaling to the sine/cosine values
		// This controls how "wide" the noise pattern is around the cylinder
		float trig_scale = scale * scaleX;
		cosx *= trig_scale;
		sinx *= trig_scale;
		
		// Calculate Y coordinate (vertical position) with speed offset for movement
		uint32_t y = height * scale + y_speed;
		
		// Calculate Z coordinate (time dimension) - controls how the pattern changes over time
		uint16_t z = millis32 / invSpeedZ;

		// Generate 16-bit Perlin noise using our 4D coordinates (x,y,z,t)
		// The << 8 shifts values left by 8 bits (multiplies by 256) to use the full 16-bit range
		// The last parameter (0) could be replaced with another time variable for more variation
		uint16_t noise16 = inoise16(cosx << 8, sinx << 8, y << 8, 0);
		
		// Convert 16-bit noise to 8-bit by taking the high byte
		uint8_t noise_val = noise16 >> 8;
		
		// Calculate how much to subtract based on vertical position (height)
		// This creates the fade-out effect from bottom to top
		// The formula maps height from 0 to max_height-1 to a value from 255 to 0
		int8_t subtraction_factor = abs8(height - (max_height - 1)) * 255 /
									(max_height - 1);
		
		// Subtract the factor from the noise value (with underflow protection)
		// qsub8 is a "saturating subtraction" - it won't go below 0
		return qsub8(noise_val, subtraction_factor);
	}
	*/

void Fire2023(uint32_t now) {
		
		/*
		// some changing values
		// these values are produced by perlin noise to add randomness and smooth transitions
		uint16_t ctrl1 = inoise16(11 * now, 0, 0);
		uint16_t ctrl2 = inoise16(13 * now, 100000, 100000);
		uint16_t  ctrl = ((ctrl1 + ctrl2) >> 1);

		// parameters for the fire heat map
		x[FIRENOISE] = 3 * inoise8(now >> 10) * FIRESPEED;
		y[FIRENOISE] = 20 * now * FIRESPEED;
		z[FIRENOISE] = 5 * now * FIRESPEED;
		//scale_x[FIRENOISE] = scale8(ctrl1, FIRENOISESCALE);
		//scale_y[FIRENOISE] = scale8(ctrl2, FIRENOISESCALE);
		scale_x[FIRENOISE] = scale8(inoise8(now >> 8), FIRENOISESCALE);        
  		scale_y[FIRENOISE] = scale8(inoise8((now >> 8) + 1000), FIRENOISESCALE);
		//scale_x[FIRENOISE] = FIRENOISESCALE;
  		//scale_y[FIRENOISE] = FIRENOISESCALE;
		*/

		// parameters for the fire heat map
		x[FIRENOISE] = now * FIRESPEED / 4;        // Gentle horizontal drift
		y[FIRENOISE] = now * FIRESPEED;            // Main upward movement
		z[FIRENOISE] = now * FIRESPEED / 8;        // Slow time evolution      
		scale_x[FIRENOISE] = FIRENOISESCALE;       // Stable horizontal scale
		scale_y[FIRENOISE] = FIRENOISESCALE;       // Stable vertical scale 


		//calculate the perlin noise data for the fire
		for (uint8_t x_count = 0; x_count < WIDTH; x_count++) {
			uint32_t xoffset = scale_x[FIRENOISE] * (x_count - CentreX);
			for (uint8_t y_count = 0; y_count < HEIGHT; y_count++) {
				uint32_t yoffset = scale_y[FIRENOISE] * (y_count - CentreY);
				uint16_t data = ((inoise16(x[FIRENOISE] + xoffset, y[FIRENOISE] + yoffset, z[FIRENOISE])) + 1);
				noise[FIRENOISE][x_count][y_count] = data >> 8;
			}
		}

		/*
		// parameters for the smoke map
		x[SMOKENOISE] = 3 * ctrl * SMOKESPEED;
		y[SMOKENOISE] = 20 * now * SMOKESPEED;
		z[SMOKENOISE] = 5 * now * SMOKESPEED;
		scale_x[SMOKENOISE] = scale8(ctrl1, SMOKENOISESCALE);
		scale_y[SMOKENOISE] = scale8(ctrl2, SMOKENOISESCALE);
		*/

		// parameters for the smoke map
		x[SMOKENOISE] = now * SMOKESPEED / 4;
		y[SMOKENOISE] = now * SMOKESPEED;
		z[SMOKENOISE] = now * SMOKESPEED / 8;
		scale_x[SMOKENOISE] = SMOKENOISESCALE;
		scale_y[SMOKENOISE] = SMOKENOISESCALE;

		//calculate the perlin noise data for the smoke
		for (uint8_t x_count = 0; x_count < WIDTH; x_count++) {
			uint32_t xoffset = scale_x[SMOKENOISE] * (x_count - CentreX);
			for (uint8_t y_count = 0; y_count < HEIGHT; y_count++) {
			uint32_t yoffset = scale_y[SMOKENOISE] * (y_count - CentreY);
			uint16_t data = ((inoise16(x[SMOKENOISE] + xoffset, y[SMOKENOISE] + yoffset, z[SMOKENOISE])) + 1);
			noise[SMOKENOISE][x_count][y_count] = data / SMOKENOISE_DIMMER;
			}
		}

		//copy everything one line up
		for (uint8_t y = 0; y < HEIGHT - 1; y++) {
			for (uint8_t x = 0; x < WIDTH; x++) {
			heat[xyFunc(x, y)] = heat[xyFunc(x, y + 1)];
			}
		}

		// draw lowest line - seed the fire where it is brightest and hottest
		/*
		for (uint8_t x = 0; x < WIDTH; x++) {
			heat[xyFunc(x, HEIGHT-1)] = noise[FIRENOISE][x][x] + (sin8(x * 42) >> 2); // CentreX
			//if (heat[XY(x, HEIGHT-1)] < 200) heat[XY(x, HEIGHT-1)] = 150; 
		}
		*/
  		for (uint8_t x = 0; x < WIDTH; x++) {
      		uint8_t base_heat = noise[FIRENOISE][x][x] + (sin8(x * 42) >> 2) + 50;
      		heat[xyFunc(x, HEIGHT-1)] = MAX(base_heat, 80);  // Ensure minimum heat of 80
  		}


		// dim the flames based on FIRENOISE noise. 
		// if the FIRENOISE noise is strong, the led goes out fast
		// if the FIRENOISE noise is weak, the led stays on stronger.
		// once the heat is gone, it stays dark.
		for (uint8_t y = 0; y < HEIGHT - 1; y++) {
			for (uint8_t x = 0; x < WIDTH; x++) {
			uint8_t dim = noise[FIRENOISE][x][y];
			// high value in FLAMEHEIGHT = less dimming = high flames
			dim = dim / FLAMEHEIGHT;
			dim = 255 - dim;
			heat[xyFunc(x, y)] = scale8(heat[xyFunc(x, y)] , dim);

			// map the colors based on heatmap
			// use the heat map to set the color of the LED from the "hot" palette
			//                               whichpalette    position      brightness     blend or not
			leds[xyFunc(x, y)] = ColorFromPalette(hotPalette, heat[xyFunc(x, y)], heat[xyFunc(x, y)], LINEARBLEND);

			// dim the result based on SMOKENOISE noise
			// this is not saved in the heat map - the flame may dim away and come back
			// next iteration.
			leds[xyFunc(x, y)].nscale8(noise[SMOKENOISE][x][y]);

			}
		}
	} // Fire2023


	//******************************************************

	void runFire() {
	
		/*
		// Get the selected color palette
		//CRGBPalette16 myPal = JuJuPink;
		CRGBPalette16 myPal = getPalette();
		
		uint32_t now = millis(); 
		
		// Update the animation speed from the UI slider
		timeScale.setSpeed(speedY);
		
		// Calculate the current y-offset for animation (makes the fire move)
		uint32_t y_speed = timeScale.update(now);
		
		// Loop through every LED in our cylindrical matrix
		for (int width = 0; width < WIDTH; width++) {
			for (int height = 0; height < HEIGHT; height++) {
				// Calculate which color to use from our palette for this LED
				// This function handles the cylindrical mapping using sine/cosine
				uint8_t palette_index =
					getPaletteIndex(now, width, WIDTH, height, HEIGHT, y_speed);
				
				// Get the actual RGB color from the palette
				// BRIGHTNESS ensures we use the full brightness range
				CRGB c = ColorFromPalette(myPal, palette_index, BRIGHTNESS);
				
				// Convert our 2D coordinates to the 1D array index
				// We use (WIDTH-1)-width and (HEIGHT-1)-height to flip the coordinates
				// This makes the fire appear to rise from the bottom
				int index = xyFunc((WIDTH - 1) - width, (HEIGHT - 1) - height);
				
				// Set the LED color in our array
				leds[index] = c;
			}
		}
		*/

		EVERY_N_MILLISECONDS(8) {
			Fire2023(millis());
		}

		FastLED.show();

	} // runfire()

} // namespace fire



	







