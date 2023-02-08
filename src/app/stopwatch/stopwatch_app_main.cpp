/****************************************************************************
 *   Aug 21 17:26:00 2020
 *   Copyright  2020  Chris McNamee
 *   Email: chris.mcna@gmail.com
 ****************************************************************************/
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "config.h"

#include "stopwatch_app.h"
#include "stopwatch_app_main.h"

#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/mainbar.h"
#include "gui/statusbar.h"
#include "gui/widget_factory.h"
#include "gui/widget_styles.h"

#ifdef NATIVE_64BIT
    #include <time.h>
    #include "utils/logging.h"
    #include "utils/millis.h"
#else
    #include <time.h>
    #include <sys/time.h>
    #include <Arduino.h>
#endif

lv_obj_t *stopwatch_app_main_tile = NULL;
lv_obj_t *stopwatch_app_main_stopwatchlabel = NULL;
lv_obj_t *stopwatch_app_mill_stopwatchlabel = NULL;
lv_obj_t *stopwatch_app_main_start_btn = NULL;
lv_obj_t *stopwatch_app_main_stop_btn = NULL;
lv_obj_t *stopwatch_app_main_reset_btn = NULL;

lv_style_t stopwatch_app_main_stopwatchstyle;
lv_style_t stopwatch_app_mill_stopwatchstyle;

lv_task_t * _stopwatch_app_task;

int sw_ms   = 0;
int sw_mins = 0;
int sw_secs = 0; 

struct timespec ts_then;
struct timespec ts_now;

int sw_unixstart = 0; 
int sw_unixnow   = 0; 

LV_FONT_DECLARE(LCD_48px);
LV_FONT_DECLARE(LCD_32px);

bool stopwatch_button_event_cb( EventBits_t event, void *arg );
bool stopwatch_style_change_event_cb( EventBits_t event, void *arg );
static void exit_stopwatch_app_main_event_cb( lv_obj_t * obj, lv_event_t event );
static void start_stopwatch_app_main_event_cb( lv_obj_t * obj, lv_event_t event );
static void stop_stopwatch_app_main_event_cb( lv_obj_t * obj, lv_event_t event );
static void reset_stopwatch_app_main_event_cb( lv_obj_t * obj, lv_event_t event );

void stopwatch_app_task( lv_task_t * task );

void stopwatch_app_main_setup( uint32_t tile_num ) {

    stopwatch_app_main_tile = mainbar_get_tile_obj( tile_num );

    lv_style_copy( &stopwatch_app_main_stopwatchstyle, APP_STYLE );
    lv_style_set_text_font( &stopwatch_app_main_stopwatchstyle, LV_STATE_DEFAULT, &LCD_48px);

    lv_style_copy( &stopwatch_app_mill_stopwatchstyle, APP_STYLE );
    lv_style_set_text_font( &stopwatch_app_mill_stopwatchstyle, LV_STATE_DEFAULT, &LCD_32px);

    lv_obj_t * stopwatch_cont = mainbar_obj_create( stopwatch_app_main_tile );
    lv_obj_set_size( stopwatch_cont, LV_HOR_RES , LV_VER_RES / 2 );
    lv_obj_add_style( stopwatch_cont, LV_OBJ_PART_MAIN, APP_STYLE );
    lv_obj_align( stopwatch_cont, stopwatch_app_main_tile, LV_ALIGN_CENTER, 0, 0 );

    stopwatch_app_main_stopwatchlabel = lv_label_create( stopwatch_cont , NULL);
    lv_label_set_text(stopwatch_app_main_stopwatchlabel, "00\'00\'");
    lv_obj_reset_style_list( stopwatch_app_main_stopwatchlabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( stopwatch_app_main_stopwatchlabel, LV_OBJ_PART_MAIN, &stopwatch_app_main_stopwatchstyle );
    lv_obj_align(stopwatch_app_main_stopwatchlabel, NULL, LV_ALIGN_IN_LEFT_MID, 15, 0);

    stopwatch_app_mill_stopwatchlabel = lv_label_create( stopwatch_cont , NULL);
    lv_label_set_text(stopwatch_app_mill_stopwatchlabel, "000");
    lv_obj_reset_style_list( stopwatch_app_mill_stopwatchlabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( stopwatch_app_mill_stopwatchlabel, LV_OBJ_PART_MAIN, &stopwatch_app_mill_stopwatchstyle );
    lv_obj_align(stopwatch_app_mill_stopwatchlabel, NULL, LV_ALIGN_IN_RIGHT_MID, -20, 0);

    stopwatch_app_main_start_btn = lv_btn_create(stopwatch_app_main_tile, NULL);  
    lv_obj_set_size(stopwatch_app_main_start_btn, 50, 50);
    lv_obj_add_style(stopwatch_app_main_start_btn, LV_IMGBTN_PART_MAIN, APP_STYLE );
    lv_obj_align(stopwatch_app_main_start_btn, stopwatch_app_main_tile, LV_ALIGN_IN_BOTTOM_MID, 0, 0 );
    lv_obj_set_event_cb( stopwatch_app_main_start_btn, start_stopwatch_app_main_event_cb );

    lv_obj_t *stopwatch_app_main_start_btn_label = lv_label_create(stopwatch_app_main_start_btn, NULL);
    lv_label_set_text(stopwatch_app_main_start_btn_label, LV_SYMBOL_PLAY);

    stopwatch_app_main_stop_btn = lv_btn_create(stopwatch_app_main_tile, NULL);  
    lv_obj_set_size(stopwatch_app_main_stop_btn, 50, 50);
    lv_obj_add_style(stopwatch_app_main_stop_btn, LV_IMGBTN_PART_MAIN, APP_STYLE );
    lv_obj_align(stopwatch_app_main_stop_btn, stopwatch_app_main_tile, LV_ALIGN_IN_BOTTOM_MID, 0, 0 );
    lv_obj_set_event_cb( stopwatch_app_main_stop_btn, stop_stopwatch_app_main_event_cb );
    lv_obj_set_hidden(stopwatch_app_main_stop_btn, true);

    lv_obj_t *stopwatch_app_main_stop_btn_label = lv_label_create(stopwatch_app_main_stop_btn, NULL);
    lv_label_set_text(stopwatch_app_main_stop_btn_label, LV_SYMBOL_STOP);

    stopwatch_app_main_reset_btn = lv_btn_create(stopwatch_app_main_tile, NULL);  
    lv_obj_set_size(stopwatch_app_main_reset_btn, 50, 50);
    lv_obj_add_style(stopwatch_app_main_reset_btn, LV_IMGBTN_PART_MAIN, APP_STYLE );
    lv_obj_align(stopwatch_app_main_reset_btn, stopwatch_app_main_tile, LV_ALIGN_IN_BOTTOM_RIGHT,  -20, 0 );
    lv_obj_set_event_cb( stopwatch_app_main_reset_btn, reset_stopwatch_app_main_event_cb );

    lv_obj_t *stopwatch_app_main_reset_btn_label = lv_label_create(stopwatch_app_main_reset_btn, NULL);
    lv_label_set_text(stopwatch_app_main_reset_btn_label, LV_SYMBOL_EJECT);

    lv_obj_t * exit_btn = wf_add_exit_button( stopwatch_app_main_tile, exit_stopwatch_app_main_event_cb );
    lv_obj_align(exit_btn, stopwatch_app_main_tile, LV_ALIGN_IN_BOTTOM_LEFT, 10, -10 );

    styles_register_cb( STYLE_CHANGE, stopwatch_style_change_event_cb, "stopwatch style change" );
    mainbar_add_tile_button_cb( tile_num, stopwatch_button_event_cb );
}

bool stopwatch_button_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case BUTTON_EXIT:   mainbar_jump_back();
                            break;
    }
    return( true );
}

bool stopwatch_style_change_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case STYLE_CHANGE:      lv_style_copy( &stopwatch_app_main_stopwatchstyle, APP_STYLE );
                                lv_style_set_text_font( &stopwatch_app_main_stopwatchstyle, LV_STATE_DEFAULT, &LCD_48px);
                                break;
    }
    return( true );
}

static void stopwatch_app_main_update_stopwatchlabel()
{
    struct timeval now;
    gettimeofday(&now, NULL);

    sw_ms = (now.tv_usec / 1000);

    if(sw_ms < 500)
    { 
       sw_unixnow = time(0); 
       sw_secs += (sw_unixnow - sw_unixstart);
       // now we become the new start time
       sw_unixstart = time(0);  
    }

    if(sw_secs > 59)
    {
        sw_mins += 1; 
        sw_secs = 0; 
    }

    lv_label_set_text_fmt(stopwatch_app_main_stopwatchlabel, "%02d'%02d'", sw_mins, sw_secs);
    lv_label_set_text_fmt(stopwatch_app_mill_stopwatchlabel, "%03d", sw_ms);
    main_tile_update_stopwatch(sw_mins, sw_secs, sw_ms);
}



static void start_stopwatch_app_main_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       // create an task that runs every secound
                                        sw_unixstart = time(0);
                                        _stopwatch_app_task = lv_task_create( stopwatch_app_task, 10, LV_TASK_PRIO_MID, NULL );
                                        lv_obj_set_hidden(stopwatch_app_main_start_btn, true);
                                        lv_obj_set_hidden(stopwatch_app_main_stop_btn, false);
                                        main_tile_show_stopwatch();
                                        stopwatch_add_widget();
                                        stopwatch_app_hide_app_icon_info( false );
                                        break;
    }
}

static void stop_stopwatch_app_main_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       // create an task that runs every secound
                                        lv_task_del(_stopwatch_app_task);
                                        lv_obj_set_hidden(stopwatch_app_main_start_btn, false);
                                        lv_obj_set_hidden(stopwatch_app_main_stop_btn, true);
                                        stopwatch_remove_widget();
                                        stopwatch_app_hide_app_icon_info( true );
                                        break;
    }
}

static void reset_stopwatch_app_main_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       sw_ms = 0; 
                                        sw_mins = 0; 
                                        sw_secs = 0; 
                                        lv_label_set_text_fmt(stopwatch_app_main_stopwatchlabel, "%02d\'%02d\'", sw_mins, sw_secs);
                                        lv_label_set_text_fmt(stopwatch_app_mill_stopwatchlabel, "%03d", sw_ms);
                                        main_tile_hide_stopwatch();
                                        break;
    }
}


static void exit_stopwatch_app_main_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mainbar_jump_back();
                                        break;
    }
}

void stopwatch_app_task( lv_task_t * task ) {
    stopwatch_app_main_update_stopwatchlabel();
}
