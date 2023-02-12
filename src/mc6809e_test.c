#define CHIPS_IMPL
#include "mc6809e.h"
#include <stdio.h>
#include <string.h>

int main() {
    // initialize the CPU
    mc6809e_t cpu;
    uint64_t pins = mc6809e_init(&cpu);
    // 64 KB memory with test program at address 0x0000
    uint8_t mem[(1<<16)]={
        0x86, 0x02, // LD A,2
        0x8B, 0x03, // ADDA 3
        0x1E, 0x89  // EXG A,B
    };
    for (int i = 0; i < 24; i++) {
        // run the CPU emulation for one tick
        pins = mc6809e_tick(&cpu, pins);
        // perform memory access
        uint16_t addr = MC6809E_GET_ADDR(pins);
        if (pins & MC6809E_RW) {
            // a memory read
            MC6809E_SET_DATA(pins, mem[addr]);
        } else {
            // a memory write
            mem[addr] = MC6809E_GET_DATA(pins);
        }
    }
    // register B should now be 5
    printf("Register A: %d\n", cpu.A);
    printf("Register B: %d\n", cpu.B);
    return 0;
}
