#include <stdio.h>
#include <assert.h>
#include "sokol_audio.h"
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_color.h"
#include "sokol_glue.h"
#include "sokol_shaders.glsl.h"
#define CHIPS_IMPL
#include "mo5.h"

static struct {
  struct {
    sg_buffer vbuf;
    sg_image pal_img;  // 256x1 palette lookup texture
    sg_image pix_img;  // 336x216 R8 framebuffer texture
    sg_image rgba_img; // 336x216 RGBA8 framebuffer texture
    sg_pipeline offscreen_pip;
    sg_pipeline display_pip;
    sg_pass offscreen_pass;
  } gfx;
  mo5_t mo5;

} app = {0};

static int8_t mem_read(uint16_t address) {
  return mo5_mem_read(&app.mo5, address);
}

static void mem_write(uint16_t address, uint8_t value) {
  mo5_mem_write(&app.mo5, address, value);
}

static void audio_push(const float *samples, int num_samples, void *user_data) {
  (void)user_data;
  saudio_push(samples, num_samples);
}

static void init(void) {
  // init MO5
  mo5_desc_t mo5_desc = {.mgetc = mem_read,
                         .mputc = mem_write,
                         .audio_callback = {.func = audio_push}};
  mo5_init(&app.mo5, &mo5_desc);

  saudio_setup(&(saudio_desc){.sample_rate = 22050});
  sg_setup(&(sg_desc){.context = sapp_sgcontext()});

  // a vertex buffer to render a fullscreen triangle
  const float verts[] = {0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 2.0f};
  app.gfx.vbuf = sg_make_buffer(&(sg_buffer_desc){
      .data = SG_RANGE(verts),
  });

  // a dynamic texture for framebuffer
  app.gfx.pix_img = sg_make_image(&(sg_image_desc){
      .width = SCREEN_WIDTH,
      .height = SCREEN_HEIGHT,
      .pixel_format = SG_PIXELFORMAT_R8,
      .usage = SG_USAGE_STREAM,
      .min_filter = SG_FILTER_NEAREST,
      .mag_filter = SG_FILTER_NEAREST,
      .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
      .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
  });

  // init palette
  static uint32_t palette_buf[256];
  mo5_display_info_t info = mo5_display_info(&app.mo5);
  memcpy(palette_buf, info.palette.ptr, info.palette.size);

  app.gfx.pal_img = sg_make_image(&(sg_image_desc){
      .width = 256,
      .height = 1,
      .pixel_format = SG_PIXELFORMAT_RGBA8,
      .min_filter = SG_FILTER_NEAREST,
      .mag_filter = SG_FILTER_NEAREST,
      .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
      .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
      .data = {.subimage[0][0] = {.ptr = palette_buf,
                                  .size = sizeof(palette_buf)}}});

  // an RGBA8 texture to hold the 'color palette expanded' image
  // and source for upscaling with linear filtering
  app.gfx.rgba_img = sg_make_image(&(sg_image_desc){
      .render_target = true,
      .width = SCREEN_WIDTH,
      .height = SCREEN_HEIGHT,
      .pixel_format = SG_PIXELFORMAT_RGBA8,
      .usage = SG_USAGE_IMMUTABLE,
      .min_filter = SG_FILTER_LINEAR,
      .mag_filter = SG_FILTER_LINEAR,
      .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
      .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
  });

  // a pipeline object for the offscreen render pass which
  // performs the color palette lookup
  app.gfx.offscreen_pip = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = sg_make_shader(offscreen_shader_desc(sg_query_backend())),
      .layout = {.attrs[0].format = SG_VERTEXFORMAT_FLOAT2},
      .cull_mode = SG_CULLMODE_NONE,
      .depth =
          {
              .write_enabled = false,
              .compare = SG_COMPAREFUNC_ALWAYS,
              .pixel_format = SG_PIXELFORMAT_NONE,
          },
      .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
  });

  // a pipeline object to upscale the offscreen RGBA8 framebuffer
  // texture to the display
  app.gfx.display_pip = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = sg_make_shader(display_shader_desc(sg_query_backend())),
      .layout = {.attrs[0].format = SG_VERTEXFORMAT_FLOAT2},
      .cull_mode = SG_CULLMODE_NONE,
      .depth =
          {
              .write_enabled = false,
              .compare = SG_COMPAREFUNC_ALWAYS,
          },
  });

  // a render pass object for the offscreen pass
  app.gfx.offscreen_pass = sg_make_pass(&(sg_pass_desc){
      .color_attachments[0].image = app.gfx.rgba_img,
  });
}

// helper function to adjust aspect ratio
static void apply_viewport(float canvas_width, float canvas_height) {
  const float canvas_aspect = canvas_width / canvas_height;
  const float app_width = (float)SCREEN_WIDTH;
  const float app_height = (float)SCREEN_HEIGHT;
  const float app_aspect = app_width / app_height;
  float vp_x, vp_y, vp_w, vp_h;
  if (app_aspect < canvas_aspect) {
    vp_y = 0.0f;
    vp_h = canvas_height;
    vp_w = canvas_height * app_aspect;
    vp_x = (canvas_width - vp_w) / 2;
  } else {
    vp_x = 0.0f;
    vp_w = canvas_width;
    vp_h = canvas_width / app_aspect;
    vp_y = (canvas_height - vp_h) / 2;
  }
  sg_apply_viewport(vp_x, vp_y, vp_w, vp_h, true);
}

static void frame(void) {
  mo5_step(&app.mo5);

  // update pixel textures
  sg_update_image(app.gfx.pix_img,
                  &(sg_image_data){.subimage[0][0] = {
                                       .ptr = app.mo5.display.screen,
                                       .size = SCREEN_WIDTH * SCREEN_HEIGHT,
                                   }});

  // offscreen render pass to perform color palette lookup
  const sg_pass_action offscreen_pass_action = {
      .colors[0] = {.action = SG_ACTION_DONTCARE}};
  sg_begin_pass(app.gfx.offscreen_pass, &offscreen_pass_action);
  sg_apply_pipeline(app.gfx.offscreen_pip);
  sg_apply_bindings(&(sg_bindings){.vertex_buffers[0] = app.gfx.vbuf,
                                   .fs_images = {
                                       [SLOT_pix_img] = app.gfx.pix_img,
                                       [SLOT_pal_img] = app.gfx.pal_img,
                                   }});
  sg_draw(0, 3, 1);
  sg_end_pass();

  // render resulting texture to display framebuffer with upscaling
  const sg_pass_action display_pass_action = {
      .colors[0] = {.action = SG_ACTION_CLEAR,
                    .value = {0.0f, 0.0f, 0.0f, 1.0f}}};
  sg_begin_default_pass(&display_pass_action, sapp_width(), sapp_height());
  sg_apply_pipeline(app.gfx.display_pip);
  sg_apply_bindings(
      &(sg_bindings){.vertex_buffers[0] = app.gfx.vbuf,
                     .fs_images[SLOT_rgba_img] = app.gfx.rgba_img});
  apply_viewport(sapp_widthf(), sapp_heightf());
  sg_draw(0, 3, 1);
  sg_end_pass();
  sg_commit();
}

static void input(const sapp_event *event) {
  const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
  switch (event->type) {
  case SAPP_EVENTTYPE_CHAR: {
    int c = (int)event->char_code;
    if ((c > 0x20) && (c < 0x7F)) {
      mo5_key_down(&app.mo5, c);
      mo5_key_up(&app.mo5, c);
    }
  } break;
  case SAPP_EVENTTYPE_KEY_DOWN:
  case SAPP_EVENTTYPE_KEY_UP: {
    int c = 0;
    int shift_c = 0;
    switch (event->key_code) {
    case SAPP_KEYCODE_SPACE:
      c = 0x20;
      break;
    case SAPP_KEYCODE_LEFT:
      c = 0x08;
      break;
    case SAPP_KEYCODE_RIGHT:
      c = 0x09;
      break;
    case SAPP_KEYCODE_DOWN:
      c = 0x0A;
      break;
    case SAPP_KEYCODE_UP:
      c = 0x0B;
      break;
    case SAPP_KEYCODE_ENTER:
      c = 0x0D;
      break;
    case SAPP_KEYCODE_LEFT_SHIFT:
      c = 0x02;
      break;
    case SAPP_KEYCODE_INSERT:
      c = 0x0E;
      break;
    case SAPP_KEYCODE_BACKSPACE:
      c = 0x01;
      shift_c = 0x0C;
      break; // 0x0C: clear screen
    case SAPP_KEYCODE_ESCAPE:
      c = 0x03;
      shift_c = 0x13;
      break; // 0x13: break
    default:
      c = 0;
      break;
    }
    if (c) {
      if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (shift_c == 0) {
          shift_c = c;
        }
        mo5_key_down(&app.mo5, shift ? shift_c : c);
      } else {
        mo5_key_up(&app.mo5, c);
        if (shift_c) {
            mo5_key_up(&app.mo5, shift_c);
        }
      }
    }
  } break;
  default:
    break;
  }
}

static void cleanup(void) {
  saudio_shutdown();
  sg_shutdown();
}

sapp_desc sokol_main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  return (sapp_desc){
      .init_cb = init,
      .frame_cb = frame,
      .event_cb = input,
      .cleanup_cb = cleanup,
      .width = 800,
      .height = 600,
      .window_title = "MO5",
  };
}
