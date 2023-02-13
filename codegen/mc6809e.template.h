#ifndef _MC6809E_H_
#define _MC6809E_H_

/*#
    # mc6809e.h

    A cycle-stepped 6809E emulator in a C header.

    Do this:
    ~~~~C
    #define CHIPS_IMPL
    ~~~~
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide
    ~~~C
    #define CHIPS_ASSERT(x) your_own_asset_macro(x)
    ~~~

    ## Emulated Pins
    ***********************************
    *           +-----------+         *
    * R/W   <---|           |---> A0  *
    * BA    <---|           |---> A1  *
    * BS    <---|           |---> A2  *
    *       <---|           |---> ..  *
    *       <---|   6809E   |---> A15 *
    *       <---|           |         *
    * NMI   --->|           |<--> D0  *
    * IRQ   --->|           |<--> D1  *
    * FIRQ  --->|           |<--> ... *
    * RESET --->|           |<--> D7  *
    *           +-----------+         *
    ***********************************

    ## Functions

    ~~~C
    uint64_t mc6809e_init(mc6809e_t* cpu);
    ~~~
        Initializes a new mc6809e_t instance, returns initial pin mask to start
        execution at address 0.

    ~~~C
    uint64_t mc6809e_reset(mc6809e_t* cpu)
    ~~~
        Resets a mc6809e_t instance, returns pin mask to start execution at
        address 0.

    ~~~C
    uint64_t mc6809e_tick(mc6809e_t* cpu, uint64_t pins)
    ~~~
        Step the mc6809e_t instance for one clock cycle.

    ## HOWTO

    Initialize a new mc6809e_t instance and the CPU emulation will be at the start of
    RESET state, and the first 7 ticks will execute the reset sequence
    (loading the reset vector at address 0xFFFE and continuing execution
    there.
     ~~~C
        // setup 64 kBytes of memory
        uint8_t mem[1<<16] = { ... };
        // initialize the CPU
        mc6809e_t cpu;
        uint64_t pins = mc6809e_init(&cpu);
        while (...) {
            // run the CPU emulation for one tick
            pins = mc6809e_tick(&cpu, pins);
            // extract 16-bit address from pin mask
            const uint16_t addr = MC6809E_GET_ADDR(pins);
            // perform memory access
            if (pins & MC6809E_RW) {
                // a memory read
                MC6809E_SET_DATA(pins, mem[addr]);
            }
            else {
                // a memory write
                mem[addr] = MC6809E_GET_DATA(pins);
            }
        }
    ~~~
#*/
/*
    zlib/libpng license

    Copyright (c) 2021 Andre Weissflog
    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.
    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:
        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.
        3. This notice may not be removed or altered from any source
        distribution.
*/

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// address pins
#define MC6809E_PIN_A0 (0)
#define MC6809E_PIN_A1 (1)
#define MC6809E_PIN_A2 (2)
#define MC6809E_PIN_A3 (3)
#define MC6809E_PIN_A4 (4)
#define MC6809E_PIN_A5 (5)
#define MC6809E_PIN_A6 (6)
#define MC6809E_PIN_A7 (7)
#define MC6809E_PIN_A8 (8)
#define MC6809E_PIN_A9 (9)
#define MC6809E_PIN_A10 (10)
#define MC6809E_PIN_A11 (11)
#define MC6809E_PIN_A12 (12)
#define MC6809E_PIN_A13 (13)
#define MC6809E_PIN_A14 (14)
#define MC6809E_PIN_A15 (15)

// data pins
#define MC6809E_PIN_D0 (16)
#define MC6809E_PIN_D1 (17)
#define MC6809E_PIN_D2 (18)
#define MC6809E_PIN_D3 (19)
#define MC6809E_PIN_D4 (20)
#define MC6809E_PIN_D5 (21)
#define MC6809E_PIN_D6 (22)
#define MC6809E_PIN_D7 (23)

// control pins
#define MC6809E_PIN_NMI   (24)  // non-maskable interrupt
#define MC6809E_PIN_IRQ   (25)  // interrupt request
#define MC6809E_PIN_FIRQ  (26)  // fast interrupt request
#define MC6809E_PIN_RW    (27)  // R/W
#define MC6809E_PIN_RST   (28)  // Reset request
#define MC6809E_PIN_BA    (29)  // Bus available
#define MC6809E_PIN_BS    (30)  // Bus state

// pin bit masks
#define MC6809E_A0 (1ULL << MC6809E_PIN_A0)
#define MC6809E_A1 (1ULL << MC6809E_PIN_A1)
#define MC6809E_A2 (1ULL << MC6809E_PIN_A2)
#define MC6809E_A3 (1ULL << MC6809E_PIN_A3)
#define MC6809E_A4 (1ULL << MC6809E_PIN_A4)
#define MC6809E_A5 (1ULL << MC6809E_PIN_A5)
#define MC6809E_A6 (1ULL << MC6809E_PIN_A6)
#define MC6809E_A7 (1ULL << MC6809E_PIN_A7)
#define MC6809E_A8 (1ULL << MC6809E_PIN_A8)
#define MC6809E_A9 (1ULL << MC6809E_PIN_A9)
#define MC6809E_A10 (1ULL << MC6809E_PIN_A10)
#define MC6809E_A11 (1ULL << MC6809E_PIN_A11)
#define MC6809E_A12 (1ULL << MC6809E_PIN_A12)
#define MC6809E_A13 (1ULL << MC6809E_PIN_A13)
#define MC6809E_A14 (1ULL << MC6809E_PIN_A14)
#define MC6809E_A15 (1ULL << MC6809E_PIN_A15)
#define MC6809E_D0 (1ULL << MC6809E_PIN_D0)
#define MC6809E_D1 (1ULL << MC6809E_PIN_D1)
#define MC6809E_D2 (1ULL << MC6809E_PIN_D2)
#define MC6809E_D3 (1ULL << MC6809E_PIN_D3)
#define MC6809E_D4 (1ULL << MC6809E_PIN_D4)
#define MC6809E_D5 (1ULL << MC6809E_PIN_D5)
#define MC6809E_D6 (1ULL << MC6809E_PIN_D6)
#define MC6809E_D7 (1ULL << MC6809E_PIN_D7)
#define MC6809E_NMI (1ULL << MC6809E_PIN_NMI)
#define MC6809E_IRQ (1ULL << MC6809E_PIN_IRQ)
#define MC6809E_FIRQ (1ULL << MC6809E_PIN_FIRQ)
#define MC6809E_RW (1ULL << MC6809E_PIN_RW)
#define MC6809E_RST (1ULL << MC6809E_PIN_RST)
#define MC6809E_BA (1ULL << MC6809E_PIN_BA)
#define MC6809E_BS (1ULL << MC6809E_PIN_BS)

// pin access helper macros
#define MC6809E_MAKE_PINS(ctrl, addr, data) ((ctrl)|((data&0xFF)<<16)|((addr)&0xFFFFULL))
#define MC6809E_GET_ADDR(p) ((uint16_t)(p))
#define MC6809E_SET_ADDR(p,a) {p=((p)&~0xFFFF)|((a)&0xFFFF);}
#define MC6809E_GET_DATA(p) ((uint8_t)((p)>>16))
#define MC6809E_SET_DATA(p,d) {p=((p)&~0xFF0000ULL)|(((d)<<16)&0xFF0000ULL);}

// CONDITION CODE REGISTER (cpu->cc=EFHI NZVC)
#define MC6809E_CF (1<<0)           // carry
#define MC6809E_VF (1<<1)           // overflow
#define MC6809E_ZF (1<<2)           // zero
#define MC6809E_NF (1<<3)           // negative
#define MC6809E_IF (1<<4)           // IRQ mask
#define MC6809E_HF (1<<5)           // half carry
#define MC6809E_FF (1<<6)           // FIRQ mask
#define MC6809E_EF (1<<7)           // entire flag
#define MC6809E_ZCF  0x05           // zero | carry
#define MC6809E_NVF  0x0a           // negative | overflow
#define MC6809E_NZCF 0x0e           // negative | zero | carry
#define MC6809E_Z0F  0xfb           // ~MC6809E_ZF

// interrupt vectors
enum
{
    VECTOR_SWI3         = 0xFFF2,
    VECTOR_SWI2         = 0xFFF4,
    VECTOR_FIRQ         = 0xFFF6,
    VECTOR_IRQ          = 0xFFF8,
    VECTOR_SWI          = 0xFFFA,
    VECTOR_NMI          = 0xFFFC,
    VECTOR_RESET        = 0xFFFE
};

typedef struct {
    uint16_t addr;      // effective address for (HL),(IX+d),(IY+d)
    // uint16_t IR;        // current opcode
    uint16_t opcode;        // current opcode
    uint8_t CC;         // condition code
    uint64_t pins;      // last pin state, used for NMI detection
    uint16_t step;      // the currently active decoder step
    uint8_t irq_state;  // the currently active decoder step

    union { struct { uint8_t XL;  uint8_t XH;  }; uint16_t X;  };
    union { struct { uint8_t YL;  uint8_t YH;  }; uint16_t Y;  };
    union { struct { uint8_t SL;  uint8_t SH;  }; uint16_t S;  };
    union { struct { uint8_t UL;  uint8_t UH;  }; uint16_t U;  };
    union { struct { uint8_t PCL; uint8_t PCH; }; uint16_t PC; };
    union { struct { uint8_t B;   uint8_t A;   }; uint16_t D;  };
    union { struct { uint8_t DD;  uint8_t DP;  }; uint16_t DA; };

} mc6809e_t;

uint64_t mc6809e_init(mc6809e_t* cpu);
uint64_t mc6809e_reset(mc6809e_t* cpu);
uint64_t mc6809e_tick(mc6809e_t* cpu, uint64_t pins);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef CHIPS_IMPL
#include <string.h>
#ifndef CHIPS_ASSERT
    #include <assert.h>
    #define CHIPS_ASSERT(c) assert(c)
#endif

/* set 16-bit address in 64-bit pin mask */
#define _SA(addr) pins=(pins&~0xFFFF)|((addr)&0xFFFFULL)
/* extract 16-bit addess from pin mask */
#define _GA() ((uint16_t)(pins&0xFFFFULL))
/* set 16-bit address and 8-bit data in 64-bit pin mask */
#define _SAD(addr,data) pins=(pins&~0xFFFFFF)|((((data)&0xFF)<<16)&0xFF0000ULL)|((addr)&0xFFFFULL)
/* fetch next opcode byte */
#define _FETCH() _SA(c->PC++);
/* set 8-bit data in 64-bit pin mask */
#define _SD(data) pins=((pins&~0xFF0000ULL)|(((data&0xFF)<<16)&0xFF0000ULL))
/* extract 8-bit data from 64-bit pin mask */
#define _GD() ((uint8_t)((pins&0xFF0000ULL)>>16))
/* enable control pins */
#define _ON(m) pins|=(m)
/* disable control pins */
#define _OFF(m) pins&=~(m)
/* a memory read tick */
#define _RD() _ON(MC6809E_RW);
/* a memory write tick */
#define _WR() _OFF(MC6809E_RW);

/* register access functions */
void mc6809e_set_a(mc6809e_t* cpu, uint8_t v) { cpu->A = v; }
void mc6809e_set_b(mc6809e_t* cpu, uint8_t v) { cpu->B = v; }
void mc6809e_set_dp(mc6809e_t* cpu, uint8_t v) { cpu->DP = v; }
void mc6809e_set_d(mc6809e_t* cpu, uint16_t v) { cpu->D = v; }
void mc6809e_set_x(mc6809e_t* cpu, uint16_t v) { cpu->X = v; }
void mc6809e_set_y(mc6809e_t* cpu, uint16_t v) { cpu->Y = v; }
void mc6809e_set_s(mc6809e_t* cpu, uint16_t v) { cpu->S = v; }
void mc6809e_set_u(mc6809e_t* cpu, uint16_t v) { cpu->U = v; }
void mc6809e_set_pc(mc6809e_t* cpu, uint16_t v) { cpu->PC = v; }
uint8_t mc6809e_a(mc6809e_t* cpu) { return cpu->A; }
uint8_t mc6809e_b(mc6809e_t* cpu) { return cpu->B; }
uint16_t mc6809e_d(mc6809e_t* cpu) { return cpu->D; }
uint16_t mc6809e_x(mc6809e_t* cpu) { return cpu->X; }
uint16_t mc6809e_y(mc6809e_t* cpu) { return cpu->Y; }
uint16_t mc6809e_s(mc6809e_t* cpu) { return cpu->S; }
uint16_t mc6809e_u(mc6809e_t* cpu) { return cpu->U; }
uint16_t mc6809e_pc(mc6809e_t* cpu) { return cpu->PC; }

static const uint16_t opstep[256] = {
$optable };

static int8_t _neg(mc6809e_t* cpu, int8_t c)
{
 cpu->CC &= 0xf0;
 if(c == -128) cpu->CC |= MC6809E_VF;
 c = - c;
 if(c != 0) cpu->CC |= MC6809E_CF;
 if(c < 0) cpu->CC |= MC6809E_NF;
 if(c == 0) cpu->CC |= MC6809E_ZF;
 return c;
}

#define _NEG(d) _neg(c,(int8_t)d)

uint64_t mc6809e_init(mc6809e_t* c) {
    CHIPS_ASSERT(c);
    memset(c, 0, sizeof(*c));
    c->CC = MC6809E_IF;
    c->pins = MC6809E_RW | MC6809E_RST;
    return c->pins;
}

static void _tstc(mc6809e_t* cpu, char c)
{
 cpu->CC &= 0xf1;
 if(c < 0) cpu->CC |= MC6809E_NF;
 if(c == 0) cpu->CC |= MC6809E_ZF;
}

static void _addc(mc6809e_t* cpu, int8_t *r, int8_t c)
{
 int i = *r + c;
 cpu->CC &= 0xd0;
 if(((*r & 0x0f) + (c & 0x0f)) & 0x10) cpu->CC |= MC6809E_HF;
 if(((*r & 0xff) + (c & 0xff)) & 0x100) cpu->CC |= MC6809E_CF;
 *r = i & 0xff;
 if(*r != i) cpu->CC |= MC6809E_VF;
 if(*r < 0) cpu->CC |= MC6809E_NF;
 if(*r == 0) cpu->CC |= MC6809E_ZF;
}

static uint8_t* _reg8(mc6809e_t* cpu, uint8_t post_byte) {
    switch(post_byte & 0x0F) {
        case 0x8: return &cpu->A;
        case 0x9: return &cpu->B;
        case 0xA: return &cpu->CC;
        case 0xB: return &cpu->DP;
        default: CHIPS_ASSERT(false);
    }
}

static uint16_t* _reg16(mc6809e_t* cpu, uint8_t post_byte) {
    switch(post_byte & 0x0F) {
        case 0x0: return &cpu->D;
        case 0x1: return &cpu->X;
        case 0x2: return &cpu->Y;
        case 0x3: return &cpu->U;
        case 0x4: return &cpu->S;
        case 0x5: return &cpu->PC;
        default: CHIPS_ASSERT(false);
    }
}

static void _exg(mc6809e_t* cpu, uint8_t post_byte) {
    if((post_byte & 0x0F)<6) {
        uint16_t* r1 = _reg16(cpu, post_byte>>4);
        uint16_t* r2 = _reg16(cpu, post_byte);
        uint16_t tmp = *r2; *r2 = *r1; *r1 = tmp;
    } else {
        uint8_t* r1 = _reg8(cpu, post_byte>>4);
        uint8_t* r2 = _reg8(cpu, post_byte);
        uint8_t tmp = *r2; *r2 = *r1; *r1 = tmp;
    }
}

uint64_t mc6809e_tick(mc6809e_t* c, uint64_t pins) {
    _RD();
    switch (c->step++) {
        // opcode fetch
        case 0: _FETCH(); break;
        case 1: c->opcode = _GD(); c->step = opstep[c->opcode]; break;
        //=== from here on code-generated
$decode_block
    }
    c->pins = pins;
    return pins;
}

#endif /* CHIPS_IMPL */
#endif /* _MC6809E_H_ */
