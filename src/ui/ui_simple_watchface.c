// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.3.4
// LVGL version: 8.3.6
// Project name: nrf-test

#include "ui.h"

void ui_simple_watchface_screen_init(void)
{
    ui_simple_watchface = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_simple_watchface, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_daymonth = lv_label_create(ui_simple_watchface);
    lv_obj_set_width(ui_daymonth, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_daymonth, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_daymonth, 0);
    lv_obj_set_y(ui_daymonth, lv_pct(-19));
    lv_obj_set_align(ui_daymonth, LV_ALIGN_CENTER);
    lv_label_set_text(ui_daymonth, "DDD MMM DD");
    lv_obj_clear_flag(ui_daymonth, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                      LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM |
                      LV_OBJ_FLAG_SCROLL_CHAIN);     /// Flags
    lv_obj_set_style_text_font(ui_daymonth, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_timehhmmss = lv_label_create(ui_simple_watchface);
    lv_obj_set_width(ui_timehhmmss, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_timehhmmss, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_timehhmmss, 0);
    lv_obj_set_y(ui_timehhmmss, lv_pct(-10));
    lv_obj_set_align(ui_timehhmmss, LV_ALIGN_CENTER);
    lv_label_set_text(ui_timehhmmss, "HH:MM:SS");
    lv_obj_clear_flag(ui_timehhmmss, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                      LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM |
                      LV_OBJ_FLAG_SCROLL_CHAIN);     /// Flags
    lv_obj_set_style_text_font(ui_timehhmmss, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_year = lv_label_create(ui_simple_watchface);
    lv_obj_set_width(ui_year, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_year, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_year, 0);
    lv_obj_set_y(ui_year, lv_pct(-1));
    lv_obj_set_align(ui_year, LV_ALIGN_CENTER);
    lv_label_set_text(ui_year, "YYYY");
    lv_obj_clear_flag(ui_year, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                      LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM |
                      LV_OBJ_FLAG_SCROLL_CHAIN);     /// Flags
    lv_obj_set_style_text_font(ui_year, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_bluetooth = lv_label_create(ui_simple_watchface);
    lv_obj_set_width(ui_bluetooth, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_bluetooth, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_bluetooth, 0);
    lv_obj_set_y(ui_bluetooth, 24);
    lv_obj_set_align(ui_bluetooth, LV_ALIGN_TOP_MID);
    lv_label_set_text(ui_bluetooth, "");
    lv_obj_add_flag(ui_bluetooth, LV_OBJ_FLAG_HIDDEN);     /// Flags
    lv_obj_set_style_text_color(ui_bluetooth, lv_color_hex(0x0F82FA), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_bluetooth, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_bluetooth, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_brightness_slider = lv_slider_create(ui_simple_watchface);
    lv_slider_set_range(ui_brightness_slider, 1, 17);
    lv_slider_set_value(ui_brightness_slider, 17, LV_ANIM_OFF);
    if(lv_slider_get_mode(ui_brightness_slider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ui_brightness_slider, 0,
                                                                                                      LV_ANIM_OFF);
    lv_obj_set_width(ui_brightness_slider, 150);
    lv_obj_set_height(ui_brightness_slider, 10);
    lv_obj_set_x(ui_brightness_slider, 0);
    lv_obj_set_y(ui_brightness_slider, -70);
    lv_obj_set_align(ui_brightness_slider, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_style_bg_color(ui_brightness_slider, lv_color_hex(0x093A6E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_brightness_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_brightness_slider, lv_color_hex(0x0F82FA), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_brightness_slider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_brightness_slider, lv_color_hex(0x0F82FA), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_brightness_slider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_brightness_slider, ui_event_brightness_slider, LV_EVENT_ALL, NULL);

}