#pragma once
/*
    Common graphics functions for the chips-test example emulators.

    REMINDER: consider using this CRT shader?

    https://github.com/mattiasgustavsson/rebasic/blob/master/source/libs/crtemu.h
*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "sokol_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int top, bottom, left, right;
} gfx_border_t;

typedef struct {
    void* ptr;
    size_t size;
} gfx_range_t;

typedef struct {
    int width, height;
} gfx_dim_t;

typedef struct {
    int x, y, width, height;
} gfx_rect_t;

typedef struct {
    struct {
        gfx_dim_t dim;        // framebuffer dimensions in pixels
        gfx_range_t buffer;
        size_t bytes_per_pixel; // 1 or 4
    } frame;
    gfx_rect_t screen;
    gfx_range_t palette;
    bool portrait;
} gfx_display_info_t;

typedef struct {
    sg_view display_texview;
    sg_sampler display_sampler;
    gfx_display_info_t display_info;
} gfx_draw_info_t;

typedef void(*gfx_init_extra_t)(void);
typedef void(*gfx_draw_extra_t)(const gfx_draw_info_t* draw_info);

typedef struct {
    bool disable_speaker_icon;
    gfx_border_t border;
    gfx_display_info_t display_info;
    gfx_dim_t pixel_aspect;   // optional pixel aspect ratio, default is 1:1
    gfx_init_extra_t init_extra_cb;
    gfx_draw_extra_t draw_extra_cb;
} gfx_desc_t;

void gfx_init(const gfx_desc_t* desc);
void gfx_draw(gfx_display_info_t display_info);
void gfx_shutdown(void);
void gfx_flash_success(void);
void gfx_flash_error(void);
void gfx_disable_speaker_icon(void);
gfx_dim_t gfx_pixel_aspect(void);
sg_view gfx_create_icon_texview(const uint8_t* packed_pixels, int width, int height, int stride, const char* label);

#ifdef __cplusplus
} /* extern "C" */
#endif
