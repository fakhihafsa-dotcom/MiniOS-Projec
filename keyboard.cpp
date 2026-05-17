#include "keyboard.h"
#include "../core/portio.h"
#include "../core/pic.h"
#include "../core/interrupts.h"

using namespace PortIO;

namespace Keyboard {

    // --- Hardware Constants ---
    constexpr uint16_t DATA_PORT    = 0x60;
    constexpr uint8_t  IRQ_NUMBER   = 1;
    constexpr uint8_t  IDT_VECTOR   = PIC::PIC1_COMMAND + IRQ_NUMBER; // Usually 32 + 1 = 33

    // --- Driver State ---
    // 'volatile' is CRITICAL here. It tells the compiler:
    // "These variables can change at ANY time (by the hardware interrupt), 
    //  so don't cache them in CPU registers. Always read them from RAM."
    static volatile char last_ascii_char = 0;
    static volatile bool new_data_available = false;

    // Use the struct defined in interrupts.h
    static interruption intr_def;

    // --- Lookup Table ---
    // Converts raw Scan Codes (hardware numbers) to ASCII characters (human letters).
    // 0 = No character.
    static const char scan_code_table[128] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0, // 0-15
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's', // 16-31
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', // 32-47
        'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, // 48-63
        0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1', // 64-79
        '2', '3', '0', '.' // 80-83
        // ... rest are 0
    };

    /*
     * handler (The Interrupt Service Routine)
     * * This function runs ASYNCHRONOUSLY. This means it interrupts whatever 
     * the CPU was doing the moment you press a key.
     * * Responsibilities:
     * 1. Read the data from the hardware (fast!).
     * 2. Process it (convert scancode to ASCII).
     * 3. Save it for the main program to use.
     * 4. Acknowledge the PIC so we can get the next interrupt.
     */
    void handler() {
        // 1. Read from the device
        uint8_t scan_code = inb(DATA_PORT);

        // 2. Check if this is a "Press" (Make Code) or "Release" (Break Code)
        // The highest bit (0x80) is set if the key is being RELEASED.
        // We only care when the key is PRESSED.
        if (!(scan_code & 0x80)) {
            
            // Ensure scan code is within our table size
            if (scan_code < sizeof(scan_code_table)) {
                char ascii = scan_code_table[scan_code];
                
                if (ascii != 0) {
                    // 3. Store data for the main program
                    last_ascii_char = ascii;
                    new_data_available = true;
                }
            }
        }

        // 4. Tell the PIC we are done. VERY IMPORTANT!
        // If we forget this, the PIC assumes we are still busy and won't send more keys.
        PIC::acknowledge(IRQ_NUMBER);
    }

    /*
     * init
     * Installs the driver.
     */
    void init() {
        // 3. Enable the IRQ line on the PIC hardware
        PIC::enable_irq(IRQ_NUMBER);
        
        // 1. Prepare the interruption structure
        intr_def.code = IDT_VECTOR;
        
        // We cast our handler function to a 32-bit address (uint32_t) 
        // because our simple IDT setup expects raw addresses.
        intr_def.callback = reinterpret_cast<uint32_t>(&handler);

        // 2. Register with Interrupt Manager (updates the IDT)
        InterruptManager::add_interrupt(&intr_def);
    }

    /*
     * get_char
     * The Public API.
     */
    char get_char() {
        // Reset the flag so we can wait for the NEXT key
        new_data_available = false;
        // Wait loop (Blocking)
        while(new_data_available == false) {
            // "hlt" pauses the CPU until the next interrupt fires.
            // This saves power compared to a "busy loop" (while(true){}).
            // When a key is pressed:
            // 1. CPU wakes up.
            // 2. Runs 'handler()' above.
            // 3. 'handler' sets new_data_available = true.
            // 4. 'handler' returns.
            // 5. This loop check passes, and we proceed.
            __asm__ volatile("hlt");
        }

        return last_ascii_char;
    }

}