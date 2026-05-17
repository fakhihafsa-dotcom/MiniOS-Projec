#pragma once
#include "ktypes.h"

/*
 * PIC (Programmable Interrupt Controller) - 8259A Chip
 * * The PIC collects hardware interrupts (IRQ0-IRQ15) and sends them to the CPU.
 * It acts as the bridge between devices (Keyboard, Timer) and the Processor.
 *
 * Architecture:
 * - Master PIC: Handles IRQ 0-7. Connected directly to the CPU.
 * - Slave PIC:  Handles IRQ 8-15. Connected to IRQ 2 of the Master PIC (Cascading).
 */
namespace PIC {

    // --- I/O Ports ---
    constexpr uint16_t PIC1_COMMAND = 0x20; // Master Command Port
    constexpr uint16_t PIC1_DATA    = 0x21; // Master Data Port
    constexpr uint16_t PIC2_COMMAND = 0xA0; // Slave Command Port
    constexpr uint16_t PIC2_DATA    = 0xA1; // Slave Data Port

    // --- Commands ---
    constexpr uint8_t PIC_EOI       = 0x20; // End of Interrupt Command

    // --- Initialization Command Words (ICW) Flags ---
    // These bits are used to configure the PIC during initialization.
    constexpr uint8_t ICW1_ICW4       = 0x01; // ICW4 (not) needed
    constexpr uint8_t ICW1_SINGLE     = 0x02; // Single (cascade) mode
    constexpr uint8_t ICW1_INTERVAL4  = 0x04; // Call address interval 4 (8)
    constexpr uint8_t ICW1_LEVEL      = 0x08; // Level triggered (edge) mode
    constexpr uint8_t ICW1_INIT       = 0x10; // Initialization - required!

    constexpr uint8_t ICW4_8086       = 0x01; // 8086/88 (MCS-80/85) mode
    constexpr uint8_t ICW4_AUTO       = 0x02; // Auto (normal) EOI
    constexpr uint8_t ICW4_BUF_SLAVE  = 0x08; // Buffered mode/slave
    constexpr uint8_t ICW4_BUF_MASTER = 0x0C; // Buffered mode/master
    constexpr uint8_t ICW4_SFNM       = 0x10; // Special fully nested (not)

    // Offsets
    constexpr uint8_t PIC1_OFFSET     = 0x20; // 32
    constexpr uint8_t PIC2_OFFSET     = 0x28; // 40

    /*
     * Remaps the PIC interrupts to a new range in the IDT.
     * By default, BIOS maps IRQs to 0-7, which conflicts with CPU Exceptions.
     * We usually remap them to 32-47.
     */
    void remap(int offset1, int offset2);

    /*
     * Sends the End Of Interrupt (EOI) signal.
     * Must be called at the end of an ISR, otherwise the PIC won't send more interrupts.
     */
    void acknowledge(int irq);

    /*
     * Unmasks (enables) a specific IRQ line.
     * irq: 0-15
     */
    void enable_irq(int irq);

    /*
     * Masks (disables) a specific IRQ line.
     * irq: 0-15
     */
    void disable_irq(int irq);
}