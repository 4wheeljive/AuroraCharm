#include "domainWarper.h"
#include "bleControl.h"

namespace DomainWarper {

    DomainWarpFilter* globalWarpFilter = nullptr;
    bool warpFilterEnabled = false;

    const uint8_t DomainWarpFilter::PERLIN_NOISE[] = {
        151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,
        225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,  190,
        6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117,
        35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
        171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158,
        231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,  46,
        245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209,
        76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
        164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,
        202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,
        58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,
        154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
        19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
        228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,
        145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184,
        84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
        222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156,
        180
    };

    DomainWarpFilter::DomainWarpFilter(int w, int h) : width(w), height(h) {
        // Allocate polar coordinate lookup tables
        polar_theta = new float*[width];
        distance = new float*[width];
        for (int x = 0; x < width; x++) {
            polar_theta[x] = new float[height];
            distance[x] = new float[height];
        }

        // Initialize oscillator parameters to defaults
        for (int i = 0; i < NUM_WARP_OSCILLATORS; i++) {
            osc.offset[i] = i * 100.0f;
            osc.ratio[i] = 1.0f + i * 0.2f;
            osc.linear[i] = 0.0f;
            osc.radial[i] = 0.0f;
            osc.directional[i] = 0.0f;
            osc.noise_angle[i] = 0.0f;
        }
    }

    DomainWarpFilter::~DomainWarpFilter() {
        for (int x = 0; x < width; x++) {
            delete[] polar_theta[x];
            delete[] distance[x];
        }
        delete[] polar_theta;
        delete[] distance;
    }

    void DomainWarpFilter::init() {
        renderPolarLookupTable((width / 2.0f) - 0.5f, (height / 2.0f) - 0.5f);
    }

    uint8_t DomainWarpFilter::P(uint8_t x) {
        const uint8_t idx = x & 255;
        return PERLIN_NOISE[idx];
    }

    float DomainWarpFilter::fade(float t) { 
        return t * t * t * (t * (t * 6 - 15) + 10); 
    }

    float DomainWarpFilter::lerp(float t, float a, float b) { 
        return a + t * (b - a); 
    }

    float DomainWarpFilter::grad(int hash, float x, float y, float z) {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    float DomainWarpFilter::pnoise(float x, float y, float z) {
        int X = (int)floorf(x) & 255;
        int Y = (int)floorf(y) & 255; 
        int Z = (int)floorf(z) & 255;
        x -= floorf(x);
        y -= floorf(y);
        z -= floorf(z);
        float u = fade(x);
        float v = fade(y);
        float w = fade(z);
        int A = P(X) + Y, AA = P(A) + Z, AB = P(A + 1) + Z;
        int B = P(X + 1) + Y, BA = P(B) + Z, BB = P(B + 1) + Z;

        return lerp(w,
                    lerp(v,
                         lerp(u, grad(P(AA), x, y, z),
                              grad(P(BA), x - 1, y, z)),
                         lerp(u, grad(P(AB), x, y - 1, z),
                              grad(P(BB), x - 1, y - 1, z))),
                    lerp(v,
                         lerp(u, grad(P(AA + 1), x, y, z - 1),
                              grad(P(BA + 1), x - 1, y, z - 1)),
                         lerp(u, grad(P(AB + 1), x, y - 1, z - 1),
                              grad(P(BB + 1), x - 1, y - 1, z - 1))));
    }

    void DomainWarpFilter::calculateOscillators() {
        double runtime = millis() * osc.master_speed * cSpeed;

        for (int i = 0; i < NUM_WARP_OSCILLATORS; i++) {
            osc.linear[i] = (runtime + osc.offset[i]) * osc.ratio[i];
            osc.radial[i] = fmodf(osc.linear[i], 2 * PI);
            osc.directional[i] = sinf(osc.radial[i]);
            osc.noise_angle[i] = PI * (1 + pnoise(osc.linear[i], 0, 0));
        }
    }

    void DomainWarpFilter::renderPolarLookupTable(float cx, float cy) {
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                float dx = x - cx;
                float dy = y - cy;
                distance[x][y] = hypotf(dx, dy);
                polar_theta[x][y] = atan2f(dy, dx);
            }
        }
    }

    float DomainWarpFilter::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
        float result = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
        if (result < out_min) result = out_min;
        if (result > out_max) result = out_max;
        return result;
    }

    float DomainWarpFilter::renderWarpValue(float dist, float angle, float z_offset) {
        // Convert polar coordinates back to cartesian with transformations
        float center_x = (width / 2.0f) - 0.5f;
        float center_y = (height / 2.0f) - 0.5f;
        
        float newx = (warp.offset_x + center_x - (cosf(angle) * dist)) * warp.scale_x;
        float newy = (warp.offset_y + center_y - (sinf(angle) * dist)) * warp.scale_y;
        float newz = (warp.offset_z + z_offset) * warp.scale_z;

        // Get noise value at transformed coordinates
        float raw_noise = pnoise(newx, newy, newz);

        // Apply contrast/brightness limits
        if (raw_noise < warp.low_limit) raw_noise = warp.low_limit;
        if (raw_noise > warp.high_limit) raw_noise = warp.high_limit;

        // Scale to 0-255 range
        return mapFloat(raw_noise, warp.low_limit, warp.high_limit, 0, 255);
    }

    float DomainWarpFilter::applyWarp(uint8_t x, uint8_t y, uint32_t timeMs) {
        if (x >= width || y >= height) return 0;
        
        calculateOscillators();
        
        float dist = distance[x][y] * cZoom;
        float angle = polar_theta[x][y] * cAngle + warp.angle_offset + osc.radial[0];
        float z = warp.z + warp.dist_offset * dist + osc.linear[0];
        
        return renderWarpValue(dist, angle, z);
    }


    void initGlobalWarpFilter(int width, int height) {
        if (globalWarpFilter) {
            delete globalWarpFilter;
        }
        globalWarpFilter = new DomainWarpFilter(width, height);
        globalWarpFilter->init();
    }

    void enableWarpFilter(bool enable) {
        warpFilterEnabled = enable && globalWarpFilter != nullptr;
    }

    bool isWarpFilterEnabled() {
        return warpFilterEnabled;
    }

} // namespace DomainWarper