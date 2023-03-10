#include <stdio.h>
#include <assert.h>
#include "sokol_audio.h"
#define SOKOL_ARGS_IMPL
#include "sokol_args.h"
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_color.h"
#include "sokol_glue.h"
#include "sokol_shaders.glsl.h"
#include "sokol_time.h"
#define CHIPS_IMPL
#include "clk.h"
#include "mo5.h"
#include "fs.h"
#include "keybuf.h"
#include "clock.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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
  uint32_t frame_time_us;
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
  keybuf_init(&(keybuf_desc_t){.key_delay_frames = 7});
  clock_init();
  fs_init();

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

  bool delay_input = false;
  if (sargs_exists("file")) {
    delay_input = true;
    fs_start_load_file(FS_SLOT_IMAGE, sargs_value("file"));
  }
  if (!delay_input) {
    if (sargs_exists("input")) {
      keybuf_put(sargs_value("input"));
    }
  }
}

static void handle_file_loading(void) {
  fs_dowork();
  const uint32_t load_delay_frames = 60;
  if (fs_success(FS_SLOT_IMAGE) &&
      ((clock_frame_count_60hz() > load_delay_frames))) {
    bool load_success = false;
    if (fs_ext(FS_SLOT_IMAGE, "k7")) {
      load_success = mo5_insert_tape(&app.mo5, fs_data(FS_SLOT_IMAGE));
    } else if (fs_ext(FS_SLOT_IMAGE, "fd")) {
      load_success = mo5_insert_disk(&app.mo5, fs_data(FS_SLOT_IMAGE));
    } else if (fs_ext(FS_SLOT_IMAGE, "rom")) {
      load_success = mo5_insert_cartridge(&app.mo5, fs_data(FS_SLOT_IMAGE));
    }
    if (load_success) {
      if (sargs_exists("input")) {
        keybuf_put(sargs_value("input"));
      }
    }
    fs_reset(FS_SLOT_IMAGE);
  }
}

static void send_keybuf_input(void) {
  uint8_t key_code;
  if (0 != (key_code = keybuf_get(app.frame_time_us))) {
    mo5_key_down(&app.mo5, key_code);
    mo5_key_up(&app.mo5, key_code);
  }
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
  app.frame_time_us = clock_frame_time();
  mo5_step(&app.mo5, app.frame_time_us);

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

  handle_file_loading();
  send_keybuf_input();
}

static void input(const sapp_event *event) {
  const bool shift = event->modifiers & SAPP_MODIFIER_SHIFT;
  switch (event->type) {
  case SAPP_EVENTTYPE_MOUSE_DOWN: {
      app.mo5.input.penbutton = true;
  } break;
  case SAPP_EVENTTYPE_MOUSE_UP: {
      app.mo5.input.penbutton = false;
  } break;
  case SAPP_EVENTTYPE_MOUSE_MOVE: {
    app.mo5.input.xpen = (SCREEN_WIDTH * ((float)event->mouse_x)/event->framebuffer_width) - 8;
    app.mo5.input.ypen = (SCREEN_HEIGHT * ((float)event->mouse_y)/event->framebuffer_height) - 8;
  } break;
  case SAPP_EVENTTYPE_FILES_DROPPED: {
    fs_start_load_dropped_file(FS_SLOT_IMAGE);
  } break;
  case SAPP_EVENTTYPE_CHAR: {
    if (shift && event->char_code == 'S') {
      uint8_t screen[SCREEN_WIDTH * SCREEN_HEIGHT * 3];
      const int stride = SCREEN_WIDTH * 3;
      mo5_display_info_t info = mo5_display_info(&app.mo5);
      for (int h = 0; h < SCREEN_HEIGHT; h++) {
        for (int w = 0; w < SCREEN_WIDTH; w++) {
            uint8_t col_pal = app.mo5.display.screen[w + h*SCREEN_WIDTH];
            uint8_t* pal_ptr = (uint8_t*)(((uint32_t*)info.palette.ptr)+col_pal);
            screen[w * 3 + h * stride+0] = pal_ptr[3];
            screen[w * 3 + h * stride+1] = pal_ptr[2];
            screen[w * 3 + h * stride+2] = pal_ptr[1];
        }
      }
      //stbi_write_png("capture.png", SCREEN_WIDTH, SCREEN_HEIGHT, 3, screen, stride);
        stbi_write_jpg("capture.jpg", SCREEN_WIDTH, SCREEN_HEIGHT, 3, screen, 100);
    } else {
      int c = (int)event->char_code;
      if ((c > 0x20) && (c < 0x7F)) {
        mo5_key_down(&app.mo5, c);
        mo5_key_up(&app.mo5, c);
      }
    }
  } break;
  case SAPP_EVENTTYPE_KEY_DOWN:
  case SAPP_EVENTTYPE_KEY_UP: {
    int c = 0;
    int shift_c = 0;
    switch (event->key_code) {
    case SAPP_KEYCODE_SPACE:
      c = 0x20;
      if(event->type == SAPP_EVENTTYPE_KEY_DOWN) {
        app.mo5.input.joy_action &= ~MO5_JOY0_BTN_MASK; } else {
            app.mo5.input.joy_action |= MO5_JOY0_BTN_MASK;
      }
      break;
    case SAPP_KEYCODE_LEFT:
      c = 0x08;
      app.mo5.input.joys_position.j1_l = event->type != SAPP_EVENTTYPE_KEY_DOWN;
      break;
    case SAPP_KEYCODE_RIGHT:
      c = 0x09;
      app.mo5.input.joys_position.j1_r = event->type != SAPP_EVENTTYPE_KEY_DOWN;
      break;
    case SAPP_KEYCODE_DOWN:
      c = 0x0A;
      app.mo5.input.joys_position.j1_d = event->type != SAPP_EVENTTYPE_KEY_DOWN;
      break;
    case SAPP_KEYCODE_UP:
      c = 0x0B;
      app.mo5.input.joys_position.j1_u = event->type != SAPP_EVENTTYPE_KEY_DOWN;
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
  sargs_setup(&(sargs_desc){
      .argc = argc,
      .argv = argv,
      .buf_size = 512 * 1024,
  });

  return (sapp_desc){
      .enable_dragndrop = true,
      .max_dropped_files = 1,
      .init_cb = init,
      .frame_cb = frame,
      .event_cb = input,
      .cleanup_cb = cleanup,
      .width = 512,
      .height = 384,
      .window_title = "MO5",
  };
}
