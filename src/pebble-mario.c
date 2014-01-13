// Copyright (C) 2013 Denis Dzyubenko
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// You may contact the author at denis@ddenis.info
//

#include <pebble.h>
#include <time.h>

// #define INVERSED_COLORS

Window *window;

Layer *blocks_layer;
Layer *mario_layer;
TextLayer *text_hour_layer;
TextLayer *text_minute_layer;
Layer *ground_layer;
TextLayer *date_layer;

static char hour_text[]   = "00";
static char minute_text[] = "00";
static char date_text[] = "Wed, Apr 17";

GRect blocks_down_rect;
GRect blocks_up_rect;
GRect mario_down_rect;
GRect mario_up_rect;
GRect ground_rect;

GRect hour_up_rect;
GRect hour_normal_rect;
GRect hour_down_rect;
GRect minute_up_rect;
GRect minute_normal_rect;
GRect minute_down_rect;

static int mario_is_down = 1;

// TODO: I can really make use of BitmapLayer here
GBitmap *mario_normal_bmp;
GBitmap *mario_jump_bmp;
GBitmap *ground_bmp;

PropertyAnimation *mario_animation_beg;
PropertyAnimation *mario_animation_end;

PropertyAnimation *block_animation_beg;
PropertyAnimation *block_animation_end;

PropertyAnimation *hour_animation_slide_away;
PropertyAnimation *hour_animation_slide_in;
PropertyAnimation *minute_animation_slide_away;
PropertyAnimation *minute_animation_slide_in;

#define BLOCK_SIZE 50
#define BLOCK_LAYER_EXTRA 3
#define BLOCK_SQUEEZE 10
#define BLOCK_SPACING -1
#define MARIO_JUMP_DURATION 50
#define CLOCK_ANIMATION_DURATION 150
#define GROUND_HEIGHT 26

#ifndef INVERSED_COLORS
#  define MainColor GColorBlack
#  define BackgroundColor GColorWhite
#else
#  define MainColor GColorWhite
#  define BackgroundColor GColorBlack
#endif

void handle_tick(struct tm *tick_time, TimeUnits units_changed);

void draw_block(GContext *ctx, GRect rect, uint8_t width)
{
    static const uint8_t radius = 1;

    graphics_context_set_fill_color(ctx, MainColor);
    graphics_fill_rect(ctx, rect, radius, GCornersAll);

    rect.origin.x += width;
    rect.origin.y += width;
    rect.size.w -= width*2;
    rect.size.h -= width*2;
    graphics_context_set_fill_color(ctx, BackgroundColor);
    graphics_fill_rect(ctx, rect, radius, GCornersAll);

    static const uint8_t dot_offset = 3;
    static const uint8_t dot_width = 6;
    static const uint8_t dot_height = 4;

    GRect dot_rect;

    graphics_context_set_fill_color(ctx, MainColor);

    // top left dot
    dot_rect = GRect(rect.origin.x + dot_offset, rect.origin.y + dot_offset,
                     dot_width, dot_height);
    graphics_fill_rect(ctx, dot_rect, 1, GCornersAll);

    // top right dot
    dot_rect = GRect(rect.origin.x + rect.size.w - dot_offset - dot_width, rect.origin.y + dot_offset,
                     dot_width, dot_height);
    graphics_fill_rect(ctx, dot_rect, 1, GCornersAll);

    // bottom left dot
    dot_rect = GRect(rect.origin.x + dot_offset, rect.origin.y + rect.size.h - dot_offset - dot_height,
                     dot_width, dot_height);
    graphics_fill_rect(ctx, dot_rect, 1, GCornersAll);

    // bottom right dot
    dot_rect = GRect(rect.origin.x + rect.size.w - dot_offset - dot_width,
                     rect.origin.y + rect.size.h - dot_offset - dot_height,
                     dot_width, dot_height);
    graphics_fill_rect(ctx, dot_rect, 1, GCornersAll);
}

void blocks_update_callback(Layer *layer, GContext *ctx)
{
    (void)layer;
    (void)ctx;

    GRect block_rect[2];

    GRect layer_bounds = layer_get_bounds(layer);
    GRect layer_frame = layer_get_frame(layer);

    block_rect[0] = GRect(layer_bounds.origin.x,
                          layer_bounds.origin.y + BLOCK_LAYER_EXTRA,
                          BLOCK_SIZE,
                          layer_frame.size.h - BLOCK_LAYER_EXTRA);
    block_rect[1] = GRect(layer_bounds.origin.x + BLOCK_SIZE + BLOCK_SPACING,
                          layer_bounds.origin.y + BLOCK_LAYER_EXTRA,
                          BLOCK_SIZE,
                          layer_frame.size.h - BLOCK_LAYER_EXTRA);

    for (uint8_t i = 0; i < 2; ++i) {
        GRect *rect = block_rect + i;

        draw_block(ctx, *rect, 4);
    }
}

void mario_update_callback(Layer *layer, GContext *ctx)
{
    (void)layer;
    (void)ctx;

    GRect destination;
    GBitmap *bmp;

    bmp = mario_is_down ? mario_normal_bmp : mario_jump_bmp;
    destination = GRect(0, 0, bmp->bounds.size.w, bmp->bounds.size.h);

    graphics_draw_bitmap_in_rect(ctx, bmp, destination);
}

void ground_update_callback(Layer *layer, GContext *ctx)
{
    (void)layer;
    (void)ctx;

    GRect image_rect = ground_bmp->bounds;
    GRect rect = layer_get_frame(layer);
    int16_t image_width = image_rect.size.w;

    image_rect.origin.x = -10;
    image_rect.origin.y = 0;
    for (int i = 0; i < rect.size.w / image_rect.size.w + 1; ++i) {
        graphics_draw_bitmap_in_rect(ctx, ground_bmp, image_rect);
        image_rect.origin.x +=  image_width;
    }

    text_layer_set_text(date_layer, date_text);
}

void handle_init()
{
    window = window_create();
    window_stack_push(window, true /* Animated */);

    window_set_background_color(window, BackgroundColor);

    blocks_down_rect = GRect(22, 7, BLOCK_SIZE*2, BLOCK_SIZE + BLOCK_LAYER_EXTRA);
    blocks_up_rect = GRect(22, 0, BLOCK_SIZE*2, BLOCK_SIZE + BLOCK_LAYER_EXTRA - BLOCK_SQUEEZE);
    mario_down_rect = GRect(32, 168-GROUND_HEIGHT-80, 80, 80);
    mario_up_rect = GRect(32, BLOCK_SIZE + BLOCK_LAYER_EXTRA - BLOCK_SQUEEZE, 80, 80);
    ground_rect = GRect(0, 168-GROUND_HEIGHT, 144, 168);

    hour_up_rect = GRect(5, -10, 40, 40);
    hour_normal_rect = GRect(5, 5 + BLOCK_LAYER_EXTRA, 40, 40);
    hour_down_rect = GRect(5, BLOCK_SIZE + BLOCK_LAYER_EXTRA, 40, 40);
    minute_up_rect = GRect(5+BLOCK_SIZE+BLOCK_SPACING, -10, 40, 40);
    minute_normal_rect = GRect(5+BLOCK_SIZE+BLOCK_SPACING, 5 + BLOCK_LAYER_EXTRA, 40, 40);
    minute_down_rect = GRect(5+BLOCK_SIZE+BLOCK_SPACING, BLOCK_SIZE + BLOCK_LAYER_EXTRA, 40, 40);

    blocks_layer = layer_create(blocks_down_rect);
    mario_layer = layer_create(mario_down_rect);
    ground_layer = layer_create(ground_rect);

    layer_set_update_proc(blocks_layer, &blocks_update_callback);
    layer_set_update_proc(mario_layer, &mario_update_callback);
    layer_set_update_proc(ground_layer, &ground_update_callback);

    Layer *window_layer = window_get_root_layer(window);

    layer_add_child(window_layer, blocks_layer);
    layer_add_child(window_layer, mario_layer);
    layer_add_child(window_layer, ground_layer);

    text_hour_layer = text_layer_create(hour_normal_rect);
    text_layer_set_text_color(text_hour_layer, MainColor);
    text_layer_set_background_color(text_hour_layer, GColorClear);
    text_layer_set_text_alignment(text_hour_layer, GTextAlignmentCenter);
    text_layer_set_font(text_hour_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    layer_add_child(blocks_layer, (Layer *)text_hour_layer);

    text_minute_layer = text_layer_create(GRect(55, 5, 40, 40));
    text_layer_set_text_color(text_minute_layer, MainColor);
    text_layer_set_background_color(text_minute_layer, GColorClear);
    text_layer_set_text_alignment(text_minute_layer, GTextAlignmentCenter);
    text_layer_set_font(text_minute_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    layer_add_child(blocks_layer, (Layer *)text_minute_layer);

    GRect date_rect = GRect(30, 6, 144-30*2, ground_rect.size.h-6*2);
    date_layer = text_layer_create(date_rect);
    text_layer_set_text_color(date_layer, BackgroundColor);
    text_layer_set_background_color(date_layer, MainColor);
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(ground_layer, (Layer *)date_layer);

#ifndef INVERSED_COLORS
    mario_normal_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARIO_NORMAL);
    mario_jump_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARIO_JUMP);
    ground_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GROUND);
#else
    mario_normal_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARIO_NORMAL_INVERSED);
    mario_jump_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MARIO_JUMP_INVERSED);
    ground_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GROUND_INVERSED);
#endif

    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}

void handle_deinit()
{
    tick_timer_service_unsubscribe();

    gbitmap_destroy(mario_normal_bmp);
    gbitmap_destroy(mario_jump_bmp);
    gbitmap_destroy(ground_bmp);

    // TODO: destroy all the resources!

    window_destroy(window);
}

void mario_down_animation_started(Animation *animation, void *data)
{
    (void)animation;
    (void)data;
}

void mario_down_animation_stopped(Animation *animation, void *data)
{
    (void)animation;
    (void)data;
    mario_is_down = 1;
    layer_mark_dirty(mario_layer);
}

void mario_jump_animation_started(Animation *animation, void *data)
{
    (void)animation;
    (void)data;
    mario_is_down = 0;
    layer_mark_dirty(mario_layer);
}

void mario_jump_animation_stopped(Animation *animation, void *data)
{
    (void)animation;
    (void)data;

    text_layer_set_text(text_hour_layer, hour_text);
    text_layer_set_text(text_minute_layer, minute_text);

    mario_animation_end = property_animation_create_layer_frame(mario_layer,
                                                                &mario_up_rect,
                                                                &mario_down_rect);
    animation_set_duration((Animation *)mario_animation_end, MARIO_JUMP_DURATION);
    animation_set_curve((Animation *)mario_animation_end, AnimationCurveEaseIn);
    animation_set_handlers((Animation *)mario_animation_end, (AnimationHandlers){
        .started = (AnimationStartedHandler)mario_down_animation_started,
        .stopped = (AnimationStoppedHandler)mario_down_animation_stopped
    }, 0);

    animation_schedule((Animation *)mario_animation_end);
}

void block_up_animation_started(Animation *animation, void *data)
{
    (void)animation;
    (void)data;
}

void block_up_animation_stopped(Animation *animation, void *data)
{
    (void)animation;
    (void)data;

    block_animation_end = property_animation_create_layer_frame(blocks_layer,
                                                                &blocks_up_rect,
                                                                &blocks_down_rect);
    animation_set_duration((Animation *)block_animation_end, MARIO_JUMP_DURATION);
    animation_set_curve((Animation *)block_animation_end, AnimationCurveEaseIn);
    animation_schedule((Animation *)block_animation_end);
}

void clock_animation_slide_away_started(Animation *animation, void *data)
{
    (void)animation;
    (void)data;
}

void clock_animation_slide_away_stopped(Animation *animation, void *data)
{
    (void)animation;
    (void)data;

    layer_set_frame((Layer *)text_hour_layer, hour_down_rect);
    layer_set_frame((Layer *)text_minute_layer, minute_down_rect);

    hour_animation_slide_in = property_animation_create_layer_frame((Layer *)text_hour_layer,
                                                                    &hour_down_rect,
                                                                    &hour_normal_rect);
    animation_set_duration((Animation *)hour_animation_slide_in, CLOCK_ANIMATION_DURATION);
    animation_set_curve((Animation *)hour_animation_slide_in, AnimationCurveLinear);
    animation_schedule((Animation *)hour_animation_slide_in);

    minute_animation_slide_in = property_animation_create_layer_frame((Layer *)text_minute_layer,
                                                                      &minute_down_rect,
                                                                      &minute_normal_rect);
    animation_set_duration((Animation *)minute_animation_slide_in, CLOCK_ANIMATION_DURATION);
    animation_set_curve((Animation *)minute_animation_slide_in, AnimationCurveLinear);
    animation_schedule((Animation *)minute_animation_slide_in);
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
    mario_animation_beg = property_animation_create_layer_frame(mario_layer,
                                                                &mario_down_rect,
                                                                &mario_up_rect);
    animation_set_duration((Animation *)mario_animation_beg, MARIO_JUMP_DURATION);
    animation_set_curve((Animation *)mario_animation_beg, AnimationCurveEaseOut);
    animation_set_handlers((Animation *)mario_animation_beg, (AnimationHandlers){
        .started = (AnimationStartedHandler)mario_jump_animation_started,
        .stopped = (AnimationStoppedHandler)mario_jump_animation_stopped
    }, 0);

    block_animation_beg = property_animation_create_layer_frame(blocks_layer,
                                                                &blocks_down_rect,
                                                                &blocks_up_rect);
    animation_set_duration((Animation *)block_animation_beg, MARIO_JUMP_DURATION);
    animation_set_curve((Animation *)block_animation_beg, AnimationCurveEaseOut);
    animation_set_handlers((Animation *)block_animation_beg, (AnimationHandlers){
        .started = (AnimationStartedHandler)block_up_animation_started,
        .stopped = (AnimationStoppedHandler)block_up_animation_stopped
    }, 0);

    hour_animation_slide_away = property_animation_create_layer_frame((Layer *)text_hour_layer,
                                                                      &hour_normal_rect,
                                                                      &hour_up_rect);
    animation_set_duration((Animation *)hour_animation_slide_away, CLOCK_ANIMATION_DURATION);
    animation_set_curve((Animation *)hour_animation_slide_away, AnimationCurveLinear);
    animation_set_handlers((Animation *)hour_animation_slide_away, (AnimationHandlers){
        .started = (AnimationStartedHandler)clock_animation_slide_away_started,
        .stopped = (AnimationStoppedHandler)clock_animation_slide_away_stopped
    }, 0);

    minute_animation_slide_away = property_animation_create_layer_frame((Layer *)text_minute_layer,
                                                                        &minute_normal_rect,
                                                                        &minute_up_rect);
    animation_set_duration((Animation *)minute_animation_slide_away, CLOCK_ANIMATION_DURATION);
    animation_set_curve((Animation *)minute_animation_slide_away, AnimationCurveLinear);

    char *hour_format;
    if (clock_is_24h_style()) {
        hour_format = "%H";
    } else {
        hour_format = "%I";
    }

    time_t current_time = time(NULL);
    struct tm *tm = localtime(&current_time);

    strftime(hour_text, sizeof(hour_text), hour_format, tm);
    if (!clock_is_24h_style() && (hour_text[0] == '0')) {
        memmove(hour_text, &hour_text[1], sizeof(hour_text) - 1);
    }

    char *minute_format = "%M";
    strftime(minute_text, sizeof(minute_text), minute_format, tm);

    strftime(date_text, sizeof(date_text), "%a, %b %d", tm);

    animation_schedule((Animation *)mario_animation_beg);
    animation_schedule((Animation *)block_animation_beg);
    animation_schedule((Animation *)hour_animation_slide_away);
    animation_schedule((Animation *)minute_animation_slide_away);
}

int main(void)
{
    handle_init();
    app_event_loop();
    handle_deinit();
}
