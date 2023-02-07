#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* setup parameters for ui_mc6809_init()
    NOTE: all string data must remain alive until ui_mc6809_discard()!
*/
typedef struct {
  const char *title;        /* window title */
  mc6809e_t *cpu;           /* pointer to CPU to track */
  int x, y;                 /* initial window pos */
  int w, h;                 /* initial window size, or 0 for default size */
  bool open;                /* initial open state */
  ui_chip_desc_t chip_desc; /* chip visualization desc */
} ui_mc6809_desc_t;

typedef struct {
  const char *title;
  mc6809e_t *cpu;
  float init_x, init_y;
  float init_w, init_h;
  bool open;
  bool valid;
  ui_chip_t chip;
} ui_mc6809_t;

void ui_mc6809_init(ui_mc6809_t *win, const ui_mc6809_desc_t *desc);
void ui_mc6809_discard(ui_mc6809_t *win);
void ui_mc6809_draw(ui_mc6809_t *win);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION (include in C++ source) ----------------------------------*/
#ifdef CHIPS_UI_IMPL
#ifndef __cplusplus
#error "implementation must be compiled as C++"
#endif
#include <string.h> /* memset */
#ifndef CHIPS_ASSERT
#include <assert.h>
#define CHIPS_ASSERT(c) assert(c)
#endif

void ui_mc6809_init(ui_mc6809_t *win, const ui_mc6809_desc_t *desc) {
  CHIPS_ASSERT(win && desc);
  CHIPS_ASSERT(desc->title);
  CHIPS_ASSERT(desc->cpu);
  memset(win, 0, sizeof(ui_mc6809_t));
  win->title = desc->title;
  win->cpu = desc->cpu;
  win->init_x = (float)desc->x;
  win->init_y = (float)desc->y;
  win->init_w = (float)((desc->w == 0) ? 360 : desc->w);
  win->init_h = (float)((desc->h == 0) ? 340 : desc->h);
  win->open = desc->open;
  win->valid = true;
  ui_chip_init(&win->chip, &desc->chip_desc);
}

void ui_mc6809_discard(ui_mc6809_t *win) {
  CHIPS_ASSERT(win && win->valid);
  win->valid = false;
}

static void _ui_mc6809_regs(ui_mc6809_t *win) {
  mc6809e_t *cpu = win->cpu;
  ImGui::Text("XH: %02X  XL: %02X", (uint8_t)cpu->xh, (uint8_t)cpu->xl);
  ImGui::Text("YH: %02X  YL: %02X", (uint8_t)cpu->yh, (uint8_t)cpu->yl);
  ImGui::Text("SH: %02X  SL: %02X", (uint8_t)cpu->sh, (uint8_t)cpu->sl);
  ImGui::Text("UH: %02X  UL: %02X", (uint8_t)cpu->uh, (uint8_t)cpu->ul);
  ImGui::Separator();
  ImGui::Text("PC: %04X", cpu->pc);
  ImGui::Text(" A: %02X   B: %02X", (uint8_t)cpu->a, (uint8_t)cpu->b);
  ImGui::Text("WH: %02X  WH: %02X", (uint8_t)cpu->wh, (uint8_t)cpu->wl);
  ImGui::Text("DP: %02X  DD: %02X", (uint8_t)cpu->dp, (uint8_t)cpu->dd);
  ImGui::Separator();
  const uint8_t cc = cpu->cc;
  char cc_str[9] = {
      (cc & MC6809E_EF) ? 'E' : '-',
      (cc & MC6809E_FF) ? 'F' : '-',
      (cc & MC6809E_HF) ? 'H' : '-',
      (cc & MC6809E_IF) ? 'I' : '-',
      (cc & MC6809E_NF) ? 'N' : '-',
      (cc & MC6809E_ZF) ? 'Z' : '-',
      (cc & MC6809E_VF) ? 'V' : '-',
      (cc & MC6809E_CF) ? 'C' : '-',
      0,
  };
  ImGui::Text("CC: %s", cc_str);
  ImGui::Separator();
  ImGui::Text("Addr:  %04X", MC6809E_GET_ADDR(cpu->pins));
  ImGui::Text("Data:  %02X", MC6809E_GET_DATA(cpu->pins));
}

void ui_mc6809_draw(ui_mc6809_t *win) {
  CHIPS_ASSERT(win && win->valid && win->title && win->cpu);
  if (!win->open) {
    return;
  }
  ImGui::SetNextWindowPos(ImVec2(win->init_x, win->init_y), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(win->init_w, win->init_h), ImGuiCond_Once);
  if (ImGui::Begin(win->title, &win->open)) {
    ImGui::BeginChild("##mc6809_chip", ImVec2(176, 0), true);
    ui_chip_draw(&win->chip, win->cpu->pins);
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##mc6809_regs", ImVec2(0, 0), true);
    _ui_mc6809_regs(win);
    ImGui::EndChild();
  }
  ImGui::End();
}
#endif /* CHIPS_UI_IMPL */
