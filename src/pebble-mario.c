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

GRect blocks_rect;
GRect mario_rect;

BmpContainer block_bmp;
BmpContainer mario_normal_bmp;
BmpContainer mario_jump_bmp;

void blocks_update_callback(Layer *layer, GContext *ctx)
{
    (void)layer;
    (void)ctx;

    GRect destination = layer_get_frame(&block_bmp.layer.layer);

    destination.origin.x = 0;
    destination.origin.y = 0;

    graphics_draw_bitmap_in_rect(ctx, &block_bmp.bmp, destination);

    destination.origin.x = destination.size.w;

    graphics_draw_bitmap_in_rect(ctx, &block_bmp.bmp, destination);
}

void mario_update_callback(Layer *layer, GContext *ctx)
{
    (void)layer;
    (void)ctx;

    GRect destination = layer_get_frame(&mario_normal_bmp.layer.layer);

    destination.origin.x = 0;
    destination.origin.y = 0;

    graphics_draw_bitmap_in_rect(ctx, &mario_normal_bmp.bmp, destination);
}

void handle_init(AppContextRef ctx)
{
    (void)ctx;

    window_init(&window, "Window Name");
    window_stack_push(&window, true /* Animated */);

    resource_init_current_app(&APP_RESOURCES);

    blocks_rect = GRect(22, 10, 100, 50);
    mario_rect = GRect(32, 70, 80, 80);

    layer_init(&blocks_layer, blocks_rect);
    layer_init(&mario_layer, mario_rect);

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

    bmp_init_container(RESOURCE_ID_IMAGE_BLOCK, &block_bmp);
    bmp_init_container(RESOURCE_ID_IMAGE_MARIO_NORMAL, &mario_normal_bmp);
    bmp_init_container(RESOURCE_ID_IMAGE_MARIO_JUMP, &mario_jump_bmp);
}

void handle_deinit(AppContextRef ctx)
{
    (void)ctx;

    bmp_deinit_container(&block_bmp);
    bmp_deinit_container(&mario_normal_bmp);
    bmp_deinit_container(&mario_jump_bmp);
}

void handle_tick(AppContextRef app_ctx, PebbleTickEvent *event)
{
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

    text_layer_set_text(&text_hour_layer, hour_text);
    text_layer_set_text(&text_minute_layer, minute_text);
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
