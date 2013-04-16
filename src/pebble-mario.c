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

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0xB6, 0xA0, 0xAB, 0x5F, 0x92, 0xB7, 0x4C, 0x2B, 0xBC, 0x0F, 0x34, 0x6B, 0x99, 0xAE, 0x30, 0xA0 }
PBL_APP_INFO(MY_UUID,
             "Mario", "Denis Dzyubenko",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

Layer blocks_layer;
Layer mario_layer;
TextLayer text_hour_layer;
TextLayer text_minute_layer;
Layer ground_layer;

static char hour_text[]   = "00";
static char minute_text[] = "00";

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

BmpContainer mario_normal_bmp;
BmpContainer mario_jump_bmp;
BmpContainer ground_bmp;

PropertyAnimation mario_animation_beg;
PropertyAnimation mario_animation_end;

PropertyAnimation block_animation_beg;
PropertyAnimation block_animation_end;

PropertyAnimation hour_animation_slide_away;
PropertyAnimation hour_animation_slide_in;
PropertyAnimation minute_animation_slide_away;
PropertyAnimation minute_animation_slide_in;

#define BLOCK_SIZE 50
#define BLOCK_LAYER_EXTRA 3
#define BLOCK_SQUEEZE 10
#define BLOCK_SPACING -1
#define MARIO_JUMP_DURATION 50
#define CLOCK_ANIMATION_DURATION 150
#define GROUND_HEIGHT 26

void draw_block(GContext *ctx, GRect rect, uint8_t width)
{
    static const uint8_t radius = 1;

    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, rect, radius, GCornersAll);

    rect.origin.x += width;
    rect.origin.y += width;
    rect.size.w -= width*2;
    rect.size.h -= width*2;
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, rect, radius, GCornersAll);

    static const uint8_t dot_offset = 3;
    static const uint8_t dot_width = 6;
    static const uint8_t dot_height = 4;

    GRect dot_rect;

    graphics_context_set_fill_color(ctx, GColorBlack);

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

    block_rect[0] = GRect(layer->bounds.origin.x,
                          layer->bounds.origin.y + BLOCK_LAYER_EXTRA,
                          BLOCK_SIZE,
                          layer->frame.size.h - BLOCK_LAYER_EXTRA);
    block_rect[1] = GRect(layer->bounds.origin.x + BLOCK_SIZE + BLOCK_SPACING,
                          layer->bounds.origin.y + BLOCK_LAYER_EXTRA,
                          BLOCK_SIZE,
                          layer->frame.size.h - BLOCK_LAYER_EXTRA);

    for (uint8_t i = 0; i < 2; ++i) {
        GRect *rect = block_rect + i;

        draw_block(ctx, *rect, 4);
    }
}

void mario_update_callback(Layer *layer, GContext *ctx)
{
    (void)layer;
    (void)ctx;

    BmpContainer *bmp = mario_is_down ? &mario_normal_bmp : &mario_jump_bmp;

    GRect destination = layer_get_frame(&bmp->layer.layer);

    destination.origin.x = 0;
    destination.origin.y = 0;

    graphics_draw_bitmap_in_rect(ctx, &bmp->bmp, destination);
}

void ground_update_callback(Layer *layer, GContext *ctx)
{
    (void)layer;
    (void)ctx;

    GRect image_rect = layer_get_frame(&ground_bmp.layer.layer);
    GRect rect = layer_get_frame(layer);
    int16_t image_width = image_rect.size.w;

    image_rect.origin.x = -10;
    image_rect.origin.y = 0;
    for (int i = 0; i < rect.size.w / image_rect.size.w + 1; ++i) {
        graphics_draw_bitmap_in_rect(ctx, &ground_bmp.bmp, image_rect);
        image_rect.origin.x +=  image_width;
    }
}

void handle_init(AppContextRef ctx)
{
    (void)ctx;

    window_init(&window, "Window Name");
    window_stack_push(&window, true /* Animated */);

    resource_init_current_app(&APP_RESOURCES);

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

    layer_init(&blocks_layer, blocks_down_rect);
    layer_init(&mario_layer, mario_down_rect);
    layer_init(&ground_layer, ground_rect);

    blocks_layer.update_proc = &blocks_update_callback;
    mario_layer.update_proc = &mario_update_callback;
    ground_layer.update_proc = &ground_update_callback;

    layer_add_child(&window.layer, &blocks_layer);
    layer_add_child(&window.layer, &mario_layer);
    layer_add_child(&window.layer, &ground_layer);

    text_layer_init(&text_hour_layer, hour_normal_rect);
    text_layer_set_text_color(&text_hour_layer, GColorBlack);
    text_layer_set_background_color(&text_hour_layer, GColorClear);
    text_layer_set_text_alignment(&text_hour_layer, GTextAlignmentCenter);
    text_layer_set_font(&text_hour_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    layer_add_child(&blocks_layer, &text_hour_layer.layer);

    text_layer_init(&text_minute_layer, GRect(55, 5, 40, 40));
    text_layer_set_text_color(&text_minute_layer, GColorBlack);
    text_layer_set_background_color(&text_minute_layer, GColorClear);
    text_layer_set_text_alignment(&text_minute_layer, GTextAlignmentCenter);
    text_layer_set_font(&text_minute_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    layer_add_child(&blocks_layer, &text_minute_layer.layer);

    bmp_init_container(RESOURCE_ID_IMAGE_MARIO_NORMAL, &mario_normal_bmp);
    bmp_init_container(RESOURCE_ID_IMAGE_MARIO_JUMP, &mario_jump_bmp);
    bmp_init_container(RESOURCE_ID_IMAGE_GROUND, &ground_bmp);
}

void handle_deinit(AppContextRef ctx)
{
    (void)ctx;

    bmp_deinit_container(&mario_normal_bmp);
    bmp_deinit_container(&mario_jump_bmp);
    bmp_deinit_container(&ground_bmp);
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
    layer_mark_dirty(&mario_layer);
}

void mario_jump_animation_started(Animation *animation, void *data)
{
    (void)animation;
    (void)data;
    mario_is_down = 0;
    layer_mark_dirty(&mario_layer);
}

void mario_jump_animation_stopped(Animation *animation, void *data)
{
    (void)animation;
    (void)data;

    text_layer_set_text(&text_hour_layer, hour_text);
    text_layer_set_text(&text_minute_layer, minute_text);

    property_animation_init_layer_frame(&mario_animation_end, &mario_layer,
                                        &mario_up_rect, &mario_down_rect);
    animation_set_duration(&mario_animation_end.animation, MARIO_JUMP_DURATION);
    animation_set_curve(&mario_animation_end.animation, AnimationCurveEaseIn);
    animation_set_handlers(&mario_animation_end.animation, (AnimationHandlers){
        .started = (AnimationStartedHandler)mario_down_animation_started,
        .stopped = (AnimationStoppedHandler)mario_down_animation_stopped
    }, 0);

    animation_schedule(&mario_animation_end.animation);
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

    property_animation_init_layer_frame(&block_animation_end, &blocks_layer,
                                        &blocks_up_rect, &blocks_down_rect);
    animation_set_duration(&block_animation_end.animation, MARIO_JUMP_DURATION);
    animation_set_curve(&block_animation_end.animation, AnimationCurveEaseIn);
    animation_schedule(&block_animation_end.animation);
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

    layer_set_frame(&text_hour_layer.layer, hour_down_rect);
    layer_set_frame(&text_minute_layer.layer, minute_down_rect);

    property_animation_init_layer_frame(&hour_animation_slide_in, &text_hour_layer.layer,
                                        &hour_down_rect, &hour_normal_rect);
    animation_set_duration(&hour_animation_slide_in.animation, CLOCK_ANIMATION_DURATION);
    animation_set_curve(&hour_animation_slide_in.animation, AnimationCurveLinear);
    animation_schedule(&hour_animation_slide_in.animation);

    property_animation_init_layer_frame(&minute_animation_slide_in, &text_minute_layer.layer,
                                        &minute_down_rect, &minute_normal_rect);
    animation_set_duration(&minute_animation_slide_in.animation, CLOCK_ANIMATION_DURATION);
    animation_set_curve(&minute_animation_slide_in.animation, AnimationCurveLinear);
    animation_schedule(&minute_animation_slide_in.animation);
}

void handle_tick(AppContextRef app_ctx, PebbleTickEvent *event)
{
    property_animation_init_layer_frame(&mario_animation_beg, &mario_layer,
                                        &mario_down_rect, &mario_up_rect);
    animation_set_duration(&mario_animation_beg.animation, MARIO_JUMP_DURATION);
    animation_set_curve(&mario_animation_beg.animation, AnimationCurveEaseOut);
    animation_set_handlers(&mario_animation_beg.animation, (AnimationHandlers){
        .started = (AnimationStartedHandler)mario_jump_animation_started,
        .stopped = (AnimationStoppedHandler)mario_jump_animation_stopped
    }, 0);

    property_animation_init_layer_frame(&block_animation_beg, &blocks_layer,
                                        &blocks_down_rect, &blocks_up_rect);
    animation_set_duration(&block_animation_beg.animation, MARIO_JUMP_DURATION);
    animation_set_curve(&block_animation_beg.animation, AnimationCurveEaseOut);
    animation_set_handlers(&block_animation_beg.animation, (AnimationHandlers){
        .started = (AnimationStartedHandler)block_up_animation_started,
        .stopped = (AnimationStoppedHandler)block_up_animation_stopped
    }, 0);

    property_animation_init_layer_frame(&hour_animation_slide_away, &text_hour_layer.layer,
                                        &hour_normal_rect, &hour_up_rect);
    animation_set_duration(&hour_animation_slide_away.animation, CLOCK_ANIMATION_DURATION);
    animation_set_curve(&hour_animation_slide_away.animation, AnimationCurveLinear);
    animation_set_handlers(&hour_animation_slide_away.animation, (AnimationHandlers){
        .started = (AnimationStartedHandler)clock_animation_slide_away_started,
        .stopped = (AnimationStoppedHandler)clock_animation_slide_away_stopped
    }, 0);

    property_animation_init_layer_frame(&minute_animation_slide_away, &text_minute_layer.layer,
                                        &minute_normal_rect, &minute_up_rect);
    animation_set_duration(&minute_animation_slide_away.animation, CLOCK_ANIMATION_DURATION);
    animation_set_curve(&minute_animation_slide_away.animation, AnimationCurveLinear);

    char *hour_format;
    if (clock_is_24h_style()) {
        hour_format = "%H";
    } else {
        hour_format = "%I";
    }
    string_format_time(hour_text, sizeof(hour_text), hour_format, event->tick_time);
    if (!clock_is_24h_style() && (hour_text[0] == '0')) {
        memmove(hour_text, &hour_text[1], sizeof(hour_text) - 1);
    }

    char *minute_format = "%M";
    string_format_time(minute_text, sizeof(minute_text), minute_format, event->tick_time);

    animation_schedule(&mario_animation_beg.animation);
    animation_schedule(&block_animation_beg.animation);
    animation_schedule(&hour_animation_slide_away.animation);
    animation_schedule(&minute_animation_slide_away.animation);
}

void pbl_main(void *params)
{
    PebbleAppHandlers handlers = {
        .init_handler = &handle_init,
        .deinit_handler = &handle_deinit,
        .tick_info = {
            .tick_handler = &handle_tick,
            .tick_units = MINUTE_UNIT
        }
    };
    app_event_loop(params, &handlers);
}
