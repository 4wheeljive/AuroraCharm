#pragma once

#include "fire_detail.hpp"

namespace fire {
    extern bool fireInstance;
    
    void initFire(uint16_t (*xy_func)(uint8_t, uint8_t));
    void runFire();

} // namespace fire