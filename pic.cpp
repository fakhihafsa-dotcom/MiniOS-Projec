#include "pic.h"
#include "portio.h"

using namespace PortIO;

namespace PIC {

    /*
     * acknowledge
     * Tells the PIC that we have finished handling the current interrupt.
     * * IMPORTANT: If we don't send this "End of Interrupt" (EOI) signal, 
     * the PIC assumes we are still busy and will REFUSE to send any more interrupts.
     * The system will appear to hang.
     */
    void acknowledge(int irq) {
        // If the IRQ came from the Slave PIC (IRQ 8-15), we must send an EOI to it first.
        if (irq >= 8) {
            outb(PIC2_COMMAND, PIC_EOI);
        }
        // Always send EOI to the Master PIC
        outb(PIC1_COMMAND, PIC_EOI);
    }

    /*
     * remap
     * Reprograms the PICs to talk to our OS correctly.
     * * Why do we need this?
     * By default, the BIOS maps hardware interrupts (IRQs) to vectors 0-7.
     * However, the CPU uses vectors 0-31 for Exceptions (like Divide by Zero or Page Fault).
     * If we don't remap, when the Timer fires (IRQ 0), the CPU thinks it's a "Divide by Zero" error!
     * * We move the Hardware Interrupts to 32-47 to avoid this collision.
     */
    void remap(int offset1, int offset2) {
        // Note: We do NOT save the current masks here. We want to wipe the slate clean.

        // --- Step 1: ICW1 (Initialization) ---
        // Tell PICs to start their initialization sequence.
        outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
        io_wait(); // Give the slow hardware a moment to process
        outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
        io_wait();

        // --- Step 2: ICW2 (Vector Offset) ---
        // This is the most important part!
        // We tell the Master PIC to start at 'offset1' (usually 32).
        // We tell the Slave PIC to start at 'offset2' (usually 40).
        outb(PIC1_DATA, offset1); 
        io_wait();
        outb(PIC2_DATA, offset2); 
        io_wait();

        // --- Step 3: ICW3 (Cascading) ---
        // Tell Master it has a Slave connected to IRQ2 (0000 0100 = 4)
        outb(PIC1_DATA, 4); 
        io_wait();
        // Tell Slave it is connected to IRQ2 on the Master (0000 0010 = 2)
        outb(PIC2_DATA, 2); 
        io_wait();

        // --- Step 4: ICW4 (Environment) ---
        // Tell PICs we are working in "8086 mode" (x86 architecture)
        outb(PIC1_DATA, ICW4_8086);
        io_wait();
        outb(PIC2_DATA, ICW4_8086);
        io_wait();

        // --- FINAL STEP: MASK EVERYTHING ---
        // We write 0xFF (11111111 binary) to the Data Ports.
        // This disables (masks) ALL hardware interrupts.
        // 
        // Why?
        // If we don't do this, the BIOS defaults (like the Timer) might fire immediately.
        // Since our OS hasn't set up the Timer driver yet, that interrupt would cause a crash.
        // We will explicitly ENABLE specific devices later (using enable_irq).
        outb(PIC1_DATA, 0xFF);
        outb(PIC2_DATA, 0xFF);
    }

    /*
     * enable_irq
     * Clears the bit in the mask register to ENABLE a specific interrupt.
     * (Logic: 0 = Enabled, 1 = Disabled/Masked)
     * * Example: To enable Keyboard, we call enable_irq(1).
     */
    void enable_irq(int irq) {
        uint16_t port;
        uint8_t value;

        if (irq < 8) {
            port = PIC1_DATA;
        } else {
            port = PIC2_DATA;
            irq -= 8; // Normalize to 0-7 for the slave
        }

        // Read current mask -> AND with inverse of bit -> Write back
        value = inb(port) & ~(1 << irq); 
        outb(port, value);
    }

    /*
     * disable_irq
     * Sets the bit in the mask register to DISABLE a specific interrupt.
     */
    void disable_irq(int irq) {
        uint16_t port;
        uint8_t value;

        if (irq < 8) {
            port = PIC1_DATA;
        } else {
            port = PIC2_DATA;
            irq -= 8;
        }

        // Read current mask -> OR with bit -> Write back
        value = inb(port) | (1 << irq); 
        outb(port, value);
    }
}