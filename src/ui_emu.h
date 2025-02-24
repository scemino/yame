#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    mo5_t* mo5;
} ui_emu_desc_t;

typedef struct {
    mo5_t*             mo5;
} ui_emu_t;

typedef struct {
    ui_display_frame_t display;
} ui_emu_frame_t;

void ui_emu_init(ui_emu_t* ui, const ui_emu_desc_t* desc);
void ui_emu_discard(ui_emu_t* ui);
void ui_emu_draw(ui_emu_t* ui, const ui_emu_frame_t* frame);

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

static void _ui_emu_draw_menu(ui_emu_t* ui) {
    EMU_ASSERT(ui && ui->mo5);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("System")) {
            ImGui::EndMenu();
        }
        ui_util_options_menu();
        ImGui::EndMainMenuBar();
    }
}

void ui_emu_init(ui_emu_t* ui, const ui_emu_desc_t* ui_desc) {
    EMU_ASSERT(ui && ui_desc);
    EMU_ASSERT(ui_desc->mo5);
    ui->mo5 = ui_desc->mo5;
}

void ui_emu_discard(ui_emu_t* ui) {
    EMU_ASSERT(ui && ui->mo5);
    ui->mo5 = 0;
}

void ui_emu_draw(ui_emu_t* ui, const ui_emu_frame_t* frame) {
    (void)frame;
    EMU_ASSERT(ui && ui->mo5);
    _ui_emu_draw_menu(ui);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif /* EMU_UI_IMPL */
