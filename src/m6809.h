#ifndef _MC6809_H_
#define _MC6809_H_

#include <stdlib.h>

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
    int n;      //cycle count
    uint8_t cc; //condition code

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
