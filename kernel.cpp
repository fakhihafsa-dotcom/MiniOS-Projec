#include "core/ktypes.h"
#include "core/portio.h"
#include "core/interrupts.h"
#include "core/pic.h"
#include "core/memory.h"
#include "core/scheduler.h"
#include "drivers/keyboard.h"
#include "drivers/vga.h"
#include "drivers/dashboard.h"

using namespace PortIO;

// ─────────────────────────────────────────────────────────────────────────────
// Kernel Entry Point
// ─────────────────────────────────────────────────────────────────────────────

extern "C" int kmain()
{
    // ── 1. Hardware Initialization ────────────────────────────────────────
    InterruptManager::init();   // Set up the Interrupt Descriptor Table (IDT)
    PIC::remap(32, 40);         // Remap hardware IRQs to vectors 32-47
    Keyboard::init();           // Install keyboard driver (IRQ1)
    enable_interrupts();        // Enable hardware interrupts (STI)

    // ── 2. Subsystem Initialization ───────────────────────────────────────
    VGA::init();                // Clear screen, set default colors
    MemoryManager::init();      // Initialize physical page allocator (256 pages)
    Scheduler::init();          // Initialize process table and dispatcher

    // ── 3. Draw the Dashboard Shell ───────────────────────────────────────
    Dashboard::init();

    // ── 4. Pre-load demo processes ────────────────────────────────────────
    //
    //  Process         Burst  Priority   Demonstrates
    //  --------------- -----  --------   -----------------------------------------
    //  SystemInit       6     HIGH(1)    First to finish - boot task
    //  ClockDaemon     12     HIGH(1)    Long high-priority (Round Robin w/ above)
    //  MemScanner       9     MEDIUM(2)  Medium bursts
    //  NetDriver        6     MEDIUM(2)  Competes with MemScanner at same priority
    //  Logger          15     LOW(3)     Long low-priority - runs when others yield
    //  IdleTask         3     LOW(3)     Short filler task
    //
    Scheduler::create_process("SystemInit",  6,  1);
    Scheduler::create_process("ClockDaemon", 12, 1);
    Scheduler::create_process("MemScanner",  9,  2);
    Scheduler::create_process("NetDriver",   6,  2);
    Scheduler::create_process("Logger",      15, 3);
    Scheduler::create_process("IdleTask",    3,  3);

    // Kick off the first dispatch
    Scheduler::dispatch();
    Dashboard::update();
    Dashboard::log("System ready. SPACE=step  A=add process  R=run all  Q=quit");

    // ── 5. Main Event Loop ────────────────────────────────────────────────
    char c;
    while (true)
    {
        c = Keyboard::get_char();

        if (c == ' ') {
            // SPACE: single tick step
            if (!Scheduler::all_done()) {
                Scheduler::tick();
                Dashboard::update();
                Dashboard::log("Tick advanced. SPACE=step  R=run all  A=add process");
            } else {
                Dashboard::log("All processes DONE. Press A to add more, or Q to quit.");
            }
        }
        else if (c == 'r' || c == 'R') {
            // R: run all remaining ticks automatically
            Dashboard::log("Running all ticks...");
            while (!Scheduler::all_done()) {
                Scheduler::tick();
                for (volatile int i = 0; i < 5000000; i++) {} // Small visual delay
                Dashboard::update();
            }
            Dashboard::log("All processes finished! Check WAIT and T= turnaround times.");
        }
        else if (c == 'a' || c == 'A') {
            // A: add a new process (cycles through pre-defined specs)
            static uint8_t add_idx = 0;

            struct ProcSpec { const char* name; uint32_t burst; uint8_t prio; };
            static const ProcSpec specs[] = {
                { "UserShell",   8, 2 },
                { "FileSync",   10, 3 },
                { "CryptoTask",  5, 1 },
                { "VideoProc",  14, 2 },
                { "BackupJob",   7, 3 },
                { "Compiler",   20, 2 },
                { "Watchdog",    4, 1 },
                { "Analytics",  18, 3 },
            };
            constexpr uint8_t NUM_SPECS = 8;

            if (Scheduler::get_process_count() < MAX_PROCESSES) {
                const ProcSpec& sp = specs[add_idx % NUM_SPECS];
                uint32_t pid = Scheduler::create_process(sp.name, sp.burst, sp.prio);
                add_idx++;

                if (pid != 0xFFFFFFFF) {
                    Dashboard::update();
                    Dashboard::log("New process added and queued!");
                } else {
                    Dashboard::log("Failed: out of memory.");
                }
            } else {
                Dashboard::log("Process table full (max 16 processes).");
            }
        }
        else if (c == 'q' || c == 'Q') {
            // Q: halt
            Dashboard::log("System halted. CPU stopped.");
            disable_interrupts();
            __asm__("hlt");
        }
    }

    return 0;
}
