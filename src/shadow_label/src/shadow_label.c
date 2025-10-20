/**
 * File:   shadow_label.c
 * Author: 
 * Brief:  
 *
 * Copyright (c) 2025 - 2025 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2025-2-21  created
 *
 */

#include "tkc/mem.h"
#include "tkc/utils.h"
#include "shadow_label.h"
#include "base/line_parser.h"
#include "base/bidi.h"
#include "tkc/color_parser.h"
#include "base/canvas_offline.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define STR_ELLIPSES L"..."

static const char* widget_get_bidi_dump(widget_t* widget) {
  value_t v;
  if (widget_get_prop(widget, WIDGET_PROP_BIDI, &v) == RET_OK) {
    return value_str(&v);
  }

  return NULL;
}

static rect_t* canvas_fix_rect(const rect_t* r, rect_t* o) {
  if (r != NULL) {
    *o = *r;

    if (o->w < 0) {
      o->w = -o->w;
      o->x = o->x - o->w + 1;
    }

    if (o->h < 0) {
      o->h = -o->h;
      o->y = o->y - o->h + 1;
    }

    return o;
  } else {
    return NULL;
  }
}

static color_t interpolateColor(color_t* color, float percent, float exponent) {
  /* 确保百分比在 0~1 之间 */
  percent = percent < 0 ? 0 : percent;
  percent = percent > 1 ? 1 : percent;

  /* 获取颜色的各个通道 */
  uint8_t r = color_r(color);
  uint8_t g = color_g(color);
  uint8_t b = color_b(color);
  uint8_t a = color_a(color);

  /* 计算插值后的透明度 */
  /* 计算指数插值后的透明度 */
  float alpha = powf(percent, exponent);  // 使用指数函数计算插值
  uint8_t new_a = (uint8_t)(a * alpha);

  return color_init(r, g, b, new_a);
}

static ret_t canvas_draw_text_in_rect_dump(widget_t* widget, canvas_t* c, const wchar_t* str,
                                           uint32_t nr, const rect_t* r_in, int32_t fill_index) {
  int x = 0;
  int y = 0;
  rect_t r_fix = rect_init(0, 0, 0, 0);
  int32_t text_w = 0;
  int32_t height = canvas_get_font_height(c);
  rect_t* r = canvas_fix_rect(r_in, &r_fix);
  return_value_if_fail(c != NULL && str != NULL && r != NULL, RET_BAD_PARAMS);

  text_w = canvas_measure_text(c, str, nr);

  switch (c->text_align_v) {
    case ALIGN_V_TOP:
      y = r->y;
      break;
    case ALIGN_V_BOTTOM:
      y = r->y + (r->h - height);
      break;
    default:
      y = r->y + ((r->h - height) >> 1);
      break;
  }

  switch (c->text_align_h) {
    case ALIGN_H_LEFT:
      x = r->x;
      break;
    case ALIGN_H_RIGHT:
      x = r->x + (r->w - text_w);
      break;
    default:
      x = r->x + ((r->w - text_w) >> 1);
      break;
  }
  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  color_t old_color = c->lcd->text_color;
  color_t s_color = color_parse(shadow_label->shadow_color);
  for (int i = 0; i < shadow_label->shadow_offset; i++) {
    canvas_set_text_color(c, interpolateColor(&s_color, (i) * 1.0f / shadow_label->shadow_offset,
                                              shadow_label->exponent));
    canvas_draw_text(c, str, nr, x + shadow_label->shadow_offset - i,
                     y + shadow_label->shadow_offset - i);  //draw_shadow

    // printf("canvas_draw_textshadow_label 9");
    canvas_set_text_color(shadow_label->c,
                          interpolateColor(&s_color, (i) * 1.0f / shadow_label->shadow_offset,
                                           shadow_label->exponent));
    canvas_draw_text(shadow_label->c, str, nr, x + shadow_label->shadow_offset - i,
                     y + shadow_label->shadow_offset - i);  //draw_shadow for copy
  }
  canvas_set_text_color(c, old_color);
  canvas_set_text_color(shadow_label->c, old_color);
  // printf("canvas_draw_textshadow_label 10\n");
  if (fill_index == 0) {
    shadow_label->text_lt.x = x;
    shadow_label->text_lt.y = y;
    shadow_label->text_rb.x = x + text_w;
    shadow_label->text_rb.y = y + height;
  } else {
    shadow_label->text_lt.x = tk_min(shadow_label->text_lt.x, x);
    shadow_label->text_lt.y = tk_min(shadow_label->text_lt.y, y);
    shadow_label->text_rb.x = tk_max(shadow_label->text_rb.x, shadow_label->text_lt.x + text_w);
    shadow_label->text_rb.y += height;
  }
  canvas_draw_text(shadow_label->c, str, nr, x, y);
  // printf("canvas_draw_textshadow_label 11\n");
  return canvas_draw_text(c, str, nr, x, y);
}

static ret_t canvas_draw_text_in_rect_ellipses(widget_t* widget, canvas_t* c, const wchar_t* str,
                                               uint32_t nr, const rect_t* r_in, bidi_type_t type,
                                               int32_t fill_index) {
  uint32_t i = 0;
  rect_t r = *r_in;
  float_t text_w = 0;
  float_t ellipses_w = canvas_measure_text(c, STR_ELLIPSES, wcslen(STR_ELLIPSES));

  for (i = 0; i < nr; i++) {
    float_t char_w = canvas_measure_text(c, str + i, 1);
    if ((text_w + char_w + ellipses_w) >= r.w) {
      break;
    }

    text_w += char_w;
  }

  r.w = text_w;
  canvas_draw_text_in_rect_dump(widget, c, str, i, &r, fill_index);
  r.x = text_w;
  r.w = ellipses_w;
  canvas_draw_text_in_rect_dump(widget, c, STR_ELLIPSES, wcslen(STR_ELLIPSES), &r, fill_index);

  return RET_OK;
}

static ret_t canvas_draw_text_bidi_in_rect_dump(widget_t* widget, canvas_t* c, const wchar_t* str,
                                                uint32_t nr, const rect_t* r_in,
                                                const char* bidi_type, bool_t ellipses,
                                                int32_t fill_index) {
  bidi_t b;
  ret_t ret = RET_FAIL;
  return_value_if_fail(c != NULL && str != NULL && r_in != NULL, RET_BAD_PARAMS);

  if (nr == 0) {
    return RET_OK;
  }

  bidi_init(&b, FALSE, FALSE, bidi_type_from_name(bidi_type));
  if (bidi_log2vis(&b, str, nr) == RET_OK) {
    float_t text_w = canvas_measure_text(c, b.vis_str, b.vis_str_size);
    if (ellipses && text_w > r_in->w) {
      ret = canvas_draw_text_in_rect_ellipses(widget, c, b.vis_str, b.vis_str_size, r_in,
                                              b.resolved_type, fill_index);
    } else {
      ret = canvas_draw_text_in_rect_dump(widget, c, b.vis_str, b.vis_str_size, r_in, fill_index);
    }
  } else {
    assert(!"log2vis failed!");
  }

  bidi_deinit(&b);

  return ret;
}

static ret_t widget_draw_text_in_rect_dump(widget_t* widget, canvas_t* c, const wchar_t* str,
                                           uint32_t size, const rect_t* r, bool_t ellipses,
                                           int32_t fill_index) {
  const char* bidi_type = widget_get_bidi_dump(widget);
  return_value_if_fail(widget != NULL && c != NULL && str != NULL && r != NULL, RET_BAD_PARAMS);

  return canvas_draw_text_bidi_in_rect_dump(widget, c, str, size, r, bidi_type, ellipses,
                                            fill_index);
}

static ret_t widget_draw_icon_text_dump(widget_t* widget, canvas_t* c, const char* icon,
                                        wstr_t* text) {
  rect_t ir;
  wh_t w = 0;
  wh_t h = 0;
  bitmap_t img;
  rect_t r_icon;
  rect_t r_text;
  int32_t margin = 0;
  int32_t spacer = 0;
  int32_t icon_at = 0;
  uint16_t font_size = 0;
  float_t text_size = 0.0f;
  int32_t margin_left = 0;
  int32_t margin_right = 0;
  int32_t margin_top = 0;
  int32_t margin_bottom = 0;
  style_t* style = widget->astyle;
  int32_t align_h = ALIGN_H_LEFT;
  int32_t align_v = ALIGN_V_MIDDLE;
  return_value_if_fail(widget->astyle != NULL, RET_BAD_PARAMS);

  spacer = style_get_int(style, STYLE_ID_SPACER, 2);
  margin = style_get_int(style, STYLE_ID_MARGIN, 0);
  margin_top = style_get_int(style, STYLE_ID_MARGIN_TOP, margin);
  margin_left = style_get_int(style, STYLE_ID_MARGIN_LEFT, margin);
  margin_right = style_get_int(style, STYLE_ID_MARGIN_RIGHT, margin);
  margin_bottom = style_get_int(style, STYLE_ID_MARGIN_BOTTOM, margin);
  icon_at = style_get_int(style, STYLE_ID_ICON_AT, ICON_AT_AUTO);

  w = widget->w - margin_left - margin_right;
  h = widget->h - margin_top - margin_bottom;
  ir = rect_init(margin_left, margin_top, w, h);

  if (text == NULL) {
    text = &(widget->text);
  }

  if (icon == NULL) {
    icon = style_get_str(style, STYLE_ID_ICON, NULL);
  }

  widget_prepare_text_style(widget, c);

  font_size = c->font_size;
  text_size = text->str ? canvas_measure_text(c, text->str, text->size) : 0;
  if (icon_at == ICON_AT_RIGHT || icon_at == ICON_AT_LEFT) {
    align_v = style_get_int(style, STYLE_ID_TEXT_ALIGN_V, ALIGN_V_MIDDLE);
    align_h = style_get_int(style, STYLE_ID_TEXT_ALIGN_H, ALIGN_H_LEFT);
  } else {
    align_v = style_get_int(style, STYLE_ID_TEXT_ALIGN_V, ALIGN_V_MIDDLE);
    align_h = style_get_int(style, STYLE_ID_TEXT_ALIGN_H, ALIGN_H_CENTER);
  }
  canvas_set_text_align(c, (align_h_t)align_h, (align_v_t)align_v);

  if (icon != NULL && widget_load_image(widget, icon, &img) == RET_OK) {
    float_t dpr = system_info()->device_pixel_ratio;

    if (text->size > 0) {
      if ((h > (img.h / dpr + font_size) && icon_at == ICON_AT_AUTO)) {
        icon_at = ICON_AT_TOP;
      }

      widget_calc_icon_text_rect(&ir, font_size, text_size, icon_at, img.w, img.h, spacer, &r_text,
                                 &r_icon);

      canvas_draw_icon_in_rect(c, &img, &r_icon);
      widget_draw_text_in_rect_dump(widget, c, text->str, text->size, &r_text, FALSE, 0);
    } else {
      if (icon_at == ICON_AT_AUTO) {
        widget_calc_icon_text_rect(&ir, font_size, text_size, icon_at, img.w, img.h, spacer, NULL,
                                   &r_icon);
      } else {
        widget_calc_icon_text_rect(&ir, font_size, text_size, icon_at, img.w, img.h, spacer,
                                   &r_text, &r_icon);
      }
      canvas_draw_icon_in_rect(c, &img, &r_icon);
    }
  } else if (text != NULL && text->size > 0) {
    widget_calc_icon_text_rect(&ir, font_size, text_size, icon_at, 0, 0, spacer, &r_text, NULL);
    widget_draw_text_in_rect_dump(widget, c, text->str, text->size, &r_text, FALSE, 0);
  }
  return RET_OK;
}

static ret_t widget_paint_helper_dump(widget_t* widget, canvas_t* c, const char* icon,
                                      wstr_t* text) {
  if (style_is_valid(widget->astyle)) {
    widget_draw_icon_text_dump(widget, c, icon, text);
  }

  return RET_OK;
}

static ret_t label_paint_text_mlines_dump(widget_t* widget, canvas_t* c, line_parser_t* p,
                                          int32_t x, int32_t y, int32_t w, int32_t h) {
  int32_t top = y;
  int32_t bottom = y + h;
  style_t* style = widget->astyle;
  int32_t font_size = c->font_size;
  int32_t line_height = font_size + style_get_int(style, STYLE_ID_SPACER, 2);
  align_v_t align_v = (align_v_t)style_get_int(style, STYLE_ID_TEXT_ALIGN_V, ALIGN_V_MIDDLE);
  align_h_t align_h = (align_h_t)style_get_int(style, STYLE_ID_TEXT_ALIGN_H, ALIGN_H_CENTER);
  int32_t h_text = p->total_lines * line_height;

  switch (align_v) {
    case ALIGN_V_MIDDLE: {
      y = (widget->h - h_text) / 2;
      break;
    }
    case ALIGN_V_BOTTOM: {
      y = y + h - h_text;
      break;
    }
    default: {
      break;
    }
  }

  y = tk_max(y, top);
  canvas_set_text_align(c, align_h, align_v);
  int32_t index = 0;

  while (line_parser_next(p) == RET_OK) {
    uint32_t size = 0;
    rect_t r = rect_init(x, y, w, font_size);

    if ((y + font_size) > bottom) {
      break;
    }

    for (size = 0; size < p->line_size; size++) {
      if (p->line[size] == '\r' || p->line[size] == '\n') {
        break;
      }
    }
    widget_draw_text_in_rect_dump(widget, c, p->line, size, &r, FALSE, index);
    index++;
    y += line_height;
  }

  return RET_OK;
}

static bitmap_mirror(bitmap_t* dst, bitmap_t* src, rect_t* rect, int32_t exponent) {
  return_value_if_fail(dst != NULL && src != NULL, NULL);
  int32_t w = dst->w;
  int32_t h = dst->h;
  uint8_t* src_data = bitmap_lock_buffer_for_read(src);
  uint8_t* dst_data = bitmap_lock_buffer_for_write(dst);
  int32_t dst_x = 0;
  int32_t dst_y = 0;
  for (int32_t y = rect->y; y < rect->y + rect->h; y++) {
    int32_t dsty = h - dst_y - 1;  // 计算镜像后的Y坐标
    // float alpha_factor = 1.0f - (float)dsty / (h - 1);  // 渐变因子（从0到1）
    // float alpha_factor = 1.0f - pow((float)y / h, 2);
    float alpha_factor = powf(1.0f - (float)dsty / (h - 1), exponent);
    dst_x = 0;
    for (int32_t x = rect->x; x < rect->x + rect->w; x++) {
      uint8_t* src_pixel = src_data + (y * src->w + x) * 4;
      uint8_t* dst_pixel = dst_data + (dsty * dst->w + dst_x) * 4;

      // 保持RGB不变，修改Alpha通道
      dst_pixel[0] = src_pixel[0];                 // R
      dst_pixel[1] = src_pixel[1];                 // G
      dst_pixel[2] = src_pixel[2];                 // B
      dst_pixel[3] = src_pixel[3] * alpha_factor;  // A（应用渐变）
      dst_x++;
    }
    dst_y++;
  }
  bitmap_unlock_buffer(src);
  bitmap_unlock_buffer(dst);
  // return dst;
}

// #pragma pack(push, 1)
// typedef struct {
//   uint16_t bfType;
//   uint32_t bfSize;
//   uint16_t bfReserved1;
//   uint16_t bfReserved2;
//   uint32_t bfOffBits;
// } BMPFileHeader;

// typedef struct {
//   uint32_t biSize;
//   int32_t biWidth;
//   int32_t biHeight;
//   uint16_t biPlanes;
//   uint16_t biBitCount;
//   uint32_t biCompression;
//   uint32_t biSizeImage;
//   int32_t biXPelsPerMeter;
//   int32_t biYPelsPerMeter;
//   uint32_t biClrUsed;
//   uint32_t biClrImportant;
// } BMPInfoHeader;
// #pragma pack(pop)

// /**
//  * 保存 bitmap_t 为 BMP 文件
//  * @param bitmap  AWTK 位图对象
//  * @param filename 输出文件名
//  * @return 成功返回 RET_OK，失败返回 RET_FAIL
//  */
// ret_t bitmap_save_to_bmp(bitmap_t* bitmap, const char* filename) {
//   uint32_t channels = 4;
//   uint32_t stride = bitmap->line_length;
//   uint8_t* data = bitmap_lock_buffer_for_read(bitmap);

//   // // 根据格式确定每像素字节数
//   // switch (bitmap->format) {
//   //     case BITMAP_FMT_RGBA8888: channels = 4; break;
//   //     case BITMAP_FMT_RGB888:   channels = 3; break;
//   //     case BITMAP_FMT_BGR565:   channels = 2; break;
//   //     default:
//   //         log_debug("Unsupported format: %d\n", bitmap->format);
//   //         return RET_FAIL;
//   // }

//   // 计算对齐后的行字节数（BMP 要求每行按4字节对齐）
//   uint32_t bmp_stride = (bitmap->w * channels + 3) & ~3;
//   uint32_t image_size = bmp_stride * bitmap->h;

//   // 初始化 BMP 头
//   BMPFileHeader file_header = {.bfType = 0x4D42,
//                                .bfSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + image_size,
//                                .bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader)};

//   BMPInfoHeader info_header = {.biSize = sizeof(BMPInfoHeader),
//                                .biWidth = bitmap->w,
//                                .biHeight = bitmap->h,
//                                .biPlanes = 1,
//                                .biBitCount = (uint16_t)(channels * 8),
//                                .biCompression = 0,
//                                .biSizeImage = image_size,
//                                .biXPelsPerMeter = 0,
//                                .biYPelsPerMeter = 0,
//                                .biClrUsed = 0,
//                                .biClrImportant = 0};

//   // 写入文件
//   FILE* file = fopen(filename, "wb");
//   if (file == NULL) {
//     log_debug("Failed to open file: %s\n", filename);
//     return RET_FAIL;
//   }

//   fwrite(&file_header, 1, sizeof(file_header), file);
//   fwrite(&info_header, 1, sizeof(info_header), file);

//   // BMP 像素数据从下到上存储
//   for (int y = bitmap->h - 1; y >= 0; y--) {
//     const uint8_t* row = data + y * stride;
//     fwrite(row, 1, bitmap->w * channels, file);

//     // 写入对齐填充字节（如果需要）
//     if (bmp_stride > bitmap->w * channels) {
//       uint8_t padding[3] = {0};
//       fwrite(padding, 1, stride - bitmap->w * channels, file);
//     }
//   }

//   fclose(file);
//   return RET_OK;
// }
// int32_t save = 0;

static ret_t label_paint_text_dump(widget_t* widget, canvas_t* c, const wchar_t* str,
                                   uint32_t size) {
  // label_t* label = LABEL(widget);
  line_parser_t p;
  ret_t ret = RET_OK;
  rect_t r = widget_get_content_area(widget);

  return_value_if_fail((r.w > 0 && widget->h >= c->font_size), RET_FAIL);
  return_value_if_fail(line_parser_init(&p, c, widget->text.str, widget->text.size, c->font_size,
                                        r.w, p.line_wrap, p.word_wrap) == RET_OK,
                       RET_BAD_PARAMS);

  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  if (shadow_label->mirror_enable) {
    if (shadow_label->c == NULL) {
      shadow_label->c = canvas_offline_create(widget->w, widget->h, BITMAP_FMT_BGRA8888);
    }
    canvas_offline_clear_canvas(shadow_label->c);
    canvas_offline_begin_draw(shadow_label->c);

    canvas_set_font_manager(shadow_label->c, font_manager());
    style_t* style = widget->astyle;
    const char* name_f = style_get_str(style, STYLE_ID_FONT_NAME, "default.ttf");
    int32_t f_size = style_get_int(style, STYLE_ID_FONT_SIZE, 22);
    // printf("name_f =%s f_size=%d  \n", name_f,f_size);
    canvas_set_font(shadow_label->c, name_f, f_size);
    // canvas_set_text_color(shadow_label->c, color_init(0xff, 0xff, 0xff, 0xff));
    // canvas_set_fill_color_str(shadow_label->c, "#FFFFFFFF");
    // canvas_set_stroke_color_str(shadow_label->c, "#FFFFFFFF");
  }

  if (p.total_lines > 1) {
    ret = label_paint_text_mlines_dump(widget, c, &p, r.x, r.y, r.w, r.h);
  } else {
    wstr_t str = widget->text;
    str.size = size;
    ret = widget_paint_helper_dump(widget, c, NULL, &str);
  }
  if (shadow_label->mirror_enable && shadow_label->c != NULL) {
    rect_t rect;
    rect.x = shadow_label->text_lt.x;
    rect.y = shadow_label->text_lt.y;
    rect.w = shadow_label->text_rb.x - shadow_label->text_lt.x;
    rect.h = shadow_label->text_rb.y - shadow_label->text_lt.y;
    if (shadow_label->mirror_enable && shadow_label->debug_enable) {
      canvas_set_stroke_color_str(c, "#00FF00FF");
      canvas_stroke_rect(c, rect.x, rect.y, rect.w, rect.h);

      printf("befor rect x=%d y=%d w=%d h=%d  shadow_label->mirror_hight=%d\n", rect.x, rect.y,
             rect.w, rect.h, shadow_label->mirror_hight);
    }
    //最大阴影高度大于0则有效
    int32_t mirror_src_h = rect.h;
    int32_t mirror_y = rect.y + rect.h;
    if (shadow_label->mirror_hight > 0 && rect.h > shadow_label->mirror_hight) {
      rect.y = rect.y + (rect.h - shadow_label->mirror_hight);
      rect.h = shadow_label->mirror_hight;
    }

    if (shadow_label->mirror_enable && shadow_label->debug_enable) {
      canvas_set_stroke_color_str(c, "#FF0000FF");
      canvas_stroke_rect(c, rect.x, rect.y, rect.w, rect.h);

      printf("fix rect x=%d y=%d w=%d h=%d\n", rect.x, rect.y, rect.w, rect.h);

      // canvas_set_stroke_color_str(shadow_label->c, "#FFFFFFFF");
      // canvas_stroke_rect(shadow_label->c, rect.x, rect.y, rect.w, rect.h);

      canvas_set_stroke_color_str(c, "#0000FFFF");
      canvas_stroke_rect(c, 0, 0, widget->w, widget->h);
    }

    canvas_offline_flush_bitmap(shadow_label->c);
    canvas_offline_end_draw(shadow_label->c);

    bitmap_t* bitmap = canvas_offline_get_bitmap(shadow_label->c);
    shadow_label->bitmap_copy = bitmap_create_ex(rect.w, rect.h, rect.w * 4, BITMAP_FMT_RGBA8888);
    bitmap_mirror(shadow_label->bitmap_copy, bitmap, &rect, shadow_label->exponent);
    // char name[32];
    // tk_snprintf(name, 32, "/tmp/bmp_%d*%d.bmp", bitmap->w, bitmap->h);
    // bitmap_save_to_bmp(bitmap, name);
    // printf("shadow_label->bitmap_copy w=%d h=%d\n", shadow_label->bitmap_copy->w,
    //        shadow_label->bitmap_copy->h);
    rect.y = mirror_y + (mirror_src_h * shadow_label->mirror_offset_ratio);
    // if (rect.y + rect.h > widget->y + widget->h) {
    //   rect.h = widget->y + widget->h - rect.y + rect.h;
    // }
    canvas_draw_image_ex(c, shadow_label->bitmap_copy, IMAGE_DRAW_DEFAULT, &rect);
    if (shadow_label->mirror_enable && shadow_label->debug_enable) {
      canvas_set_stroke_color_str(shadow_label->c, "#000000FF");
      canvas_stroke_rect(shadow_label->c, rect.x, rect.y, rect.w, rect.h);
    }
    if (shadow_label->bitmap_copy != NULL) {
      bitmap_destroy(shadow_label->bitmap_copy);
      shadow_label->bitmap_copy = NULL;
    }

    if (shadow_label->c != NULL) {
      canvas_offline_destroy(shadow_label->c);
      shadow_label->c = NULL;
    }
  }
  line_parser_deinit(&p);
  return ret;
}

static wh_t label_get_text_line_max_w(widget_t* widget, canvas_t* c) {
  wh_t line_max_w = 0;
  line_parser_t parser;
  line_parser_t* p = &parser;
  wstr_t* str = &(widget->text);

  return_value_if_fail(
      line_parser_init(p, c, str->str, str->size, c->font_size, 0xffff, FALSE, FALSE) == RET_OK,
      RET_BAD_PARAMS);

  while (line_parser_next(p) == RET_OK) {
    uint32_t line_w = 0;
    line_w = canvas_measure_text(c, p->line, p->line_size);
    if (line_w > line_max_w) {
      line_max_w = line_w;
    }
  }
  line_parser_deinit(p);
  return line_max_w;
}

ret_t shadow_label_set_mirror_offset_ratio(widget_t* widget, float_t mirror_offset_ratio) {
  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  return_value_if_fail(shadow_label != NULL, RET_BAD_PARAMS);
  shadow_label->mirror_offset_ratio = mirror_offset_ratio;
  return RET_OK;
}
ret_t shadow_label_set_shadow_offset(widget_t* widget, int32_t shadow_offset) {
  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  return_value_if_fail(shadow_label != NULL, RET_BAD_PARAMS);
  shadow_label->shadow_offset = shadow_offset;
  return RET_OK;
}

ret_t shadow_label_set_mirror_hight(widget_t* widget, int32_t mirror_hight) {
  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  return_value_if_fail(shadow_label != NULL, RET_BAD_PARAMS);
  shadow_label->mirror_hight = mirror_hight;
  return RET_OK;
}

ret_t shadow_label_set_debug_enable(widget_t* widget, bool_t debug_enable) {
  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  return_value_if_fail(shadow_label != NULL, RET_BAD_PARAMS);
  shadow_label->debug_enable = debug_enable;
  return RET_OK;
}

ret_t shadow_label_set_mirrror_enable(widget_t* widget, bool_t mirrror_enable) {
  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  return_value_if_fail(shadow_label != NULL, RET_BAD_PARAMS);
  shadow_label->mirror_enable = mirrror_enable;
  return RET_OK;
}
ret_t shadow_label_set_exponent(widget_t* widget, float_t exponent) {
  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  return_value_if_fail(shadow_label != NULL, RET_BAD_PARAMS);
  shadow_label->exponent = exponent;

  return RET_OK;
}

ret_t shadow_label_set_shadow_color(widget_t* widget, const char* shadow_color) {
  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  return_value_if_fail(shadow_label != NULL, RET_BAD_PARAMS);

  shadow_label->shadow_color = tk_str_copy(shadow_label->shadow_color, shadow_color);

  return RET_OK;
}

static ret_t shadow_label_get_prop(widget_t* widget, const char* name, value_t* v) {
  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  return_value_if_fail(shadow_label != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(SHADOW_LABEL_PROP_DEBUG_ENABLE, name)) {
    value_set_int32(v, shadow_label->debug_enable);
    return RET_OK;
  } else if (tk_str_eq(SHADOW_LABEL_PROP_MIRROR_HIGHT, name)) {
    value_set_int32(v, shadow_label->mirror_hight);
    return RET_OK;
  } else if (tk_str_eq(SHADOW_LABEL_PROP_MIRROR_ENABLE, name)) {
    value_set_bool(v, shadow_label->mirror_enable);
    return RET_OK;
  } else if (tk_str_eq(SHADOW_LABEL_PROP_MIRROR_OFFSET_RATIO, name)) {
    value_set_float(v, shadow_label->mirror_offset_ratio);
    return RET_OK;
  } else if (tk_str_eq(SHADOW_LABEL_PROP_SHADOW_OFFSET, name)) {
    value_set_int32(v, shadow_label->shadow_offset);
    return RET_OK;
  } else if (tk_str_eq(SHADOW_LABEL_PROP_SHADOW_COLOR, name)) {
    value_set_str(v, shadow_label->shadow_color);
    return RET_OK;
  } else if (tk_str_eq(SHADOW_LABEL_PROP_EXPONENT, name)) {
    value_set_float(v, shadow_label->exponent);
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t shadow_label_set_prop(widget_t* widget, const char* name, const value_t* v) {
  // shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  return_value_if_fail(widget != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(SHADOW_LABEL_PROP_DEBUG_ENABLE, name)) {
    shadow_label_set_debug_enable(widget, value_bool(v));
    return RET_OK;
  } else if (tk_str_eq(SHADOW_LABEL_PROP_MIRROR_HIGHT, name)) {
    shadow_label_set_mirror_hight(widget, value_int32(v));
    return RET_OK;
  } else if (tk_str_eq(SHADOW_LABEL_PROP_MIRROR_ENABLE, name)) {
    shadow_label_set_mirrror_enable(widget, value_bool(v));
    return RET_OK;
  } else if (tk_str_eq(SHADOW_LABEL_PROP_MIRROR_OFFSET_RATIO, name)) {
    shadow_label_set_mirror_offset_ratio(widget, value_float(v));
    return RET_OK;
  } else if (tk_str_eq(SHADOW_LABEL_PROP_SHADOW_OFFSET, name)) {
    shadow_label_set_shadow_offset(widget, value_int32(v));
    return RET_OK;
  } else if (tk_str_eq(SHADOW_LABEL_PROP_SHADOW_COLOR, name)) {
    shadow_label_set_shadow_color(widget, value_str(v));
    return RET_OK;
  } else if (tk_str_eq(SHADOW_LABEL_PROP_EXPONENT, name)) {
    shadow_label_set_exponent(widget, value_float(v));
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t shadow_label_on_destroy(widget_t* widget) {
  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  return_value_if_fail(widget != NULL && shadow_label != NULL, RET_BAD_PARAMS);

  TKMEM_FREE(shadow_label->shadow_color);

  return RET_OK;
}

static ret_t shadow_label_on_paint_self(widget_t* widget, canvas_t* c) {
  // shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  // if (widget->text.size > 0 && style_is_valid(widget->astyle)) {
  label_t* label = LABEL(widget);
  //   uint32_t size =
  //       label->length >= 0 ? tk_min(label->length, widget->text.size) : widget->text.size;

  widget_prepare_text_style(widget, c);

  label_paint_text_dump(widget, c, widget->text.str, widget->text.size);
  // }
  return RET_OK;
}

static ret_t shadow_label_on_event(widget_t* widget, event_t* e) {
  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  return_value_if_fail(widget != NULL && shadow_label != NULL, RET_BAD_PARAMS);

  (void)shadow_label;

  return RET_OK;
}

const char* s_shadow_label_properties[] = {SHADOW_LABEL_PROP_SHADOW_OFFSET,
                                           SHADOW_LABEL_PROP_SHADOW_COLOR,
                                           SHADOW_LABEL_PROP_MIRROR_ENABLE,
                                           SHADOW_LABEL_PROP_MIRROR_OFFSET_RATIO,
                                           SHADOW_LABEL_PROP_EXPONENT,
                                           SHADOW_LABEL_PROP_MIRROR_HIGHT,
                                           NULL};

TK_DECL_VTABLE(shadow_label) = {.size = sizeof(shadow_label_t),
                                .type = WIDGET_TYPE_SHADOW_LABEL,
                                .clone_properties = s_shadow_label_properties,
                                .persistent_properties = s_shadow_label_properties,
                                .parent = TK_PARENT_VTABLE(widget),
                                .create = shadow_label_create,
                                .on_paint_self = shadow_label_on_paint_self,
                                .set_prop = shadow_label_set_prop,
                                .get_prop = shadow_label_get_prop,
                                .on_event = shadow_label_on_event,
                                .on_destroy = shadow_label_on_destroy};

widget_t* shadow_label_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  widget_t* widget = widget_create(parent, TK_REF_VTABLE(shadow_label), x, y, w, h);
  shadow_label_t* shadow_label = SHADOW_LABEL(widget);
  return_value_if_fail(shadow_label != NULL, NULL);

  shadow_label->shadow_offset = 3;
  shadow_label->exponent = 1.2f;
  shadow_label->shadow_color = tk_strdup("#333333");
  shadow_label->mirror_enable = FALSE;
  shadow_label->mirror_hight = 50;
  shadow_label->mirror_offset_ratio = 0;
  shadow_label->debug_enable = FALSE;
  return widget;
}

widget_t* shadow_label_cast(widget_t* widget) {
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, shadow_label), NULL);

  return widget;
}
