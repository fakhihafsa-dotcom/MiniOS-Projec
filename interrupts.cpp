#include "interrupts.h"
#include "portio.h"

// Define the IDT array. 
// __attribute__((aligned(0x10))) ensures the table starts on a 16-byte boundary 
// for optimal CPU performance.
__attribute__((aligned(0x10))) 
static idt_entry_t idt[IDT_MAX_DESCRIPTORS];

static idtr_t idtr;

// Keeps track of which interrupts have custom handlers registered
static uint8_t vectors[IDT_MAX_DESCRIPTORS];

// Array of function pointers for custom PIC handlers (e.g., Keyboard, Timer)
static uint32_t pic_isr[IDT_PIC_DESCRIPTORS]; 

// Imported from Assembly: Table of addresses for the raw ISR stubs
extern "C" void* isr_table[]; 

/*
 * Helper: Get Code Segment
 * Instead of hardcoding '0x08', we ask the CPU what the current Code Segment is.
 * This makes the code portable even if the GDT structure changes.
 */
static uint16_t get_code_segment() {
    uint16_t cs;
    __asm__ volatile("mov %%cs, %0" : "=r"(cs));
    return cs;
}

/*
 * Helper: Set IDT Descriptor
 * Populates a single entry in the interrupt table.
 */
static void set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];
    
    uint32_t isr_address = reinterpret_cast<uint32_t>(isr);

    // Low 16 bits of address
    descriptor->isr_low        = isr_address & 0xFFFF;
    
    // Kernel Code Segment. We use the dynamic helper to be safe.
    descriptor->kernel_cs      = get_code_segment(); 
    
    // Access Flags (Present, Privilege Level, Type)
    descriptor->attributes     = flags;
    
    // High 16 bits of address
    descriptor->isr_high       = (isr_address >> 16) & 0xFFFF;
    
    // Reserved always 0
    descriptor->reserved       = 0;
}

/*
 * Interrupt Handler
 * This is the "Central Dispatcher". All interrupts flow through here.
 * * Arguments are populated by the Assembly 'common_interrupt_handler' 
 * pushing registers onto the stack before calling this.
 * * NOTE: We use pointers (cpu_state*) to avoid copying large structs, 
 * which can cause stack corruption in kernel mode.
 */
extern "C" void interrupt_handler(cpu_state* cpu, uint32_t interrupt, stack_state* stack) {
    
    // 1. Handle Intel Reserved Exceptions (0-31)
    // These are fatal errors or CPU events like Divide-by-Zero or Page Fault.
    if(interrupt < IDT_INTEL_DESCRIPTORS) {
        // In a real OS, you would print a Kernel Panic screen here (Like the Blue Screen of Death for Windows).
        PortIO::disable_interrupts(); // Stop the CPU from processing more interrupts
        __asm__("hlt");       // Halt the CPU completely
    } 
    // 2. Handle PIC (Hardware) Interrupts (32-47)
    // These are devices: Timer, Keyboard, Hard Drive, etc.
    else if (interrupt < IDT_INTEL_DESCRIPTORS + IDT_PIC_DESCRIPTORS) {
        // Check if we have a registered callback (e.g., a keyboard driver)
        if (vectors[interrupt]) { 
            // Calculate the index in the pic_isr array
            int pic_index = interrupt - IDT_INTEL_DESCRIPTORS;
            
            // Cast the stored integer address back to a function pointer and call it
            auto handler_func = reinterpret_cast<void(*)()>(pic_isr[pic_index]);
            handler_func();
        }
    }
}

namespace InterruptManager {

    void init() {
        // Setup the IDT pointer (location and size)
        idtr.base = reinterpret_cast<uint32_t>(&idt[0]);
        idtr.limit = (uint16_t)(sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS) - 1;
    
        // Fill the IDT with the default Assembly stubs (from interrupts_s.asm)
        // 0x8E = 10001110 binary: Present, Ring 0, 32-bit Interrupt Gate
        for (uint8_t vector = 0; vector < IDT_INTEL_DESCRIPTORS; vector++) {
            set_descriptor(vector, isr_table[vector], 0x8E);
            vectors[vector] = 1; // Mark as active
        }
        
        // Tell the CPU to use this table
        load_idt(&idtr);
    }

    /*
     * add_interrupt
     * Allows drivers (like Keyboard) to register a specific function to run.
     */
    void add_interrupt(interruption *inr) {
        // Store the callback function
        int pic_index = inr->code - IDT_INTEL_DESCRIPTORS;
        pic_isr[pic_index] = inr->callback;
        
        // Update the IDT to point to the generic ASM stub for this vector
        set_descriptor(inr->code, isr_table[inr->code], 0x8E);
        
        // Mark enabled
        vectors[inr->code] = 1;
    }
}