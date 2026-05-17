#pragma once
#include "ktypes.h"

namespace Math {

    /*
     * Rounds a floating point number to the nearest integer.
     * * Since we are in a kernel without a standard library (no <cmath>),
     * and possibly without hardware Floating Point Unit (FPU) support enabled 
     * at this stage, we handle the floating point math using integer bitwise operations.
     */
    int round(float x);

}