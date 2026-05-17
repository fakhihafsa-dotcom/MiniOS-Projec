#pragma once
#include "ktypes.h"

/*
 * PortIO (Port Input/Output)
 * * In the x86 architecture, the CPU communicates with hardware devices (keyboard,
 * screen, timer, etc.) using "I/O Ports". These are NOT memory addresses; they
 * are a separate address space accessed via special instructions (IN/OUT).
 * * Example: Reading from port 0x60 gives us the key code of the last pressed key.
 */
namespace PortIO {

    /**
     * outb (Output Byte)
     * Sends an 8-bit value (byte) to a specific hardware port.
     * * @param port The 16-bit address of the hardware port (0-65535).
     * @param data The 8-bit data value to send.
     */
    void outb(uint16_t port, uint8_t data);

    /**
     * outw (Output Word)
     * Sends a 16-bit value (word) to a specific hardware port.
     * Used for devices that handle larger data chunks at once.
     * * @param port The 16-bit address of the hardware port.
     * @param data The 16-bit data value to send.
     */
    void outw(uint16_t port, uint16_t data);

    /**
     * inb (Input Byte)
     * Reads an 8-bit value (byte) from a specific hardware port.
     * * @param port The 16-bit address of the hardware port to read from.
     * @return The 8-bit value read from the device.
     */
    uint8_t inb(uint16_t port);

    /**
     * inw (Input Word)
     * Reads a 16-bit value (word) from a specific hardware port.
     * * @param port The 16-bit address of the hardware port.
     * @return The 16-bit value read from the device.
     */
    uint16_t inw(uint16_t port);

    /**
     * enable_interrupts
     * Executes the 'STI' (Set Interrupt Flag) instruction.
     * This tells the CPU to start accepting hardware interrupts again.
     */
    void enable_interrupts(void);

    /**
     * disable_interrupts
     * Executes the 'CLI' (Clear Interrupt Flag) instruction.
     * This tells the CPU to ignore maskable hardware interrupts.
     * Critical for atomic operations where you don't want to be disturbed.
     */
    void disable_interrupts(void);

    /**
     * io_wait
     * Forces the CPU to wait for a tiny amount of time.
     * Used when older hardware is too slow to keep up with the CPU.
     */
    void io_wait(void);
}