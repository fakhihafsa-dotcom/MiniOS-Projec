#include "ktypes.h"
#include "portio.h"

namespace PortIO {

    /*
     * outb
     * Implementation using GCC Inline Assembly.
     * * "__asm__ volatile": Tells compiler not to optimize or reorder this instruction.
     * "outb %0, %1": The assembly instruction (AT&T syntax: src, dest).
     * : : "a"(data), "Nd"(port) : Output, Input, Clobber lists.
     * * "a"(data) -> Put the 'data' variable into the AL register (lower 8 bits of EAX).
     * "Nd"(port) -> Put the 'port' variable into the DX register.
     * ('N' allows immediate 8-bit constants for optimization).
     */
    void outb(uint16_t port, uint8_t data) {
        __asm__ volatile (
            "outb %0, %1" 
            : 
            : "a"(data), "Nd"(port)
        );
    }

    /*
     * outw
     * Same as outb, but sends 16 bits.
     * "a"(data) now puts 'data' into the AX register (lower 16 bits of EAX).
     */
    void outw(uint16_t port, uint16_t data) {
        __asm__ volatile (
            "outw %0, %1" 
            : 
            : "a"(data), "Nd"(port)
        );
    }

    /*
     * inb
     * Reads a byte from a port.
     * * "=a"(result) -> The output constraint. It tells GCC: "The result of this 
     * assembly will be in the AL register. Move it into the 
     * 'result' variable when done."
     */
    uint8_t inb(uint16_t port) {
        uint8_t result;
        __asm__ volatile (
            "inb %1, %0" 
            : "=a"(result) 
            : "Nd"(port)
        );
        return result;
    }

    /*
     * inw
     * Reads a word (16 bits) from a port.
     * Result comes from AX register.
     */
    uint16_t inw(uint16_t port) {
        uint16_t result;
        __asm__ volatile (
            "inw %1, %0" 
            : "=a"(result) 
            : "Nd"(port)
        );
        return result;
    }

    /*
     * enable_interrupts
     * Simple wrapper for the STI instruction.
     */
    void enable_interrupts(void) {
        __asm__ volatile ("sti");
    }

    /*
     * disable_interrupts
     * Simple wrapper for the CLI instruction.
     */
    void disable_interrupts(void){
        __asm__ volatile ("cli");
    }

    /*
     * io_wait
     * Writes a value to an unused port (0x80 is traditionally used for BIOS POST codes).
     * This operation takes about 1-4 microseconds, which is just enough time
     * for slow hardware (like the PIC) to process a command before we send the next one.
     */
    void io_wait(void) {
        outb(0x80, 0);
    }

}