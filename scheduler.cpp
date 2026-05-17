#include "scheduler.h"
#include "memory.h"

namespace Scheduler {

    // ─────────────────────────────────────────────────────────────────────────
    // Internal State
    // ─────────────────────────────────────────────────────────────────────────

    static PCB      process_table[MAX_PROCESSES]; // All PCBs
    static uint8_t  process_count  = 0;           // How many PCBs are active
    static uint32_t next_pid       = 1;           // PID counter (starts at 1)
    static uint32_t tick_count     = 0;           // Global clock
    static int32_t  running_index  = -1;          // Index of currently RUNNING process (-1 = idle)

    // ─────────────────────────────────────────────────────────────────────────
    // Internal Helpers
    // ─────────────────────────────────────────────────────────────────────────

    /*
     * copy_name
     * Copies at most MAX_NAME_LEN-1 characters from src to dst, null-terminating.
     * Replaces strncpy (no std lib available).
     */
    static void copy_name(char* dst, const char* src) {
        int i = 0;
        while (i < MAX_NAME_LEN - 1 && src[i] != '\0') {
            dst[i] = src[i];
            i++;
        }
        dst[i] = '\0';
    }

    /*
     * pick_next
     * THE SCHEDULER ALGORITHM: Priority-based Round Robin.
     *
     * Scans priority queues 1→2→3. Within each priority, starts the search
     * AFTER the last running index (Round Robin fairness).
     *
     * Returns the index of the next READY process to run, or -1 if none.
     */
    static int32_t pick_next() {
        // Try each priority level from highest (1) to lowest (3)
        for (uint8_t priority = 1; priority <= NUM_PRIORITIES; priority++) {

            // Round Robin: start searching AFTER the current running index
            // so we rotate through processes at the same priority fairly.
            int32_t start = (running_index >= 0) ? running_index : 0;

            // Search from start+1 wrapping around
            for (uint8_t offset = 1; offset <= process_count; offset++) {
                int32_t idx = (start + offset) % process_count;
                PCB* p = &process_table[idx];

                if (p->state == READY && p->priority == priority) {
                    return idx; // Found: highest-priority, next-in-rotation
                }
            }

            // Also check processes before 'start' at this priority
            // (handles the case where all READY processes are before current index)
            for (int32_t idx = 0; idx <= start && idx < process_count; idx++) {
                PCB* p = &process_table[idx];
                if (p->state == READY && p->priority == priority) {
                    return idx;
                }
            }
        }
        return -1; // No READY process found — CPU is idle
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Public API
    // ─────────────────────────────────────────────────────────────────────────

    void init() {
        process_count = 0;
        next_pid      = 1;
        tick_count    = 0;
        running_index = -1;
        for (uint8_t i = 0; i < MAX_PROCESSES; i++) {
            process_table[i].state = DONE; // All slots empty
        }
    }

    uint32_t create_process(const char* name, uint32_t burst_time, uint8_t priority) {
        if (process_count >= MAX_PROCESSES) return 0xFFFFFFFF;

        // Clamp priority to valid range
        if (priority < 1) priority = 1;
        if (priority > NUM_PRIORITIES) priority = NUM_PRIORITIES;

        // Allocate a physical memory page for this process
        uint32_t page = MemoryManager::alloc_page();
        if (page == 0xFFFFFFFF) return 0xFFFFFFFF; // Out of memory

        // Fill the PCB
        PCB* p             = &process_table[process_count];
        p->pid             = next_pid++;
        p->state           = READY;
        p->priority        = priority;
        p->page_num        = page;
        p->pc              = MemoryManager::get_page_address(page); // Simulated PC = page base
        p->sp              = p->pc + MemoryManager::PAGE_SIZE - 4;  // Stack at top of page
        p->burst_time      = burst_time;
        p->remaining       = burst_time;
        p->wait_time       = 0;
        p->turn_time       = 0;
        p->ticks_this_slice = 0;
        copy_name(p->name, name);

        process_count++;
        return p->pid;
    }

    /*
     * dispatch — THE DISPATCHER
     *
     * This is called whenever the scheduler decides a context switch is needed.
     * It saves the state of the current process and loads the state of the next.
     *
     * In a real OS, this would use assembly to save/restore actual CPU registers.
     * Here we simulate the concept by updating our PCB fields.
     */
    void dispatch() {
        // 1. SAVE CONTEXT of the currently running process
        if (running_index >= 0) {
            PCB* current = &process_table[running_index];

            if (current->state == RUNNING) {
                if (current->remaining == 0) {
                    // Process finished: calculate turnaround time
                    current->state     = DONE;
                    current->turn_time = current->wait_time + current->burst_time;
                    // Free its memory page
                    MemoryManager::free_page(current->page_num);
                } else {
                    // Preempted by Round Robin timeout: move back to READY
                    current->state = READY;
                    current->ticks_this_slice = 0; // Reset quantum counter
                }
            }
        }

        // 2. SELECT NEXT process (Scheduler picks)
        int32_t next_index = pick_next();

        // 3. LOAD CONTEXT of the next process
        if (next_index >= 0) {
            PCB* next        = &process_table[next_index];
            next->state      = RUNNING;
            next->ticks_this_slice = 0;
            running_index    = next_index;
        } else {
            // CPU is idle — no READY processes
            running_index = -1;
        }
    }

    void tick() {
        tick_count++;

        // Increment wait_time for all READY processes
        for (uint8_t i = 0; i < process_count; i++) {
            if (process_table[i].state == READY) {
                process_table[i].wait_time++;
            }
        }

        // Run the current process for 1 tick
        if (running_index >= 0) {
            PCB* running = &process_table[running_index];

            if (running->state == RUNNING && running->remaining > 0) {
                running->remaining--;
                running->ticks_this_slice++;

                // Simulate PC advancing (one "instruction" per tick)
                running->pc += 4;

                // Check if process is done
                if (running->remaining == 0) {
                    dispatch(); // Process finished → switch to next
                    return;
                }

                // Check if time quantum expired (Round Robin preemption)
                if (running->ticks_this_slice >= TIME_QUANTUM) {
                    dispatch(); // Quantum expired → preempt
                }
            }
        } else {
            // CPU was idle, try to dispatch something
            dispatch();
        }
    }

    PCB* get_process(uint8_t index) {
        if (index >= process_count) return nullptr;
        return &process_table[index];
    }

    uint8_t get_process_count() { return process_count; }

    uint32_t get_running_pid() {
        if (running_index < 0) return 0;
        return process_table[running_index].pid;
    }

    uint32_t get_tick_count() { return tick_count; }

    bool all_done() {
        for (uint8_t i = 0; i < process_count; i++) {
            if (process_table[i].state != DONE) return false;
        }
        return true;
    }

}
