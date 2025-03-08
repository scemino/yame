/*
   UI implementation for mo5.c, this must live in a .cc file.
*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "gfx.h"
#include "sokol_time.h"
#include "gfx.h"
#define EMU_UI_IMPL
#include "mo5.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui_settings.h"
#include "ui_util.h"
#include "ui_display.h"
#include "ui.h"
#include "ui_snapshot.h"
#include "ui_memedit.h"
#include "ui_kbd.h"
#include "ui_emu.h"
