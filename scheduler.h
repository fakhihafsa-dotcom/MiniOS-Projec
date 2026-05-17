#pragma once
#include "../core/ktypes.h"

/*
 * Process Control Block (PCB) & Process Scheduler
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * PROCESS CONTROL BLOCK (PCB)
 * ─────────────────────────────────────────────────────────────────────────────
 * The PCB is the kernel's data structure that fully describes a process.
 * It is the "identity card" of every process in the system.
 *
 * Fields:
 *   pid        — Unique Process ID assigned by the kernel (monotonically increasing)
 *   name       — Human-readable name for display purposes
 *   state      — Current lifecycle state (see ProcessState enum)
 *   priority   — Scheduling priority (1=HIGH, 2=MEDIUM, 3=LOW)
 *   page_num   — The physical page number allocated for this process's data
 *   pc         — Program Counter: the simulated instruction pointer
 *   sp         — Stack Pointer: simulated stack pointer
 *   burst_time — Total CPU time (ticks) this process needs to complete
 *   remaining  — CPU time still needed (counts down to 0 = done)
 *   wait_time  — Total time this process has spent waiting in the READY queue
 *   turn_time  — Total time from creation to completion (wait + burst)
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * SCHEDULER — Round Robin with Priority
 * ─────────────────────────────────────────────────────────────────────────────
 * Algorithm chosen: Priority-based Round Robin
 *   - Processes are divided into 3 priority levels (1=HIGH, 2=MEDIUM, 3=LOW).
 *   - Within each priority level, Round Robin is used (equal time slices).
 *   - Higher priority queues are always drained before lower priority ones.
 *   - Time Quantum = 3 ticks (configurable via TIME_QUANTUM constant).
 *
 * Why this algorithm?
 *   - Round Robin alone: fair but ignores urgency (bad for real OS work).
 *   - Priority alone: can starve low-priority processes.
 *   - Priority + RR: balances urgency with fairness. Used in real OSes (Linux CFS,
 *     Windows Task Scheduler all use priority-based preemption under the hood).
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * DISPATCHER
 * ─────────────────────────────────────────────────────────────────────────────
 * The Dispatcher is the low-level mechanism that performs the actual context switch.
 * While the Scheduler DECIDES which process runs next, the Dispatcher DOES IT.
 *
 * Responsibilities:
 *   1. Save the context (PC, SP) of the currently running process.
 *   2. Load the context of the next selected process.
 *   3. Update process states (RUNNING → READY, READY → RUNNING).
 *   4. Track wait time and turnaround time statistics.
 */

// --- Process States (Lifecycle) ---
enum ProcessState : uint8_t {
    NEW     = 0,  // Just created, not yet in the ready queue
    READY   = 1,  // Waiting in the ready queue for CPU time
    RUNNING = 2,  // Currently executing on the CPU
    WAITING = 3,  // Blocked waiting for I/O or an event
    DONE    = 4   // Finished execution, resources being freed
};

// --- Maximum constants ---
constexpr uint8_t  MAX_PROCESSES  = 16;   // Maximum processes at one time
constexpr uint8_t  MAX_NAME_LEN   = 12;   // Max characters in a process name
constexpr uint8_t  NUM_PRIORITIES = 3;    // 1=HIGH, 2=MEDIUM, 3=LOW
constexpr uint8_t  TIME_QUANTUM   = 3;    // Ticks per time slice (Round Robin)

/*
 * PCB — Process Control Block
 */
struct PCB {
    uint32_t      pid;                // Unique Process ID
    char          name[MAX_NAME_LEN]; // Human-readable name
    ProcessState  state;              // Current lifecycle state
    uint8_t       priority;           // 1 (HIGH) → 2 (MEDIUM) → 3 (LOW)
    uint32_t      page_num;           // Physical memory page allocated

    // Simulated CPU Context
    uint32_t      pc;                 // Program Counter (simulated)
    uint32_t      sp;                 // Stack Pointer (simulated)

    // Scheduling Statistics
    uint32_t      burst_time;         // Total CPU time needed (ticks)
    uint32_t      remaining;          // Remaining CPU time (counts down)
    uint32_t      wait_time;          // Total time spent waiting in READY
    uint32_t      turn_time;          // Total turnaround time (wait + burst)
    uint32_t      ticks_this_slice;   // Ticks used in current time quantum
};

/*
 * Scheduler & Dispatcher Namespace
 */
namespace Scheduler {

    /*
     * init
     * Clears all process slots and resets the system tick counter.
     */
    void init();

    /*
     * create_process
     * Creates a new PCB, allocates a memory page, and adds it to the ready queue.
     *   name       — display name (max 11 chars)
     *   burst_time — total CPU ticks this process needs
     *   priority   — 1 (HIGH), 2 (MEDIUM), 3 (LOW)
     * Returns the new PID, or 0xFFFFFFFF on failure.
     */
    uint32_t create_process(const char* name, uint32_t burst_time, uint8_t priority);

    /*
     * tick
     * Advances the simulation by one clock tick.
     *   - Runs the currently RUNNING process for 1 tick.
     *   - Checks if its time quantum has expired (Round Robin preemption).
     *   - Checks if the process has finished (remaining == 0).
     *   - Increments wait_time for all READY processes.
     *   - Calls the Dispatcher if a context switch is needed.
     */
    void tick();

    /*
     * dispatch
     * THE DISPATCHER.
     * Saves context of the current process, loads context of the next,
     * and performs the state transitions.
     */
    void dispatch();

    /*
     * get_process
     * Returns a pointer to the PCB at the given index (for display).
     */
    PCB* get_process(uint8_t index);

    /*
     * get_process_count
     * Returns the total number of processes (all states).
     */
    uint8_t get_process_count();

    /*
     * get_running_pid
     * Returns the PID of the currently running process, or 0 if idle.
     */
    uint32_t get_running_pid();

    /*
     * get_tick_count
     * Returns the total number of clock ticks elapsed.
     */
    uint32_t get_tick_count();

    /*
     * all_done
     * Returns true when every process has reached the DONE state.
     */
    bool all_done();

}
