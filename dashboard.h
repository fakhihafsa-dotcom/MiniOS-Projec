#pragma once

/*
 * Dashboard
 *
 * Renders a live OS simulation dashboard on the VGA screen.
 *
 * Screen Layout (80x25):
 * ┌──────────────────────────────────────────────────────────────────────────┐
 * │ Row 0  : Title bar                                                       │
 * │ Row 1  : System stats (Tick, Running PID, Free RAM, Used RAM)            │
 * │ Row 2  : Separator                                                       │
 * │ Row 3  : Process table header                                            │
 * │ Row 4-19: Process rows (max 16 processes)                                │
 * │ Row 20 : Separator                                                       │
 * │ Row 21 : Memory map (256 blocks = 256 pages)                             │
 * │ Row 22 : Memory map legend                                               │
 * │ Row 23 : Key hints                                                       │
 * │ Row 24 : Log / status line                                               │
 * └──────────────────────────────────────────────────────────────────────────┘
 */
namespace Dashboard {

    /*
     * init
     * Draws the static chrome (title bar, borders, column headers).
     * Call once at startup.
     */
    void init();

    /*
     * update
     * Redraws the dynamic portions (process rows, stats, memory map).
     * Call after every tick.
     */
    void update();

    /*
     * log
     * Writes a message to the status line at the bottom.
     */
    void log(const char* msg);

}
