#include "dashboard.h"
#include "vga.h"
#include "../core/scheduler.h"
#include "../core/memory.h"

namespace Dashboard {

    using namespace VGA;

    // ── Layout Constants ───────────────────────────────────────────────────
    constexpr uint16_t ROW_TITLE    = 0;
    constexpr uint16_t ROW_STATS    = 1;
    constexpr uint16_t ROW_SEP1     = 2;
    constexpr uint16_t ROW_HDR      = 3;
    constexpr uint16_t ROW_PROC     = 4;   // Process rows start here
    constexpr uint16_t ROW_SEP2     = 20;
    constexpr uint16_t ROW_MEM      = 21;
    constexpr uint16_t ROW_MEMLEG   = 22;
    constexpr uint16_t ROW_KEYS     = 23;
    constexpr uint16_t ROW_LOG      = 24;

    // ── Helpers ────────────────────────────────────────────────────────────

    static void print_pad(const char* s, uint16_t col, uint16_t row,
                          uint8_t width, Color fg, Color bg) {
        uint8_t i = 0;
        uint16_t c = col;
        while (s[i] && i < width) {
            uint8_t attr = (uint8_t)((bg << 4) | (fg & 0x0F));
            volatile char* vmem = (volatile char*)0xB8000;
            uint32_t idx = (row * 80 + c) * 2;
            vmem[idx]   = s[i];
            vmem[idx+1] = attr;
            c++; i++;
        }
        // Pad remaining with spaces
        while (i < width) {
            uint8_t attr = (uint8_t)((bg << 4) | (fg & 0x0F));
            volatile char* vmem = (volatile char*)0xB8000;
            uint32_t idx = (row * 80 + c) * 2;
            vmem[idx]   = ' ';
            vmem[idx+1] = attr;
            c++; i++;
        }
    }

    static void put_char_at(char ch, uint16_t col, uint16_t row, Color fg, Color bg) {
        uint8_t attr = (uint8_t)((bg << 4) | (fg & 0x0F));
        volatile char* vmem = (volatile char*)0xB8000;
        uint32_t idx = (row * 80 + col) * 2;
        vmem[idx]   = ch;
        vmem[idx+1] = attr;
    }

    // Minimal itoa for in-place use
    static void uint_to_str(uint32_t n, char* buf, uint8_t min_width) {
        char tmp[12];
        int i = 0;
        if (n == 0) { tmp[i++] = '0'; }
        else { while (n > 0) { tmp[i++] = '0' + (n % 10); n /= 10; } }
        // Reverse
        int len = i;
        for (int j = 0; j < len; j++) buf[j] = tmp[len-1-j];
        // Pad left with spaces to min_width
        if (len < min_width) {
            // Shift right
            for (int j = len; j >= 0; j--) buf[j + (min_width - len)] = buf[j];
            for (int j = 0; j < min_width - len; j++) buf[j] = ' ';
            len = min_width;
        }
        buf[len] = '\0';
    }

    static const char* state_name(ProcessState s) {
        switch(s) {
            case NEW:     return "NEW    ";
            case READY:   return "READY  ";
            case RUNNING: return "RUNNING";
            case WAITING: return "WAITING";
            case DONE:    return "DONE   ";
            default:      return "???    ";
        }
    }

    static Color state_color(ProcessState s) {
        switch(s) {
            case RUNNING: return LIGHT_GREEN;
            case READY:   return YELLOW;
            case DONE:    return DARK_GREY;
            case WAITING: return LIGHT_CYAN;
            default:      return WHITE;
        }
    }

    static const char* priority_name(uint8_t p) {
        switch(p) {
            case 1: return "HIGH";
            case 2: return "MED ";
            case 3: return "LOW ";
            default: return "??? ";
        }
    }

    static Color priority_color(uint8_t p) {
        switch(p) {
            case 1: return LIGHT_RED;
            case 2: return YELLOW;
            case 3: return LIGHT_GREY;
            default: return WHITE;
        }
    }

    // ── Progress Bar ───────────────────────────────────────────────────────

    static void draw_progress_bar(uint16_t col, uint16_t row,
                                  uint32_t done, uint32_t total,
                                  uint8_t width, Color fg, Color bg) {
        if (total == 0) total = 1;
        uint32_t filled = (done * width) / total;
        for (uint8_t i = 0; i < width; i++) {
            if (i < filled)
                put_char_at((char)0xDB, col + i, row, fg, bg); // Solid block █
            else
                put_char_at((char)0xB0, col + i, row, DARK_GREY, bg); // Light shade ░
        }
    }

    // ── Public API ─────────────────────────────────────────────────────────

    void init() {
        // Clear entire screen to dark
        for (uint16_t r = 0; r < 25; r++)
            for (uint16_t c = 0; c < 80; c++)
                put_char_at(' ', c, r, WHITE, BLACK);

        // Row 0: Title bar (cyan on dark blue)
        for (uint16_t c = 0; c < 80; c++)
            put_char_at(' ', c, ROW_TITLE, WHITE, BLUE);
        print_at("  LiziOS  |  Process & Memory Manager  |  Priority Round Robin Scheduler",
                 0, ROW_TITLE, WHITE, BLUE);

        // Row 2: Separator
        draw_hline(ROW_SEP1, (char)0xCD, DARK_GREY, BLACK); // ═══

        // Row 3: Table header
        print_at("PID  NAME         STATE    PRI  BURST  LEFT   WAIT  PROGRESS         ",
                  0, ROW_HDR, LIGHT_CYAN, BLACK);

        // Row 20: Separator
        draw_hline(ROW_SEP2, (char)0xCD, DARK_GREY, BLACK);

        // Row 21 label
        print_at("RAM [", 0, ROW_MEM, LIGHT_GREY, BLACK);
        print_at("]", 75, ROW_MEM, LIGHT_GREY, BLACK);

        // Row 22: Legend
        print_at("[", 0, ROW_MEMLEG, LIGHT_GREY, BLACK);
        put_char_at((char)0xDB, 1, ROW_MEMLEG, GREEN, BLACK);
        print_at("]Used  [", 2, ROW_MEMLEG, LIGHT_GREY, BLACK);
        put_char_at((char)0xB0, 10, ROW_MEMLEG, DARK_GREY, BLACK);
        print_at("]Free", 11, ROW_MEMLEG, LIGHT_GREY, BLACK);

        // Row 23: Controls
        print_at("[SPACE] Next Tick   [A] Add Process   [R] Run All   [Q] Quit",
                  0, ROW_KEYS, DARK_GREY, BLACK);
    }

    void update() {
        char buf[16];

        // ── Row 1: Stats bar ─────────────────────────────────────────────
        for (uint16_t c = 0; c < 80; c++)
            put_char_at(' ', c, ROW_STATS, WHITE, DARK_GREY);

        uint_to_str(Scheduler::get_tick_count(), buf, 5);
        print_at("Tick:", 1, ROW_STATS, YELLOW, DARK_GREY);
        print_at(buf, 7, ROW_STATS, WHITE, DARK_GREY);

        uint32_t rpid = Scheduler::get_running_pid();
        print_at("  CPU:", 14, ROW_STATS, YELLOW, DARK_GREY);
        if (rpid == 0) {
            print_at("IDLE ", 21, ROW_STATS, LIGHT_CYAN, DARK_GREY);
        } else {
            print_at("PID ", 21, ROW_STATS, LIGHT_GREEN, DARK_GREY);
            uint_to_str(rpid, buf, 1);
            print_at(buf, 25, ROW_STATS, LIGHT_GREEN, DARK_GREY);
        }

        uint_to_str(MemoryManager::get_free_count(), buf, 4);
        print_at("  Free Pages:", 30, ROW_STATS, YELLOW, DARK_GREY);
        print_at(buf, 44, ROW_STATS, WHITE, DARK_GREY);

        uint_to_str(MemoryManager::get_used_count(), buf, 4);
        print_at("  Used:", 49, ROW_STATS, YELLOW, DARK_GREY);
        print_at(buf, 57, ROW_STATS, LIGHT_RED, DARK_GREY);

        print_at(" /256 pages", 61, ROW_STATS, DARK_GREY, DARK_GREY);

        // ── Rows 4-19: Process Table ─────────────────────────────────────
        uint8_t count = Scheduler::get_process_count();
        for (uint8_t i = 0; i < MAX_PROCESSES; i++) {
            uint16_t row = ROW_PROC + i;
            // Clear the row first
            for (uint16_t c = 0; c < 80; c++)
                put_char_at(' ', c, row, WHITE, BLACK);

            if (i >= count) continue;

            PCB* p = Scheduler::get_process(i);
            if (!p) continue;

            Color sc = state_color(p->state);

            // PID
            uint_to_str(p->pid, buf, 2);
            print_at(buf, 0, row, LIGHT_GREY, BLACK);

            // Name
            print_at(p->name, 5, row, WHITE, BLACK);

            // State
            print_at(state_name(p->state), 18, row, sc, BLACK);

            // Priority
            print_at(priority_name(p->priority), 27, row,
                     priority_color(p->priority), BLACK);

            // Burst
            uint_to_str(p->burst_time, buf, 4);
            print_at(buf, 32, row, LIGHT_GREY, BLACK);

            // Remaining
            uint_to_str(p->remaining, buf, 4);
            print_at(buf, 39, row, (p->remaining > 0 ? YELLOW : DARK_GREY), BLACK);

            // Wait
            uint_to_str(p->wait_time, buf, 4);
            print_at(buf, 46, row, LIGHT_CYAN, BLACK);

            // Progress bar (18 chars wide at col 52)
            if (p->burst_time > 0) {
                uint32_t done = p->burst_time - p->remaining;
                draw_progress_bar(52, row, done, p->burst_time, 18, sc, BLACK);
            }

            // Mark running with arrow
            if (p->state == RUNNING)
                put_char_at('>', 71, row, LIGHT_GREEN, BLACK);

            // Turnaround for DONE processes
            if (p->state == DONE) {
                print_at("T=", 73, row, DARK_GREY, BLACK);
                uint_to_str(p->turn_time, buf, 3);
                print_at(buf, 75, row, DARK_GREY, BLACK);
            }
        }

        // ── Row 21: Memory Map ───────────────────────────────────────────
        // 256 pages → show first 70 (fits in screen)
        for (uint8_t pg = 0; pg < 70; pg++) {
            char ch = MemoryManager::is_page_free(pg) ? (char)0xB0 : (char)0xDB;
            Color fc = MemoryManager::is_page_free(pg) ? DARK_GREY : GREEN;
            put_char_at(ch, 5 + pg, ROW_MEM, fc, BLACK);
        }
    }

    void log(const char* msg) {
        // Clear log row
        for (uint16_t c = 0; c < 80; c++)
            put_char_at(' ', c, ROW_LOG, WHITE, BLACK);
        print_at("> ", 0, ROW_LOG, LIGHT_GREEN, BLACK);
        print_at(msg, 2, ROW_LOG, WHITE, BLACK);
    }

}
