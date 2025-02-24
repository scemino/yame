#include <stdio.h>
#include <assert.h>
#include "sokol_audio.h"
#define SOKOL_ARGS_IMPL
#include "sokol_args.h"
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "clock.h"
#include "gfx.h"
#include "fs.h"
#define EMU_IMPL
#include "clk.h"
#include "mo5.h"
#include "keybuf.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "ui_util.h"
#if defined(EMU_USE_UI)
    #include "ui.h"
    #include "ui_emu.h"
#endif

static struct {
  uint32_t frame_time_us;
  mo5_t mo5;
  #ifdef EMU_USE_UI
    ui_emu_t ui;
  #endif
} app = {0};

#ifdef EMU_USE_UI
static void ui_draw_cb(const ui_draw_info_t* draw_info) {
    ui_emu_draw(&app.ui, &(ui_emu_frame_t){
        .display = draw_info->display,
    });
}
#endif

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
  mo5_desc_t mo5_desc = {
    .mgetc = mem_read,
    .mputc = mem_write,
    .audio_callback = {.func = audio_push}
  };
  mo5_init(&app.mo5, &mo5_desc);
  keybuf_init(&(keybuf_desc_t){.key_delay_frames = 7});
  clock_init();
  fs_init();

  saudio_setup(&(saudio_desc){
    .sample_rate = 22050,
    .logger.func = slog_func
  });

  sg_setup(&(sg_desc){
    .environment = sglue_environment(),
    .logger.func = slog_func,
  });

  gfx_init(&(gfx_desc_t){
    .display_info = mo5_display_info(&app.mo5),
    #ifdef EMU_USE_UI
        .draw_extra_cb = ui_draw,
    #endif
  });

  #ifdef EMU_USE_UI
    ui_init(&(ui_desc_t){
      .draw_cb = ui_draw_cb,
      .imgui_ini_key = "scemino.yame",
    });
    ui_emu_init(&app.ui, &(ui_emu_desc_t){
        .mo5 = &app.mo5,
    });
  #endif

  bool delay_input = false;
  if (sargs_exists("file")) {
    delay_input = true;
    fs_load_file_async(FS_CHANNEL_IMAGES, sargs_value("file"));
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
  if (fs_success(FS_CHANNEL_IMAGES) &&
      ((clock_frame_count_60hz() > load_delay_frames))) {
    bool load_success = false;
    if (fs_ext(FS_CHANNEL_IMAGES, "k7")) {
      load_success = mo5_insert_tape(&app.mo5, fs_data(FS_CHANNEL_IMAGES));
    } else if (fs_ext(FS_CHANNEL_IMAGES, "fd")) {
      load_success = mo5_insert_disk(&app.mo5, fs_data(FS_CHANNEL_IMAGES));
    } else if (fs_ext(FS_CHANNEL_IMAGES, "rom")) {
      load_success = mo5_insert_cartridge(&app.mo5, fs_data(FS_CHANNEL_IMAGES));
    }
    if (load_success) {
      if (sargs_exists("input")) {
        keybuf_put(sargs_value("input"));
      }
    }
    fs_reset(FS_CHANNEL_IMAGES);
  }
}

static void send_keybuf_input(void) {
  uint8_t key_code;
  if (0 != (key_code = keybuf_get(app.frame_time_us))) {
    mo5_key_down(&app.mo5, key_code);
    mo5_key_up(&app.mo5, key_code);
  }
}

static void frame(void) {
  app.frame_time_us = clock_frame_time();
  mo5_step(&app.mo5, app.frame_time_us);

  gfx_draw(mo5_display_info(&app.mo5));

  handle_file_loading();
  send_keybuf_input();
}

static void input(const sapp_event *event) {
#ifdef EMU_USE_UI
  if (ui_input(event)) {
    // input was handled by UI
    return;
  }
#endif

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
    fs_load_dropped_file_async(FS_CHANNEL_IMAGES);
  } break;
  case SAPP_EVENTTYPE_CHAR: {
    if (shift && event->char_code == 'S') {
      uint8_t screen[SCREEN_WIDTH * SCREEN_HEIGHT * 3];
      const int stride = SCREEN_WIDTH * 3;
      gfx_display_info_t info = mo5_display_info(&app.mo5);
      for (int h = 0; h < SCREEN_HEIGHT; h++) {
        for (int w = 0; w < SCREEN_WIDTH; w++) {
            uint8_t col_pal = app.mo5.display.screen[w + h*SCREEN_WIDTH];
            uint8_t* pal_ptr = (uint8_t*)(((uint32_t*)info.palette.ptr)+col_pal);
            screen[w * 3 + h * stride+0] = pal_ptr[0];
            screen[w * 3 + h * stride+1] = pal_ptr[1];
            screen[w * 3 + h * stride+2] = pal_ptr[2];
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
  #ifdef EMU_USE_UI
    ui_emu_discard(&app.ui);
    ui_discard();
  #endif
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
      .logger.func = slog_func,
  };
}
