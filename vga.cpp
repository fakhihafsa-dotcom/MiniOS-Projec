#include "vga.h"

namespace VGA {

    // --- Driver State ---
    static volatile char* const vmem = (volatile char*)VIDEO_MEMORY;
    static uint16_t cursor_col = 0;
    static uint16_t cursor_row = 0;
    static uint8_t  current_color = 0x0F; // Default: White on Black

    /*
     * make_color_attr
     * Packs foreground + background into a single attribute byte.
     * Format: [BG(4bits) | FG(4bits)]
     */
    static uint8_t make_color_attr(Color fg, Color bg) {
        return (uint8_t)((bg << 4) | (fg & 0x0F));
    }

    /*
     * put_cell
     * Directly writes a character and its color attribute into VGA memory.
     */
    static void put_cell(uint16_t col, uint16_t row, char c, uint8_t attr) {
        uint32_t index = (row * COLS + col) * 2;
        vmem[index]     = c;
        vmem[index + 1] = attr;
    }

    /*
     * scroll
     * Moves every row up by one, clearing the bottom row.
     * Called automatically when the cursor passes the last row.
     */
    static void scroll() {
        // Move rows 1..ROWS-1 up to rows 0..ROWS-2
        for (uint16_t row = 1; row < ROWS; row++) {
            for (uint16_t col = 0; col < COLS; col++) {
                uint32_t src = (row * COLS + col) * 2;
                uint32_t dst = ((row - 1) * COLS + col) * 2;
                vmem[dst]     = vmem[src];
                vmem[dst + 1] = vmem[src + 1];
            }
        }
        // Clear the last row
        for (uint16_t col = 0; col < COLS; col++) {
            put_cell(col, ROWS - 1, ' ', current_color);
        }
        // Keep cursor on the last row
        cursor_row = ROWS - 1;
    }

    // --- Public API ---

    void init() {
        current_color = make_color_attr(WHITE, BLACK);
        clear();
    }

    void clear() {
        for (uint16_t row = 0; row < ROWS; row++)
            for (uint16_t col = 0; col < COLS; col++)
                put_cell(col, row, ' ', current_color);
        cursor_col = 0;
        cursor_row = 0;
    }

    void set_color(Color fg, Color bg) {
        current_color = make_color_attr(fg, bg);
    }

    void print_char(char c) {
        if (c == '\n') {
            cursor_col = 0;
            cursor_row++;
        } else {
            put_cell(cursor_col, cursor_row, c, current_color);
            cursor_col++;
            if (cursor_col >= COLS) {
                cursor_col = 0;
                cursor_row++;
            }
        }
        if (cursor_row >= ROWS) {
            scroll();
        }
    }

    void print(const char* str) {
        for (int i = 0; str[i] != '\0'; i++)
            print_char(str[i]);
    }

    void print_int(uint32_t n) {
        if (n == 0) { print_char('0'); return; }
        char buf[12];
        int i = 0;
        while (n > 0) {
            buf[i++] = '0' + (n % 10);
            n /= 10;
        }
        // Reverse
        for (int j = i - 1; j >= 0; j--)
            print_char(buf[j]);
    }

    void print_hex(uint32_t n) {
        const char* hex_chars = "0123456789ABCDEF";
        print("0x");
        for (int i = 7; i >= 0; i--)
            print_char(hex_chars[(n >> (i * 4)) & 0xF]);
    }

    void print_at(const char* str, uint16_t col, uint16_t row, Color fg, Color bg) {
        uint8_t attr = make_color_attr(fg, bg);
        for (int i = 0; str[i] != '\0'; i++) {
            if (col < COLS)
                put_cell(col++, row, str[i], attr);
        }
    }

    void print_int_at(uint32_t n, uint16_t col, uint16_t row, Color fg, Color bg) {
        char buf[12];
        int len = 0;
        if (n == 0) {
            buf[len++] = '0';
        } else {
            char tmp[12];
            int i = 0;
            while (n > 0) { tmp[i++] = '0' + (n % 10); n /= 10; }
            for (int j = i - 1; j >= 0; j--) buf[len++] = tmp[j];
        }
        buf[len] = '\0';
        print_at(buf, col, row, fg, bg);
    }

    void draw_hline(uint16_t row, char c, Color fg, Color bg) {
        uint8_t attr = make_color_attr(fg, bg);
        for (uint16_t col = 0; col < COLS; col++)
            put_cell(col, row, c, attr);
    }

    uint16_t get_cursor_row() {
        return cursor_row;
    }

}
