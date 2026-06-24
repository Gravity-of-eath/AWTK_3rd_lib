#include "recycle_view_math.h"
#include <stddef.h>

int32_t recycle_linear_content_size(int32_t item_count, int32_t item_extent) {
  if (item_count <= 0 || item_extent <= 0) {
    return 0;
  }
  return item_count * item_extent;
}

int32_t recycle_linear_item_main_pos(int32_t index, int32_t item_extent) {
  if (index < 0 || item_extent <= 0) {
    return 0;
  }
  return index * item_extent;
}

int32_t recycle_linear_visible_range(int32_t offset, int32_t viewport_main, int32_t item_extent,
                                     int32_t item_count, int32_t* first, int32_t* last) {
  if (first == NULL || last == NULL) {
    return -1;
  }
  *first = 0;
  *last = -1;
  if (item_count <= 0 || item_extent <= 0 || viewport_main <= 0) {
    return 0;
  }
  if (offset < 0) {
    offset = 0;
  }
  *first = offset / item_extent;
  *last = (offset + viewport_main - 1) / item_extent;
  if (*first < 0) {
    *first = 0;
  }
  if (*last > item_count - 1) {
    *last = item_count - 1;
  }
  if (*first > *last) {
    /* offset 越过末尾：退回最后一项 */
    *first = item_count - 1;
    *last = item_count - 1;
  }
  return 0;
}

int32_t recycle_grid_content_size(int32_t item_count, int32_t span, int32_t item_extent) {
  int32_t rows = 0;
  if (item_count <= 0 || span <= 0 || item_extent <= 0) {
    return 0;
  }
  rows = (item_count + span - 1) / span;
  return rows * item_extent;
}

int32_t recycle_grid_cell(int32_t index, int32_t span, int32_t* row, int32_t* col) {
  if (row == NULL || col == NULL || span <= 0 || index < 0) {
    return -1;
  }
  *row = index / span;
  *col = index % span;
  return 0;
}

int32_t recycle_grid_visible_range(int32_t offset, int32_t viewport_main, int32_t item_extent,
                                   int32_t span, int32_t item_count, int32_t* first, int32_t* last) {
  int32_t first_row = 0;
  int32_t last_row = 0;
  if (first == NULL || last == NULL) {
    return -1;
  }
  *first = 0;
  *last = -1;
  if (item_count <= 0 || span <= 0 || item_extent <= 0 || viewport_main <= 0) {
    return 0;
  }
  if (offset < 0) {
    offset = 0;
  }
  first_row = offset / item_extent;
  last_row = (offset + viewport_main - 1) / item_extent;
  *first = first_row * span;
  *last = last_row * span + (span - 1);
  if (*first < 0) {
    *first = 0;
  }
  if (*last > item_count - 1) {
    *last = item_count - 1;
  }
  if (*first > item_count - 1) {
    *first = item_count - 1;
    *last = item_count - 1;
  }
  return 0;
}

int32_t recycle_clamp_offset(int32_t offset, int32_t content_size, int32_t viewport_main) {
  int32_t max_offset = content_size - viewport_main;
  if (max_offset < 0) {
    max_offset = 0;
  }
  if (offset < 0) {
    return 0;
  }
  if (offset > max_offset) {
    return max_offset;
  }
  return offset;
}

float recycle_fling_next_v(float v, float friction) {
  return v * friction;
}
