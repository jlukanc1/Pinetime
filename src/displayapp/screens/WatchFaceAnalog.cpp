#include <libs/lvgl/lvgl.h>
#include "WatchFaceAnalog.h"
#include "BatteryIcon.h"
#include "BleIcon.h"
#include "Symbols.h"
#include "NotificationIcon.h"

#include <cmath>

//LV_IMG_DECLARE(bg_clock);

using namespace Pinetime::Applications::Screens;

#define HOUR_LENGTH   70
#define MINUTE_LENGTH 90
#define SECOND_LENGTH 110
#define PI            3.14159265358979323846

// ##
static int16_t coordinate_x_relocate(int16_t x) {
  return ((x) + LV_HOR_RES / 2);
}

// ##
static int16_t coordinate_y_relocate(int16_t y) {
  return (((y) -LV_HOR_RES / 2) < 0) ? (0 - ((y) -LV_HOR_RES / 2)) : ((y) -LV_HOR_RES / 2);
}

WatchFaceAnalog::WatchFaceAnalog(Pinetime::Applications::DisplayApp* app,
                                 Controllers::DateTime& dateTimeController,
                                 Controllers::Battery& batteryController,
                                 Controllers::Ble& bleController,
                                 Controllers::NotificationManager& notificatioManager,
                                 Controllers::Settings& settingsController)
  : Screen(app),
    currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificatioManager {notificatioManager},
    settingsController {settingsController} {
  settingsController.SetClockFace(1);

  sHour = 99;
  sMinute = 99;
  sSecond = 99;
/*
  lv_obj_t* bg_clock_img = lv_img_create(lv_scr_act(), NULL);
  lv_img_set_src(bg_clock_img, &bg_clock);
  lv_obj_align(bg_clock_img, NULL, LV_ALIGN_CENTER, 0, 0);
*/
  //create bars at 3,6,9, and 2 bars at 12
  three_bar = lv_line_create(lv_scr_act(), nullptr);
  six_bar = lv_line_create(lv_scr_act(), nullptr);
  nine_bar = lv_line_create(lv_scr_act(), nullptr);
  twelve_bar_1 = lv_line_create(lv_scr_act(), nullptr);
  twelve_bar_2 = lv_line_create(lv_scr_act(), nullptr);
  //create dots for other hours
  one_dot = lv_led_create(lv_scr_act(), nullptr);
  two_dot = lv_led_create(lv_scr_act(), nullptr);
  four_dot = lv_led_create(lv_scr_act(), nullptr);
  five_dot = lv_led_create(lv_scr_act(), nullptr);
  seven_dot = lv_led_create(lv_scr_act(), nullptr);
  eight_dot = lv_led_create(lv_scr_act(), nullptr);
  ten_dot = lv_led_create(lv_scr_act(), nullptr);
  eleven_dot = lv_led_create(lv_scr_act(), nullptr);

  lv_style_init(&dots_style);
  //lv_style_set_size(&dots_style, LV_STATE_DEFAULT, 8);
  lv_style_set_radius(&dots_style, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
  lv_style_set_border_color(&dots_style, LV_STATE_DEFAULT, lv_color_hex(0x18FFFF));

  lv_obj_add_style(one_dot, LV_LINE_PART_MAIN, &dots_style);
  lv_obj_add_style(two_dot, LV_LINE_PART_MAIN, &dots_style);
  lv_obj_add_style(four_dot, LV_LINE_PART_MAIN, &dots_style);
  lv_obj_add_style(five_dot, LV_LINE_PART_MAIN, &dots_style);
  lv_obj_add_style(seven_dot, LV_LINE_PART_MAIN, &dots_style);
  lv_obj_add_style(eight_dot, LV_LINE_PART_MAIN, &dots_style);
  lv_obj_add_style(ten_dot, LV_LINE_PART_MAIN, &dots_style);
  lv_obj_add_style(eleven_dot, LV_LINE_PART_MAIN, &dots_style);

  lv_obj_align(one_dot, NULL, LV_ALIGN_CENTER, 0, 0);
  //lv_obj_align(one_dot, NULL, LV_ALIGN_CENTER, 120 * sin(1 * 30 * PI / 180), -120 * cos(1 * 30 * PI / 180));
  lv_obj_align(two_dot, NULL, LV_ALIGN_CENTER, 120 * sin(2 * 30 * PI / 180), -120 * cos(2 * 30 * PI / 180));
  lv_obj_align(four_dot, NULL, LV_ALIGN_CENTER, 120 * sin(4 * 30 * PI / 180), -120 * cos(4 * 30 * PI / 180));
  lv_obj_align(five_dot, NULL, LV_ALIGN_CENTER, 120 * sin(5 * 30 * PI / 180), -120 * cos(5 * 30 * PI / 180));
  lv_obj_align(seven_dot, NULL, LV_ALIGN_CENTER, 120 * sin(7 * 30 * PI / 180), -120 * cos(7 * 30 * PI / 180));
  lv_obj_align(eight_dot, NULL, LV_ALIGN_CENTER, 120 * sin(8 * 30 * PI / 180), -120 * cos(8 * 30 * PI / 180));
  lv_obj_align(ten_dot, NULL, LV_ALIGN_CENTER, 120 * sin(10 * 30 * PI / 180), -120 * cos(10 * 30 * PI / 180));
  lv_obj_align(eleven_dot, NULL, LV_ALIGN_CENTER, 120 * sin(11 * 30 * PI / 180), -120 * cos(11 * 30 * PI / 180));
  lv_led_on(one_dot);

  batteryIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(batteryIcon, Symbols::batteryHalf);
  lv_obj_align(batteryIcon, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -8, -4);

  notificationIcon = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_label_set_text(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 8, -4);

  // Date - Day / Week day

  label_date_day = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_style_local_text_color(label_date_day, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xf0a500));
  lv_label_set_text_fmt(label_date_day, "%s\n%02i", dateTimeController.DayOfWeekShortToString(), dateTimeController.Day());
  lv_label_set_align(label_date_day, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_date_day, NULL, LV_ALIGN_CENTER, 50, 0);

  minute_body = lv_line_create(lv_scr_act(), NULL);
  minute_body_trace = lv_line_create(lv_scr_act(), NULL);
  hour_body = lv_line_create(lv_scr_act(), NULL);
  hour_body_trace = lv_line_create(lv_scr_act(), NULL);
  second_body = lv_line_create(lv_scr_act(), NULL);

  lv_style_init(&second_line_style);
  lv_style_set_line_width(&second_line_style, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&second_line_style, LV_STATE_DEFAULT, LV_COLOR_RED);
  lv_style_set_line_rounded(&second_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(second_body, LV_LINE_PART_MAIN, &second_line_style);

  lv_style_init(&minute_line_style);
  lv_style_set_line_width(&minute_line_style, LV_STATE_DEFAULT, 7);
  lv_style_set_line_color(&minute_line_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&minute_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(minute_body, LV_LINE_PART_MAIN, &minute_line_style);

  lv_style_init(&minute_line_style_trace);
  lv_style_set_line_width(&minute_line_style_trace, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&minute_line_style_trace, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&minute_line_style_trace, LV_STATE_DEFAULT, false);
  lv_obj_add_style(minute_body_trace, LV_LINE_PART_MAIN, &minute_line_style_trace);

  lv_style_init(&hour_line_style);
  lv_style_set_line_width(&hour_line_style, LV_STATE_DEFAULT, 7);
  lv_style_set_line_color(&hour_line_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&hour_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(hour_body, LV_LINE_PART_MAIN, &hour_line_style);

  lv_style_init(&hour_line_style_trace);
  lv_style_set_line_width(&hour_line_style_trace, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&hour_line_style_trace, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&hour_line_style_trace, LV_STATE_DEFAULT, false);
  lv_obj_add_style(hour_body_trace, LV_LINE_PART_MAIN, &hour_line_style_trace);

  UpdateClock();
}

WatchFaceAnalog::~WatchFaceAnalog() {

  lv_style_reset(&hour_line_style);
  lv_style_reset(&hour_line_style_trace);
  lv_style_reset(&minute_line_style);
  lv_style_reset(&minute_line_style_trace);
  lv_style_reset(&second_line_style);

  lv_obj_clean(lv_scr_act());
}

void WatchFaceAnalog::UpdateClock() {

  hour = dateTimeController.Hours();
  minute = dateTimeController.Minutes();
  second = dateTimeController.Seconds();

  if (sMinute != minute) {
    minute_point[0].x = coordinate_x_relocate(30 * sin(minute * 6 * PI / 180));
    minute_point[0].y = coordinate_y_relocate(30 * cos(minute * 6 * PI / 180));
    minute_point[1].x = coordinate_x_relocate(MINUTE_LENGTH * sin(minute * 6 * PI / 180));
    minute_point[1].y = coordinate_y_relocate(MINUTE_LENGTH * cos(minute * 6 * PI / 180));

    minute_point_trace[0].x = coordinate_x_relocate(5 * sin(minute * 6 * PI / 180));
    minute_point_trace[0].y = coordinate_y_relocate(5 * cos(minute * 6 * PI / 180));
    minute_point_trace[1].x = coordinate_x_relocate(31 * sin(minute * 6 * PI / 180));
    minute_point_trace[1].y = coordinate_y_relocate(31 * cos(minute * 6 * PI / 180));

    lv_line_set_points(minute_body, minute_point, 2);
    lv_line_set_points(minute_body_trace, minute_point_trace, 2);
  }

  if (sHour != hour || sMinute != minute) {
    sHour = hour;
    sMinute = minute;
    hour_point[0].x = coordinate_x_relocate(30 * sin((((hour > 12 ? hour - 12 : hour) * 30) + (minute * 0.5)) * PI / 180));
    hour_point[0].y = coordinate_y_relocate(30 * cos((((hour > 12 ? hour - 12 : hour) * 30) + (minute * 0.5)) * PI / 180));
    hour_point[1].x = coordinate_x_relocate(HOUR_LENGTH * sin((((hour > 12 ? hour - 12 : hour) * 30) + (minute * 0.5)) * PI / 180));
    hour_point[1].y = coordinate_y_relocate(HOUR_LENGTH * cos((((hour > 12 ? hour - 12 : hour) * 30) + (minute * 0.5)) * PI / 180));

    hour_point_trace[0].x = coordinate_x_relocate(5 * sin((((hour > 12 ? hour - 12 : hour) * 30) + (minute * 0.5)) * PI / 180));
    hour_point_trace[0].y = coordinate_y_relocate(5 * cos((((hour > 12 ? hour - 12 : hour) * 30) + (minute * 0.5)) * PI / 180));
    hour_point_trace[1].x = coordinate_x_relocate(31 * sin((((hour > 12 ? hour - 12 : hour) * 30) + (minute * 0.5)) * PI / 180));
    hour_point_trace[1].y = coordinate_y_relocate(31 * cos((((hour > 12 ? hour - 12 : hour) * 30) + (minute * 0.5)) * PI / 180));

    lv_line_set_points(hour_body, hour_point, 2);
    lv_line_set_points(hour_body_trace, hour_point_trace, 2);
  }

  if (sSecond != second) {
    sSecond = second;
    second_point[0].x = coordinate_x_relocate(20 * sin((180 + second * 6) * PI / 180));
    second_point[0].y = coordinate_y_relocate(20 * cos((180 + second * 6) * PI / 180));
    second_point[1].x = coordinate_x_relocate(SECOND_LENGTH * sin(second * 6 * PI / 180));
    second_point[1].y = coordinate_y_relocate(SECOND_LENGTH * cos(second * 6 * PI / 180));
    lv_line_set_points(second_body, second_point, 2);
  }
}

bool WatchFaceAnalog::Refresh() {

  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated()) {
    auto batteryPercent = batteryPercentRemaining.Get();
    lv_label_set_text(batteryIcon, BatteryIcon::GetBatteryIcon(batteryPercent));
  }

  notificationState = notificatioManager.AreNewNotificationsAvailable();

  if (notificationState.IsUpdated()) {
    if (notificationState.Get() == true)
      lv_label_set_text(notificationIcon, NotificationIcon::GetIcon(true));
    else
      lv_label_set_text(notificationIcon, NotificationIcon::GetIcon(false));
  }

  currentDateTime = dateTimeController.CurrentDateTime();

  if (currentDateTime.IsUpdated()) {

    month = dateTimeController.Month();
    day = dateTimeController.Day();
    dayOfWeek = dateTimeController.DayOfWeek();

    UpdateClock();

    if ((month != currentMonth) || (dayOfWeek != currentDayOfWeek) || (day != currentDay)) {

      lv_label_set_text_fmt(label_date_day, "%s\n%02i", dateTimeController.DayOfWeekShortToString(), day);

      currentMonth = month;
      currentDayOfWeek = dayOfWeek;
      currentDay = day;
    }
  }

  return true;
}