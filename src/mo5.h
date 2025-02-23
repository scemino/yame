#ifndef _MO5_H_
#define _MO5_H_

#include <stdlib.h>
#include "m6809.h"
#include "kbd.h"
#include "gfx.h"

#define SCREEN_WIDTH (336)  // screen width = 320 + 2 borders of 8 pixels
#define SCREEN_HEIGHT (216) // screen height = 200 + 2 boarders of 8 pixels
// max size of a cassette tape image
#define MO5_MAX_TAPE_SIZE (512*1024)
// 4x16KB
#define MO5_MAX_CARTRIDGE_SIZE (0x10000)
#define MO5_JOY0_BTN_MASK (0x40)
#define MO5_JOY1_BTN_MASK (0x80)

typedef struct {
  void (*func)(const float *samples, int num_samples, void *user_data);
  void *user_data;
} chips_audio_callback_t;

typedef struct {
  struct {
    uint8_t cartridge[MO5_MAX_CARTRIDGE_SIZE];
    uint8_t ram[0xc000]; // 48K
    uint8_t port[0x40];
    uint8_t *video;
    uint8_t sound;
    uint8_t *rom_bank; // rom bank or cartridge bank
    const uint8_t *sys_rom;
    uint8_t *usr_ram;
  } mem;
  struct {
    uint8_t line_cycle;   // line count (0-63)
    uint16_t line_number; // video line displayed (0-311)
    uint8_t border_color; // screen border color
    uint8_t screen[SCREEN_WIDTH * SCREEN_HEIGHT];
  } display;
  struct {
    int bit;
    int pos;
    uint8_t buf[MO5_MAX_TAPE_SIZE];
  } tape;
  struct {
    size_t size;
    uint8_t buf[MO5_MAX_TAPE_SIZE];
  } disk;
  struct {
    int type;  // cartridge type (0=simple 1=switch bank, 2=os-9)
    int flags; // bits0,1,4=bank, 2=cart-enabled, 3=write-enabled
    size_t size;
  } cartridge;
  struct {
    float buffer[1024];
    int sample;
    chips_audio_callback_t callback;
  } audio;
  struct {
    uint8_t key_buffer;
    union {
        struct {
            uint8_t j1_u: 1;
            uint8_t j1_d: 1;
            uint8_t j1_l: 1;
            uint8_t j1_r: 1;
            uint8_t j2_u: 1;
            uint8_t j2_d: 1;
            uint8_t j2_l: 1;
            uint8_t j2_r: 1;
        };
        uint8_t value;
    } joys_position;      // joysticks position
    uint8_t joy_action;   // joystick buttons state
    int xpen, ypen;       // lightpen coordinates
    bool penbutton;       // lightpen click
  } input;
  mc6809e_t cpu;
  kbd_t kbd;
  int clocks;
  uint32_t clock_excess;
} mo5_t;

typedef struct {
  int8_t (*mgetc)(uint16_t);
  void (*mputc)(uint16_t, uint8_t);
  chips_audio_callback_t audio_callback;
} mo5_desc_t;

void mo5_init(mo5_t *mo5, const mo5_desc_t *desc);
void mo5_step(mo5_t *mo5, uint32_t micro_seconds);
int8_t mo5_mem_read(mo5_t *mo5, uint16_t address);
void mo5_mem_write(mo5_t *mo5, uint16_t address, uint8_t value);
gfx_display_info_t mo5_display_info(mo5_t *mo5);
void mo5_key_down(mo5_t *sys, int key_code);
void mo5_key_up(mo5_t *sys, int key_code);
// insert tape as .k7 file
bool mo5_insert_tape(mo5_t* sys, gfx_range_t data);
bool mo5_insert_disk(mo5_t* sys, gfx_range_t data);
bool mo5_insert_cartridge(mo5_t* sys, gfx_range_t data);

#endif
