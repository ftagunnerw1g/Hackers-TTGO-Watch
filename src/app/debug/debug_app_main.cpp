/****************************************************************************
 *   linuxthor 2022 debug tool
 ****************************************************************************/
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the musty tentacles of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "config.h"
#include "HTTPClient.h"

#include <WiFiClientSecure.h>

#include <TTGO.h>
#include <lwip/sockets.h>

#include "debug_app.h"
#include "debug_app_main.h"

#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/mainbar.h"
#include "gui/statusbar.h"
#include "gui/keyboard.h"
#include "gui/widget_factory.h"
#include "gui/widget_styles.h"

#include "utils/json_psram_allocator.h"

lv_obj_t * debug_result_cont = NULL;
lv_obj_t *debug_app_main_tile = NULL;
lv_style_t debug_app_main_style;
lv_style_t debug_app_weather_style;
lv_obj_t *debug_place_textfield = NULL;

lv_task_t * _debug_app_task;

LV_IMG_DECLARE(next_32px);
LV_IMG_DECLARE(exit_32px);
LV_IMG_DECLARE(refresh_32px);
LV_IMG_DECLARE(debug_app_32px);
LV_FONT_DECLARE(Ubuntu_72px);
LV_FONT_DECLARE(weather_48px);

lv_font_t *weather_font = &weather_48px;

static void exit_debug_app_main_event_cb( lv_obj_t * obj, lv_event_t event );
static void debug_textarea_event_cb( lv_obj_t * obj, lv_event_t event );
static void enter_debug_app_next_event_cb( lv_obj_t * obj, lv_event_t event );
void fetch_info(void); 
void debug_app_task( lv_task_t * task );

void debug_app_task(void * pvParameters)
{
    fetch_info();
    vTaskDelay(100); 
    vTaskDelete(NULL); 
}

void weathercode_to_weather(int code)
{
    lv_obj_t * label;

    label = lv_label_create(debug_result_cont, NULL);
    lv_obj_reset_style_list( label, LV_OBJ_PART_MAIN );
    lv_obj_add_style( label, LV_OBJ_PART_MAIN, &debug_app_weather_style );
    switch(code)
    {
      case 0:
        // U+F00D "sunshine" 
        lv_label_set_text(label, "\357\200\215");
        log_i("Clear sky");
        break;
      case 1:
        // U+F00C "sunshine with cloud" 
        lv_label_set_text(label, "\357\200\214");
        log_i("Mostly clear");
        break;
      case 2:
        // U+F031 "mostly a cloud with sun"
        lv_label_set_text(label, "\uf031");
        log_i("Partly cloudy");
        break;
      case 3:
        // U+F013 "two clouds"
        lv_label_set_text(label, "\uf013");
        log_i("Overcast");
        break;
      case 45:
        // U+F021 "fog lines"
        lv_label_set_text(label, WEATHER_FOG_SYMBOL);
        log_i("Fog");
        break;
      case 48:
        // U+F049 "aliens three"
        lv_label_set_text(label, "\uf049");
        log_i("Depositing rime fog");
        break;
      case 51:
        // U+F004 "sunshine cloud rains"
        lv_label_set_text(label, "\uf004");
        log_i("Light drizzle");
        break;
      case 53:
        // U+F007 "sunshine cloud + light rain"
        lv_label_set_text(label, "\uf007");
        log_i("Moderate drizzle");
        break;
      case 55:
        // U+F008 "sunshine cloud heavy rain"
        lv_label_set_text(label, "\uf008");
        log_i("Dense drizzle");
        break;
      case 56:
        // U+F006 "sunshine cloud light freeze"
        lv_label_set_text(label, "\uf006");
        log_i("Light freezing drizzle");
        break;
      case 57:
        // U+F00A "sunshine cloud heavy freeze"
        lv_label_set_text(label, "\uf00a");
        log_i("Heavy freezing drizzle");
        break;
      case 61:
        // U+F01A "cloud and rain"
        lv_label_set_text(label, WEATHER_CLOUD_RAIN_SYMBOL);
        log_i("Slight rain");
        break;
      case 63:
        // U+F018 "cloud rain dots lines"
        lv_label_set_text(label, "\uf018");
        log_i("Medium rain");
        break;
      case 65:
        // U+F019 "cloud and rain lines"
        lv_label_set_text(label, "\uf019");
        log_i("Heavy rain");
        break;
      case 66:
        // U+F034 "cloud freezerain"
        lv_label_set_text(label, "\uf034");
        log_i("Light freezing rain");
        break;
      case 67:
        // U+F038 "cloud sun and frozen"
        lv_label_set_text(label, "\uf038");
        log_i("Heavy freezing rain");
        break;
      case 71:
        // U+F017 "slight" 
        lv_label_set_text(label, "\uf017");
        log_i("Slight snow");
        break;
      case 73:
        // U+F01B "heavy"
        lv_label_set_text(label, "\uf01b");
        log_i("Moderate snow");
        break;
      case 75:
        // U+F01B "heavy"
        lv_label_set_text(label, "\uf01b");
        log_i("Heavy snow");
        break;
      case 77: 
        // U+F01B "heavy"
        lv_label_set_text(label, "\uf01b");
        log_i("Snow grains");
        break; 
      case 80: 
        // U+F01C "cloud and drops"
        lv_label_set_text(label, WEATHER_CLOUD_DROPS_SYMBOL);
        log_i("Slight rain showers");
        break; 
      case 81:
        // U+F018 "cloud and rain"
        lv_label_set_text(label, "\uf018");
        log_i("Moderate rain showers");
        break;
      case 82:
        // U+F036 "sun cloud rain"
        lv_label_set_text(label, "\uf036");
        log_i("Violent rain showers");
        break;
      case 85:
        // U+F017 
        lv_label_set_text(label, "\uf017");
        log_i("Slight snow showers");
        break;
      case 86: 
        // U+F01B
        lv_label_set_text(label, "\uf01b");
        log_i("Heavy snow showers");
        break;
      case 95:
        // U+F00E
        lv_label_set_text(label, "\uf00e");
        log_i("Slight / moderate thunderstorm");
        break;
      case 96:
        // U+F02C
        lv_label_set_text(label, "\uf02c");
        log_i("Thunderstorm slight hail");
        break;
      case 99:
        // U+F02D
        lv_label_set_text(label, "\uf02d");
        log_i("Thunderstorm heavy hail");
        break;
    }
}

void fetch_info( void ) {
    char url[255]="";

    char date0[12];
    char date1[12];
    char date2[12];
    char date3[12];
    char date4[12];
    char date5[12];
    char date6[12];

    char sunr0[18];
    char sunr1[18];
    char sunr2[18];
    char sunr3[18];
    char sunr4[18];
    char sunr5[18];
    char sunr6[18];

    char suns0[18];
    char suns1[18];
    char suns2[18];
    char suns3[18];
    char suns4[18];
    char suns5[18];
    char suns6[18];

    char timezone[8];
    lv_obj_t * label;

    //snprintf( url, sizeof( url ), "https://geocoding-api.open-meteo.com/v1/search?name=%s", lv_textarea_get_text(debug_place_textfield));
    snprintf( url, sizeof( url ), "https://api.open-meteo.com/v1/forecast?latitude=53.80&longitude=-1.55&daily=weathercode,temperature_2m_max,temperature_2m_min,sunrise,sunset&timezone=GMT");
    log_i("request for %s", url);

    WiFiClientSecure *client = new WiFiClientSecure;
    log_i("Client ready");
    HTTPClient http;    

    if(client) 
    {
        client->setInsecure();
        http.begin( *client, url );
        http.useHTTP10(true);
        int httpCode = http.GET();
        log_i("the code is %d", httpCode);

        if(httpCode != 200) 
        {
            log_e("Attempted to request url but http response was %d", httpCode);
            return; 
        }

        SpiRamJsonDocument doc(8192);
  
        DeserializationError error = deserializeJson( doc, http.getStream());
        if (error) {
            log_e("debug deserializeJson() failed: %s", error.c_str() );
            doc.clear();
            http.end();
            client->stop();
            label = lv_label_create(debug_result_cont, NULL);
            lv_label_set_text(label, "JSON error");
            return;
        }

        strncpy(timezone, doc["timezone"], 8); 
        log_i("timezone: %s", timezone);

        strncpy(date0, doc["daily"]["time"][0], 12); 
        strncpy(date1, doc["daily"]["time"][1], 12); 
        strncpy(date2, doc["daily"]["time"][2], 12); 
        strncpy(date3, doc["daily"]["time"][3], 12); 
        strncpy(date4, doc["daily"]["time"][4], 12); 
        strncpy(date5, doc["daily"]["time"][5], 12); 
        strncpy(date6, doc["daily"]["time"][6], 12); 

        strncpy(sunr0, doc["daily"]["sunrise"][0], 18); 
        strncpy(sunr1, doc["daily"]["sunrise"][1], 18); 
        strncpy(sunr2, doc["daily"]["sunrise"][2], 18); 
        strncpy(sunr3, doc["daily"]["sunrise"][3], 18); 
        strncpy(sunr4, doc["daily"]["sunrise"][4], 18); 
        strncpy(sunr5, doc["daily"]["sunrise"][5], 18); 
        strncpy(sunr6, doc["daily"]["sunrise"][6], 18); 

        strncpy(suns0, doc["daily"]["sunset"][0], 18); 
        strncpy(suns1, doc["daily"]["sunset"][1], 18); 
        strncpy(suns2, doc["daily"]["sunset"][2], 18); 
        strncpy(suns3, doc["daily"]["sunset"][3], 18); 
        strncpy(suns4, doc["daily"]["sunset"][4], 18); 
        strncpy(suns5, doc["daily"]["sunset"][5], 18); 
        strncpy(suns6, doc["daily"]["sunset"][6], 18); 

        int wc_man0 = doc["daily"]["weathercode"][0];
        int wc_man1 = doc["daily"]["weathercode"][1];
        int wc_man2 = doc["daily"]["weathercode"][2];
        int wc_man3 = doc["daily"]["weathercode"][3];
        int wc_man4 = doc["daily"]["weathercode"][4];
        int wc_man5 = doc["daily"]["weathercode"][5];
        int wc_man6 = doc["daily"]["weathercode"][6];

        double wc_tmp0 = doc["daily"]["temperature_2m_max"][0]; 
        double wc_tmp1 = doc["daily"]["temperature_2m_max"][1]; 
        double wc_tmp2 = doc["daily"]["temperature_2m_max"][2]; 
        double wc_tmp3 = doc["daily"]["temperature_2m_max"][3]; 
        double wc_tmp4 = doc["daily"]["temperature_2m_max"][4]; 
        double wc_tmp5 = doc["daily"]["temperature_2m_max"][5]; 
        double wc_tmp6 = doc["daily"]["temperature_2m_max"][6]; 

        double wc_low0 = doc["daily"]["temperature_2m_min"][0]; 
        double wc_low1 = doc["daily"]["temperature_2m_min"][1]; 
        double wc_low2 = doc["daily"]["temperature_2m_min"][2]; 
        double wc_low3 = doc["daily"]["temperature_2m_min"][3]; 
        double wc_low4 = doc["daily"]["temperature_2m_min"][4]; 
        double wc_low5 = doc["daily"]["temperature_2m_min"][5]; 
        double wc_low6 = doc["daily"]["temperature_2m_min"][6]; 

        log_i("((%s) High: %.1f / Low: %.1f) Today the weather is....", date0, wc_tmp0, wc_low0);
        log_i("Sunrise: %s / Sunset: %s", sunr0, suns0);
        weathercode_to_weather(wc_man0);
        log_i("Upcoming the weather is....");
        log_i("((%s) High: %.1f / Low: %.1f)", date1, wc_tmp1, wc_low1);
        log_i("Sunrise: %s / Sunset: %s", sunr1, suns1);
        weathercode_to_weather(wc_man1);
        log_i("((%s) High: %.1f / Low: %.1f)", date2, wc_tmp2, wc_low2);
        log_i("Sunrise: %s / Sunset: %s", sunr2, suns2);
        weathercode_to_weather(wc_man2);
        log_i("((%s) High: %.1f / Low: %.1f)", date3, wc_tmp3, wc_low3);
        log_i("Sunrise: %s / Sunset: %s", sunr3, suns3);
        weathercode_to_weather(wc_man3);
        log_i("((%s) High: %.1f / Low: %.1f)", date4, wc_tmp4, wc_low4);
        log_i("Sunrise: %s / Sunset: %s", sunr4, suns4);
        weathercode_to_weather(wc_man4);
        log_i("((%s) High: %.1f / Low: %.1f)", date5, wc_tmp5, wc_low5);
        log_i("Sunrise: %s / Sunset: %s", sunr5, suns5);
        weathercode_to_weather(wc_man5);
        log_i("((%s) High: %.1f / Low: %.1f)", date6, wc_tmp6, wc_low6);
        log_i("Sunrise: %s / Sunset: %s", sunr6, suns6);
        weathercode_to_weather(wc_man6);

        doc.clear();
        http.end();
        client->stop();
    }
    return;
}

void debug_app_main_setup( uint32_t tile_num ) {

    debug_app_main_tile = mainbar_get_tile_obj( tile_num );

    lv_style_copy( &debug_app_main_style, APP_STYLE );

    lv_style_init(&debug_app_weather_style);
    lv_style_copy( &debug_app_weather_style, APP_STYLE);
    lv_style_set_text_font( &debug_app_weather_style, LV_STATE_DEFAULT, weather_font );
    lv_style_set_text_color(&debug_app_weather_style, LV_STATE_DEFAULT, LV_COLOR_RED);

    lv_obj_t * exit_btn = lv_imgbtn_create( debug_app_main_tile, NULL);
    lv_imgbtn_set_src(exit_btn, LV_BTN_STATE_RELEASED, &exit_32px);
    lv_imgbtn_set_src(exit_btn, LV_BTN_STATE_PRESSED, &exit_32px);
    lv_imgbtn_set_src(exit_btn, LV_BTN_STATE_CHECKED_RELEASED, &exit_32px);
    lv_imgbtn_set_src(exit_btn, LV_BTN_STATE_CHECKED_PRESSED, &exit_32px);
    lv_obj_add_style(exit_btn, LV_IMGBTN_PART_MAIN, &debug_app_main_style );
    lv_obj_align(exit_btn, debug_app_main_tile, LV_ALIGN_IN_BOTTOM_LEFT, 10, -10 );
    lv_obj_set_event_cb( exit_btn, exit_debug_app_main_event_cb );

    lv_obj_t * next_btn = lv_imgbtn_create( debug_app_main_tile, NULL);
    lv_imgbtn_set_src(next_btn, LV_BTN_STATE_RELEASED, &debug_app_32px);
    lv_imgbtn_set_src(next_btn, LV_BTN_STATE_PRESSED, &debug_app_32px);
    lv_imgbtn_set_src(next_btn, LV_BTN_STATE_CHECKED_RELEASED, &debug_app_32px);
    lv_imgbtn_set_src(next_btn, LV_BTN_STATE_CHECKED_PRESSED, &debug_app_32px);
    lv_obj_add_style(next_btn, LV_IMGBTN_PART_MAIN, &debug_app_main_style );
    lv_obj_align(next_btn, debug_app_main_tile, LV_ALIGN_IN_BOTTOM_LEFT, (LV_HOR_RES / 2) -15 , -10 );
    lv_obj_set_event_cb( next_btn, enter_debug_app_next_event_cb );

    // text entry 
    lv_obj_t *debug_place_cont = lv_obj_create( debug_app_main_tile, NULL );
    lv_obj_set_size(debug_place_cont, lv_disp_get_hor_res( NULL ) , 40);
    lv_obj_align( debug_place_cont, debug_app_main_tile, LV_ALIGN_IN_TOP_LEFT, 0, 20 );
    lv_obj_t *debug_place_label = lv_label_create( debug_place_cont, NULL);
    lv_label_set_text( debug_place_label, "Place");
    lv_obj_align( debug_place_label, debug_place_cont, LV_ALIGN_IN_LEFT_MID, 5, 0 );
    debug_place_textfield = lv_textarea_create( debug_place_cont, NULL);
    lv_textarea_set_text( debug_place_textfield, "Leeds" );
    lv_textarea_set_pwd_mode( debug_place_textfield, false);
    lv_textarea_set_one_line( debug_place_textfield, true);
    lv_textarea_set_cursor_hidden( debug_place_textfield, true);
    lv_obj_set_width( debug_place_textfield, LV_HOR_RES /4 * 2 );
    lv_obj_align( debug_place_textfield, debug_place_cont, LV_ALIGN_IN_RIGHT_MID, -5, 0 );
    lv_obj_set_event_cb( debug_place_textfield, debug_textarea_event_cb );
}

static void debug_textarea_event_cb( lv_obj_t * obj, lv_event_t event ) {
    if( event == LV_EVENT_CLICKED ) {
        keyboard_set_textarea( obj );
    }
}

static void enter_debug_app_next_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):      if(debug_result_cont != NULL)
                                       {
                                           lv_obj_del( debug_result_cont );      
                                           debug_result_cont = NULL;   
                                       }            
                                       debug_result_cont = lv_cont_create( debug_app_main_tile, NULL);
                                       lv_obj_set_auto_realign(debug_result_cont, true);                    
                                       lv_obj_align_origo(debug_result_cont, NULL, LV_ALIGN_CENTER, 0, 0);  
                                       lv_cont_set_fit(debug_result_cont, LV_FIT_TIGHT);
                                       lv_cont_set_layout(debug_result_cont, LV_LAYOUT_COLUMN_MID);
                                       xTaskCreate     (   debug_app_task,                               /* Function to implement the task */
                                                           "Debug Task",                                 /* Name of the task */
                                                            8192,                                        /* Stack size in words */
                                                            NULL,                                        /* Task input parameter */
                                                            1,                                           /* Priority of the task */
                                                            NULL);

                                       break;
    }
}

static void exit_debug_app_main_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       if(debug_result_cont != NULL)
                                        {
                                            lv_obj_del( debug_result_cont );      
                                            debug_result_cont = NULL;   
                                        }            
                                        mainbar_jump_to_maintile( LV_ANIM_OFF );
                                        break;
    }
}

