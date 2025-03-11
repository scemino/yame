#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "mo5rom.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int             x, y;
    int             w, h;
    bool            open;
    const float*    buffer;  /* pointer to audio sample buffer */
    int*            num_samples;    /* max number of samples in sample buffer */
} ui_emu_audio_t;

typedef struct {
    int x, y;
    int w, h;
    bool open;
} ui_emu_video_t;

typedef struct {
    mo5_t* mo5;
    ui_snapshot_desc_t snapshot;                // snapshot ui setup params
} ui_emu_desc_t;

typedef struct {
    mo5_t*             mo5;
    ui_emu_audio_t     audio;
    ui_kbd_t           kbd;
    ui_display_t       display;
    ui_emu_video_t     video;
    ui_snapshot_t      snapshot;
    ui_memedit_t       memedit[4];
    ui_dasm_t          dasm[4];
} ui_emu_t;

typedef struct {
    ui_display_frame_t display;
} ui_emu_frame_t;

void ui_emu_init(ui_emu_t* ui, const ui_emu_desc_t* desc);
void ui_emu_discard(ui_emu_t* ui);
void ui_emu_draw(ui_emu_t* ui, const ui_emu_frame_t* frame);
void ui_emu_save_settings(ui_emu_t* ui, ui_settings_t* settings);
void ui_emu_load_settings(ui_emu_t* ui, const ui_settings_t* settings);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION (include in C++ source) ----------------------------------*/
#ifdef EMU_UI_IMPL
#ifndef __cplusplus
#error "implementation must be compiled as C++"
#endif
#include <string.h> /* memset */
#ifndef EMU_ASSERT
    #include <assert.h>
    #define EMU_ASSERT(c) assert(c)
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#define _UI_MO5_MEMLAYER_CPU      (0)
#define _UI_MO5_MEMLAYER_BASIC    (1)
#define _UI_MO5_MEMLAYER_MONITOR  (2)
#define _UI_MO5_MEMLAYER_RAM      (3)
#define _UI_MO5_MEMLAYER_NUM      (4)

static const char* _ui_mo5_memlayer_names[_UI_MO5_MEMLAYER_NUM] = {
    "CPU Mapped", "BASIC", "MONITOR", "RAM"
};

static void _ui_emu_draw_menu(ui_emu_t* ui) {
    EMU_ASSERT(ui && ui->mo5);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("System")) {
            if(ImGui::MenuItem("Reset", 0)) {
                mo5_reset(ui->mo5);
            }
            if(ImGui::MenuItem("Soft Reset", 0)) {
                mo5_prog_init(ui->mo5);
            }
            ImGui::Separator();
            ui_snapshot_menus(&ui->snapshot);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Info")) {
            ImGui::MenuItem("Video", 0, &ui->video.open);
            ImGui::MenuItem("Audio", 0, &ui->audio.open);
            ImGui::MenuItem("Keyboard Matrix", 0, &ui->kbd.open);
            ImGui::MenuItem("Display", 0, &ui->display.open);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            if (ImGui::BeginMenu("Memory Editor")) {
                ImGui::MenuItem("Window #1", 0, &ui->memedit[0].open);
                ImGui::MenuItem("Window #2", 0, &ui->memedit[1].open);
                ImGui::MenuItem("Window #3", 0, &ui->memedit[2].open);
                ImGui::MenuItem("Window #4", 0, &ui->memedit[3].open);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Disassembler")) {
                ImGui::MenuItem("Window #1", 0, &ui->dasm[0].open);
                ImGui::MenuItem("Window #2", 0, &ui->dasm[1].open);
                ImGui::MenuItem("Window #3", 0, &ui->dasm[2].open);
                ImGui::MenuItem("Window #4", 0, &ui->dasm[3].open);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ui_util_options_menu();
        ImGui::EndMainMenuBar();
    }
}

static void _ui_emu_draw_audio(ui_emu_t* ui) {
    if (!ui->audio.open) {
        return;
    }
    ImGui::SetNextWindowPos(ImVec2((float)ui->audio.x, (float)ui->audio.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2((float)ui->audio.w, (float)ui->audio.h), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Audio", &ui->audio.open)) {
        const ImVec2 area = ImGui::GetContentRegionAvail();
        ImGui::PlotLines("##samples", ui->audio.buffer, *ui->audio.num_samples, 0, 0, -1.0f, +1.0f, area);
    }
    ImGui::End();
}

static void _ui_emu_draw_video(ui_emu_t* ui) {
    if (!ui->video.open) {
        return;
    }
    ImGui::SetNextWindowPos(ImVec2((float)ui->video.x, (float)ui->video.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2((float)ui->video.w, (float)ui->video.h), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Video", &ui->video.open)) {
        ImGui::Text("Palette");
        gfx_display_info_t display = mo5_display_info(ui->mo5);
        uint32_t* palette = (uint32_t*)display.palette.ptr;
        for (int col = 0; col < 16; col++) {
            ImGui::PushID(col);
            ImColor c = ImColor(palette[col]);
            if(ImGui::ColorEdit3("##hw_color", &c.Value.x, ImGuiColorEditFlags_NoInputs)) {
                palette[col] = (ImU32)c;
            }
            ImGui::PopID();
            if (col != 7) {
                ImGui::SameLine();
            }
        }
        ImGui::NewLine();
        ImGui::Text("Border color (%u)", ui->mo5->display.border_color);
        ImColor c = ImColor(palette[ui->mo5->display.border_color]);
        ImGui::ColorEdit3("##border_color", &c.Value.x, ImGuiColorEditFlags_NoInputs);
        ImGui::Separator();
        ImGui::Text("Line cycle: %u", ui->mo5->display.line_cycle);
        ImGui::Text("Line number: %u", ui->mo5->display.line_number);
    }
    ImGui::End();
}

static const uint8_t* _ui_mo5_rd_memptr(mo5_t* mo5, int layer, uint16_t addr) {
    EMU_ASSERT((layer >= _UI_MO5_MEMLAYER_BASIC) && (layer < _UI_MO5_MEMLAYER_NUM));
    if (layer == _UI_MO5_MEMLAYER_BASIC) {
        if (addr >= 0xc000 && addr < 0xf000) {
            const uint8_t* rom = mo5rom;
            return rom + addr - 0xc000;
        }
    } else if (layer == _UI_MO5_MEMLAYER_MONITOR) {
        if (addr >= 0xf000) {
            const uint8_t* rom = mo5rom;
            return rom + addr - 0xc000;
        }
    } else if (layer == _UI_MO5_MEMLAYER_RAM) {
        if (addr < 0xc000) {
            return &mo5->mem.ram[addr];
        }
    }
    /* fallthrough: address isn't mapped to physical RAM */
    return 0;
}

static uint8_t* _ui_mo5_memptr(mo5_t* mo5, int layer, uint16_t addr) {
    EMU_ASSERT((layer >= _UI_MO5_MEMLAYER_BASIC) && (layer < _UI_MO5_MEMLAYER_NUM));
    if (layer == _UI_MO5_MEMLAYER_RAM) {
        if (addr < 0xc000) {
            return &mo5->mem.ram[addr];
        }
    }
    /* fallthrough: address isn't mapped to physical RAM */
    return 0;
}

static uint8_t _ui_mo5_mem_read(int layer, uint16_t addr, void* user_data) {
    CHIPS_ASSERT(user_data);
    ui_emu_t* ui = (ui_emu_t*) user_data;
    mo5_t* mo5 = ui->mo5;
    if (layer == _UI_MO5_MEMLAYER_CPU) {
        /* TODO: MO5 mapped RAM layer */
        // return mem_rd(&mo5->mem, addr);
    } else {
        const uint8_t* ptr = _ui_mo5_rd_memptr(mo5, layer, addr);
        if (ptr) {
            return *ptr;
        } else {
            return 0xFF;
        }
    }
    /* fallthrough: address isn't mapped to physical RAM */
    return 0;
}

static void _ui_mo5_mem_write(int layer, uint16_t addr, uint8_t data, void* user_data) {
    CHIPS_ASSERT(user_data);
    ui_emu_t* ui = (ui_emu_t*) user_data;
    mo5_t* mo5 = ui->mo5;
    if (layer == _UI_MO5_MEMLAYER_CPU) {
        // TODO: mem_wr(&mo5->mem, addr, data);
    } else {
        uint8_t* ptr = _ui_mo5_memptr(mo5, layer, addr);
        if (ptr) {
            *ptr = data;
        }
    }
}

void ui_emu_init(ui_emu_t* ui, const ui_emu_desc_t* ui_desc) {
    EMU_ASSERT(ui && ui_desc);
    EMU_ASSERT(ui_desc->mo5);
    ui->mo5 = ui_desc->mo5;
    ui_snapshot_init(&ui->snapshot, &ui_desc->snapshot);
    int x = 20, y = 20, dx = 10, dy = 10;
    {
        ui->video.x = x;
        ui->video.y = y;
        ui->video.w = 200;
        ui->video.h = 100;
    }
    x += dx; y += dy;
    {
        ui->audio.x = x;
        ui->audio.y = y;
        ui->audio.w = 200;
        ui->audio.h = 100;
        ui->audio.buffer = ui->mo5->audio.buffer;
        ui->audio.num_samples = &ui->mo5->audio.sample;
    }
    x += dx; y += dy;
    {
        ui_display_desc_t desc = {0};
        desc.title = "Display";
        desc.x = x;
        desc.y = y;
        desc.w = SCREEN_WIDTH + 20;
        desc.h = SCREEN_HEIGHT + 20;
        ui_display_init(&ui->display, &desc);
    }
    x += dx; y += dy;
    {
        ui_kbd_desc_t desc = {0};
        desc.title = "Keyboard Matrix";
        desc.kbd = &ui->mo5->kbd;
        desc.layers[0] = "None";
        desc.layers[1] = "Shift";
        desc.layers[2] = "Ctrl";
        desc.x = x;
        desc.y = y;
        ui_kbd_init(&ui->kbd, &desc);
    }
    x += dx; y += dy;
    {
        ui_memedit_desc_t desc = {0};
        for (int i = 0; i < _UI_MO5_MEMLAYER_NUM; i++) {
            desc.layers[i] = _ui_mo5_memlayer_names[i];
        }
        desc.read_cb = _ui_mo5_mem_read;
        desc.write_cb = _ui_mo5_mem_write;
        desc.user_data = ui;
        static const char* titles[] = { "Memory Editor #1", "Memory Editor #2", "Memory Editor #3", "Memory Editor #4" };
        for (int i = 0; i < 4; i++) {
            desc.title = titles[i]; desc.x = x; desc.y = y;
            ui_memedit_init(&ui->memedit[i], &desc);
            x += dx; y += dy;
        }
    }
    x += dx; y += dy;
    {
        ui_dasm_desc_t desc = {0};
        for (int i = 0; i < _UI_MO5_MEMLAYER_NUM; i++) {
            desc.layers[i] = _ui_mo5_memlayer_names[i];
        }
        desc.start_addr = 0x0000;
        desc.read_cb = _ui_mo5_mem_read;
        desc.jump_tgt_cb = m6809_jump_tgt;
        desc.dasm_cb = m6809dasm_op;
        desc.user_data = ui;
        static const char* titles[4] = { "Disassembler #1", "Disassembler #2", "Disassembler #2", "Dissassembler #3" };
        for (int i = 0; i < 4; i++) {
            desc.title = titles[i]; desc.x = x; desc.y = y;
            ui_dasm_init(&ui->dasm[i], &desc);
            x += dx; y += dy;
        }
    }
}

void ui_emu_discard(ui_emu_t* ui) {
    EMU_ASSERT(ui && ui->mo5);
    ui_display_discard(&ui->display);
    ui_kbd_discard(&ui->kbd);
    for (int i = 0; i < 4; i++) {
        ui_memedit_discard(&ui->memedit[i]);
        ui_dasm_discard(&ui->dasm[i]);
    }
    ui->mo5 = 0;
}

void ui_emu_draw(ui_emu_t* ui, const ui_emu_frame_t* frame) {
    (void)frame;
    EMU_ASSERT(ui && ui->mo5);
    _ui_emu_draw_menu(ui);
    _ui_emu_draw_video(ui);
    _ui_emu_draw_audio(ui);
    ui_kbd_draw(&ui->kbd);
    ui_display_draw(&ui->display, &frame->display);
    for (int i = 0; i < 4; i++) {
        ui_memedit_draw(&ui->memedit[i]);
        ui_dasm_draw(&ui->dasm[i]);
    }
}

void ui_emu_save_settings(ui_emu_t* ui, ui_settings_t* settings) {
    EMU_ASSERT(ui && settings);
    ui_settings_add(settings, "Video", ui->video.open);
    ui_settings_add(settings, "Audio", ui->audio.open);
    ui_settings_add(settings, "Display", ui->display.open);
    ui_kbd_save_settings(&ui->kbd, settings);
    ui_display_save_settings(&ui->display, settings);
    for (int i = 0; i < 4; i++) {
        ui_memedit_save_settings(&ui->memedit[i], settings);
    }
    for (int i = 0; i < 4; i++) {
        ui_dasm_save_settings(&ui->dasm[i], settings);
    }
}

void ui_emu_load_settings(ui_emu_t* ui, const ui_settings_t* settings) {
    EMU_ASSERT(ui && settings);
    ui->video.open = ui_settings_isopen(settings, "Video");
    ui->audio.open = ui_settings_isopen(settings, "Audio");
    ui->display.open = ui_settings_isopen(settings, "Display");
    ui_kbd_load_settings(&ui->kbd, settings);
    ui_display_load_settings(&ui->display, settings);
    for (int i = 0; i < 4; i++) {
        ui_memedit_load_settings(&ui->memedit[i], settings);
    }
    for (int i = 0; i < 4; i++) {
        ui_dasm_load_settings(&ui->dasm[i], settings);
    }
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif /* EMU_UI_IMPL */
