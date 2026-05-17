#include "math.h"

namespace Math {

    /*
     * round
     * Converts a raw float to an int using bitwise manipulation of IEEE 754 standards.
     * * IEEE 754 Single Precision (32-bit) layout:
     * Bit 31:     Sign (0 = positive, 1 = negative)
     * Bits 30-23: Exponent (8 bits) - Bias is 127
     * Bits 22-0:  Mantissa (23 bits) - The fractional part
     */
    int round(float x) {
        // We use a union to inspect the raw bits of the float.
        // This allows us to view the same 32 bits of memory as either a float or a uint32_t.
        union {
            float as_float;
            uint32_t as_int;
        } val;
        
        val.as_float = x;
        uint32_t input = val.as_int;

        int result;
        
        // 1. Extract the Exponent
        // Shift right by 23 to move bits 30-23 to the bottom.
        // Mask with 0xFF (11111111) to isolate the 8 exponent bits.
        // Subtract 126? Standard bias is 127 for 2^0. 
        // We use 126 here effectively to "pre-calculate" a shift by 1 later or handle rounding logic.
        int exponent = (int)((input >> 23) & 0xFF) - 126;
        
        // 2. Extract the Mantissa
        // Mask with 0x7FFFFF (23 ones) to get the fraction bits.
        // OR with (1 << 23) to add the "Implicit Leading 1" (IEEE 754 assumes 1.xxxxx).
        // We shift left by 8 to align it for integer extraction.
        uint32_t mantissa = ((0x00000001 << 23) | (input & 0x7fffff)) << 8;
        
        // Case A: The number is very small (less than 0.5)
        if(exponent < 0){
            return 0;
        }
        // Case B: The number is essentially 1.0 (or close enough to round to it)
        else if(exponent == 0) {
            return 1;
        }
        // Case C: The number is too large to fit in our specific bitwise shift logic
        // (Exponent > 24 means the number is > 16 million, precision loss makes integer cast safe enough)
        else if(exponent > 24) {
            return (int)x;
        }
        // Case D: Normal range - Calculate the integer value
        else {
            // Shift the mantissa to the right based on the exponent to get the whole number part
            result = mantissa >> (32 - exponent);
            
            // Check the last bit shifted out to see if we need to round up
            result += (mantissa >> (31 - exponent)) & 1;
            
            return result;
        }
    }
}