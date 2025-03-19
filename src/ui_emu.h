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

typedef enum {
    SEARCH_LOWER,
    SEARCH_LOWER_OR_EQUALS,
    SEARCH_GREATER,
    SEARCH_GREATER_OR_EQUALS,
    SEARCH_EQUALS,
    SEARCH_NOT_EQUALS
} ui_emu_cheats_search_op_t;

typedef struct {
    int x, y;
    int w, h;
    bool open;
    bool two_bytes;
    ui_emu_cheats_search_op_t search_op;
    uint16_t value;
    uint16_t addresses[32256];
    uint16_t num_addresses;
} ui_emu_cheats_search_t;

typedef struct {
    int x, y;
    int w, h;
    bool open;
    uint16_t address;
    bool two_bytes;
    uint16_t value;
} ui_emu_cheats_add_t;

typedef struct {
    uint16_t address;
    uint16_t value;
    bool two_bytes;
} ui_emu_cheat_t;

typedef struct {
    int x, y;
    int w, h;
    bool open;
    ui_emu_cheat_t cheats[10];
    uint8_t num_cheats;
} ui_emu_cheat_list_t;

/* user-defined hotkeys (all strings must be static) */
typedef struct ui_dbg_key_desc_t {
    int keycode;    // ImGuiKey_*
    const char* name;
} ui_dbg_key_desc_t;

typedef struct ui_dbg_keys_desc_t {
    ui_dbg_key_desc_t cont;
    ui_dbg_key_desc_t stop;
    ui_dbg_key_desc_t step_over;
} ui_dbg_keys_desc_t;

typedef struct {
    mo5_t* mo5;
    ui_snapshot_desc_t snapshot;    // snapshot ui setup params
    ui_dbg_keys_desc_t dbg_keys;        // user-defined hotkeys
} ui_emu_desc_t;

/* current step mode */
enum {
    UI_DBG_STEPMODE_NONE = 0,
    UI_DBG_STEPMODE_INTO,
    UI_DBG_STEPMODE_OVER,
    UI_DBG_STEPMODE_TICK,
};

typedef struct ui_dbg_t {
    int x, y;
    int w, h;
    bool open;
    bool stopped;
    int step_mode;
} ui_dbg_t;

typedef struct {
    mo5_t*                  mo5;
    ui_emu_audio_t          audio;
    ui_kbd_t                kbd;
    ui_display_t            display;
    ui_emu_video_t          video;
    ui_emu_cheats_search_t  cheats_search;
    ui_emu_cheats_add_t     cheats_add;
    ui_emu_cheat_list_t     cheat_list;
    ui_snapshot_t           snapshot;
    ui_memedit_t            memedit[4];
    ui_dasm_t               dasm[4];
    ui_dbg_t                dbg;
    ui_dbg_keys_desc_t      keys;
} ui_emu_t;

typedef struct {
    ui_display_frame_t display;
} ui_emu_frame_t;

void ui_emu_init(ui_emu_t* ui, const ui_emu_desc_t* desc);
void ui_emu_discard(ui_emu_t* ui);
void ui_emu_draw(ui_emu_t* ui, const ui_emu_frame_t* frame);
void ui_emu_save_settings(ui_emu_t* ui, ui_settings_t* settings);
void ui_emu_load_settings(ui_emu_t* ui, const ui_settings_t* settings);
mo5_debug_t ui_mo5_get_debug(ui_emu_t* ui);

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
#define _UI_MO5_MEMLAYER_VIDEO    (4)
#define _UI_MO5_MEMLAYER_NUM      (5)

static const char* _ui_mo5_memlayer_names[_UI_MO5_MEMLAYER_NUM] = {
    "CPU Mapped", "BASIC", "MONITOR", "RAM", "VIDEO"
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
            ImGui::MenuItem("Search cheats", 0, &ui->cheats_search.open);
            ImGui::MenuItem("Add cheat", 0, &ui->cheats_add.open);
            ImGui::MenuItem("Cheat list", 0, &ui->cheat_list.open);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("CPU", 0, &ui->dbg.open);
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

static bool _ui_emu_cheats_search_addr(ui_emu_t* ui, uint16_t address) {
    const uint16_t data = ui->cheats_search.two_bytes ? (mo5_mem_read(ui->mo5, address) << 8) | (mo5_mem_read(ui->mo5, address + 1) & 0xff) : (uint8_t)mo5_mem_read(ui->mo5, address);
    switch(ui->cheats_search.search_op) {
        case SEARCH_LOWER:
        return data < ui->cheats_search.value;
        case SEARCH_LOWER_OR_EQUALS:
        return (data <= ui->cheats_search.value);
        case SEARCH_GREATER:
        return (data > ui->cheats_search.value);
        case SEARCH_GREATER_OR_EQUALS:
        return (data >= ui->cheats_search.value);
        case SEARCH_EQUALS:
        return (data == ui->cheats_search.value);
        case SEARCH_NOT_EQUALS:
        return (data != ui->cheats_search.value);
    }
    return false;
}

static void _ui_emu_cheats_search_go(ui_emu_t* ui) {
    if(ui->cheats_search.num_addresses == 0) {
        for(int address = 0x2200; address < 0xa000; address++) {
            if(_ui_emu_cheats_search_addr(ui, address)) {
                ui->cheats_search.addresses[ui->cheats_search.num_addresses++] = address;
            }
        }
    } else {
        int count = 0;
        for(int i = 0; i < ui->cheats_search.num_addresses; i++) {
            if(_ui_emu_cheats_search_addr(ui, ui->cheats_search.addresses[i])) {
                ui->cheats_search.addresses[count++] = ui->cheats_search.addresses[i];
            }
        }
        ui->cheats_search.num_addresses = count;
    }
}

static void _ui_emu_draw_cheats_search(ui_emu_t* ui) {
    if (!ui->cheats_search.open) {
        return;
    }
    ImGui::SetNextWindowPos(ImVec2((float)ui->cheats_search.x, (float)ui->cheats_search.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2((float)ui->cheats_search.w, (float)ui->cheats_search.h), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Cheats search", &ui->cheats_search.open)) {
        ImGui::Text("Size");
        if(ImGui::RadioButton("1 byte  [0..255]", !ui->cheats_search.two_bytes)) {
            ui->cheats_search.two_bytes = false;
        }
        if(ImGui::RadioButton("2 bytes [0..65535]", ui->cheats_search.two_bytes)) {
            ui->cheats_search.two_bytes = true;
        }
        ImGui::Separator();
        ImGui::Text("Operator");
        if(ImGui::RadioButton("< ", ui->cheats_search.search_op == SEARCH_LOWER)) {
            ui->cheats_search.search_op = SEARCH_LOWER;
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("<=", ui->cheats_search.search_op == SEARCH_LOWER_OR_EQUALS)) {
            ui->cheats_search.search_op = SEARCH_LOWER_OR_EQUALS;
        }
        if(ImGui::RadioButton("> ", ui->cheats_search.search_op == SEARCH_GREATER)) {
            ui->cheats_search.search_op = SEARCH_GREATER;
        }
        ImGui::SameLine();
        if(ImGui::RadioButton(">=", ui->cheats_search.search_op == SEARCH_GREATER_OR_EQUALS)) {
            ui->cheats_search.search_op = SEARCH_GREATER_OR_EQUALS;
        }
        if(ImGui::RadioButton("==", ui->cheats_search.search_op == SEARCH_EQUALS)) {
            ui->cheats_search.search_op = SEARCH_EQUALS;
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("!=", ui->cheats_search.search_op == SEARCH_NOT_EQUALS)) {
            ui->cheats_search.search_op = SEARCH_NOT_EQUALS;
        }
        ImGui::Separator();
        ImGui::Text("Compare to");
        uint16_t max = ui->cheats_search.two_bytes ? 0xFFFF: 0xFF;
        ImGui::DragScalar("Specific value", ImGuiDataType_U16, &ui->cheats_search.value, 1.0f, 0, &max);
        if(ImGui::Button("Search")) {
            _ui_emu_cheats_search_go(ui);
        }
        ImGui::Text("%u results", ui->cheats_search.num_addresses);
        if (ui->cheats_search.num_addresses < 5) {
            if (ImGui::BeginListBox("results")) {
                for(int i = 0; i < ui->cheats_search.num_addresses; i++) {
                    ImGui::Text("0x%4x", ui->cheats_search.addresses[i]);
                }
              ImGui::EndListBox();
            }
        }
        if(ImGui::Button("Clear")) {
            ui->cheats_search.num_addresses = 0;
        }

    }
    ImGui::End();
}

static void _ui_emu_draw_cheats_add(ui_emu_t* ui) {
    if (!ui->cheats_add.open) {
        return;
    }
    ImGui::SetNextWindowPos(ImVec2((float)ui->cheats_add.x, (float)ui->cheats_add.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2((float)ui->cheats_add.w, (float)ui->cheats_add.h), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Add cheats", &ui->cheats_add.open)) {
        ImGui::InputScalar("Address", ImGuiDataType_U16, &ui->cheats_add.address, 0, 0, "%04X", ImGuiInputTextFlags_CharsHexadecimal);
        if(ImGui::RadioButton("1 byte  [0..255]", !ui->cheats_add.two_bytes)) {
            ui->cheats_add.two_bytes = false;
        }
        if(ImGui::RadioButton("2 bytes [0..65535]", ui->cheats_add.two_bytes)) {
            ui->cheats_add.two_bytes = true;
        }
        ImGui::Separator();
        ImGui::InputScalar("Value", ImGuiDataType_U16, &ui->cheats_add.value);
        if(ImGui::Button("Add") && ui->cheat_list.num_cheats < 10) {
            ui->cheat_list.cheats[ui->cheat_list.num_cheats].address = ui->cheats_add.address;
            ui->cheat_list.cheats[ui->cheat_list.num_cheats].value = ui->cheats_add.value;
            ui->cheat_list.cheats[ui->cheat_list.num_cheats].two_bytes = ui->cheats_add.two_bytes;
            ui->cheat_list.num_cheats++;
        }
    }
    ImGui::End();
}

static void _ui_emu_draw_cheats_list(ui_emu_t* ui) {
    for (int i=0; i<ui->cheat_list.num_cheats; i++) {
        const ui_emu_cheat_t *cheat = &ui->cheat_list.cheats[i];
        if(cheat->two_bytes) {
            mo5_mem_write(ui->mo5, cheat->address, (cheat->value >> 8) & 0xFF);
            mo5_mem_write(ui->mo5, cheat->address + 1, cheat->value & 0xFF);
        } else {
            mo5_mem_write(ui->mo5, cheat->address, cheat->value & 0xFF);
        }
    }

    if (!ui->cheat_list.open) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2((float)ui->cheat_list.x, (float)ui->cheat_list.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2((float)ui->cheat_list.w, (float)ui->cheat_list.h), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Cheat list", &ui->cheat_list.open)) {
        if (ImGui::BeginTable("Cheats", 3, 0)) {
            int del_index = -1;
            for (int i=0; i<ui->cheat_list.num_cheats; i++) {
                const ui_emu_cheat_t *cheat = &ui->cheat_list.cheats[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%04X", cheat->address);
                ImGui::TableNextColumn();
                ImGui::Text("%u", cheat->value);
                ImGui::TableNextColumn();
                if(ImGui::SmallButton("Del")) {
                    del_index =  1;
                }
            }
            ImGui::EndTable();
            if(del_index != -1) {
                // remove deleted cheat
                ui->cheat_list.num_cheats--;
                for (int i=del_index; i<ui->cheat_list.num_cheats; i++) {
                    ui->cheat_list.cheats[i] = ui->cheat_list.cheats[i+1];
                }
            }
        }
    }
    ImGui::End();
}

static inline const char* _ui_dbg_str_or_def(const char* str, const char* def) {
    if (str) {
        return str;
    } else {
        return def;
    }
}

static void _ui_dbg_break(ui_emu_t* ui) {
    ui->dbg.stopped = true;
    ui->dbg.step_mode = UI_DBG_STEPMODE_NONE;
}

static void _ui_dbg_continue(ui_emu_t* ui) {
    ui->dbg.stopped = false;
    ui->dbg.step_mode = UI_DBG_STEPMODE_NONE;
}

static void _ui_dbg_step(ui_emu_t* ui) {
    ui->dbg.stopped = false;
    ui->dbg.step_mode = UI_DBG_STEPMODE_OVER;
}

/* handle keyboard input, the debug window must be focused for hotkeys to work! */
static void _ui_dbg_handle_input(ui_emu_t* ui) {
    /* unused hotkeys are defined as 0 and will never be triggered */
    if (ui->dbg.stopped) {
        if (0 != ui->keys.cont.keycode) {
            if (ImGui::IsKeyPressed((ImGuiKey)ui->keys.cont.keycode)) {
                _ui_dbg_continue(ui);
            }
        }
        if (0 != ui->keys.step_over.keycode) {
            if (ImGui::IsKeyPressed((ImGuiKey)ui->keys.step_over.keycode)) {
                _ui_dbg_step(ui);
            }
        }
    } else {
        if (ImGui::IsKeyPressed((ImGuiKey)ui->keys.stop.keycode)) {
            _ui_dbg_break(ui);
        }
    }
}

void _ui_dbg_draw_cpu(ui_emu_t* ui) {
    if (!ui->dbg.open) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2((float)ui->dbg.x, (float)ui->dbg.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2((float)ui->dbg.w, (float)ui->dbg.h), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("CPU", &ui->dbg.open)) {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
            ImGui::SetNextFrameWantCaptureKeyboard(true);
            _ui_dbg_handle_input(ui);
        }

        mc6809e_t* c = &ui->mo5->cpu;
        if (ImGui::BeginTable("##reg_columns", 4)) {
            for (int i = 0; i < 4; i++) {
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 64);
            }
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            c->x  = ui_util_input_u16("X", c->x); ImGui::TableNextColumn();
            c->y  = ui_util_input_u16("Y", c->y); ImGui::TableNextColumn();
            c->u  = ui_util_input_u16("U", c->u); ImGui::TableNextColumn();
            c->s  = ui_util_input_u16("S", c->s); ImGui::TableNextColumn();
            c->d  = ui_util_input_u16("D", c->d); ImGui::TableNextColumn();
            c->w = ui_util_input_u16("W'", c->w); ImGui::TableNextColumn();
            c->da = ui_util_input_u16("DA'", c->da); ImGui::TableNextColumn();
            c->pc  = ui_util_input_u16("PC", c->pc); ImGui::TableNextColumn();
            char cc_str[9] = {
                (c->cc & MC6809E_EF) ? 'E':'-',
                (c->cc & MC6809E_FF) ? 'F':'-',
                (c->cc & MC6809E_HF) ? 'H':'-',
                (c->cc & MC6809E_IF) ? 'I':'-',
                (c->cc & MC6809E_NF) ? 'N':'-',
                (c->cc & MC6809E_ZF) ? 'Z':'-',
                (c->cc & MC6809E_VF) ? 'V':'-',
                (c->cc & MC6809E_CF) ? 'C':'-',
                0,
            };
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", cc_str);
            ImGui::EndTable();
        }
        ImGui::Separator();
        char str[32];
        if (ui->dbg.stopped) {
            snprintf(str, sizeof(str), "Continue (%s)", _ui_dbg_str_or_def(ui->keys.cont.name, "-"));
            if (ImGui::Button(str)) {
                _ui_dbg_continue(ui);
            }
            ImGui::SameLine();
            snprintf(str, sizeof(str), "Step (%s)", _ui_dbg_str_or_def(ui->keys.step_over.name, "-"));
            if (ImGui::Button(str)) {
                _ui_dbg_step(ui);
            }
        } else {
            snprintf(str, sizeof(str), "Break (%s)", _ui_dbg_str_or_def(ui->keys.stop.name, "-"));
            if (ImGui::Button(str)) {
                _ui_dbg_break(ui);
            }
        }
    }
    ImGui::End();
}

static const uint8_t* _ui_mo5_rd_memptr(mo5_t* mo5, int layer, uint16_t addr) {
    EMU_ASSERT((layer >= _UI_MO5_MEMLAYER_BASIC) && (layer < _UI_MO5_MEMLAYER_NUM));
    if (layer == _UI_MO5_MEMLAYER_VIDEO) {
        if (addr < 0x2000) {
            return mo5->mem.video + addr;
        }
    } else if (layer == _UI_MO5_MEMLAYER_BASIC) {
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
        if (addr >= 0x2000 && addr < 0xa000) {
            return &mo5->mem.ram[addr + 0x2000];
        }
    }
    /* fallthrough: address isn't mapped to physical RAM */
    return 0;
}

static uint8_t* _ui_mo5_memptr(mo5_t* mo5, int layer, uint16_t addr) {
    EMU_ASSERT((layer >= _UI_MO5_MEMLAYER_BASIC) && (layer < _UI_MO5_MEMLAYER_NUM));
    if (layer == _UI_MO5_MEMLAYER_VIDEO) {
        if (addr < 0x2000) {
            return &mo5->mem.video[addr];
        }
    } else if (layer == _UI_MO5_MEMLAYER_RAM) {
        if (addr >= 0x2000 && addr < 0xa000) {
            return &mo5->mem.ram[addr + 0x2000];
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
    ui->keys = ui_desc->dbg_keys;
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
        ui->cheats_search.x = x;
        ui->cheats_search.y = y;
        ui->cheats_search.w = 200;
        ui->cheats_search.h = 100;
    }
    x += dx; y += dy;
    {
        ui->cheats_add.x = x;
        ui->cheats_add.y = y;
        ui->cheats_add.w = 200;
        ui->cheats_add.h = 100;
    }
    x += dx; y += dy;
    {
        ui->cheat_list.x = x;
        ui->cheat_list.y = y;
        ui->cheat_list.w = 200;
        ui->cheat_list.h = 100;
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
    _ui_emu_draw_cheats_search(ui);
    _ui_emu_draw_cheats_add(ui);
    _ui_emu_draw_cheats_list(ui);
    _ui_emu_draw_audio(ui);
    _ui_dbg_draw_cpu(ui);
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
    ui_settings_add(settings, "Cheats search", ui->cheats_search.open);
    ui_settings_add(settings, "Add cheats", ui->cheats_add.open);
    ui_settings_add(settings, "Cheat list", ui->cheat_list.open);
    ui_settings_add(settings, "CPU", ui->dbg.open);
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
    ui->cheats_search.open = ui_settings_isopen(settings, "Cheats search");
    ui->cheats_add.open = ui_settings_isopen(settings, "Add cheats");
    ui->cheat_list.open = ui_settings_isopen(settings, "Cheat list");
    ui->dbg.open = ui_settings_isopen(settings, "CPU");
    ui_kbd_load_settings(&ui->kbd, settings);
    ui_display_load_settings(&ui->display, settings);
    for (int i = 0; i < 4; i++) {
        ui_memedit_load_settings(&ui->memedit[i], settings);
    }
    for (int i = 0; i < 4; i++) {
        ui_dasm_load_settings(&ui->dasm[i], settings);
    }
}

static void _ui_dbg_tick(ui_emu_t* ui) {
    if(ui->dbg.step_mode == UI_DBG_STEPMODE_OVER) {
        ui->dbg.stopped = true;
    }
}

mo5_debug_t ui_mo5_get_debug(ui_emu_t* ui) {
    EMU_ASSERT(ui);
    mo5_debug_t res = {};
    res.callback.func = (mo5_debug_func_t)_ui_dbg_tick;
    res.callback.user_data = ui;
    res.stopped = &ui->dbg.stopped;
    return res;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif /* EMU_UI_IMPL */
