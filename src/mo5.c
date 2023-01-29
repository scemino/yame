#include <assert.h>
#include <string.h>
#include "mo5.h"
#include "mo5rom.h"

#define _MO5_FREQUENCY (4000000)

static uint32_t _mo5_palette[] = {
    0x000000FF, 0xF00000FF, 0x00F000FF, 0xF0F000FF, 0x0000F0FF, 0xF000F0FF,
    0x00F0F0FF, 0xF0F0F0FF, 0x636363FF, 0xF06363FF, 0x63F063FF, 0xF0F063FF,
    0x0063F0FF, 0xF063F0FF, 0x63F0F0FF, 0xF06300FF,
};

static inline void MO5videoram(mo5_t *mo5) {
  mo5->mem.video = mo5->mem.ram + ((mo5->mem.port[0] & 1) << 13);
  mo5->display.border_color = (mo5->mem.port[0] >> 1) & 0x0f;
}

static void MO5rombank(mo5_t *mo5) {
  if ((mo5->cartridge.flags & 4) == 0) {
    mo5->mem.rom_bank = (uint8_t *)(mo5rom - 0xc000);
    return;
  }
  mo5->mem.rom_bank =
      mo5->mem.cartridge - 0xb000 + ((mo5->cartridge.flags & 0x03) << 14);
  if (mo5->cartridge.type == 2)
    if (mo5->cartridge.flags & 0x10)
      mo5->mem.rom_bank += 0x10000;
}

static void prog_init(mo5_t *mo5) {
  int16_t Mgetw(uint16_t a);

  mo5->input.joy_position = 0xff;    // center joysticks
  mo5->input.joy_action = 0xc0;      // buttons released
  mo5->cartridge.flags &= 0xec;
  mo5->mem.sound = 0;
  MO5videoram(mo5);
  MO5rombank(mo5);

  m6809_reset(&mo5->cpu);
}

static void mo5_reset(mo5_t *mo5) {
  mo5->display.line_cycle = 0;
  mo5->display.line_number = 0;
  mo5->mem.sys_rom = mo5rom - 0xc000;
  mo5->mem.usr_ram = mo5->mem.ram + 0x2000;
  for (size_t i = 0; i < sizeof(mo5->mem.ram); i++)
    mo5->mem.ram[i] = -((i & 0x80) >> 7);
  for (size_t i = 0; i < sizeof(mo5->mem.port); i++)
    mo5->mem.port[i] = 0;
  for (size_t i = 0; i < sizeof(mo5->mem.cartridge); i++)
    mo5->mem.cartridge[i] = 0;

  prog_init(mo5);
}

static void screen_draw_border(mo5_t *mo5) {
  const uint8_t bc = mo5->display.border_color;
  uint8_t *pixels = mo5->display.screen;

  // draw top/bottom borders
  for (size_t y = 0; y < 8; y++) {
    memset(&pixels[y * SCREEN_WIDTH], bc, SCREEN_WIDTH);
    memset(&pixels[(SCREEN_HEIGHT - 8 + y) * SCREEN_WIDTH], bc, SCREEN_WIDTH);
  }

  // draw left/right borders
  for (size_t y = 0; y < SCREEN_HEIGHT - 16; y++) {
    memset(&pixels[(y + 8) * SCREEN_WIDTH], bc, 8);
    memset(&pixels[(y + 9) * SCREEN_WIDTH - 8], bc, 8);
  }
}

static uint8_t video_shape(mo5_t *mo5, int line) {
  return mo5->mem.ram[0x2000 | line];
}

static uint8_t video_color(mo5_t *mo5, int line) { return mo5->mem.ram[line]; }

static void screen_draw(mo5_t *mo5) {
  screen_draw_border(mo5);

  uint8_t *pixels = mo5->display.screen;
  int i = 0;
  for (size_t y = 0; y < 200; y++) {
    size_t offset = (y + 8) * SCREEN_WIDTH + 8;
    size_t x = 0;

    for (int xx = 0; xx < 40; xx++) {
      const uint8_t col = video_color(mo5, i);
      uint8_t c1 = col & 0x0F;
      uint8_t c2 = col >> 4;
      assert(c1 >= 0);
      assert(c1 < 16);

      // TODO: oh I need to clean this ugly code

      const uint8_t pt = video_shape(mo5, i);
      if ((0x80 & pt) != 0)
        pixels[x + offset] = c2;
      else
        pixels[x + offset] = c1;
      x++;
      if ((0x40 & pt) != 0)
        pixels[x + offset] = c2;
      else
        pixels[x + offset] = c1;
      x++;
      if ((0x20 & pt) != 0)
        pixels[x + offset] = c2;
      else
        pixels[x + offset] = c1;
      x++;
      if ((0x10 & pt) != 0)
        pixels[x + offset] = c2;
      else
        pixels[x + offset] = c1;
      x++;
      if ((0x08 & pt) != 0)
        pixels[x + offset] = c2;
      else
        pixels[x + offset] = c1;
      x++;
      if ((0x04 & pt) != 0)
        pixels[x + offset] = c2;
      else
        pixels[x + offset] = c1;
      x++;
      if ((0x02 & pt) != 0)
        pixels[x + offset] = c2;
      else
        pixels[x + offset] = c1;
      x++;
      if ((0x01 & pt) != 0)
        pixels[x + offset] = c2;
      else
        pixels[x + offset] = c1;
      x++;
      i++;
    }
  }
}

static void diskerror(mo5_t *mo5, int n) {
  mo5->cpu.mputc(0x204e, n - 1); // erreur 53 = erreur entree/sortie
  mo5->cpu.cc |= 0x01;           // indicateur d'erreur
}

static void mo5_step_special_opcode(mo5_t *mo5, int io) {
  // thanks to D.Coulom for the next instructions
  // used by his emulator dcmoto
  // TODO:
  switch (io) {
  case 0x14:
    diskerror(mo5, 53);
    break; // lit secteur qd-fd
  case 0x15:
    diskerror(mo5, 53);
    break; // ecrit secteur qd-fd
  case 0x18:
    diskerror(mo5, 53);
    break; // formatage qd-fd
  case 0x41:
    diskerror(mo5, 53);
    break; // lit bit cassette
  case 0x42:
    diskerror(mo5, 53);
    break; // lit octet cassette
  case 0x45:
    diskerror(mo5, 53);
    break; // ecrit octet cassette
  case 0x4b:
    diskerror(mo5, 53);
    break; // lit position crayon
  case 0x51:
    diskerror(mo5, 53);
    break; // imprime un caractere
  default:
    // TODO: std::cerr << "Invalid opcode " << std::hex << io << '\n';
    break; // code op. invalide
  }
}

inline static int mem_initLn(
    mo5_t *mo5) { // 11 microsecondes - 41 microsecondes - 12 microsecondes
  if (mo5->display.line_cycle < 23)
    return 0;
  return 0x20;
}

static int mem_initN(mo5_t *mo5) { // debut à 12 microsecondes ligne 56, fin à
                                   // 51 microsecondes ligne 255
  if (mo5->display.line_number < 56)
    return 0;
  if (mo5->display.line_number > 255)
    return 0;
  if (mo5->display.line_number == 56 && mo5->display.line_cycle < 24)
    return 0;
  if (mo5->display.line_number == 255 && mo5->display.line_cycle > 62)
    return 0;
  return 0x80;
}

static void Switchmemo5bank(mo5_t *mo5, uint16_t address) {
  if (mo5->cartridge.type != 1)
    return;
  if ((address & 0xfffc) != 0xbffc)
    return;
  mo5->cartridge.flags = (mo5->cartridge.flags & 0xfc) | (address & 3);
  MO5rombank(mo5);
}

static void mo5_step_n(mo5_t *mo5, int clock) {
  if (clock != 1) {
    clock -= mo5->clock_excess;
  }
  int c = 0;
  while (c < clock) {
    int result = m6809_run_op(&mo5->cpu);
    if (result < 0) {
      mo5_step_special_opcode(mo5, -result);
      result = 64;
    }
    c += result;
    mo5->clocks += result;
    if (mo5->clocks >= 45) {
      // 1 frame is 20000 cycles
      // CPU_FREQUENCY / AUDIO_SAMPLE_RATE => 1000000/22050 = 45 cycles at 1MHz
      // => 1 sample at 22050
      const int n_samples = sizeof(mo5->audio.buffer) / sizeof(float);
      mo5->audio.buffer[mo5->audio.sample] = (float)mo5->mem.sound / 255.f;
      mo5->audio.sample = (mo5->audio.sample + 1) % n_samples;
      // when buffer is full, send audio buffer to sound card
      if (mo5->audio.sample == 0) {
        mo5->audio.callback.func(mo5->audio.buffer, n_samples, &mo5);
      }
      mo5->clocks -= 45;
    }

    mo5->display.line_cycle += result;
    // wait for end of line
    if (mo5->display.line_cycle < 64)
      continue;
    mo5->display.line_cycle -= 64;
    mo5->display.line_number++;
    // wait end of frame
    if (mo5->display.line_number < 312)
      continue;
    mo5->display.line_number -= 312;
    m6809_irq(&mo5->cpu);
  }
  mo5->clock_excess = c - clock;
}

// keyboard matrix initialization
static void _mo5_init_keymap(mo5_t *sys) {
  /*
      http://pulkomandy.tk/wiki/doku.php?id=documentations:hardware:mo5
  */

  //    0     1     2   3   4   5   6   7
  // 0	N	  EFF 	J	H	U	Y	7	6
  // 1	,	  INS 	K	G	I	T	8	5
  // 2	.	  BACK 	L	F	O	R	9	4
  // 3	@	  RIGHT	M	D	P	E	0	3
  // 4		  DOWN 	B	S	/	Z	-	2
  // 5	X	  LEFT 	V	Q	*	A	+	1
  // 6	W	  UP	C	RAZ	ENT	CNT	ACC	STOP
  // 7	SHIFT BASIC

  kbd_init(&sys->kbd, 1);
  const char *keymap =
      // no shift
      "n jhuy76"
      ", kgit85"
      ". lfor94"
      "@ mdpe03"
      "  bs/z-2"
      "x vq*a+1"
      "w c     "
      "        "

      // shift
      "      '&"
      "<     (%"
      ">     )$"
      "^     `#"
      "    ? =\""
      "    : ;!"
      "        "
      "        ";
  // shift key is on column 0, line 7
  kbd_register_modifier(&sys->kbd, 0, 0, 7);
  // ctrl key is on column 5, line 6
  kbd_register_modifier(&sys->kbd, 1, 5, 6);

  for (int shift = 0; shift < 2; shift++) {
    for (int col = 0; col < 8; col++) {
      for (int line = 0; line < 8; line++) {
        int c = keymap[shift * 64 + line * 8 + col];
        if (c != 0x20) {
          kbd_register_key(&sys->kbd, c, col, line, shift ? (1 << 0) : 0);
        }
      }
    }
  }

  // special keys
 kbd_register_key(&sys->kbd, 0x20, 0, 4, 0); // space
 kbd_register_key(&sys->kbd, 0x01, 1, 0, 0); // delete
 kbd_register_key(&sys->kbd, 0x08, 1, 5, 0); // cursor left
 kbd_register_key(&sys->kbd, 0x09, 1, 3, 0); // cursor right
 kbd_register_key(&sys->kbd, 0x0A, 1, 4, 0); // cursor down
 kbd_register_key(&sys->kbd, 0x0B, 1, 6, 0); // cursor up
 kbd_register_key(&sys->kbd, 0x0C, 3, 6, 0); // clr
 kbd_register_key(&sys->kbd, 0x0D, 4, 6, 0); // return
 kbd_register_key(&sys->kbd, 0x0E, 4, 6, 0); // insert
}

void mo5_init(mo5_t *mo5, const mo5_desc_t *desc) {
  m6809_init(&mo5->cpu);
  mo5->cpu.mgetc = desc->mgetc;
  mo5->cpu.mputc = desc->mputc;
  mo5->audio.callback = desc->audio_callback;
  mo5_reset(mo5);
  _mo5_init_keymap(mo5);
}

void mo5_step(mo5_t *mo5) {
  const uint32_t micro_seconds = 0.02 * 10e6;
  mo5_step_n(mo5, 20000);
  kbd_update(&mo5->kbd, micro_seconds);
  screen_draw(mo5);
}

uint8_t _mo5_test_key(mo5_t *mo5, uint8_t key) {
    uint8_t line = (key >> 4) & 0x0F;
    uint8_t col = (key & 0x0F) >> 1;
    uint8_t res = mo5->kbd.scanout_column_masks[line];
    return (res & (1 << col))? 0 : 0x80;
}

int8_t mo5_mem_read(mo5_t *mo5, uint16_t address) {
  switch (address >> 12) {
  case 0x0:
  case 0x1:
    return (int8_t)mo5->mem.video[address];
  case 0xa:
    switch (address) {
    case 0xa7c0:
      return mo5->mem.port[0] | 0x80 | (mo5->input.penbutton << 5);
    case 0xa7c1:
      return (int8_t)(mo5->mem.port[1] | _mo5_test_key(mo5, mo5->mem.port[1] & 0xfe));
    case 0xa7c2:
      return (int8_t)mo5->mem.port[2];
    case 0xa7c3:
      return (int8_t)(mo5->mem.port[3] | ~mem_initN(mo5));
    case 0xa7cb:
      return (mo5->cartridge.flags & 0x3f) |
             ((mo5->cartridge.flags & 0x80) >> 1) |
             ((mo5->cartridge.flags & 0x40) << 1);
    case 0xa7cc:
      return (int8_t)((mo5->mem.port[0x0e] & 4) ? mo5->input.joy_position
                                                : mo5->mem.port[0x0c]);
    case 0xa7cd:
      return (int8_t)((mo5->mem.port[0x0f] & 4)
                          ? mo5->input.joy_action | mo5->mem.sound
                          : mo5->mem.port[0x0d]);
    case 0xa7ce:
      return 4;
    case 0xa7d8:
      return ~mem_initN(mo5); // disk state byte
    case 0xa7e1:
      return (int8_t)0xff; // printer error number 53 occurs when set to 0
    case 0xa7e6:
      return mem_initLn(mo5) << 1;
    case 0xa7e7:
      return mem_initN(mo5);
    default:
      if (address < 0xa7c0)
        return (int8_t)(cd90640rom[address & 0x7ff]);
      if (address < 0xa800)
        return (int8_t)(mo5->mem.port[address & 0x3f]);
      return 0;
    }
  case 0xb:
    Switchmemo5bank(mo5, address);
    return (int8_t)mo5->mem.rom_bank[address];
  case 0xc:
  case 0xd:
  case 0xe:
    return (int8_t)mo5->mem.rom_bank[address];
  case 0xf:
    return (int8_t)mo5->mem.sys_rom[address];
  default:
    return (int8_t)mo5->mem.usr_ram[address];
  }
}

void mo5_mem_write(mo5_t *mo5, uint16_t a, uint8_t c) {
  switch (a >> 12) {
  case 0x0:
  case 0x1:
    mo5->mem.video[a] = c;
    break;
  case 0xa:
    switch (a) {
    case 0xa7c0:
      mo5->mem.port[0] = c & 0x5f;
      MO5videoram(mo5);
      break;
    case 0xa7c1:
      mo5->mem.port[1] = c & 0x7f;
      mo5->mem.sound = (c & 1) << 5;
      break;
    case 0xa7c2:
      mo5->mem.port[2] = c & 0x3f;
      break;
    case 0xa7c3:
      mo5->mem.port[3] = c & 0x3f;
      break;
    case 0xa7cb:
      mo5->cartridge.flags = c;
      MO5rombank(mo5);
      break;
    case 0xa7cc:
      mo5->mem.port[0x0c] = c;
      return;
    case 0xa7cd:
      mo5->mem.port[0x0d] = c;
      mo5->mem.sound = c & 0x3f;
      return;
    case 0xa7ce:
      mo5->mem.port[0x0e] = c;
      return; // registre controle position joysticks
    case 0xa7cf:
      mo5->mem.port[0x0f] = c;
      return; // registre controle action - musique
    }
    break;
  case 0xb:
  case 0xc:
  case 0xd:
  case 0xe:
    if (mo5->cartridge.flags & 8)
      if (mo5->cartridge.type == 0)
        mo5->mem.rom_bank[a] = c;
    break;
  case 0xf:
    break;
  default:
    mo5->mem.usr_ram[a] = c;
  }
}

mo5_display_info_t mo5_display_info(mo5_t *mo5) {
  (void)mo5;
  const mo5_display_info_t res = {
      .palette.size = 16 * sizeof(uint32_t),
      .palette.ptr = _mo5_palette,
  };
  return res;
}

void mo5_key_down(mo5_t *sys, int key_code) {
  kbd_key_down(&sys->kbd, key_code);
}

void mo5_key_up(mo5_t *sys, int key_code) {
  kbd_key_up(&sys->kbd, key_code);
}
