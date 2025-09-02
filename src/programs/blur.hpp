#pragma once

//#include "fx/fx2d.h"
#include "blur_detail.hpp"


namespace blur {
    extern bool blurInstance;
    
    void initBlur(XYMap& myXYmap, XYMap& xyRect);
    void runBlur();

    
}
