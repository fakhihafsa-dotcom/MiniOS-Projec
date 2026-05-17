#pragma once
#include "ktypes.h"

// Define constants using C++ constexpr instead of #define for type safety
constexpr uint16_t IDT_MAX_DESCRIPTORS = 256;
constexpr uint16_t IDT_INTEL_DESCRIPTORS = 32;
constexpr uint16_t IDT_PIC_DESCRIPTORS = 16;

/* * The IDT Entry (Interrupt Descriptor Table).
 * This structure represents a single entry in the table that tells the CPU 
 * where to jump when a specific interrupt occurs.
 * * Hardware Constraint: This structure MUST be exactly 8 bytes and packed.
 */
struct __attribute__((packed)) idt_entry_t {
    uint16_t    isr_low;      // The lower 16 bits of the function address to jump to.
    uint16_t    kernel_cs;    // The Code Segment Selector. Tells CPU to switch to Kernel Code Segment.
    uint8_t     reserved;     // Always set to zero (legacy requirement).
    uint8_t     attributes;   // Flags: Is this entry present? (P), execution privilege (DPL), etc.
    uint16_t    isr_high;     // The higher 16 bits of the function address.
};

/*
 * IDT Pointer.
 * This special structure is passed to the CPU via the 'lidt' assembly instruction.
 * It tells the CPU the size and location of the IDT in memory.
 */
struct __attribute__((packed)) idtr_t {
    uint16_t    limit;        // Size of the IDT table in bytes - 1.
    uint32_t    base;         // Linear address where the IDT array starts.
};

/*
 * CPU State.
 * Represents the state of General Purpose Registers (GPRs) pushed by 'pushad'.
 * We use this to restore the exact state of the CPU after the interrupt is handled.
 */
struct __attribute__((packed)) cpu_state {
    uint32_t edi; 
    uint32_t esi; 
    uint32_t ebp; 
    uint32_t esp; // Kernel stack pointer (from pushad)
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
};

/*
 * Stack State.
 * These values are automatically pushed onto the stack by the CPU *before* * our handler runs.
 */
struct __attribute__((packed)) stack_state {
    uint32_t error_code;      // Pushed by CPU for some exceptions, or dummy 0 by our ASM stub.
    uint32_t eip;             // Instruction Pointer: Where to resume execution.
    uint32_t cs;              // Code Segment: What code segment we were in.
    uint32_t eflags;          // CPU Flags: State of the CPU (math results, interrupt status, etc).
};

// Structure for registering a custom handler callback
struct __attribute__((packed)) interruption {
    uint8_t code;             // The interrupt vector number (e.g., 33 for Keyboard)
    uint32_t callback;        // Function pointer to the handler
};

/*
 * InterruptManager
 * Namespace to group interrupt management functions.
 */
namespace InterruptManager {
    void init();                                   // Initialize the IDT
    void add_interrupt(interruption* intr);        // Register a new handler (e.g., for Keyboard)
}

/* * extern "C" Block
 * These functions are interfaced with Assembly code. 
 * We must prevent C++ "name mangling" so the linker can find them.
 */
extern "C" {
    // The main C++ handler called by the assembly stub.
    // NOTE: Arguments are POINTERS to prevent the compiler from making unsafe copies of the stack.
    void interrupt_handler(cpu_state* cpu, uint32_t interrupt, stack_state* stack);
    
    // Assembly routine to load the IDT pointer
    void load_idt(idtr_t* _idtr);
}