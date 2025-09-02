#pragma once

#include "FastLED.h"
#include <math.h>

#define NUM_WARP_OSCILLATORS 6

namespace DomainWarper {

    struct WarpParameters {
        float scale_x = 0.1f;
        float scale_y = 0.1f; 
        float scale_z = 0.1f;
        float offset_x = 0.0f;
        float offset_y = 0.0f;
        float offset_z = 0.0f;
        float angle_offset = 0.0f;
        float dist_offset = 0.0f;
        float z = 0.0f;
        float low_limit = 0.0f;
        float high_limit = 1.0f;
    };

    struct Oscillator {
        float master_speed = 0.01f;
        float offset[NUM_WARP_OSCILLATORS];
        float ratio[NUM_WARP_OSCILLATORS]; 
        float linear[NUM_WARP_OSCILLATORS];
        float radial[NUM_WARP_OSCILLATORS];
        float directional[NUM_WARP_OSCILLATORS];
        float noise_angle[NUM_WARP_OSCILLATORS];
    };

    class DomainWarpFilter {
    private:
        int width, height;
        float **polar_theta;
        float **distance; 
        Oscillator osc;
        WarpParameters warp;
        
        static const uint8_t PERLIN_NOISE[];
        uint8_t P(uint8_t x);
        float fade(float t);
        float lerp(float t, float a, float b);
        float grad(int hash, float x, float y, float z);
        float pnoise(float x, float y, float z);
        
        void calculateOscillators();
        void renderPolarLookupTable(float cx, float cy);
        float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
        float renderWarpValue(float dist, float angle, float z_offset);
        
    public:
        DomainWarpFilter(int w, int h);
        ~DomainWarpFilter();
        
        void init();
        void setSpeed(float speed) { osc.master_speed = speed; }
        void setScale(float scale) { warp.scale_x = warp.scale_y = scale; }
        void setZoom(float zoom) { warp.scale_x = warp.scale_y = zoom; }
        
        // Apply domain warping transformation to input coordinates
        // Returns a warp intensity value 0-255 that can be used to modulate colors
        float applyWarp(uint8_t x, uint8_t y, uint32_t timeMs);
        
        // Apply warp as a color filter to existing LED array
        template<typename XYMapFunc>
        void applyWarpFilter(CRGB* leds, XYMapFunc xyMap, uint32_t timeMs, float intensity = 1.0f) {
            calculateOscillators();
            
            for (uint8_t x = 0; x < width; x++) {
                for (uint8_t y = 0; y < height; y++) {
                    uint16_t ledIndex = xyMap(x, y);
                    
                    float dist = distance[x][y] * cZoom;
                    float angle = polar_theta[x][y] * cAngle + warp.angle_offset + osc.radial[0];
                    float z = warp.z + warp.dist_offset * dist + osc.linear[0];
                    
                    float warpValue = renderWarpValue(dist, angle, z) * intensity / 255.0f;
                    
                    // Apply warp as a brightness/color modulator
                    leds[ledIndex].r = (uint8_t)(leds[ledIndex].r * warpValue);
                    leds[ledIndex].g = (uint8_t)(leds[ledIndex].g * warpValue);
                    leds[ledIndex].b = (uint8_t)(leds[ledIndex].b * warpValue);
                }
            }
        }
    };

    // Global instance that can be enabled/disabled per visualizer
    extern DomainWarpFilter* globalWarpFilter;
    extern bool warpFilterEnabled;
    
    void initGlobalWarpFilter(int width, int height);
    void enableWarpFilter(bool enable);
    bool isWarpFilterEnabled();

} // namespace DomainWarper