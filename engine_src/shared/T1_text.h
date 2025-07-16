#ifndef TEXT_H
#define TEXT_H

#include "T1_platform_layer.h"
#include "T1_zspriteid.h"
#include "T1_zsprite.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FontSettings {
    GPULockedMaterial mat;
    int32_t touchable_id;
    uint32_t remove_shadow;
    uint32_t alpha_blending_enabled;
    float extra_offset_xy[2];
    float scale_factor;
    float ignore_lighting;
    float ignore_camera;
    float font_height; // = 30.0;
    float alpha;
} FontSettings;

extern FontSettings * font_settings;

void text_init(
    void * (* arg_text_malloc_func)(size_t size),
    const char * raw_fontmetrics_file_contents,
    const uint64_t raw_fontmetrics_file_size);

/*
This function includes offsets, which is only useful if you
are setting up your label as part of a group of objects and
you're intending to move the object (that includes this label)
to a new location later with a ScheduledAnimation. It will then
retain the offsets. If you don't care about this just use the
function below which keeps all offsets at 0
*/
void text_request_label_offset_around(
    const int32_t with_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width);

void text_request_label_around_x_at_top_y(
    const int32_t with_object_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float top_y_pixelspace,
    const float z,
    const float max_width);

void text_request_label_around(
    const int32_t with_object_id,
    const char * text_to_draw,
    const float mid_x_pixelspace,
    const float mid_y_pixelspace,
    const float z,
    const float max_width);

void text_request_label_renderable(
    const int32_t with_object_id,
    const char * text_to_draw,
    const float left_pixelspace,
    const float top_pixelspace,
    const float z,
    const float max_width);

void text_request_debug_text(const char * text);
void text_request_fps_counter(uint64_t elapsed_us);

void text_request_top_touchable_id(
    int32_t top_touchable_id);

#ifdef __cplusplus
}
#endif

#endif
