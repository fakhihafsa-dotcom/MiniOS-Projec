#pragma once

/*
 * KTypes (Kernel Types)
 * * In a standard C++ program, you would include <cstdint> to get these types.
 * However, we are writing an Operating System kernel running on "bare metal".
 * There is no standard library available yet, so we must define these 
 * fixed-width integer types ourselves to ensure exact memory layout.
 */

// 8-bit unsigned integer (0 to 255) - Often used for characters or byte buffers
using uint8_t = unsigned char;

// 16-bit unsigned integer (0 to 65,535) - Used for port addresses and short values
using uint16_t = unsigned short;

// 32-bit unsigned integer (0 to 4,294,967,295) - The standard "word" size on x86
using uint32_t = unsigned int;

// 64-bit unsigned integer - Used for large memory addresses or high-precision counters
using uint64_t = unsigned long long; // Note: 'unsigned long' is 32-bit on some 32-bit systems, 'long long' ensures 64-bit.