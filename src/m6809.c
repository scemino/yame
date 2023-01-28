//////////////////////////////////////////////////////////////////////////
// DC6809EMUL.C - Motorola 6809 micropocessor emulation
// Author   : Daniel Coulom - danielcoulom@gmail.com
// Web site : http://dcmo5.free.fr
// Created  : 1997-08
// Version  : 2006-10-25
//
// This file is part of DCMO5 v11.
//
// DCMO5 v11 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DCMO5 v11 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DCMO5 v11.  If not, see <http://www.gnu.org/licenses/>.
//
//////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "m6809.h"

/*
conditional jump summary

C BCC BCS      Z BNE BEQ      V BVC BVS     NZV BGT BLE
0 yes no       0 yes no       0 yes no      000 yes no
1 no  yes      1 no  yes      1 no  yes     100 no  yes
                                            010 no  yes
N BPL BMI     ZC BHI BLS     NV BGE BLT     001 no  yes
0 yes no      00 yes no      00 yes no      110 no  no
1 no  yes     10 no  yes     10 no  yes     101 yes no
              01 no  yes     01 no  yes     011 no  no
              11 no  no      11 yes no      111 no  yes
*/
#define BHI (cpu->cc&MC6809E_ZCF)==0
#define BLS (cpu->cc&MC6809E_ZCF)==MC6809E_ZF||(cpu->cc&MC6809E_ZCF)==MC6809E_CF
#define BCC (cpu->cc&MC6809E_CF)==0  // BCC = BHS
#define BCS (cpu->cc&MC6809E_CF)==MC6809E_CF // BCS = BLO
#define BNE (cpu->cc&MC6809E_ZF)==0
#define BEQ (cpu->cc&MC6809E_ZF)==MC6809E_ZF
#define BVC (cpu->cc&MC6809E_VF)==0
#define BVS (cpu->cc&MC6809E_VF)==MC6809E_VF
#define BPL (cpu->cc&MC6809E_NF)==0
#define BMI (cpu->cc&MC6809E_NF)==MC6809E_NF
#define BGE (cpu->cc&MC6809E_NVF)==0||(cpu->cc&MC6809E_NVF)==MC6809E_NVF
#define BLT (cpu->cc&MC6809E_NVF)==MC6809E_NF||(cpu->cc&MC6809E_NVF)==MC6809E_VF
#define BGT (cpu->cc&MC6809E_NZCF)==0||(cpu->cc&0x0e)==MC6809E_NVF
#define BLE (cpu->cc&MC6809E_NZCF)==MC6809E_NF||(cpu->cc&MC6809E_NZCF)==MC6809E_ZF||(cpu->cc&MC6809E_NZCF)==MC6809E_VF||(cpu->cc&MC6809E_NZCF)==MC6809E_NZCF
#define BRANCH {cpu->pc+=cpu->mgetc(cpu->pc);}
#define LBRANCH {cpu->pc+=mgetw(cpu, cpu->pc);cpu->n++;}

//repetitive code
#define IND Mgeti(cpu)
#define DIR cpu->dd=cpu->mgetc(cpu->pc++)
#define EXT cpu->w=mgetw(cpu, cpu->pc);cpu->pc+=2
#define SETZERO {if(cpu->w) cpu->cc &= MC6809E_Z0F; else cpu->cc |= MC6809E_ZF;}

//memory is accessed through :
//mgetw : reads two bytes from address a
//mputw : writes two bytes to address a

static int16_t mgetw(mc6809e_t* cpu, uint16_t address) {
  return (cpu->mgetc(address) << 8 | ((cpu->mgetc(address + 1) & 0xff)));
}

static void mputw(mc6809e_t* cpu, uint16_t address, int16_t value) {
    cpu->mputc(address, value >> 8);
    cpu->mputc(address + 1, value);
}

void m6809_init(mc6809e_t* cpu) {
}

void m6809_reset(mc6809e_t* cpu) {
    cpu->cc = 0x10;
    cpu->pc = mgetw(cpu, 0xfffe);
}

// Get memory (indexed) //////////////////////////////////////////////////////
static void Mgeti(mc6809e_t* cpu)
{
 int i;
 uint16_t *r;
 i = cpu->mgetc(cpu->pc++);
 switch (i & 0x60)
 {
  case 0x00: r = &cpu->x; break;
  case 0x20: r = &cpu->y; break;
  case 0x40: r = &cpu->u; break;
  case 0x60: r = &cpu->s; break;
  default: r = &cpu->x;
 }
 switch(i &= 0x9f)
 {
  case 0x80: cpu->n = 2; cpu->w = *r; *r += 1; return;                    // ,R+
  case 0x81: cpu->n = 3; cpu->w = *r; *r += 2; return;                    // ,R++
  case 0x82: cpu->n = 2; *r -= 1; cpu->w = *r; return;                    // ,-R
  case 0x83: cpu->n = 3; *r -= 2; cpu->w = *r; return;                    // ,--R
  case 0x84: cpu->n = 0; cpu->w = *r; return;                             // ,R
  case 0x85: cpu->n = 1; cpu->w = *r + cpu->b; return;                    // B,R
  case 0x86: cpu->n = 1; cpu->w = *r + cpu->a; return;                    // A,R
  case 0x87: cpu->n = 0; cpu->w = *r; return;                             // invalid
  case 0x88: cpu->n = 1; cpu->w = *r + cpu->mgetc(cpu->pc++); return;          // char,R
  case 0x89: cpu->n = 4; EXT; cpu->w += *r; return;                       // word,R
  case 0x8a: cpu->n = 0; cpu->w = *r; return;                             // invalid
  case 0x8b: cpu->n = 4; cpu->w = *r + cpu->d; return;                    // D,R
  case 0x8c: cpu->n = 1; cpu->w = cpu->mgetc(cpu->pc++); cpu->w += cpu->pc; return; // char,PCR
  case 0x8d: cpu->n = 5; EXT; cpu->w += cpu->pc; return;                       // word,PCR
  case 0x8e: cpu->n = 0; cpu->w = *r; return;                             // invalid
  case 0x8f: cpu->n = 0; cpu->w = *r; return;                             // invalid
  case 0x90: cpu->n = 3; cpu->w = mgetw(cpu, *r); return;                      // invalid
  case 0x91: cpu->n = 6; *r += 2; cpu->w = mgetw(cpu, *r - 2); return;         // [,R++]
  case 0x92: cpu->n = 3; cpu->w = mgetw(cpu, *r); return;                      // invalid
  case 0x93: cpu->n = 6; *r -= 2; cpu->w = mgetw(cpu, *r); return;             // [,--R]
  case 0x94: cpu->n = 3; cpu->w = mgetw(cpu, *r); return;                      // [,R]
  case 0x95: cpu->n = 4; cpu->w = mgetw(cpu, *r + cpu->b); return;             // [B,R]
  case 0x96: cpu->n = 4; cpu->w = mgetw(cpu, *r + cpu->a); return;             // [A,R]
  case 0x97: cpu->n = 3; cpu->w = mgetw(cpu, *r); return;                      // invalid
  case 0x98: cpu->n = 4; cpu->w = mgetw(cpu, *r + cpu->mgetc(cpu->pc++)); return;   // [char,R]
  case 0x99: cpu->n = 7; EXT; cpu->w = mgetw(cpu, *r + cpu->w); return;        // [word,R]
  case 0x9a: cpu->n = 3; cpu->w = mgetw(cpu, *r); return;                      // invalid
  case 0x9b: cpu->n = 7; cpu->w = mgetw(cpu, *r + cpu->d); return;             // [D,R]
  case 0x9c: cpu->n = 4; cpu->w = mgetw(cpu, cpu->pc+1+cpu->mgetc(cpu->pc)); cpu->pc++; return; // [char,PCR]
  case 0x9d: cpu->n = 8; EXT; cpu->w = mgetw(cpu, cpu->pc + cpu->w); return;               // [word,PCR]
  case 0x9e: cpu->n = 3; cpu->w = mgetw(cpu, *r); return;                      // invalid
  case 0x9f: cpu->n = 5; EXT; cpu->w = mgetw(cpu, cpu->w); return;             // [word]
  default  : cpu->n = 1; if(i & 0x10) i -= 0x20; cpu->w = *r + i; return; // 5 bits,R
  //Assumes 0x84 for invalid bytes 0x87 0x8a 0x8e 0x8f
  //Assumes 0x94 for invalid bytes 0x90 0x92 0x97 0x9a 0x9e
 }
}

// PSH, PUL, EXG, TFR /////////////////////////////////////////////////////////
static void Pshs(mc6809e_t* cpu, char c)
{
 //Pshs(0xff) = 12 cycles
 //Pshs(0xfe) = 11 cycles
 //Pshs(0x80) = 2 cycles
 if(c & 0x80) {cpu->mputc(--cpu->s,cpu->pcl); cpu->mputc(--cpu->s,cpu->pch); cpu->n += 2;}
 if(c & 0x40) {cpu->mputc(--cpu->s, cpu->ul); cpu->mputc(--cpu->s, cpu->uh); cpu->n += 2;}
 if(c & 0x20) {cpu->mputc(--cpu->s, cpu->yl); cpu->mputc(--cpu->s, cpu->yh); cpu->n += 2;}
 if(c & 0x10) {cpu->mputc(--cpu->s, cpu->xl); cpu->mputc(--cpu->s, cpu->xh); cpu->n += 2;}
 if(c & 0x08) {cpu->mputc(--cpu->s, cpu->dp); cpu->n += 1;}
 if(c & 0x04) {cpu->mputc(--cpu->s,  cpu->b); cpu->n += 1;}
 if(c & 0x02) {cpu->mputc(--cpu->s,  cpu->a); cpu->n += 1;}
 if(c & 0x01) {cpu->mputc(--cpu->s, cpu->cc); cpu->n += 1;}
}

static void Pshu(mc6809e_t* cpu, char c)
{
 if(c & 0x80) {cpu->mputc(--cpu->u,cpu->pcl); cpu->mputc(--cpu->u,cpu->pch); cpu->n += 2;}
 if(c & 0x40) {cpu->mputc(--cpu->u, cpu->sl); cpu->mputc(--cpu->u, cpu->sh); cpu->n += 2;}
 if(c & 0x20) {cpu->mputc(--cpu->u, cpu->yl); cpu->mputc(--cpu->u, cpu->yh); cpu->n += 2;}
 if(c & 0x10) {cpu->mputc(--cpu->u, cpu->xl); cpu->mputc(--cpu->u, cpu->xh); cpu->n += 2;}
 if(c & 0x08) {cpu->mputc(--cpu->u, cpu->dp); cpu->n += 1;}
 if(c & 0x04) {cpu->mputc(--cpu->u,  cpu->b); cpu->n += 1;}
 if(c & 0x02) {cpu->mputc(--cpu->u,  cpu->a); cpu->n += 1;}
 if(c & 0x01) {cpu->mputc(--cpu->u, cpu->cc); cpu->n += 1;}
}

static void Puls(mc6809e_t* cpu, char c)
{
 if(c & 0x01) {cpu->cc = cpu->mgetc(cpu->s); cpu->s++; cpu->n += 1;}
 if(c & 0x02) { cpu->a = cpu->mgetc(cpu->s); cpu->s++; cpu->n += 1;}
 if(c & 0x04) { cpu->b = cpu->mgetc(cpu->s); cpu->s++; cpu->n += 1;}
 if(c & 0x08) {cpu->dp = cpu->mgetc(cpu->s); cpu->s++; cpu->n += 1;}
 if(c & 0x10) {cpu->xh = cpu->mgetc(cpu->s); cpu->s++; cpu->xl = cpu->mgetc(cpu->s); cpu->s++; cpu->n += 2;}
 if(c & 0x20) {cpu->yh = cpu->mgetc(cpu->s); cpu->s++; cpu->yl = cpu->mgetc(cpu->s); cpu->s++; cpu->n += 2;}
 if(c & 0x40) {cpu->uh = cpu->mgetc(cpu->s); cpu->s++; cpu->ul = cpu->mgetc(cpu->s); cpu->s++; cpu->n += 2;}
 if(c & 0x80) {cpu->pch= cpu->mgetc(cpu->s); cpu->s++; cpu->pcl= cpu->mgetc(cpu->s); cpu->s++; cpu->n += 2;}
}

static void Pulu(mc6809e_t* cpu, char c)
{
 if(c & 0x01) {cpu->cc = cpu->mgetc(cpu->u); cpu->u++; cpu->n += 1;}
 if(c & 0x02) { cpu->a = cpu->mgetc(cpu->u); cpu->u++; cpu->n += 1;}
 if(c & 0x04) { cpu->b = cpu->mgetc(cpu->u); cpu->u++; cpu->n += 1;}
 if(c & 0x08) {cpu->dp = cpu->mgetc(cpu->u); cpu->u++; cpu->n += 1;}
 if(c & 0x10) {cpu->xh = cpu->mgetc(cpu->u); cpu->u++; cpu->xl = cpu->mgetc(cpu->u); cpu->u++; cpu->n += 2;}
 if(c & 0x20) {cpu->yh = cpu->mgetc(cpu->u); cpu->u++; cpu->yl = cpu->mgetc(cpu->u); cpu->u++; cpu->n += 2;}
 if(c & 0x40) {cpu->sh = cpu->mgetc(cpu->u); cpu->u++; cpu->sl = cpu->mgetc(cpu->u); cpu->u++; cpu->n += 2;}
 if(c & 0x80) {cpu->pch= cpu->mgetc(cpu->u); cpu->u++; cpu->pcl= cpu->mgetc(cpu->u); cpu->u++; cpu->n += 2;}
}

static void Exg(mc6809e_t* cpu, char c)
{
 switch(c & 0xff)
 {
  case 0x01: cpu->w = cpu->d; cpu->d = cpu->x; cpu->x = cpu->w; return;    //D-X
  case 0x02: cpu->w = cpu->d; cpu->d = cpu->y; cpu->y = cpu->w; return;    //D-Y
  case 0x03: cpu->w = cpu->d; cpu->d = cpu->u; cpu->u = cpu->w; return;    //D-U
  case 0x04: cpu->w = cpu->d; cpu->d = cpu->s; cpu->s = cpu->w; return;    //D-S
  case 0x05: cpu->w = cpu->d; cpu->d = cpu->pc; cpu->pc = cpu->w; return;  //D-PC
  case 0x10: cpu->w = cpu->x; cpu->x = cpu->d; cpu->d = cpu->w; return;    //X-D
  case 0x12: cpu->w = cpu->x; cpu->x = cpu->y; cpu->y = cpu->w; return;    //X-Y
  case 0x13: cpu->w = cpu->x; cpu->x = cpu->u; cpu->u = cpu->w; return;    //X-U
  case 0x14: cpu->w = cpu->x; cpu->x = cpu->s; cpu->s = cpu->w; return;    //X-S
  case 0x15: cpu->w = cpu->x; cpu->x = cpu->pc; cpu->pc = cpu->w; return;  //X-PC
  case 0x20: cpu->w = cpu->y; cpu->y = cpu->d; cpu->d = cpu->w; return;    //Y-D
  case 0x21: cpu->w = cpu->y; cpu->y = cpu->x; cpu->x = cpu->w; return;    //Y-X
  case 0x23: cpu->w = cpu->y; cpu->y = cpu->u; cpu->u = cpu->w; return;    //Y-U
  case 0x24: cpu->w = cpu->y; cpu->y = cpu->s; cpu->s = cpu->w; return;    //Y-S
  case 0x25: cpu->w = cpu->y; cpu->y = cpu->pc; cpu->pc = cpu->w; return;  //Y-PC
  case 0x30: cpu->w = cpu->u; cpu->u = cpu->d; cpu->d = cpu->w; return;    //U-D
  case 0x31: cpu->w = cpu->u; cpu->u = cpu->x; cpu->x = cpu->w; return;    //U-X
  case 0x32: cpu->w = cpu->u; cpu->u = cpu->y; cpu->y = cpu->w; return;    //U-Y
  case 0x34: cpu->w = cpu->u; cpu->u = cpu->s; cpu->s = cpu->w; return;    //U-S
  case 0x35: cpu->w = cpu->u; cpu->u = cpu->pc; cpu->pc = cpu->w; return;  //U-PC
  case 0x40: cpu->w = cpu->s; cpu->s = cpu->d; cpu->d = cpu->w; return;
  case 0x41: cpu->w = cpu->s; cpu->s = cpu->x; cpu->x = cpu->w; return;
  case 0x42: cpu->w = cpu->s; cpu->s = cpu->y; cpu->y = cpu->w; return;
  case 0x43: cpu->w = cpu->s; cpu->s = cpu->u; cpu->u = cpu->w; return;
  case 0x45: cpu->w = cpu->s; cpu->s = cpu->pc; cpu->pc = cpu->w; return;
  case 0x50: cpu->w = cpu->pc; cpu->pc = cpu->d; cpu->d = cpu->w; return;
  case 0x51: cpu->w = cpu->pc; cpu->pc = cpu->x; cpu->x = cpu->w; return;
  case 0x52: cpu->w = cpu->pc; cpu->pc = cpu->y; cpu->y = cpu->w; return;
  case 0x53: cpu->w = cpu->pc; cpu->pc = cpu->u; cpu->u = cpu->w; return;
  case 0x54: cpu->w = cpu->pc; cpu->pc = cpu->s; cpu->s = cpu->w; return;
  case 0x89: cpu->w = cpu->a; cpu->a = cpu->b; cpu->b = cpu->w; return;
  case 0x8a: cpu->w = cpu->a; cpu->a = cpu->cc; cpu->cc = cpu->w; return;
  case 0x8b: cpu->w = cpu->a; cpu->a = cpu->dp; cpu->dp = cpu->w; return;
  case 0x98: cpu->w = cpu->b; cpu->b = cpu->a; cpu->a = cpu->w; return;
  case 0x9a: cpu->w = cpu->b; cpu->b = cpu->cc; cpu->cc = cpu->w; return;
  case 0x9b: cpu->w = cpu->b; cpu->b = cpu->dp; cpu->dp = cpu->w; return;
  case 0xa8: cpu->w = cpu->cc; cpu->cc = cpu->a; cpu->a = cpu->w; return;
  case 0xa9: cpu->w = cpu->cc; cpu->cc = cpu->b; cpu->b = cpu->w; return;
  case 0xab: cpu->w = cpu->cc; cpu->cc = cpu->dp; cpu->dp = cpu->w; return;
  case 0xb8: cpu->w = cpu->dp; cpu->dp = cpu->a; cpu->a = cpu->w; return;
  case 0xb9: cpu->w = cpu->dp; cpu->dp = cpu->b; cpu->b = cpu->w; return;
  case 0xba: cpu->w = cpu->dp; cpu->dp = cpu->cc; cpu->cc = cpu->w; return;
 }
}

static void Tfr(mc6809e_t* cpu, char c)
{
 switch(c & 0xff)
 {
  case 0x01: cpu->x = cpu->d; return;
  case 0x02: cpu->y = cpu->d; return;
  case 0x03: cpu->u = cpu->d; return;
  case 0x04: cpu->s = cpu->d; return;
  case 0x05: cpu->pc = cpu->d; return;
  case 0x10: cpu->d = cpu->x; return;
  case 0x12: cpu->y = cpu->x; return;
  case 0x13: cpu->u = cpu->x; return;
  case 0x14: cpu->s = cpu->x; return;
  case 0x15: cpu->pc = cpu->x; return;
  case 0x20: cpu->d = cpu->y; return;
  case 0x21: cpu->x = cpu->y; return;
  case 0x23: cpu->u = cpu->y; return;
  case 0x24: cpu->s = cpu->y; return;
  case 0x25: cpu->pc = cpu->y; return;
  case 0x30: cpu->d = cpu->u; return;
  case 0x31: cpu->x = cpu->u; return;
  case 0x32: cpu->y = cpu->u; return;
  case 0x34: cpu->s = cpu->u; return;
  case 0x35: cpu->pc = cpu->u; return;
  case 0x40: cpu->d = cpu->s; return;
  case 0x41: cpu->x = cpu->s; return;
  case 0x42: cpu->y = cpu->s; return;
  case 0x43: cpu->u = cpu->s; return;
  case 0x45: cpu->pc = cpu->s; return;
  case 0x50: cpu->d = cpu->pc; return;
  case 0x51: cpu->x = cpu->pc; return;
  case 0x52: cpu->y = cpu->pc; return;
  case 0x53: cpu->u = cpu->pc; return;
  case 0x54: cpu->s = cpu->pc; return;
  case 0x89: cpu->b = cpu->a; return;
  case 0x8a: cpu->cc = cpu->a; return;
  case 0x8b: cpu->dp = cpu->a; return;
  case 0x98: cpu->a = cpu->b; return;
  case 0x9a: cpu->cc = cpu->b; return;
  case 0x9b: cpu->dp = cpu->b; return;
  case 0xa8: cpu->a = cpu->cc; return;
  case 0xa9: cpu->b = cpu->cc; return;
  case 0xab: cpu->dp = cpu->cc; return;
  case 0xb8: cpu->a = cpu->dp; return;
  case 0xb9: cpu->b = cpu->dp; return;
  case 0xba: cpu->cc = cpu->dp; return;
 }
}

// CLR, NEG, COM, INC, DEC  (cc=EFHINZVC) /////////////////////////////////////
static char Clr(mc6809e_t* cpu)
{
 cpu->cc &= 0xf0;
 cpu->cc |= MC6809E_ZF;
 return 0;
}

static char Neg(mc6809e_t* cpu, char c)
{
 cpu->cc &= 0xf0;
 if(c == -128) cpu->cc |= MC6809E_VF;
 c = - c;
 if(c != 0) cpu->cc |= MC6809E_CF;
 if(c < 0) cpu->cc |= MC6809E_NF;
 if(c == 0) cpu->cc |= MC6809E_ZF;
 return c;
}

static char Com(mc6809e_t* cpu, char c)
{
 cpu->cc &= 0xf0;
 c = ~c;
 cpu->cc |= MC6809E_CF;
 if(c < 0) cpu->cc |= MC6809E_NF;
 if(c == 0) cpu->cc |= MC6809E_ZF;
 return c;
}

static char Inc(mc6809e_t* cpu, char c)
{
 cpu->cc &= 0xf1;
 if(c == 127) cpu->cc |= MC6809E_VF;
 c++;
 if(c < 0) cpu->cc |= MC6809E_NF;
 if(c == 0) cpu->cc |= MC6809E_ZF;
 return c;
}

static char Dec(mc6809e_t* cpu, char c)
{
 cpu->cc &= 0xf1;
 if(c == -128) cpu->cc |= MC6809E_VF;
 c--;
 if(c < 0) cpu->cc |= MC6809E_NF;
 if(c == 0) cpu->cc |= MC6809E_ZF;
 return c;
}

// Registers operations  (cc=EFHINZVC) ////////////////////////////////////////
static void Mul(mc6809e_t* cpu)
{
 cpu->d = (cpu->a & 0xff) * (cpu->b & 0xff);
 cpu->cc &= 0xf2;
 if(cpu->d < 0) cpu->cc |= MC6809E_CF;
 if(cpu->d == 0) cpu->cc |= MC6809E_ZF;
}

static void Addc(mc6809e_t* cpu, char *r, char c)
{
 int i = *r + c;
 cpu->cc &= 0xd0;
 if(((*r & 0x0f) + (c & 0x0f)) & 0x10) cpu->cc |= MC6809E_HF;
 if(((*r & 0xff) + (c & 0xff)) & 0x100) cpu->cc |= MC6809E_CF;
 *r = i & 0xff;
 if(*r != i) cpu->cc |= MC6809E_VF;
 if(*r < 0) cpu->cc |= MC6809E_NF;
 if(*r == 0) cpu->cc |= MC6809E_ZF;
}

static void Adc(mc6809e_t* cpu, char *r, char c)
{
 int carry = (cpu->cc & MC6809E_CF);
 int i = *r + c + carry;
 cpu->cc &= 0xd0;
 if(((*r & 0x0f) + (c & 0x0f) + carry) & 0x10) cpu->cc |= MC6809E_HF;
 if(((*r & 0xff) + (c & 0xff) + carry) & 0x100) cpu->cc |= MC6809E_CF;
 *r = i & 0xff;
 if(*r != i) cpu->cc |= MC6809E_VF;
 if(*r < 0) cpu->cc |= MC6809E_NF;
 if(*r == 0) cpu->cc |= MC6809E_ZF;
}

static void Addw(mc6809e_t* cpu, short *r, short w)
{
 int i = *r + w;
 cpu->cc &= 0xf0;
 if(((*r & 0xffff) + (w & 0xffff)) & 0xf0000) cpu->cc |= MC6809E_CF;
 *r = i & 0xffff;
 if(*r != i) cpu->cc |= MC6809E_VF;
 if(*r < 0) cpu->cc |= MC6809E_NF;
 if(*r == 0) cpu->cc |= MC6809E_ZF;
}

static void Subc(mc6809e_t* cpu, char *r, char c)
{
 int i = *r - c;
 cpu->cc &= 0xf0;
 if(((*r & 0xff) - (c & 0xff)) & 0x100) cpu->cc |= MC6809E_CF;
 *r = i & 0xff;
 if(*r != i) cpu->cc |= MC6809E_VF;
 if(*r < 0) cpu->cc |= MC6809E_NF;
 if(*r == 0) cpu->cc |= MC6809E_ZF;
}

static void Sbc(mc6809e_t* cpu, char *r, char c)
{
 int carry = (cpu->cc & MC6809E_CF);
 int i = *r - c - carry;
 cpu->cc &= 0xf0;
 if(((*r & 0xff) - (c & 0xff) - carry) & 0x100) cpu->cc |= MC6809E_CF;
 *r = i & 0xff;
 if(*r != i) cpu->cc |= MC6809E_VF;
 if(*r < 0) cpu->cc |= MC6809E_NF;
 if(*r == 0) cpu->cc |= MC6809E_ZF;
}

static void Subw(mc6809e_t* cpu, short *r, short w)
{
 int i = *r - w;
 cpu->cc &= 0xf0;
 if(((*r & 0xffff) - (w & 0xffff)) & 0x10000) cpu->cc |= MC6809E_CF;
 *r = i & 0xffff;
 if(*r != i) cpu->cc |= MC6809E_VF;
 if(*r < 0) cpu->cc |= MC6809E_NF;
 if(*r == 0) cpu->cc |= MC6809E_ZF;
}

static void Daa(mc6809e_t* cpu)
{
 int i = cpu->a & 0xff;
 if((cpu->cc & MC6809E_HF) || ((i & 0x00f) > 0x09)) i += 0x06;
 if((cpu->cc & MC6809E_CF) || ((i & 0x1f0) > 0x90)) i += 0x60;
 cpu->a = i & 0xff;
 i = (i >> 1 & 0xff) | (cpu->cc << 7);
 cpu->cc &= 0xf0;
 if(i & 0x80) cpu->cc |= MC6809E_CF;
 if((cpu->a ^ i) & 0x80) cpu->cc |= MC6809E_VF;
 if(cpu->a < 0) cpu->cc |= MC6809E_NF;
 if(cpu->a == 0) cpu->cc |= MC6809E_ZF;
}

// Shift and rotate  (cpu->cc=EFHINZVC) ////////////////////////////////////////////
static char Lsr(mc6809e_t* cpu, char c)
{
 cpu->cc &= 0xf2;
 if(c & 1) cpu->cc |= MC6809E_CF;
 c = (c & 0xff) >> 1;
 if(c == 0) cpu->cc |= MC6809E_ZF;
 return c;
}

static char Ror(mc6809e_t* cpu, char c)
{
 int carry = cpu->cc & MC6809E_CF;
 cpu->cc &= 0xf2;
 if(c & 1) cpu->cc |= MC6809E_CF;
 c = ((c & 0xff) >> 1) | (carry << 7);
 if(c < 0) cpu->cc |= MC6809E_NF;
 if(c == 0) cpu->cc |= MC6809E_ZF;
 return c;
}

static char Rol(mc6809e_t* cpu, char c)
{
 int carry = cpu->cc & MC6809E_CF;
 cpu->cc &= 0xf0;
 if(c < 0) cpu->cc |= MC6809E_CF;
 c = ((c & 0x7f) << 1) | carry;
 if((c >> 7 & 1) ^ (cpu->cc & MC6809E_CF)) cpu->cc |= MC6809E_VF;
 if(c < 0) cpu->cc |= MC6809E_NF;
 if(c == 0) cpu->cc |= MC6809E_ZF;
 return c;
}

static char Asr(mc6809e_t* cpu, char c)
{
 cpu->cc &= 0xf2;
 if(c & 1) cpu->cc |= MC6809E_CF;
 c = ((c & 0xff) >> 1) | (c & 0x80);
 if(c < 0) cpu->cc |= MC6809E_NF;
 if(c == 0) cpu->cc |= MC6809E_ZF;
 return c;
}

static char Asl(mc6809e_t* cpu, char c)
{
 cpu->cc &= 0xf0;
 if(c < 0) cpu->cc |= MC6809E_CF;
 c = (c & 0xff) << 1;
 if((c >> 7 & 1) ^ (cpu->cc & MC6809E_CF)) cpu->cc |= MC6809E_VF;
 if(c < 0) cpu->cc |= MC6809E_NF;
 if(c == 0) cpu->cc |= MC6809E_ZF;
 return c;
}

// Test and compare  (cpu->cc=EFHINZVC) ////////////////////////////////////////////
static void Tstc(mc6809e_t* cpu, char c)
{
 cpu->cc &= 0xf1;
 if(c < 0) cpu->cc |= MC6809E_NF;
 if(c == 0) cpu->cc |= MC6809E_ZF;
}

static void Tstw(mc6809e_t* cpu, short w)
{
 cpu->cc &= 0xf1;
 if(w < 0) cpu->cc |= MC6809E_NF;
 if(w == 0) cpu->cc |= MC6809E_ZF;
}

static void Cmpc(mc6809e_t* cpu, char *reg, char c)
{
 char r = *reg;
 int i = *reg - c;
 cpu->cc &= 0xf0;
 if(((r & 0xff) - (c & 0xff)) & 0x100) cpu->cc |= MC6809E_CF;
 r = i & 0xff;
 if(r != i) cpu->cc |= MC6809E_VF;
 if(r < 0) cpu->cc |= MC6809E_NF;
 if(r == 0) cpu->cc |= MC6809E_ZF;
}

static void Cmpw(mc6809e_t* cpu, short *reg, short w)
{
 short r = *reg;
 int i = *reg - w;
 cpu->cc &= 0xf0;
 if(((r & 0xffff) - (w & 0xffff)) & 0x10000) cpu->cc |= MC6809E_CF;
 r = i & 0xffff;
 if(r != i) cpu->cc |= MC6809E_VF;
 if(r < 0) cpu->cc |= MC6809E_NF;
 if(r == 0) cpu->cc |= MC6809E_ZF;
}

// Interrupt requests  (cc=EFHINZVC) //////////////////////////////////////////
static void Swi(mc6809e_t* cpu, int n)
{
 cpu->cc |= MC6809E_EF;
 Pshs(cpu, 0xff);
 cpu->cc |= MC6809E_IF;
 if(n == 1) {cpu->pc = mgetw(cpu, 0xfffa); return;}
 if(n == 2) {cpu->pc = mgetw(cpu, 0xfff4); return;}
 if(n == 3) {cpu->pc = mgetw(cpu, 0xfff2); return;}
}

void m6809_irq(mc6809e_t* cpu)
{
 if((cpu->cc & MC6809E_IF) == 0)
 {cpu->cc |= MC6809E_EF; Pshs(cpu, 0xff); cpu->cc |= MC6809E_IF; cpu->pc = mgetw(cpu, 0xfff8);}
}

static void Firq(mc6809e_t* cpu)
{
 if((cpu->cc & MC6809E_FF) == 0)
 {cpu->cc &= ~MC6809E_EF; Pshs(cpu, 0x81); cpu->cc |= MC6809E_FF; cpu->cc |= MC6809E_IF; cpu->pc = mgetw(cpu, 0xfff6);}
}

// RTI ////////////////////////////////////////////////////////////////////////
static void Rti(mc6809e_t* cpu) {Puls(cpu, 0x01); if(cpu->cc & MC6809E_EF) Puls(cpu, 0xfe); else Puls(cpu, 0x80);}

// Execute one operation at PC address and set PC to next opcode address //////
int m6809_run_op(mc6809e_t* cpu)
/*
Return value is set to :
- cycle count for the executed instruction when operation code is legal
- negative value (-code) when operation code is illegal
*/
{
 int precode, code;
 cpu->n = 0; //par defaut pas de cycles supplementaires
 precode = 0; //par defaut pas de precode
 while(1)
 {
  code = cpu->mgetc(cpu->pc++) & 0xff;
  if(code == 0x10) {precode = 0x1000; continue;}
  if(code == 0x11) {precode = 0x1100; continue;}
  code |= precode; break;
 }

 switch(code)
 {
  case 0x00: DIR; cpu->mputc(cpu->da, Neg(cpu, cpu->mgetc(cpu->da))); return 6;      /* NEG  /$ */
  case 0x01: DIR; return 3;                            /*undoc BRN     */
//case 0x02:
// if(cpu->cc&MC6809E_CF){cpu->mputc(wd, Com(cpu->mgetc(cpu->da))); return 6;}     /*undoc COM  /$ */
//      else{cpu->mputc(cpu->da, Neg(mgetc(cpu->da))); return 6;}     /*undoc NEG  /$ */
  case 0x03: DIR; cpu->mputc(cpu->da, Com(cpu, cpu->mgetc(cpu->da))); return 6;      /* COM  /$ */
  case 0x04: DIR; cpu->mputc(cpu->da, Lsr(cpu, cpu->mgetc( cpu->da))); return 6;      /* LSR  /$ */
//case 0x05: cpu->mputc(cpu->da, Lsr(mgetc(cpu->da))); return 6;      /*undoc LSR  /$ */
  case 0x06: DIR; cpu->mputc(cpu->da, Ror(cpu, cpu->mgetc(cpu->da))); return 6;      /* ROR  /$ */
  case 0x07: DIR; cpu->mputc(cpu->da, Asr(cpu, cpu->mgetc(cpu->da))); return 6;      /* ASR  /$ */
  case 0x08: DIR; cpu->mputc(cpu->da, Asl(cpu, cpu->mgetc(cpu->da))); return 6;      /* ASL  /$ */
  case 0x09: DIR; cpu->mputc(cpu->da, Rol(cpu, cpu->mgetc(cpu->da))); return 6;      /* ROL  /$ */
  case 0x0a: DIR; cpu->mputc(cpu->da, Dec(cpu, cpu->mgetc(cpu->da))); return 6;      /* DEC  /$ */
  case 0x0c: DIR; cpu->mputc(cpu->da, Inc(cpu, cpu->mgetc(cpu->da))); return 6;      /* INC  /$ */
  case 0x0d: DIR; Tstc(cpu, cpu->mgetc(cpu->da)); return 6;                /* TST  /$ */
  case 0x0e: DIR; cpu->pc = cpu->da; return 3;                        /* JMP  /$ */
  case 0x0f: DIR; cpu->mputc(cpu->da, Clr(cpu)); return 6;               /* CLR  /$ */

  case 0x12: return 2;                                      /* NOP     */
  case 0x13: return 4;                                      /* SYNC    */
  case 0x16: cpu->pc += mgetw(cpu, cpu->pc) + 2; return 5;                 /* LBRA    */
  case 0x17: EXT; Pshs(cpu, 0x80); cpu->pc += cpu->w; return 9;            /* LBSR    */
  case 0x19: Daa(cpu); return 2;                               /* DAA     */
  case 0x1a: cpu->cc |= cpu->mgetc(cpu->pc++); return 3;                   /* ORCC #$ */
  case 0x1c: cpu->cc &= cpu->mgetc(cpu->pc++); return 3;                   /* ANDC #$ */
  case 0x1d: Tstw(cpu, cpu->d = cpu->b); return 2;                         /* SEX     */
  case 0x1e: Exg(cpu, cpu->mgetc(cpu->pc++)); return 8;                    /* EXG     */
  case 0x1f: Tfr(cpu, cpu->mgetc(cpu->pc++)); return 6;                    /* TFR     */

  case 0x20: BRANCH; cpu->pc++; return 3;                        /* BRA     */
  case 0x21: cpu->pc++; return 3;                                /* BRN     */
  case 0x22: if(BHI) BRANCH; cpu->pc++; return 3;                /* BHI     */
  case 0x23: if(BLS) BRANCH; cpu->pc++; return 3;                /* BLS     */
  case 0x24: if(BCC) BRANCH; cpu->pc++; return 3;                /* BCC     */
  case 0x25: if(BCS) BRANCH; cpu->pc++; return 3;                /* BCS     */
  case 0x26: if(BNE) BRANCH; cpu->pc++; return 3;                /* BNE     */
  case 0x27: if(BEQ) BRANCH; cpu->pc++; return 3;                /* BEQ     */
  case 0x28: if(BVC) BRANCH; cpu->pc++; return 3;                /* BVC     */
  case 0x29: if(BVS) BRANCH; cpu->pc++; return 3;                /* BVS     */
  case 0x2a: if(BPL) BRANCH; cpu->pc++; return 3;                /* BPL     */
  case 0x2b: if(BMI) BRANCH; cpu->pc++; return 3;                /* BMI     */
  case 0x2c: if(BGE) BRANCH; cpu->pc++; return 3;                /* BGE     */
  case 0x2d: if(BLT) BRANCH; cpu->pc++; return 3;                /* BLT     */
  case 0x2e: if(BGT) BRANCH; cpu->pc++; return 3;                /* BGT     */
  case 0x2f: if(BLE) BRANCH; cpu->pc++; return 3;                /* BLE     */

  case 0x30: IND; cpu->x = cpu->w; SETZERO; return 4 + cpu->n;           /* LEAX    */
  case 0x31: IND; cpu->y = cpu->w; SETZERO; return 4 + cpu->n;           /* LEAY    */
  //d'apres Prehisto, LEAX et LEAY positionnent aussi le bit N de cpu->cc
  //il faut donc modifier l'�mulation de ces deux instructions
  case 0x32: IND; cpu->s = cpu->w; return 4 + cpu->n; /*cpu->cc not set*/       /* LEAS    */
  case 0x33: IND; cpu->u = cpu->w; return 4 + cpu->n; /*cpu->cc not set*/       /* LEAU    */
  case 0x34: Pshs(cpu, cpu->mgetc(cpu->pc++)); return 5 + cpu->n;               /* PSHS    */
  case 0x35: Puls(cpu, cpu->mgetc(cpu->pc++)); return 5 + cpu->n;               /* PULS    */
  case 0x36: Pshu(cpu, cpu->mgetc(cpu->pc++)); return 5 + cpu->n;               /* PSHU    */
  case 0x37: Pulu(cpu, cpu->mgetc(cpu->pc++)); return 5 + cpu->n;               /* PULU    */
  case 0x39: Puls(cpu, 0x80); return 5;                          /* RTS     */
  case 0x3a: cpu->x += cpu->b & 0xff; return 3;                       /* ABX     */
  case 0x3b: Rti(cpu); return 4 + cpu->n;                           /* RTI     */
  case 0x3c: cpu->cc &= cpu->mgetc(cpu->pc++); cpu->cc |= MC6809E_EF; return 20;        /* CWAI    */
  case 0x3d: Mul(cpu); return 11;                              /* MUL     */
  case 0x3f: Swi(cpu, 1); return 19;                             /* SWI     */

  case 0x40: cpu->a = Neg(cpu, cpu->a); return 2;                          /* NEGA    */
  case 0x43: cpu->a = Com(cpu, cpu->a); return 2;                          /* COMA    */
  case 0x44: cpu->a = Lsr(cpu, cpu->a); return 2;                          /* LSRA    */
  case 0x46: cpu->a = Ror(cpu, cpu->a); return 2;                          /* RORA    */
  case 0x47: cpu->a = Asr(cpu, cpu->a); return 2;                          /* ASRA    */
  case 0x48: cpu->a = Asl(cpu, cpu->a); return 2;                          /* ASLA    */
  case 0x49: cpu->a = Rol(cpu, cpu->a); return 2;                          /* ROLA    */
  case 0x4a: cpu->a = Dec(cpu, cpu->a); return 2;                          /* DECA    */
  case 0x4c: cpu->a = Inc(cpu, cpu->a); return 2;                          /* INCA    */
  case 0x4d: Tstc(cpu, cpu->a); return 2;                             /* TSTA    */
  case 0x4f: cpu->a = Clr(cpu); return 2;                           /* CLRA    */

  case 0x50: cpu->b = Neg(cpu, cpu->b); return 2;                          /* NEGB    */
  case 0x53: cpu->b = Com(cpu, cpu->b); return 2;                          /* COMB    */
  case 0x54: cpu->b = Lsr(cpu, cpu->b); return 2;                          /* LSRB    */
  case 0x56: cpu->b = Ror(cpu, cpu->b); return 2;                          /* RORB    */
  case 0x57: cpu->b = Asr(cpu, cpu->b); return 2;                          /* ASRB    */
  case 0x58: cpu->b = Asl(cpu, cpu->b); return 2;                          /* ASLB    */
  case 0x59: cpu->b = Rol(cpu, cpu->b); return 2;                          /* ROLB    */
  case 0x5a: cpu->b = Dec(cpu, cpu->b); return 2;                          /* DECB    */
  case 0x5c: cpu->b = Inc(cpu, cpu->b); return 2;                          /* INCB    */
  case 0x5d: Tstc(cpu, cpu->b); return 2;                             /* TSTB    */
  case 0x5f: cpu->b = Clr(cpu); return 2;                           /* CLRB    */

  case 0x60: IND; cpu->mputc(cpu->w, Neg(cpu, cpu->mgetc(cpu->w))); return 6 + cpu->n;    /* NEG  IX */
  case 0x63: IND; cpu->mputc(cpu->w, Com(cpu, cpu->mgetc(cpu->w))); return 6 + cpu->n;    /* COM  IX */
  case 0x64: IND; cpu->mputc(cpu->w, Lsr(cpu, cpu->mgetc(cpu->w))); return 6 + cpu->n;    /* LSR  IX */
  case 0x66: IND; cpu->mputc(cpu->w, Ror(cpu, cpu->mgetc(cpu->w))); return 6 + cpu->n;    /* ROR  IX */
  case 0x67: IND; cpu->mputc(cpu->w, Asr(cpu, cpu->mgetc(cpu->w))); return 6 + cpu->n;    /* ASR  IX */
  case 0x68: IND; cpu->mputc(cpu->w, Asl(cpu, cpu->mgetc(cpu->w))); return 6 + cpu->n;    /* ASL  IX */
  case 0x69: IND; cpu->mputc(cpu->w, Rol(cpu, cpu->mgetc(cpu->w))); return 6 + cpu->n;    /* ROL  IX */
  case 0x6a: IND; cpu->mputc(cpu->w, Dec(cpu, cpu->mgetc(cpu->w))); return 6 + cpu->n;    /* DEC  IX */
  case 0x6c: IND; cpu->mputc(cpu->w, Inc(cpu, cpu->mgetc(cpu->w))); return 6 + cpu->n;    /* INC  IX */
  case 0x6d: IND; Tstc(cpu, cpu->mgetc(cpu->w)); return 6 + cpu->n;             /* TST  IX */
  case 0x6e: IND; cpu->pc = cpu->w; return 3 + cpu->n;                     /* JMP  IX */
  case 0x6f: IND; cpu->mputc(cpu->w, Clr(cpu)); return 6 + cpu->n;            /* CLR  IX */

  case 0x70: EXT; cpu->mputc(cpu->w, Neg(cpu, cpu->mgetc(cpu->w))); return 7;        /* NEG  $  */
  case 0x73: EXT; cpu->mputc(cpu->w, Com(cpu, cpu->mgetc(cpu->w))); return 7;        /* COM  $  */
  case 0x74: EXT; cpu->mputc(cpu->w, Lsr(cpu, cpu->mgetc(cpu->w))); return 7;        /* LSR  $  */
  case 0x76: EXT; cpu->mputc(cpu->w, Ror(cpu, cpu->mgetc(cpu->w))); return 7;        /* ROR  $  */
  case 0x77: EXT; cpu->mputc(cpu->w, Asr(cpu, cpu->mgetc(cpu->w))); return 7;        /* ASR  $  */
  case 0x78: EXT; cpu->mputc(cpu->w, Asl(cpu, cpu->mgetc(cpu->w))); return 7;        /* ASL  $  */
  case 0x79: EXT; cpu->mputc(cpu->w, Rol(cpu, cpu->mgetc(cpu->w))); return 7;        /* ROL  $  */
  case 0x7a: EXT; cpu->mputc(cpu->w, Dec(cpu, cpu->mgetc(cpu->w))); return 7;        /* DEC  $  */
  case 0x7c: EXT; cpu->mputc(cpu->w, Inc(cpu, cpu->mgetc(cpu->w))); return 7;        /* INC  $  */
  case 0x7d: EXT; Tstc(cpu, cpu->mgetc(cpu->w)); return 7;                 /* TST  $  */
  case 0x7e: EXT; cpu->pc = cpu->w; return 4;                         /* JMP  $  */
  case 0x7f: EXT; cpu->mputc(cpu->w, Clr(cpu)); return 7;                /* CLR  $  */

  case 0x80: Subc(cpu, &cpu->a, cpu->mgetc(cpu->pc++)); return 2;               /* SUBA #$ */
  case 0x81: Cmpc(cpu, &cpu->a, cpu->mgetc(cpu->pc++)); return 2;               /* CMPA #$ */
  case 0x82: Sbc(cpu, &cpu->a, cpu->mgetc(cpu->pc++)); return 2;                /* SBCA #$ */
  case 0x83: EXT; Subw(cpu, &cpu->d, cpu->w); return 4;                    /* SUBD #$ */
  case 0x84: Tstc(cpu, cpu->a &= cpu->mgetc(cpu->pc++)); return 2;              /* ANDA #$ */
  case 0x85: Tstc(cpu, cpu->a & cpu->mgetc(cpu->pc++)); return 2;               /* BITA #$ */
  case 0x86: Tstc(cpu, cpu->a = cpu->mgetc(cpu->pc++)); return 2;               /* LDA  #$ */
  case 0x88: Tstc(cpu, cpu->a ^= cpu->mgetc(cpu->pc++)); return 2;              /* EORA #$ */
  case 0x89: Adc(cpu, &cpu->a, cpu->mgetc(cpu->pc++)); return 2;                /* ADCA #$ */
  case 0x8a: Tstc(cpu, cpu->a |= cpu->mgetc(cpu->pc++)); return 2;              /* ORA  #$ */
  case 0x8b: Addc(cpu, &cpu->a, cpu->mgetc(cpu->pc++)); return 2;               /* ADDA #$ */
  case 0x8c: EXT; Cmpw(cpu, &cpu->x, cpu->w); return 4;                    /* CMPX #$ */
  case 0x8d: DIR; Pshs(cpu, 0x80); cpu->pc += cpu->dd; return 7;           /* BSR     */
  case 0x8e: EXT; Tstw(cpu, cpu->x = cpu->w); return 3;                    /* LDX  #$ */

  case 0x90: DIR; Subc(cpu, &cpu->a, cpu->mgetc(cpu->da)); return 4;            /* SUBA /$ */
  case 0x91: DIR; Cmpc(cpu, &cpu->a, cpu->mgetc(cpu->da)); return 4;            /* CMPA /$ */
  case 0x92: DIR; Sbc(cpu, &cpu->a, cpu->mgetc(cpu->da)); return 4;             /* SBCA /$ */
  case 0x93: DIR; Subw(cpu, &cpu->d, mgetw(cpu, cpu->da));return 6;             /* SUBD /$ */
  case 0x94: DIR; Tstc(cpu, cpu->a &= cpu->mgetc(cpu->da)); return 4;           /* ANDA /$ */
  case 0x95: DIR; Tstc(cpu, cpu->a & cpu->mgetc(cpu->da)); return 4;            /* BITA /$ */
  case 0x96: DIR; Tstc(cpu, cpu->a = cpu->mgetc(cpu->da)); return 4;            /* LDA  /$ */
  case 0x97: DIR; cpu->mputc(cpu->da, cpu->a); Tstc(cpu, cpu->a); return 4;          /* STA  /$ */
  case 0x98: DIR; Tstc(cpu, cpu->a ^= cpu->mgetc(cpu->da)); return 4;           /* EORA /$ */
  case 0x99: DIR; Adc(cpu, &cpu->a, cpu->mgetc(cpu->da)); return 4;             /* ADCA /$ */
  case 0x9a: DIR; Tstc(cpu, cpu->a |= cpu->mgetc(cpu->da)); return 4;           /* ORA  /$ */
  case 0x9b: DIR; Addc(cpu, &cpu->a, cpu->mgetc(cpu->da)); return 4;            /* ADDA /$ */
  case 0x9c: DIR; Cmpw(cpu, &cpu->x, mgetw(cpu, cpu->da)); return 6;            /* CMPX /$ */
  case 0x9d: DIR; Pshs(cpu, 0x80); cpu->pc = cpu->da; return 7;            /* JSR  /$ */
  case 0x9e: DIR; Tstw(cpu, cpu->x = mgetw(cpu, cpu->da)); return 5;            /* LDX  /$ */
  case 0x9f: DIR; mputw(cpu, cpu->da, cpu->x); Tstw(cpu, cpu->x); return 5;          /* STX  /$ */

  case 0xa0: IND; Subc(cpu, &cpu->a, cpu->mgetc(cpu->w)); return 4 + cpu->n;         /* SUBA IX */
  case 0xa1: IND; Cmpc(cpu, &cpu->a, cpu->mgetc(cpu->w)); return 4 + cpu->n;         /* CMPA IX */
  case 0xa2: IND; Sbc(cpu, &cpu->a, cpu->mgetc(cpu->w)); return 4 + cpu->n;          /* SBCA IX */
  case 0xa3: IND; Subw(cpu, &cpu->d, mgetw(cpu, cpu->w)); return 6 + cpu->n;         /* SUBD IX */
  case 0xa4: IND; Tstc(cpu, cpu->a &= cpu->mgetc(cpu->w)); return 4 + cpu->n;        /* ANDA IX */
  case 0xa5: IND; Tstc(cpu, cpu->mgetc(cpu->w) & cpu->a); return 4 + cpu->n;         /* BITA IX */
  case 0xa6: IND; Tstc(cpu, cpu->a = cpu->mgetc(cpu->w)); return 4 + cpu->n;         /* LDA  IX */
  case 0xa7: IND; cpu->mputc(cpu->w,cpu->a); Tstc(cpu, cpu->a); return 4 + cpu->n;        /* STA  IX */
  case 0xa8: IND; Tstc(cpu, cpu->a ^= cpu->mgetc(cpu->w)); return 4 + cpu->n;        /* EORA IX */
  case 0xa9: IND; Adc(cpu, &cpu->a, cpu->mgetc(cpu->w)); return 4 + cpu->n;          /* ADCA IX */
  case 0xaa: IND; Tstc(cpu, cpu->a |= cpu->mgetc(cpu->w)); return 4 + cpu->n;        /* ORA  IX */
  case 0xab: IND; Addc(cpu, &cpu->a, cpu->mgetc(cpu->w)); return 4 + cpu->n;         /* ADDA IX */
  case 0xac: IND; Cmpw(cpu, &cpu->x, mgetw(cpu, cpu->w)); return 4 + cpu->n;         /* CMPX IX */
  case 0xad: IND; Pshs(cpu, 0x80); cpu->pc = cpu->w; return 5 + cpu->n;         /* JSR  IX */
  case 0xae: IND; Tstw(cpu, cpu->x = mgetw(cpu, cpu->w)); return 5 + cpu->n;         /* LDX  IX */
  case 0xaf: IND; mputw(cpu, cpu->w, cpu->x); Tstw(cpu, cpu->x); return 5 + cpu->n;       /* STX  IX */

  case 0xb0: EXT; Subc(cpu, &cpu->a, cpu->mgetc(cpu->w)); return 5;             /* SUBA $  */
  case 0xb1: EXT; Cmpc(cpu, &cpu->a, cpu->mgetc(cpu->w)); return 5;             /* CMPA $  */
  case 0xb2: EXT; Sbc(cpu, &cpu->a, cpu->mgetc(cpu->w)); return 5;              /* SBCA $  */
  case 0xb3: EXT; Subw(cpu, &cpu->d, mgetw(cpu, cpu->w)); return 7;             /* SUBD $  */
  case 0xb4: EXT; Tstc(cpu, cpu->a &= cpu->mgetc(cpu->w)); return 5;            /* ANDA $  */
  case 0xb5: EXT; Tstc(cpu, cpu->a & cpu->mgetc(cpu->w)); return 5;             /* BITA $  */
  case 0xb6: EXT; Tstc(cpu, cpu->a = cpu->mgetc(cpu->w)); return 5;             /* LDA  $  */
  case 0xb7: EXT; cpu->mputc(cpu->w, cpu->a); Tstc(cpu, cpu->a); return 5;           /* STA  $  */
  case 0xb8: EXT; Tstc(cpu, cpu->a ^= cpu->mgetc(cpu->w)); return 5;            /* EORA $  */
  case 0xb9: EXT; Adc(cpu, &cpu->a, cpu->mgetc(cpu->w)); return 5;              /* ADCA $  */
  case 0xba: EXT; Tstc(cpu, cpu->a |= cpu->mgetc(cpu->w)); return 5;            /* ORA  $  */
  case 0xbb: EXT; Addc(cpu, &cpu->a, cpu->mgetc(cpu->w)); return 5;             /* ADDA $  */
  case 0xbc: EXT; Cmpw(cpu, &cpu->x, mgetw(cpu, cpu->w)); return 7;             /* CMPX $  */
  case 0xbd: EXT; Pshs(cpu, 0x80); cpu->pc = cpu->w; return 8;             /* JSR  $  */
  case 0xbe: EXT; Tstw(cpu, cpu->x = mgetw(cpu, cpu->w)); return 6;             /* LDX  $  */
  case 0xbf: EXT; mputw(cpu, cpu->w, cpu->x); Tstw(cpu, cpu->x); return 6;           /* STX  $  */

  case 0xc0: Subc(cpu, &cpu->b, cpu->mgetc(cpu->pc++)); return 2;               /* SUBB #$ */
  case 0xc1: Cmpc(cpu, &cpu->b, cpu->mgetc(cpu->pc++)); return 2;               /* CMPB #$ */
  case 0xc2: Sbc(cpu, &cpu->b, cpu->mgetc(cpu->pc++)); return 2;                /* SBCB #$ */
  case 0xc3: EXT; Addw(cpu, &cpu->d, cpu->w); return 4;                    /* ADDD #$ */
  case 0xc4: Tstc(cpu, cpu->b &= cpu->mgetc(cpu->pc++)); return 2;              /* ANDB #$ */
  case 0xc5: Tstc(cpu, cpu->b & cpu->mgetc(cpu->pc++)); return 2;               /* BITB #$ */
  case 0xc6: Tstc(cpu, cpu->b = cpu->mgetc(cpu->pc++)); return 2;               /* LDB  #$ */
  case 0xc8: Tstc(cpu, cpu->b ^= cpu->mgetc(cpu->pc++)); return 2;              /* EORB #$ */
  case 0xc9: Adc(cpu, &cpu->b, cpu->mgetc(cpu->pc++)); return 2;                /* ADCB #$ */
  case 0xca: Tstc(cpu, cpu->b |= cpu->mgetc(cpu->pc++)); return 2;              /* ORB  #$ */
  case 0xcb: Addc(cpu, &cpu->b, cpu->mgetc(cpu->pc++));return 2;                /* ADDB #$ */
  case 0xcc: EXT; Tstw(cpu, cpu->d = cpu->w); return 3;                    /* LDD  #$ */
  case 0xce: EXT; Tstw(cpu, cpu->u = cpu->w); return 3;                    /* LDU  #$ */

  case 0xd0: DIR; Subc(cpu, &cpu->b, cpu->mgetc(cpu->da)); return 4;            /* SUBB /$ */
  case 0xd1: DIR; Cmpc(cpu, &cpu->b, cpu->mgetc(cpu->da)); return 4;            /* CMPB /$ */
  case 0xd2: DIR; Sbc(cpu, &cpu->b, cpu->mgetc(cpu->da)); return 4;             /* SBCB /$ */
  case 0xd3: DIR; Addw(cpu, &cpu->d, mgetw(cpu, cpu->da)); return 6;            /* ADDD /$ */
  case 0xd4: DIR; Tstc(cpu, cpu->b &= cpu->mgetc(cpu->da)); return 4;           /* ANDB /$ */
  case 0xd5: DIR; Tstc(cpu,cpu->mgetc(cpu->da) & cpu->b); return 4;            /* BITB /$ */
  case 0xd6: DIR; Tstc(cpu, cpu->b = cpu->mgetc(cpu->da)); return 4;            /* LDB  /$ */
  case 0xd7: DIR; cpu->mputc(cpu->da, cpu->b); Tstc(cpu, cpu->b); return 4;           /* STB  /$ */
  case 0xd8: DIR; Tstc(cpu, cpu->b ^= cpu->mgetc(cpu->da)); return 4;           /* EORB /$ */
  case 0xd9: DIR; Adc(cpu, &cpu->b, cpu->mgetc(cpu->da)); return 4;             /* ADCB /$ */
  case 0xda: DIR; Tstc(cpu, cpu->b |= cpu->mgetc(cpu->da)); return 4;           /* ORB  /$ */
  case 0xdb: DIR; Addc(cpu, &cpu->b, cpu->mgetc(cpu->da)); return 4;            /* ADDB /$ */
  case 0xdc: DIR; Tstw(cpu, cpu->d = mgetw(cpu, cpu->da)); return 5;            /* LDD  /$ */
  case 0xdd: DIR; mputw(cpu, cpu->da, cpu->d); Tstw(cpu, cpu->d); return 5;          /* STD  /$ */
  case 0xde: DIR; Tstw(cpu, cpu->u = mgetw(cpu, cpu->da)); return 5;            /* LDU  /$ */
  case 0xdf: DIR; mputw(cpu, cpu->da, cpu->u); Tstw(cpu, cpu->u); return 5;          /* STU  /$ */

  case 0xe0: IND; Subc(cpu, &cpu->b, cpu->mgetc(cpu->w)); return 4 + cpu->n;         /* SUBB IX */
  case 0xe1: IND; Cmpc(cpu, &cpu->b, cpu->mgetc(cpu->w)); return 4 + cpu->n;         /* CMPB IX */
  case 0xe2: IND; Sbc(cpu, &cpu->b, cpu->mgetc(cpu->w)); return 4 + cpu->n;          /* SBCB IX */
  case 0xe3: IND; Addw(cpu, &cpu->d, mgetw(cpu, cpu->w)); return 6 + cpu->n;         /* ADDD IX */
  case 0xe4: IND; Tstc(cpu, cpu->b &= cpu->mgetc(cpu->w)); return 4 + cpu->n;        /* ANDB IX */
  case 0xe5: IND; Tstc(cpu,cpu->mgetc(cpu->w) & cpu->b); return 4 + cpu->n;         /* BITB IX */
  case 0xe6: IND; Tstc(cpu, cpu->b = cpu->mgetc(cpu->w)); return 4 + cpu->n;         /* LDB  IX */
  case 0xe7: IND; cpu->mputc(cpu->w, cpu->b); Tstc(cpu, cpu->b); return 4 + cpu->n;       /* STB  IX */
  case 0xe8: IND; Tstc(cpu, cpu->b ^= cpu->mgetc(cpu->w)); return 4 + cpu->n;        /* EORB IX */
  case 0xe9: IND; Adc(cpu, &cpu->b, cpu->mgetc(cpu->w)); return 4 + cpu->n;          /* ADCB IX */
  case 0xea: IND; Tstc(cpu, cpu->b |=cpu->mgetc(cpu->w)); return 4 + cpu->n;        /* ORB  IX */
  case 0xeb: IND; Addc(cpu, &cpu->b,cpu->mgetc(cpu->w)); return 4 + cpu->n;         /* ADDB IX */
  case 0xec: IND; Tstw(cpu, cpu->d = mgetw(cpu, cpu->w)); return 5 + cpu->n;         /* LDD  IX */
  case 0xed: IND; mputw(cpu, cpu->w, cpu->d); Tstw(cpu, cpu->d); return 5 + cpu->n;       /* STD  IX */
  case 0xee: IND; Tstw(cpu, cpu->u = mgetw(cpu, cpu->w)); return 5 + cpu->n;         /* LDU  IX */
  case 0xef: IND; mputw(cpu, cpu->w, cpu->u); Tstw(cpu, cpu->u); return 5 + cpu->n;       /* STU  IX */

  case 0xf0: EXT; Subc(cpu, &cpu->b,cpu->mgetc(cpu->w)); return 5;             /* SUBB $  */
  case 0xf1: EXT; Cmpc(cpu, &cpu->b,cpu->mgetc(cpu->w)); return 5;             /* CMPB $  */
  case 0xf2: EXT; Sbc(cpu, &cpu->b,cpu->mgetc(cpu->w)); return 5;              /* SBCB $  */
  case 0xf3: EXT; Addw(cpu, &cpu->d, mgetw(cpu, cpu->w)); return 7;             /* ADDD $  */
  case 0xf4: EXT; Tstc(cpu, cpu->b &=cpu->mgetc(cpu->w)); return 5;            /* ANDB $  */
  case 0xf5: EXT; Tstc(cpu, cpu->b &cpu->mgetc(cpu->w)); return 5;             /* BITB $  */
  case 0xf6: EXT; Tstc(cpu, cpu->b =cpu->mgetc(cpu->w)); return 5;             /* LDB  $  */
  case 0xf7: EXT; cpu->mputc(cpu->w, cpu->b); Tstc(cpu, cpu->b); return 5;      /* STB  $  */
  case 0xf8: EXT; Tstc(cpu, cpu->b ^=cpu->mgetc(cpu->w)); return 5;            /* EORB $  */
  case 0xf9: EXT; Adc(cpu, &cpu->b,cpu->mgetc(cpu->w)); return 5;              /* ADCB $  */
  case 0xfa: EXT; Tstc(cpu, cpu->b |=cpu->mgetc(cpu->w)); return 5;            /* ORB  $  */
  case 0xfb: EXT; Addc(cpu, &cpu->b,cpu->mgetc(cpu->w)); return 5;             /* ADDB $  */
  case 0xfc: EXT; Tstw(cpu, cpu->d = mgetw(cpu, cpu->w)); return 6;             /* LDD  $  */
  case 0xfd: EXT; mputw(cpu, cpu->w, cpu->d); Tstw(cpu, cpu->d); return 6;      /* STD  $  */
  case 0xfe: EXT; Tstw(cpu, cpu->u = mgetw(cpu, cpu->w)); return 6;             /* LDU  $  */
  case 0xff: EXT; mputw(cpu, cpu->w, cpu->u); Tstw(cpu, cpu->u); return 6;      /* STU  $  */

  case 0x1021: cpu->pc += 2; return 5;                           /* LBRN    */
  case 0x1022: if(BHI) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBHI    */
  case 0x1023: if(BLS) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBLS    */
  case 0x1024: if(BCC) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBCC    */
  case 0x1025: if(BCS) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBCS    */
  case 0x1026: if(BNE) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBNE    */
  case 0x1027: if(BEQ) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBEQ    */
  case 0x1028: if(BVC) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBVC    */
  case 0x1029: if(BVS) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBVS    */
  case 0x102a: if(BPL) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBPL    */
  case 0x102b: if(BMI) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBMI    */
  case 0x102c: if(BGE) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBGE    */
  case 0x102d: if(BLT) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBLT    */
  case 0x102e: if(BGT) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBGT    */
  case 0x102f: if(BLE) LBRANCH; cpu->pc += 2; return 5 + cpu->n; /* LBLE    */
  case 0x103f: Swi(cpu, 2); return 20;                           /* SWI2    */

  case 0x1083: EXT; Cmpw(cpu, &cpu->d, cpu->w); return 5;                  /* CMPD #$ */
  case 0x108c: EXT; Cmpw(cpu, &cpu->y, cpu->w); return 5;                  /* CMPY #$ */
  case 0x108e: EXT; Tstw(cpu, cpu->y = cpu->w); return 4;                  /* LDY  #$ */
  case 0x1093: DIR; Cmpw(cpu, &cpu->d, mgetw(cpu, cpu->da)); return 7;          /* CMPD /$ */
  case 0x109c: DIR; Cmpw(cpu, &cpu->y, mgetw(cpu, cpu->da)); return 7;          /* CMPY /$ */
  case 0x109e: DIR; Tstw(cpu, cpu->y = mgetw(cpu, cpu->da)); return 6;          /* LDY  /$ */
  case 0x109f: DIR; mputw(cpu, cpu->da, cpu->y); Tstw(cpu, cpu->y); return 6;        /* STY  /$ */
  case 0x10a3: IND; Cmpw(cpu, &cpu->d, mgetw(cpu, cpu->w)); return 7 + cpu->n;       /* CMPD IX */
  case 0x10ac: IND; Cmpw(cpu, &cpu->y, mgetw(cpu, cpu->w)); return 7 + cpu->n;       /* CMPY IX */
  case 0x10ae: IND; Tstw(cpu, cpu->y = mgetw(cpu, cpu->w)); return 6 + cpu->n;       /* LDY  IX */
  case 0x10af: IND; mputw(cpu, cpu->w, cpu->y); Tstw(cpu, cpu->y); return 6 + cpu->n;     /* STY  IX */
  case 0x10b3: EXT; Cmpw(cpu, &cpu->d, mgetw(cpu, cpu->w)); return 8;           /* CMPD $  */
  case 0x10bc: EXT; Cmpw(cpu, &cpu->y, mgetw(cpu, cpu->w)); return 8;           /* CMPY $  */
  case 0x10be: EXT; Tstw(cpu, cpu->y = mgetw(cpu, cpu->w)); return 7;           /* LDY  $  */
  case 0x10bf: EXT; mputw(cpu, cpu->w, cpu->y); Tstw(cpu, cpu->y); return 7;         /* STY  $  */
  case 0x10ce: EXT; Tstw(cpu, cpu->s = cpu->w); return 4;                  /* LDS  #$ */
  case 0x10de: DIR; Tstw(cpu, cpu->s = mgetw(cpu, cpu->da)); return 6;          /* LDS  /$ */
  case 0x10df: DIR; mputw(cpu, cpu->da, cpu->s); Tstw(cpu, cpu->s); return 6;        /* STS  /$ */
  case 0x10ee: IND; Tstw(cpu, cpu->s = mgetw(cpu, cpu->w)); return 6 + cpu->n;       /* LDS  IX */
  case 0x10ef: IND; mputw(cpu, cpu->w, cpu->s); Tstw(cpu, cpu->s); return 6 + cpu->n;     /* STS  IX */
  case 0x10fe: EXT; Tstw(cpu, cpu->s = mgetw(cpu, cpu->w)); return 7;           /* LDS  $  */
  case 0x10ff: EXT; mputw(cpu, cpu->w, cpu->s); Tstw(cpu, cpu->s); return 7;         /* STS  $  */

  case 0x113f: Swi(cpu, 3); return 20;                                    /* SWI3    */
  case 0x1183: EXT; Cmpw(cpu, &cpu->u, cpu->w); return 5;                 /* CMPU #$ */
  case 0x118c: EXT; Cmpw(cpu, &cpu->s, cpu->w); return 5;                 /* CMPS #$ */
  case 0x1193: DIR; Cmpw(cpu, &cpu->u, mgetw(cpu, cpu->da)); return 7;         /* CMPU /$ */
  case 0x119c: DIR; Cmpw(cpu, &cpu->s, mgetw(cpu, cpu->da)); return 7;         /* CMPS /$ */
  case 0x11a3: IND; Cmpw(cpu, &cpu->u, mgetw(cpu, cpu->w)); return 7 + cpu->n; /* CMPU IX */
  case 0x11ac: IND; Cmpw(cpu, &cpu->s, mgetw(cpu, cpu->w)); return 7 + cpu->n; /* CMPS IX */
  case 0x11b3: EXT; Cmpw(cpu, &cpu->u, mgetw(cpu, cpu->w)); return 8;          /* CMPU $  */
  case 0x11bc: EXT; Cmpw(cpu, &cpu->s, mgetw(cpu, cpu->w)); return 8;          /* CMPS $  */

  default: return -code;                                    /* Illegal */
 }
}
