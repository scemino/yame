#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "data.h"

// standard loading slots
#define FS_SLOT_IMAGE (0)
#define FS_SLOT_SNAPSHOTS (1)
#define FS_NUM_SLOTS (2)

typedef enum {
    FS_RESULT_IDLE,
    FS_RESULT_FAILED,
    FS_RESULT_PENDING,
    FS_RESULT_SUCCESS,
} fs_result_t;

void fs_init(void);
void fs_dowork(void);
void fs_reset(size_t slot_index);
void fs_start_load_file(size_t slot_index, const char* path);
void fs_start_load_dropped_file(size_t slot_index);

bool fs_success(size_t slot_index);
bool fs_failed(size_t slot_index);
bool fs_pending(size_t slot_index);
data_t fs_data(size_t slot_index);
bool fs_ext(size_t slot_index, const char* str);
