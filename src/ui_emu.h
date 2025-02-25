#pragma once
#include <stdint.h>
#include <stdbool.h>

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
} ui_emu_desc_t;

typedef struct {
    mo5_t*             mo5;
    ui_emu_audio_t     audio;
    ui_display_t       display;
    ui_emu_video_t     video;
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

static void _ui_emu_draw_menu(ui_emu_t* ui) {
    EMU_ASSERT(ui && ui->mo5);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Info")) {
            ImGui::MenuItem("Video", 0, &ui->video.open);
            ImGui::MenuItem("Audio", 0, &ui->audio.open);
            ImGui::MenuItem("Display", 0, &ui->display.open);
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

void ui_emu_init(ui_emu_t* ui, const ui_emu_desc_t* ui_desc) {
    EMU_ASSERT(ui && ui_desc);
    EMU_ASSERT(ui_desc->mo5);
    ui->mo5 = ui_desc->mo5;
    int x = 20, y = 20, dx = 10, dy = 10;
    {
        ui->video.x = x;
        ui->video.y = y;
        ui->video.w = 200;
        ui->video.h = 100;
        x += dx; y += dy;
    }
    {
        ui->audio.x = x;
        ui->audio.y = y;
        ui->audio.w = 200;
        ui->audio.h = 100;
        ui->audio.buffer = ui->mo5->audio.buffer;
        ui->audio.num_samples = &ui->mo5->audio.sample;
        x += dx; y += dy;
    }
    {
        ui_display_desc_t desc = {0};
        desc.title = "Display";
        desc.x = x;
        desc.y = y;
        desc.w = SCREEN_WIDTH + 20;
        desc.h = SCREEN_HEIGHT + 20;
        ui_display_init(&ui->display, &desc);
        x += dx; y += dy;
    }
}

void ui_emu_discard(ui_emu_t* ui) {
    EMU_ASSERT(ui && ui->mo5);
    ui_display_discard(&ui->display);
    ui->mo5 = 0;
}

void ui_emu_draw(ui_emu_t* ui, const ui_emu_frame_t* frame) {
    (void)frame;
    EMU_ASSERT(ui && ui->mo5);
    _ui_emu_draw_menu(ui);
    _ui_emu_draw_video(ui);
    _ui_emu_draw_audio(ui);
    ui_display_draw(&ui->display, &frame->display);
}

void ui_emu_save_settings(ui_emu_t* ui, ui_settings_t* settings) {
    EMU_ASSERT(ui && settings);
    ui_settings_add(settings, "Video", ui->video.open);
    ui_settings_add(settings, "Audio", ui->audio.open);
    ui_settings_add(settings, "Display", ui->display.open);
    ui_display_save_settings(&ui->display, settings);
}

void ui_emu_load_settings(ui_emu_t* ui, const ui_settings_t* settings) {
    EMU_ASSERT(ui && settings);
    ui->video.open = ui_settings_isopen(settings, "Video");
    ui->audio.open = ui_settings_isopen(settings, "Audio");
    ui->display.open = ui_settings_isopen(settings, "Display");
    ui_display_load_settings(&ui->display, settings);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif /* EMU_UI_IMPL */
