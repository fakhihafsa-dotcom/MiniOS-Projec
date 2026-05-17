#pragma once
#include "../core/ktypes.h"

/*
 * VGA Terminal Driver
 *
 * The VGA text mode hardware maps a 80x25 grid of characters directly into memory.
 * Each cell is 2 bytes:
 *   Byte 0: ASCII character
 *   Byte 1: Color attribute (High nibble = background, Low nibble = foreground)
 *
 * Memory address: 0xB8000 (physical)
 */
namespace VGA {

    // --- Screen Dimensions ---
    constexpr uint16_t COLS  = 80;
    constexpr uint16_t ROWS  = 25;
    constexpr uint32_t VIDEO_MEMORY = 0xB8000;

    // --- Color Codes (matches VGA hardware palette) ---
    enum Color : uint8_t {
        BLACK         = 0,
        BLUE          = 1,
        GREEN         = 2,
        CYAN          = 3,
        RED           = 4,
        MAGENTA       = 5,
        BROWN         = 6,
        LIGHT_GREY    = 7,
        DARK_GREY     = 8,
        LIGHT_BLUE    = 9,
        LIGHT_GREEN   = 10,
        LIGHT_CYAN    = 11,
        LIGHT_RED     = 12,
        LIGHT_MAGENTA = 13,
        YELLOW        = 14,
        WHITE         = 15,
    };

    /*
     * init
     * Clears the screen and resets the cursor to (0,0).
     */
    void init();

    /*
     * clear
     * Fills the entire screen with spaces using the current color.
     */
    void clear();

    /*
     * set_color
     * Sets the foreground and background color for subsequent print calls.
     */
    void set_color(Color fg, Color bg);

    /*
     * print_char
     * Prints a single character at the current cursor position.
     * Handles '\n' for newline and auto-scrolls when the screen is full.
     */
    void print_char(char c);

    /*
     * print
     * Prints a null-terminated C-style string.
     */
    void print(const char* str);

    /*
     * print_int
     * Prints an unsigned 32-bit integer in decimal.
     */
    void print_int(uint32_t n);

    /*
     * print_hex
     * Prints a 32-bit integer in hexadecimal with "0x" prefix.
     */
    void print_hex(uint32_t n);

    /*
     * print_at
     * Prints a string at a specific (col, row) position without moving the cursor.
     * Useful for drawing UI panels that don't disrupt the scrolling log.
     */
    void print_at(const char* str, uint16_t col, uint16_t row, Color fg, Color bg);

    /*
     * print_int_at
     * Prints an integer at a specific position.
     */
    void print_int_at(uint32_t n, uint16_t col, uint16_t row, Color fg, Color bg);

    /*
     * draw_hline
     * Draws a horizontal line of a given character across the screen.
     */
    void draw_hline(uint16_t row, char c, Color fg, Color bg);

    /*
     * get_cursor_row
     * Returns the current row of the software cursor.
     */
    uint16_t get_cursor_row();

}
