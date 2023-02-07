#ifndef _MC6809_H_
#define _MC6809_H_

#include <stdint.h>
#include <stdbool.h>

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
#define MC6809E_PIN_NMI (24)  // non-maskable interrupt
#define MC6809E_PIN_IRQ (25)  // interrupt request
#define MC6809E_PIN_FIRQ (26) // fast interrupt request
#define MC6809E_PIN_HALT (27) // halt state
#define MC6809E_PIN_RST (28)  // reset requested
#define MC6809E_PIN_RW (29)   // read/write

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
#define MC6809E_M1 (1ULL << MC6809E_PIN_M1)
#define MC6809E_NMI (1ULL << MC6809E_PIN_NMI)
#define MC6809E_IRQ (1ULL << MC6809E_PIN_IRQ)
#define MC6809E_FIRQ (1ULL << MC6809E_PIN_FIRQ)
#define MC6809E_HALT (1ULL << MC6809E_PIN_HALT)
#define MC6809E_RW (1ULL << MC6809E_PIN_RW)

// pin access helper macros
#define MC6809E_MAKE_PINS(ctrl, addr, data) ((ctrl)|((data&0xFF)<<16)|((addr)&0xFFFFULL))
#define MC6809E_GET_ADDR(p) ((uint16_t)(p))
#define MC6809E_SET_ADDR(p,a) {p=((p)&~0xFFFF)|((a)&0xFFFF);}
#define MC6809E_GET_DATA(p) ((uint8_t)((p)>>16))
#define MC6809E_SET_DATA(p,d) {p=((p)&~0xFF0000ULL)|(((d)<<16)&0xFF0000ULL);}

// CONDITION CODE REGISTER (cpu->cc=EFHINZVC)
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

typedef struct {
    int n;         //cycle count
    uint8_t cc;    //condition code
    uint64_t pins; // last pin state, used for NMI detection

    union { struct { int8_t xl;  int8_t xh;  }; uint16_t x; };
    union { struct { int8_t yl;  int8_t yh;  }; uint16_t y; };
    union { struct { int8_t ul;  int8_t uh;  }; uint16_t u; };
    union { struct { int8_t sl;  int8_t sh;  }; uint16_t s; };
    union { struct { int8_t pcl; int8_t pch; }; uint16_t pc; };
    union { struct { int8_t b;   int8_t a;   }; uint16_t d; };
    union { struct { int8_t wl;  int8_t wh;  }; uint16_t w; };
    union { struct { int8_t dd;  int8_t dp;  }; uint16_t da; };

    //mgetc : reads one byte from address a
    //mputc : writes one byte to address a
    int8_t (*mgetc)(uint16_t);
    void (*mputc)(uint16_t, uint8_t);
} mc6809e_t;

void m6809_init(mc6809e_t* cpu);
void m6809_reset(mc6809e_t* cpu);
int  m6809_run_op(mc6809e_t* cpu);
void m6809_irq(mc6809e_t* cpu);

#endif
