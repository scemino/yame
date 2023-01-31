#include "fs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdalign.h>
#include "sokol_app.h"
#include "sokol_fetch.h"
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
#if defined(WIN32)
#include <windows.h>
#endif

#define FS_EXT_SIZE (16)
#define FS_PATH_SIZE (256)
#define FS_MAX_SIZE (2024 * 1024)

typedef struct {
    char cstr[FS_PATH_SIZE];
    size_t len;
    bool clamped;
} fs_path_t;

typedef struct {
    fs_path_t path;
    fs_result_t result;
    uint8_t* ptr;
    size_t size;
    alignas(64) uint8_t buf[FS_MAX_SIZE + 1];
} fs_slot_t;

typedef struct {
    bool valid;
    fs_slot_t slots[FS_NUM_SLOTS];
} fs_state_t;
static fs_state_t state;

void fs_init(void) {
    memset(&state, 0, sizeof(state));
    state.valid = true;
    sfetch_setup(&(sfetch_desc_t){
        .max_requests = 128,
        .num_channels = FS_NUM_SLOTS,
        .num_lanes = 1,
    });
}

void fs_dowork(void) {
    assert(state.valid);
    sfetch_dowork();
}

static void fs_path_reset(fs_path_t* path) {
    path->len = 0;
}

static void fs_path_append(fs_path_t* path, const char* str) {
    const size_t max_len = sizeof(path->cstr) - 1;
    char c;
    while ((c = *str++) && (path->len < max_len)) {
        path->cstr[path->len++] = c;
    }
    path->cstr[path->len] = 0;
    path->clamped = (c != 0);
}

static bool fs_path_extract_extension(fs_path_t* path, char* buf, size_t buf_size) {
    const char* ext = strrchr(path->cstr, '.');
    if (ext) {
        size_t i = 0;
        char c = 0;
        while ((c = *++ext) && (i < (buf_size-1))) {
            buf[i] = tolower(c);
            i++;
        }
        buf[i] = 0;
        return true;
    }
    else {
        return false;
    }
}

bool fs_ext(size_t slot_index, const char* ext) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    char buf[FS_EXT_SIZE];
    if (fs_path_extract_extension(&state.slots[slot_index].path, buf, sizeof(buf))) {
        return 0 == strcmp(ext, buf);
    }
    else {
        return false;
    }
}

void fs_reset(size_t slot_index) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    fs_slot_t* slot = &state.slots[slot_index];
    fs_path_reset(&slot->path);
    slot->result = FS_RESULT_IDLE;
    slot->ptr = 0;
    slot->size = 0;
}

static void fs_fetch_callback(const sfetch_response_t* response) {
    assert(state.valid);
    size_t slot_index = *(size_t*)response->user_data;
    assert(slot_index < FS_NUM_SLOTS);
    fs_slot_t* slot = &state.slots[slot_index];
    if (response->fetched) {
        slot->result = FS_RESULT_SUCCESS;
        slot->ptr = (uint8_t*)response->data.ptr;
        slot->size = response->data.size;
        assert(slot->size < sizeof(slot->buf));
        // in case it's a text file, zero-terminate the data
        slot->buf[slot->size] = 0;
    }
    else if (response->failed) {
        slot->result = FS_RESULT_FAILED;
    }
}

#if defined(__EMSCRIPTEN__)
static void fs_emsc_dropped_file_callback(const sapp_html5_fetch_response* response) {
    size_t slot_index = (size_t)(uintptr_t)response->user_data;
    assert(slot_index < FS_NUM_SLOTS);
    fs_slot_t* slot = &state.slots[slot_index];
    if (response->succeeded) {
        slot->result = FS_RESULT_SUCCESS;
        slot->ptr = (uint8_t*)response->data.ptr;
        slot->size = response->data.size;
        assert(slot->size < sizeof(slot->buf));
        // in case it's a text file, zero-terminate the data
        slot->buf[slot->size] = 0;
    }
    else {
        slot->result = FS_RESULT_FAILED;
    }
}
#endif

void fs_start_load_file(size_t slot_index, const char* path) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    fs_reset(slot_index);
    fs_slot_t* slot = &state.slots[slot_index];
    fs_path_append(&slot->path, path);
    slot->result = FS_RESULT_PENDING;
    sfetch_send(&(sfetch_request_t){
        .path = path,
        .channel = slot_index,
        .callback = fs_fetch_callback,
        .buffer = { .ptr = slot->buf, .size = FS_MAX_SIZE },
        .user_data = { .ptr = &slot_index, .size = sizeof(slot_index) },
    });
}

void fs_start_load_dropped_file(size_t slot_index) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    fs_reset(slot_index);
    fs_slot_t* slot = &state.slots[slot_index];
    const char* path = sapp_get_dropped_file_path(0);
    fs_path_append(&slot->path, path);
    slot->result = FS_RESULT_PENDING;
    #if defined(__EMSCRIPTEN__)
        sapp_html5_fetch_dropped_file(&(sapp_html5_fetch_request){
            .dropped_file_index = 0,
            .callback = fs_emsc_dropped_file_callback,
            .buffer = { .ptr = slot->buf, .size = FS_MAX_SIZE },
            .user_data = (void*)(intptr_t)slot_index,
        });
    #else
        fs_start_load_file(slot_index, path);
    #endif
}

fs_result_t fs_result(size_t slot_index) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    return state.slots[slot_index].result;
}

bool fs_success(size_t slot_index) {
    return fs_result(slot_index) == FS_RESULT_SUCCESS;
}

bool fs_failed(size_t slot_index) {
    return fs_result(slot_index) == FS_RESULT_FAILED;
}

bool fs_pending(size_t slot_index) {
    return fs_result(slot_index) == FS_RESULT_PENDING;
}

data_t fs_data(size_t slot_index) {
    assert(state.valid);
    assert(slot_index < FS_NUM_SLOTS);
    fs_slot_t* slot = &state.slots[slot_index];
    if (slot->result == FS_RESULT_SUCCESS) {
        return (data_t){ .ptr = slot->ptr, .size = slot->size };
    }
    else {
        return (data_t){0};
    }
}

