#ifdef __cplusplus
extern "C" {
#endif

/* the input callback type */
typedef uint8_t (*m6502dasm_input_t)(void* user_data);
/* the output callback type */
typedef void (*m6502dasm_output_t)(char c, void* user_data);

/* disassemble a single 6809 instruction into a stream of ASCII characters */
void m6809dasm_op(uint16_t pc, ui_dasm_in_cb_t in_cb, ui_dasm_out_cb_t out_cb, void* user_data);
/* check if the current 6809 instruction contains a jump target */
bool m6809_jump_tgt(void* w, uint16_t pc, uint16_t* out_addr, void* user_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef EMU_UTIL_IMPL
#ifndef EMU_ASSERT
    #include <assert.h>
    #define EMU_ASSERT(c) assert(c)
#endif

/* output string */
#ifdef _STR
#undef _STR
#endif
#define _STR(s) _mc6809dasm_str(s,out_cb,user_data);
/* output number as unsigned 8-bit string (hex) */
#ifdef _STR_U8
#undef _STR_U8
#endif
#define _STR_U8(u8) _mc6809dasm_u8((uint8_t)(u8),out_cb,user_data);
/* output number as unsigned 16-bit string (hex) */
#ifdef _STR_U16
#undef _STR_U16
#endif
#define _STR_U16(u16) _mc6809dasm_u16((uint16_t)(u16),out_cb,user_data);
/* output number as unsigned 16-bit string (hex) */
#ifdef _STR_I16
#undef _STR_I16
#endif
/* output number as signed 16-bit string (hex) */
#define _STR_I16(i) _mc6809dasm_i16(i,out_cb,user_data);
#ifdef _STR_MGETI
#undef _STR_MGETI
#endif
#define _STR_MGETI() _mc6809dasm_Mgeti(pc,in_cb,out_cb,user_data);
#ifdef _STR_EXG
#undef _STR_EXG
#endif
#define _STR_EXG(c) _mc6809dasm_exg(c,out_cb,user_data);
#ifdef _STR_TFR
#undef _STR_TFR
#endif
#define _STR_TFR(c) _mc6809dasm_tfr(c,out_cb,user_data);
#ifdef _STR_PULU
#undef _STR_PULU
#endif
#define _STR_PULU(c) _mc6809dasm_pulu(c,out_cb,user_data);
#ifdef _STR_PULS
#undef _STR_PULS
#endif
#define _STR_PULS(c) _mc6809dasm_puls(c,out_cb,user_data);
#ifdef _STR_PSHS
#undef _STR_PSHS
#endif
#define _STR_PSHS(c) _mc6809dasm_pshs(c,out_cb,user_data);
#ifdef _STR_PSHU
#undef _STR_PSHU
#endif
#define _STR_PSHU(c) _mc6809dasm_pshu(c,out_cb,user_data);
/* fetch unsigned 8-bit value and track pc */
#ifdef _FETCH_U8
#undef _FETCH_U8
#endif
#define _FETCH_U8(v) v=in_cb(user_data);pc++;
/* fetch signed 8-bit value and track pc */
#ifdef _FETCH_I8
#undef _FETCH_I8
#endif
#define _FETCH_I8(v) v=(int8_t)in_cb(user_data);pc++;
/* fetch unsigned 16-bit value and track pc */
#ifdef _FETCH_U16
#undef _FETCH_U16
#endif
#define _FETCH_U16(v) v=in_cb(user_data)<<8;v|=in_cb(user_data);pc+=2;

bool m6809_jump_tgt(void* w, uint16_t pc, uint16_t* out_addr, void* user_data) {
    (void)user_data;
    ui_dasm_t* win = (ui_dasm_t*)w;
    if (win->bin_pos == 3) {
        switch (win->bin_buf[0]) {
            /* LBRA, LBSR, JMP, JSR */
            case 0x0e:
            case 0x17:
            case 0x7e:
            case 0xbd:
                *out_addr = (win->bin_buf[1] << 8) | win->bin_buf[2];
            return true;
        }
    } else if (win->bin_pos == 2) {
        switch (win->bin_buf[0]) {
            case 0x10:
                switch (win->bin_buf[1]) {
                    case 0x21:
                    case 0x22:
                    case 0x23:
                    case 0x24:
                    case 0x25:
                    case 0x26:
                    case 0x27:
                    case 0x28:
                    case 0x29:
                    case 0x2a:
                    case 0x2b:
                    case 0x2c:
                    case 0x2d:
                    case 0x2e:
                    case 0x2f:
                        *out_addr = pc + 2;
                        return true;
                    default:
                    return false;
                }
            /* BRA, BRN, BHI, BLS, BCC, BCS, BNE, BEQ, BVC, BVS, BPL, BMI, BGE, BLT, BGT, BLE, JSR */
            case 0x20:
            case 0x22:
            case 0x23:
            case 0x24:
            case 0x25:
            case 0x26:
            case 0x27:
            case 0x28:
            case 0x29:
            case 0x2a:
            case 0x2b:
            case 0x2c:
            case 0x2d:
            case 0x2e:
            case 0x2f:
            case 0x9d:
                *out_addr = pc + (int8_t)win->bin_buf[1];
            return true;
        }
    }
    return false;
}

/* helper function to output string */
static void _mc6809dasm_str(const char* str, ui_dasm_out_cb_t out_cb, void* user_data) {
    if (out_cb) {
        char c;
        while (0 != (c = *str++)) {
            out_cb(c, user_data);
        }
    }
}

static const char* _mc6809dasm_hex = "0123456789ABCDEF";

/* helper function to output an unsigned 8-bit value as hex string */
static void _mc6809dasm_u8(uint8_t val, ui_dasm_out_cb_t out_cb, void* user_data) {
    if (out_cb) {
        out_cb('$', user_data);
        for (int i = 1; i >= 0; i--) {
            out_cb(_mc6809dasm_hex[(val>>(i*4)) & 0xF], user_data);
        }
    }
}

/* helper function to output an unsigned 16-bit value as hex string */
static void _mc6809dasm_u16(uint16_t val, ui_dasm_out_cb_t out_cb, void* user_data) {
    if (out_cb) {
        out_cb('$', user_data);
        for (int i = 3; i >= 0; i--) {
            out_cb(_mc6809dasm_hex[(val>>(i*4)) & 0xF], user_data);
        }
    }
}

/* helper function to output an signed 16-bit value as hex string */
static void _mc6809dasm_i16(int16_t val, ui_dasm_out_cb_t out_cb, void* user_data) {
    if (out_cb) {
        uint16_t u = (uint16_t)val;
        out_cb('$', user_data);
        for (int i = 3; i >= 0; i--) {
            out_cb(_mc6809dasm_hex[(u>>(i*4)) & 0xF], user_data);
        }
    }
}

static void _mc6809dasm_Mgeti(uint16_t pc, ui_dasm_in_cb_t in_cb, ui_dasm_out_cb_t out_cb, void* user_data) {
    (void)pc;
    int16_t i;
    uint8_t u8;
    uint16_t u16;
    const char* r;
    _FETCH_U8(i);
    switch (i & 0x60) {
    case 0x00:r = "X";
      break;
    case 0x20:r = "Y";
      break;
    case 0x40:r = "U";
      break;
    case 0x60:r = "S";
      break;
    default: r = "X";
    }
    switch (i &= 0x9f) {
    case 0x80: _STR(","); _STR(r); _STR("+"); break;                                            // ,R+
    case 0x81: _STR(","); _STR(r); _STR("++"); break;                                           // ,R++
    case 0x82: _STR(",-"); _STR(r); break;                                                      // ,-R
    case 0x83: _STR(",--"); _STR(r); break;                                                     // ,--R
    case 0x84: _STR(","); _STR(r); break;                                                       // ,R
    case 0x85: _STR("B,"); _STR(r); break;                                                      // B,R
    case 0x86: _STR("A,"); _STR(r); break;                                                      // A,R
    case 0x88: _FETCH_U8(u8); _STR_U8(u8); _STR(","); _STR(r); break;                           // char,R
    case 0x89: _FETCH_U16(u16); _STR_U16(u16); _STR(","); _STR(r); break;                       // word,R
    case 0x8a: _STR("invalid"); break;                                                          // invalid
    case 0x8b: _STR("D,"); _STR(r); break;                                                      // D,R
      //    case 0x8c: n = 1; W = Mgetc(PC()++); W += PC(); return;                             // char,PCR
      //    case 0x8d: n = 5; EXT; W += PC(); return;                                           // word,PCR
    case 0x8e: _STR("invalid"); break;                                                          // invalid
    case 0x8f: _STR("invalid"); break;                                                          // invalid
    case 0x90: _STR("invalid"); break;                                                          // invalid
    case 0x91: _STR("[,"); _STR(r); _STR("++]"); break;                                         // [,R++]
    case 0x92: _STR("invalid"); break;                                                          // invalid
    case 0x93: _STR("[,--"); _STR(r); _STR("]"); break;                                         // [,--R]
    case 0x94: _STR("[,"); _STR(r); _STR("]"); break;                                           // [,R]
    case 0x95: _STR("[B,"); _STR(r); _STR("]"); break;                                          // [B,R]
    case 0x96: _STR("[A,"); _STR(r); _STR("]"); break;                                          // [A,R]
    case 0x97: _STR("invalid"); break;                                                          // invalid
    case 0x98: _STR("[$"); _FETCH_U8(u8); _STR_U8(u8); _STR(","); _STR(r); _STR("]"); break;    // [char,R]
    case 0x99: _STR("[$"); _FETCH_U16(u16); _STR_U16(u16); _STR(","); _STR(r); _STR("]"); break;// [word,R]
    case 0x9a: _STR("invalid"); break;                                                          // invalid
    case 0x9b: _STR("[D,"); _STR(r); _STR("]"); break;                                          // [D,R]
      //    case 0x9c: n = 4; W = Mgetw(PC()+1+Mgetc(PC())); PC()++; return;                    // [char,PCR]
      //    case 0x9d: n = 8; EXT; W = Mgetw(PC() + W); return;                                 // [word,PCR]
    case 0x9e: _STR("invalid"); break;                                                          // invalid
    case 0x9f: _STR("[$"); _FETCH_U16(u16); _STR_U16(u16); _STR("]"); break;                    // [word]
    default:
      if (i & 0x10) {
        i -= 0x20;
      }
      if (i < 0) {
        _STR("-"); _STR_I16(-i); _STR(","); _STR(r); return;                                     // 5 bits,R
      }
      _STR_I16(i); _STR(","); _STR(r);                                                           // 5 bits,R
    }
}

static void _mc6809dasm_exg(uint8_t c, ui_dasm_out_cb_t out_cb, void* user_data) {
    const char* r = NULL;
    switch (c & 0xf0) {
    case 0x00: r = "D";
      break;
    case 0x10: r = "X";
      break;
    case 0x20: r = "Y";
      break;
    case 0x30: r = "U";
      break;
    case 0x40: r = "S";
      break;
    case 0x50: r = "PC";
      break;
    case 0x80: r = "A";
      break;
    case 0x90: r = "B";
      break;
    case 0xa0: r = "CC";
      break;
    case 0xb0: r = "DP";
      break;
    default:r = "?";
      break;
    }
    _STR(r);
    _STR(",");
    switch (c & 0x0f) {
    case 0x00: r = "D";
      break;
    case 0x01: r = "X";
      break;
    case 0x02: r = "Y";
      break;
    case 0x03: r = "U";
      break;
    case 0x04: r = "S";
      break;
    case 0x05: r = "PC";
      break;
    case 0x08: r = "A";
      break;
    case 0x09: r = "B";
      break;
    case 0x0a: r = "CC";
      break;
    case 0x0b: r = "DP";
      break;
    default:r = "?";
      break;
    }
    _STR(r);
}

static void _mc6809dasm_tfr(uint8_t c, ui_dasm_out_cb_t out_cb, void* user_data) {
    switch (c) {
        case 0x01: _STR("D,X"); break;
        case 0x02: _STR("D,Y"); break;
        case 0x03: _STR("D,U"); break;
        case 0x04: _STR("D,S"); break;
        case 0x05: _STR("D,PC"); break;
        case 0x10: _STR("X,D"); break;
        case 0x12: _STR("X,Y"); break;
        case 0x13: _STR("X,U"); break;
        case 0x14: _STR("X,S"); break;
        case 0x15: _STR("X,PC"); break;
        case 0x20: _STR("Y,D"); break;
        case 0x21: _STR("Y,X"); break;
        case 0x23: _STR("Y,U"); break;
        case 0x24: _STR("Y,S"); break;
        case 0x25: _STR("Y,PC"); break;
        case 0x30: _STR("U,D"); break;
        case 0x31: _STR("U,X"); break;
        case 0x32: _STR("U,Y"); break;
        case 0x34: _STR("U,S"); break;
        case 0x35: _STR("U,PC"); break;
        case 0x40: _STR("S,D"); break;
        case 0x41: _STR("S,X"); break;
        case 0x42: _STR("S,Y"); break;
        case 0x43: _STR("S,U"); break;
        case 0x45: _STR("S,PC"); break;
        case 0x50: _STR("PC,D"); break;
        case 0x51: _STR("PC,X"); break;
        case 0x52: _STR("PC,Y"); break;
        case 0x53: _STR("PC,U"); break;
        case 0x54: _STR("PC,S"); break;
        case 0x89: _STR("A,B"); break;
        case 0x8a: _STR("A,CC"); break;
        case 0x8b: _STR("A,DP"); break;
        case 0x98: _STR("B,A"); break;
        case 0x9a: _STR("B,CC"); break;
        case 0x9b: _STR("B,DP"); break;
        case 0xa8: _STR("CC,A"); break;
        case 0xa9: _STR("CC,B"); break;
        case 0xab: _STR("CC,DP"); break;
        case 0xb8: _STR("DP,A"); break;
        case 0xb9: _STR("DP,B"); break;
        case 0xba: _STR("DP,CC"); break;
        default: _STR("??");
    }
}

static void _mc6809dasm_pshs(uint8_t c, ui_dasm_out_cb_t out_cb, void* user_data) {
    const char* regs[8];
    int i = 0;
    if (c & 0x80)
      regs[i++] = "PC";
    if (c & 0x40)
      regs[i++] = "U";
    if (c & 0x20)
      regs[i++] = "Y";
    if (c & 0x10)
      regs[i++] = "X";
    if (c & 0x08)
      regs[i++] = "DP";
    if (c & 0x04)
      regs[i++] = "B";
    if (c & 0x02)
      regs[i++] = "A";
    if (c & 0x01)
      regs[i++] = "CC";
    for(int j=0;j<i-1;j++) {
        _STR(regs[j]);
        _STR(",");
    }
    if(i > 0) {
        _STR(regs[i-1]);
    }
}

static void _mc6809dasm_pshu(uint8_t c, ui_dasm_out_cb_t out_cb, void* user_data) {
    const char* regs[8];
    int i = 0;
    if (c & 0x80)
      regs[i++] = "PC";
    if (c & 0x40)
      regs[i++] = "S";
    if (c & 0x20)
      regs[i++] = "Y";
    if (c & 0x10)
      regs[i++] = "X";
    if (c & 0x08)
      regs[i++] = "DP";
    if (c & 0x04)
      regs[i++] = "B";
    if (c & 0x02)
      regs[i++] = "A";
    if (c & 0x01)
      regs[i++] = "CC";
    for(int j=0;j<i-1;j++) {
        _STR(regs[j]);
        _STR(",");
    }
    if(i > 0) {
        _STR(regs[i-1]);
    }
}

static void _mc6809dasm_pulu(uint8_t c, ui_dasm_out_cb_t out_cb, void* user_data) {
    const char* regs[8];
    int i = 0;
    if (c & 0x01)
      regs[i++] = "CC";
    if (c & 0x02)
      regs[i++] = "A";
    if (c & 0x04)
      regs[i++] = "B";
    if (c & 0x08)
      regs[i++] = "DP";
    if (c & 0x10)
      regs[i++] = "X";
    if (c & 0x20)
      regs[i++] = "Y";
    if (c & 0x40)
      regs[i++] = "S";
    if (c & 0x80)
      regs[i++] = "PC";
    for(int j=0;j<i-1;j++) {
        _STR(regs[j]);
        _STR(",");
    }
    if(i > 0) {
        _STR(regs[i-1]);
    }
}

static void _mc6809dasm_puls(uint8_t c, ui_dasm_out_cb_t out_cb, void* user_data) {
    const char* regs[8];
    int i = 0;
    if (c & 0x01)
      regs[i++] = "CC";
    if (c & 0x02)
      regs[i++] = "A";
    if (c & 0x04)
      regs[i++] = "B";
    if (c & 0x08)
      regs[i++] = "DP";
    if (c & 0x10)
      regs[i++] = "X";
    if (c & 0x20)
      regs[i++] = "Y";
    if (c & 0x40)
      regs[i++] = "U";
    if (c & 0x80)
      regs[i++] = "PC";
    for(int j=0;j<i-1;j++) {
        _STR(regs[j]);
        _STR(",");
    }
    if(i > 0) {
        _STR(regs[i-1]);
    }
}

void m6809dasm_op(uint16_t pc, ui_dasm_in_cb_t in_cb, ui_dasm_out_cb_t out_cb, void* user_data) {
    int opcode;
    _FETCH_U8(opcode);
    if (opcode == 0x10) {
        _FETCH_U8(opcode);
        opcode |= 0x1000;
    }
    if (opcode == 0x11) {
        _FETCH_U8(opcode);
        opcode |= 0x1100;
    }
    uint8_t u8;
    int8_t i8;
    uint16_t u16;
    switch (opcode) {
        case 0x00: _STR("NEG <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x01: _STR("NEG <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x03: _STR("COM <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x04: _STR("LSR <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x05: _STR("LSR <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x06: _STR("ROR <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x07: _STR("ASR <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x08: _STR("ASL <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x09: _STR("ROL <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x0a: _STR("DEC <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x0c: _STR("INC <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x0d: _STR("TST <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x0e: _STR("JMP <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x0f: _STR("CLR <"); _FETCH_U8(u8); _STR_U8(u8); break;

        case 0x12: _STR("NOP"); break;
        case 0x13: _STR("SYNC"); break;
        case 0x16: _STR("LBRA "); _FETCH_U16(u16); _STR_U16(pc+u16); break;
        case 0x17: _STR("LBSR "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x19: _STR("DAA"); break;
        case 0x1a: _STR("ORCC #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x1c: _STR("ANDCC #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x1d: _STR("SEX"); break;
        case 0x1e: _STR("EXG "); _FETCH_U8(u8); _STR_EXG(u8); break;
        case 0x1f: _STR("TFR "); _FETCH_U8(u8); _STR_TFR(u8); break;

        case 0x20:_STR("BRA "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x21:_STR("BRN "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x22:_STR("BHI "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x23:_STR("BLS "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x24:_STR("BCC "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x25:_STR("BCS "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x26:_STR("BNE "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x27:_STR("BEQ "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x28:_STR("BVC "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x29:_STR("BVS "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x2a:_STR("BPL "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x2b:_STR("BMI "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x2c:_STR("BGE "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x2d:_STR("BLT "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x2e:_STR("BGT "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x2f:_STR("BLE "); _FETCH_I8(i8); _STR_I16(pc+i8); break;

        case 0x30:_STR("LEAX "); _STR_MGETI(); break;
        case 0x31:_STR("LEAY "); _STR_MGETI(); break;
        case 0x32:_STR("LEAS "); _STR_MGETI(); break;
        case 0x33:_STR("LEAU "); _STR_MGETI(); break;
        case 0x34:_STR("PSHS "); _FETCH_U8(u8); _STR_PSHS(u8); break;
        case 0x35:_STR("PULS "); _FETCH_U8(u8); _STR_PULS(u8); break;
        case 0x36:_STR("PSHU "); _FETCH_U8(u8); _STR_PSHU(u8); break;
        case 0x37:_STR("PULU "); _FETCH_U8(u8); _STR_PULU(u8); break;
        case 0x39:_STR("RTS"); break;
        case 0x3a:_STR("ABX"); break;
        case 0x3b:_STR("RTI"); break;
        case 0x3c:_STR("CWAI #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x3d:_STR("MUL"); break;
        case 0x3f:_STR("SWI #"); _FETCH_U8(u8); _STR_U8(u8); break;

        case 0x40: _STR("NEGA"); break;
        case 0x43: _STR("COMA"); break;
        case 0x44: _STR("LSRA"); break;
        case 0x46: _STR("RORA"); break;
        case 0x47: _STR("ASRA"); break;
        case 0x48: _STR("ASLA"); break;
        case 0x49: _STR("ROLA"); break;
        case 0x4a: _STR("DECA"); break;
        case 0x4c: _STR("INCA"); break;
        case 0x4d: _STR("TSTA"); break;
        case 0x4f: _STR("CLRA"); break;

        case 0x50: _STR("NEGB"); break;
        case 0x53: _STR("COMB"); break;
        case 0x54: _STR("LSRB"); break;
        case 0x56: _STR("RORB"); break;
        case 0x57: _STR("ASRB"); break;
        case 0x58: _STR("ASLB"); break;
        case 0x59: _STR("ROLB"); break;
        case 0x5a: _STR("DECB"); break;
        case 0x5c: _STR("INCB"); break;
        case 0x5d: _STR("TSTB"); break;
        case 0x5f: _STR("CLRB"); break;

        case 0x60:_STR("NEG "); _STR_MGETI(); break;
        case 0x63:_STR("COM "); _STR_MGETI(); break;
        case 0x64:_STR("LSR "); _STR_MGETI(); break;
        case 0x66:_STR("ROR "); _STR_MGETI(); break;
        case 0x67:_STR("ASR "); _STR_MGETI(); break;
        case 0x68:_STR("ASL "); _STR_MGETI(); break;
        case 0x69:_STR("ROL "); _STR_MGETI(); break;
        case 0x6a:_STR("DEC "); _STR_MGETI(); break;
        case 0x6c:_STR("INC "); _STR_MGETI(); break;
        case 0x6d:_STR("TST "); _STR_MGETI(); break;
        case 0x6e:_STR("JMP "); _STR_MGETI(); break;
        case 0x6f:_STR("CLR "); _STR_MGETI(); break;

        case 0x70:_STR("NEG "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x73:_STR("COM "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x74:_STR("LSR "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x76:_STR("ROR "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x77:_STR("ASR "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x78:_STR("ASL "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x79:_STR("ROL "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x7a:_STR("DEC "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x7c:_STR("INC "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x7d:_STR("INC "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x7e:_STR("JMP "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x7f:_STR("CLR "); _FETCH_U16(u16); _STR_U16(u16); break;

        case 0x80:_STR("SUBA #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x81:_STR("CMPA #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x82:_STR("SBCA #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x83:_STR("SUBD #"); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x84:_STR("ANDA #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x85:_STR("BITA #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x86:_STR("LDA #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x88:_STR("EORA #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x89:_STR("ADCA #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x8a:_STR("ORA #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x8b:_STR("ADDA #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x8c:_STR("CMPX #"); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x8d:_STR("BSR "); _FETCH_I8(i8); _STR_I16(pc+i8); break;
        case 0x8e:_STR("LDX #"); _FETCH_U16(u16); _STR_U16(u16); break;

        case 0x90:_STR("SUBA <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x91:_STR("CMPA <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x92:_STR("SBCA <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x93:_STR("SUBD <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x94:_STR("ANDA <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x95:_STR("BITA <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x96:_STR("LDA <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x97:_STR("STA <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x98:_STR("EORA <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x99:_STR("ADCA <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x9a:_STR("ORA <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x9b:_STR("ADDA <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x9c:_STR("CMPX <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x9d:_STR("JSR <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x9e:_STR("LDX <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x9f:_STR("STX <"); _FETCH_U8(u8); _STR_U8(u8); break;

        case 0xa0:_STR("SUBA "); _STR_MGETI(); break;
        case 0xa1:_STR("CMPA "); _STR_MGETI(); break;
        case 0xa2:_STR("SBCA "); _STR_MGETI(); break;
        case 0xa3:_STR("SUBD "); _STR_MGETI(); break;
        case 0xa4:_STR("ANDA "); _STR_MGETI(); break;
        case 0xa5:_STR("BITA "); _STR_MGETI(); break;
        case 0xa6:_STR("LDA " ); _STR_MGETI(); break;
        case 0xa7:_STR("STA " ); _STR_MGETI(); break;
        case 0xa8:_STR("EORA "); _STR_MGETI(); break;
        case 0xa9:_STR("ADCA "); _STR_MGETI(); break;
        case 0xaa:_STR("ORA " ); _STR_MGETI(); break;
        case 0xab:_STR("ADDA "); _STR_MGETI(); break;
        case 0xac:_STR("CMPX "); _STR_MGETI(); break;
        case 0xad:_STR("JSR " ); _STR_MGETI(); break;
        case 0xae:_STR("LDX " ); _STR_MGETI(); break;
        case 0xaf:_STR("STX " ); _STR_MGETI(); break;

        case 0xb0:_STR("SUBA "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xb1:_STR("CMPA "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xb2:_STR("SBCA "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xb3:_STR("SUBD "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xb4:_STR("ANDA "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xb5:_STR("BITA "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xb6:_STR("LDA "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xb7:_STR("STA "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xb8:_STR("EORA "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xb9:_STR("ADCA "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xba:_STR("ORA "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xbb:_STR("ADDA "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xbc:_STR("CMPX "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xbd:_STR("JSR "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xbe:_STR("LDX "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xbf:_STR("STX "); _FETCH_U16(u16); _STR_U16(u16); break;

        case 0xc0:_STR("SUBB #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xc1:_STR("CMPB #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xc2:_STR("SBCB #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xc3:_STR("ADDD #"); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xc4:_STR("ANDB #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xc5:_STR("BITB #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xc6:_STR("LDB #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xc8:_STR("EORB #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xc9:_STR("ADCB #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xca:_STR("ORB #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xcb:_STR("ADDB #"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xcc:_STR("LDD #"); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xce:_STR("LDU #"); _FETCH_U16(u16); _STR_U16(u16); break;

        case 0xd0:_STR("SUBB <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xd1:_STR("CMPB <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xd2:_STR("SBCB <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xd3:_STR("ADDD <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xd4:_STR("ANDB <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xd5:_STR("BITB <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xd6:_STR("LDB <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xd7:_STR("STB <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xd8:_STR("EORB <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xd9:_STR("ADCB <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xda:_STR("ORB <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xdb:_STR("ADDB <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xdc:_STR("LDD <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xdd:_STR("STD <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xde:_STR("LDU <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0xdf:_STR("STU <"); _FETCH_U8(u8); _STR_U8(u8); break;

        case 0xe0:_STR("SUBB "); _STR_MGETI(); break;
        case 0xe1:_STR("CMPB "); _STR_MGETI(); break;
        case 0xe2:_STR("SBCB "); _STR_MGETI(); break;
        case 0xe3:_STR("ADDD "); _STR_MGETI(); break;
        case 0xe4:_STR("ANDB "); _STR_MGETI(); break;
        case 0xe5:_STR("BITB "); _STR_MGETI(); break;
        case 0xe6:_STR("LDB "); _STR_MGETI(); break;
        case 0xe7:_STR("STB "); _STR_MGETI(); break;
        case 0xe8:_STR("EORB "); _STR_MGETI(); break;
        case 0xe9:_STR("ADCB "); _STR_MGETI(); break;
        case 0xea:_STR("ORB "); _STR_MGETI(); break;
        case 0xeb:_STR("ADDB "); _STR_MGETI(); break;
        case 0xec:_STR("LDD "); _STR_MGETI(); break;
        case 0xed:_STR("STD "); _STR_MGETI(); break;
        case 0xee:_STR("LDU "); _STR_MGETI(); break;
        case 0xef:_STR("STU "); _STR_MGETI(); break;

        case 0xf0:_STR("SUBB "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xf1:_STR("CMPB "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xf2:_STR("SBCB "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xf3:_STR("ADDD "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xf4:_STR("ANDB "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xf5:_STR("BITB "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xf6:_STR("LDB "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xf7:_STR("STB "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xf8:_STR("EORB "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xf9:_STR("ADCB "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xfa:_STR("ORB "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xfb:_STR("ADDB "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xfc:_STR("LDD "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xfd:_STR("STD "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xfe:_STR("LDU "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0xff:_STR("STU "); _FETCH_U16(u16); _STR_U16(u16); break;

        case 0x1021:_STR("LBRN"); break;
        case 0x1022:_STR("LBHI "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x1023:_STR("LBLS "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x1024:_STR("LBCC "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x1025:_STR("LBCS "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x1026:_STR("LBNE "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x1027:_STR("LBEQ "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x1028:_STR("LBVC "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x1029:_STR("LBVS "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x102a:_STR("LBPL "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x102b:_STR("LBMI "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x102c:_STR("LBGE "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x102d:_STR("LBLT "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x102e:_STR("LBGT "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x102f:_STR("LBLE "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x103f:_STR("SWI2 #"); _FETCH_U16(u16); _STR_U16(u16); break;

        case 0x1083:_STR("CMPD #"); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x108c:_STR("CMPY #"); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x108e:_STR("LDY #"); _FETCH_U16(u16); _STR_U16(u16); break;

        case 0x1093:_STR("CMPD <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x109c:_STR("CMPY <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x109e:_STR("LDY <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x109f:_STR("STY <"); _FETCH_U8(u8); _STR_U8(u8); break;

        case 0x10a3:_STR("CMPD "); _STR_MGETI(); break;
        case 0x10ac:_STR("CMPY "); _STR_MGETI(); break;
        case 0x10ae:_STR("LDY "); _STR_MGETI(); break;
        case 0x10af:_STR("STY "); _STR_MGETI(); break;

        case 0x10b3:_STR("CMPD "); _STR_MGETI(); break;
        case 0x10bc:_STR("CMPY "); _STR_MGETI(); break;
        case 0x10be:_STR("LDY "); _STR_MGETI(); break;
        case 0x10bf:_STR("STY "); _STR_MGETI(); break;

        case 0x10ce:_STR("LDS #"); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x10de:_STR("LDS <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x10df:_STR("STS <"); _FETCH_U8(u8); _STR_U8(u8); break;

        case 0x10ee:_STR("LDS " ); _STR_MGETI(); break;
        case 0x10ef:_STR("STS " ); _STR_MGETI(); break;
        case 0x10fe:_STR("LDS ") ; _STR_MGETI(); break;
        case 0x10ff:_STR("STS ") ; _STR_MGETI(); break;

        case 0x113f:_STR("SWI3 #"); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x1183:_STR("CMPU #"); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x118c:_STR("CMPS #"); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x1193:_STR("CMPU <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x119c:_STR("CMPS <"); _FETCH_U8(u8); _STR_U8(u8); break;
        case 0x11a3:_STR("CMPU "); _STR_MGETI(); break;
        case 0x11ac:_STR("CMPS "); _STR_MGETI(); break;
        case 0x11b3:_STR("CMPU "); _FETCH_U16(u16); _STR_U16(u16); break;
        case 0x11bc:_STR("CMPS "); _FETCH_U16(u16); _STR_U16(u16); break;
        default: _STR("???"); break;
    }
}

#undef _STR
#undef _STR_U8
#undef _STR_U16
#undef _STR_I16
#undef _STR_MGETI
#undef _STR_EXG
#undef _STR_TFR
#undef _STR_PULU
#undef _STR_PULS
#undef _STR_PSHS
#undef _STR_PSHU
#undef _FETCH_U8
#undef _FETCH_I8
#undef _FETCH_U16

#endif /* EMU_UTIL_IMPL */
