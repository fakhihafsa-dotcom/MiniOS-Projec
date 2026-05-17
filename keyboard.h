#pragma once
#include "../core/ktypes.h"

/*
 * Keyboard Driver
 * * Hardware Resources:
 * - IRQ Line: 1 (The keyboard triggers interrupt #33 in our remapped IDT)
 * - I/O Port: 0x60 (Data Port - Read key codes here)
 */
namespace Keyboard {

    /*
     * init
     * Configures the keyboard driver.
     * 1. Unmasks (enables) IRQ1 on the PIC.
     * 2. Registers our custom 'handler' function to run whenever a key is pressed.
     */
    void init();

    /*
     * get_char
     * A "Blocking" function.
     * This function will HALT the program and wait until the user presses a key.
     * * How it works:
     * 1. Checks a flag to see if a new key has been pressed.
     * 2. If not, it loops (or halts CPU) waiting for the Interrupt Handler to run.
     * 3. Once the handler runs and sets the flag, this function returns the character.
     * * @return The ASCII character of the key pressed.
     */
    char get_char();

}