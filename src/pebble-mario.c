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

static char hour_text[]   = "00";
static char minute_text[] = "00";

GRect blocks_down_rect;
GRect blocks_up_rect;
GRect mario_down_rect;
GRect mario_up_rect;

static int mario_is_down = 1;

BmpContainer mario_normal_bmp;
BmpContainer mario_jump_bmp;

PropertyAnimation mario_animation_beg;
PropertyAnimation mario_animation_end;

PropertyAnimation block_animation_beg;
PropertyAnimation block_animation_end;

#define BLOCK_SIZE 50
#define MARIO_JUMP_DURATION 50

void draw_block(GContext *ctx, GRect rect, uint8_t width)
{
    GPoint p1;
    GPoint p2;

    int16_t x = rect.origin.x;
    int16_t y = rect.origin.y;
    int16_t w = rect.size.w;
    int16_t h = rect.size.h;

    for (; width != 0; --width) {
        // top
        p1 = GPoint(x, y);
        p2 = GPoint(x + w, y);
        graphics_draw_line(ctx, p1, p2);

        // left
        p1 = GPoint(x, y);
        p2 = GPoint(x, y + h);
        graphics_draw_line(ctx, p1, p2);

        // right
        p1 = GPoint(x + w, y);
        p2 = GPoint(x + w, y + h);
        graphics_draw_line(ctx, p1, p2);

        // bottom
        p1 = GPoint(x, y + h);
        p2 = GPoint(x + w, y + h);
        graphics_draw_line(ctx, p1, p2);

        x += 1;
        y += 1;
        w -= 2;
        h -= 2;
    }
}

void blocks_update_callback(Layer *layer, GContext *ctx)
{
    (void)layer;
    (void)ctx;

    GRect rect;

    rect = GRect(layer->bounds.origin.x, layer->bounds.origin.y,
                 BLOCK_SIZE - 1, layer->frame.size.h - 1);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    draw_block(ctx, rect, 5);

    rect = GRect(layer->bounds.origin.x + BLOCK_SIZE, layer->bounds.origin.y,
                 BLOCK_SIZE - 1, layer->frame.size.h - 1);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    draw_block(ctx, rect, 5);
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

void handle_init(AppContextRef ctx)
{
    (void)ctx;

    window_init(&window, "Window Name");
    window_stack_push(&window, true /* Animated */);

    resource_init_current_app(&APP_RESOURCES);

    blocks_down_rect = GRect(22, 7, BLOCK_SIZE*2, BLOCK_SIZE);
    blocks_up_rect = GRect(22, 2, BLOCK_SIZE*2, BLOCK_SIZE-10);
    mario_down_rect = GRect(32, 70, 80, 80);
    mario_up_rect = GRect(32, 42, 80, 80);

    layer_init(&blocks_layer, blocks_down_rect);
    layer_init(&mario_layer, mario_down_rect);

    blocks_layer.update_proc = &blocks_update_callback;
    mario_layer.update_proc = &mario_update_callback;

    layer_add_child(&window.layer, &blocks_layer);
    layer_add_child(&window.layer, &mario_layer);

    text_layer_init(&text_hour_layer, GRect(5, 5, 40, 40));
    text_layer_set_text_color(&text_hour_layer, GColorBlack);
    text_layer_set_background_color(&text_hour_layer, GColorClear);
    text_layer_set_text_alignment(&text_hour_layer, GTextAlignmentCenter);
    text_layer_set_font(&text_hour_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    // text_layer_set_font(&text_hour_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT)));
    layer_add_child(&blocks_layer, &text_hour_layer.layer);

    text_layer_init(&text_minute_layer, GRect(55, 5, 40, 40));
    text_layer_set_text_color(&text_minute_layer, GColorBlack);
    text_layer_set_background_color(&text_minute_layer, GColorClear);
    text_layer_set_text_alignment(&text_minute_layer, GTextAlignmentCenter);
    text_layer_set_font(&text_minute_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    // text_layer_set_font(&text_minute_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT)));
    layer_add_child(&blocks_layer, &text_minute_layer.layer);

    bmp_init_container(RESOURCE_ID_IMAGE_MARIO_NORMAL, &mario_normal_bmp);
    bmp_init_container(RESOURCE_ID_IMAGE_MARIO_JUMP, &mario_jump_bmp);
}

void handle_deinit(AppContextRef ctx)
{
    (void)ctx;

    bmp_deinit_container(&mario_normal_bmp);
    bmp_deinit_container(&mario_jump_bmp);
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

    if (!clock_is_24h_style() && (minute_text[0] == '0')) {
        memmove(minute_text, &minute_text[1], sizeof(minute_text) - 1);
    }

    animation_schedule(&mario_animation_beg.animation);
    animation_schedule(&block_animation_beg.animation);
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
