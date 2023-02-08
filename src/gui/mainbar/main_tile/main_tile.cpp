/****************************************************************************
 *   Tu May 22 21:23:51 2020
 *   Copyright  2020  Dirk Brosswick
 *   Email: dirk.brosswick@googlemail.com
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
#include <stdio.h>
#include <time.h>

#include "config.h"

#include "gui/mainbar/mainbar.h"
#include "gui/widget_styles.h"

#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/note_tile/note_tile.h"
#include "gui/mainbar/setup_tile/setup_tile.h"
#include "gui/widget_styles.h"

#include "hardware/timesync.h"
#include "hardware/powermgm.h"
#include "hardware/pmu.h"
#include "hardware/sensor.h"

#include "utils/alloc.h"

#ifdef NATIVE_64BIT
    #include "utils/logging.h"
#else
    #include <Arduino.h>
#endif

static bool maintile_init = false;

static lv_obj_t *main_cont = NULL;
static lv_obj_t *clock_cont = NULL;
static lv_obj_t *timelabel = NULL;
static lv_obj_t *shadow_timelabel = NULL;
static lv_obj_t *secslabel = NULL;
static lv_obj_t *stopwlabel = NULL;
static lv_obj_t *unixlabel = NULL;
static lv_obj_t *infolabel = NULL;
static lv_obj_t *templabel = NULL;
static lv_obj_t *datelabel = NULL;

uint32_t main_tile_num;

static lv_style_t *style;
static lv_style_t timestyle;
static lv_style_t stopwstyle;
static lv_style_t shadow_timestyle;
static lv_style_t secsstyle;
static lv_style_t datestyle;
static lv_style_t infostyle;
static lv_style_t tempstyle;
static lv_style_t iconstyle;

icon_t widget_entry[ MAX_WIDGET_NUM ];

LV_FONT_DECLARE(LCD_144px);
LV_FONT_DECLARE(LCD_72px);
LV_FONT_DECLARE(LCD_48px);
LV_FONT_DECLARE(LCD_32px);
LV_FONT_DECLARE(LCD_16px);

#if defined( M5PAPER )
    lv_font_t *time_font = &LCD_144px;
    lv_font_t *secs_font = &LCD_48px;
    lv_font_t *stop_font = &LCD_32px;
    lv_font_t *date_font = &LCD_48px;
    lv_font_t *info_font = &LCD_32px;
    lv_font_t *temp_font = &LCD_32px;
    lv_font_t *icon_font = &LCD_16px;
#else
    lv_font_t *time_font = &LCD_72px;
    lv_font_t *secs_font = &LCD_32px;
    lv_font_t *stop_font = &LCD_32px;
    lv_font_t *date_font = &LCD_16px;
    lv_font_t *info_font = &LCD_16px;    
    lv_font_t *temp_font = &LCD_16px;    
    lv_font_t *icon_font = &LCD_16px;
#endif

lv_task_t * main_tile_task;

void main_tile_update_task( lv_task_t * task );
void main_tile_align_widgets( void );
void main_tile_show_stopwatch( void );
void main_tile_hide_stopwatch( void );
void main_tile_update_stopwatch( int stop_min, int stop_sec, int stop_ms );
bool main_tile_powermgm_event_cb( EventBits_t event, void *arg );
bool main_tile_time_update_ebent_cb( EventBits_t event, void *arg );
bool main_tile_style_event_cb( EventBits_t event, void *arg );
static bool main_tile_button_event_cb( EventBits_t event, void *arg );
static bool main_tile_sensor_event_cb( EventBits_t event, void *arg );

void main_tile_setup( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( maintile_init ) {
        log_e("maintile already initialized");
        return;
    }

    main_tile_num = mainbar_add_tile( 0, 0, "main tile", ws_get_mainbar_style() );
    main_cont = mainbar_get_tile_obj( main_tile_num );
    style = ws_get_mainbar_style();

    lv_style_copy( &timestyle, style);
    lv_style_set_text_font( &timestyle, LV_STATE_DEFAULT, time_font );

    lv_style_copy( &shadow_timestyle, style);
    lv_style_set_text_opa(&shadow_timestyle, LV_STATE_DEFAULT, LV_OPA_90);
#if defined (LILYGO_WATCH_2020_V1)
    lv_style_set_text_color(&shadow_timestyle, LV_STATE_DEFAULT, LV_COLOR_GREEN);
#elif defined (LILYGO_WATCH_2020_V2)
    lv_style_set_text_color(&shadow_timestyle, LV_STATE_DEFAULT, LV_COLOR_ORANGE);
#elif defined (LILYGO_WATCH_2020_V3)
    lv_style_set_text_color(&shadow_timestyle, LV_STATE_DEFAULT, LV_COLOR_CYAN);
#endif
    lv_style_set_text_font( &shadow_timestyle, LV_STATE_DEFAULT, time_font );

    lv_style_copy( &secsstyle, style);
    lv_style_set_text_font( &secsstyle, LV_STATE_DEFAULT, secs_font );

    lv_style_copy( &stopwstyle, style);
    lv_style_set_text_font( &stopwstyle, LV_STATE_DEFAULT, stop_font );

    lv_style_copy( &datestyle, style);
    lv_style_set_text_font( &datestyle, LV_STATE_DEFAULT, date_font );

    lv_style_copy( &infostyle, style);
    lv_style_set_text_font( &infostyle, LV_STATE_DEFAULT, info_font );

    lv_style_copy( &tempstyle, style);
    lv_style_set_text_font( &tempstyle, LV_STATE_DEFAULT, temp_font );

    lv_style_copy( &iconstyle, style);
    lv_style_set_text_font( &iconstyle, LV_STATE_DEFAULT, icon_font );

    clock_cont = mainbar_obj_create( main_cont );
    lv_obj_set_size( clock_cont, lv_disp_get_hor_res( NULL ) , lv_disp_get_ver_res( NULL ) );
    lv_obj_add_style( clock_cont, LV_OBJ_PART_MAIN, style );
    lv_obj_align( clock_cont, main_cont, LV_ALIGN_CENTER, 0, 0 );
 
    unixlabel = lv_label_create( clock_cont , NULL);
    lv_label_set_text(unixlabel, "0000000000");
    lv_obj_reset_style_list( unixlabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( unixlabel, LV_OBJ_PART_MAIN, &secsstyle );
    lv_obj_align( unixlabel, clock_cont, LV_ALIGN_IN_LEFT_MID, 0, -60 );
 
    stopwlabel = lv_label_create( clock_cont , NULL);
    lv_label_set_text(stopwlabel, "00'00'000");
    lv_obj_reset_style_list( stopwlabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( stopwlabel, LV_OBJ_PART_MAIN, &stopwstyle );
    lv_obj_align( stopwlabel, clock_cont, LV_ALIGN_IN_LEFT_MID, 58, 52 );

    main_tile_hide_stopwatch();
 
    shadow_timelabel = lv_label_create( clock_cont , NULL);
    lv_label_set_text(shadow_timelabel, "00:00:");
    lv_obj_reset_style_list( shadow_timelabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( shadow_timelabel, LV_OBJ_PART_MAIN, &shadow_timestyle );
    lv_obj_align(shadow_timelabel, NULL, LV_ALIGN_CENTER, 0, 3);

    timelabel = lv_label_create( clock_cont , NULL);
    lv_label_set_text(timelabel, "00:00:");
    lv_obj_reset_style_list( timelabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( timelabel, LV_OBJ_PART_MAIN, &timestyle );
    lv_obj_align(timelabel, NULL, LV_ALIGN_CENTER, 0, 3);

    secslabel = lv_label_create( clock_cont , NULL);
    lv_label_set_text(secslabel, "00");
    lv_obj_reset_style_list( secslabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( secslabel, LV_OBJ_PART_MAIN, &secsstyle );
    lv_obj_align(secslabel, NULL, LV_ALIGN_IN_RIGHT_MID, 0, 52);

    datelabel = lv_label_create( clock_cont , NULL);
    lv_label_set_text(datelabel, "1.Jan 1970");
    lv_obj_reset_style_list( datelabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( datelabel, LV_OBJ_PART_MAIN, &datestyle );
    lv_obj_align( datelabel, clock_cont, LV_ALIGN_IN_BOTTOM_RIGHT, -45, -27);

    infolabel = lv_label_create( clock_cont , NULL);
    lv_label_set_text(infolabel, "battery: n/a");
    lv_obj_reset_style_list( infolabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( infolabel, LV_OBJ_PART_MAIN, &infostyle );
    lv_obj_align( infolabel, datelabel, LV_ALIGN_OUT_TOP_MID, 0, 0 );
    #ifndef M5PAPER
        lv_obj_set_hidden( infolabel, true );
    #endif

    templabel = lv_label_create( clock_cont , NULL);
    lv_label_set_text(templabel, "temp/humidity: n/a");
    lv_obj_reset_style_list( templabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( templabel, LV_OBJ_PART_MAIN, &tempstyle );
    lv_obj_align( templabel, infolabel, LV_ALIGN_OUT_TOP_MID, 0, 0 );
    if ( !sensor_get_available() )
        lv_obj_set_hidden( templabel, true );

    main_tile_update_time( true );

    for ( int widget = 0 ; widget < MAX_WIDGET_NUM ; widget++ ) {
        widget_entry[ widget ].active = false;

        widget_entry[ widget ].icon_cont = mainbar_obj_create( main_cont );
        lv_obj_reset_style_list( widget_entry[ widget ].icon_cont, LV_OBJ_PART_MAIN );
        lv_obj_add_style( widget_entry[ widget ].icon_cont, LV_OBJ_PART_MAIN, style );
        lv_obj_set_size( widget_entry[ widget ].icon_cont, WIDGET_X_SIZE, WIDGET_Y_SIZE );
        lv_obj_set_hidden( widget_entry[ widget ].icon_cont, true );
        // create app label
        widget_entry[ widget ].label = lv_label_create( widget_entry[ widget ].icon_cont , NULL );
        mainbar_add_slide_element( widget_entry[ widget ].label);
        lv_obj_reset_style_list( widget_entry[ widget ].label, LV_OBJ_PART_MAIN );
        lv_obj_add_style( widget_entry[ widget ].label, LV_OBJ_PART_MAIN, &iconstyle );
        lv_obj_set_size( widget_entry[ widget ].label, WIDGET_X_SIZE, WIDGET_LABEL_Y_SIZE );
        lv_obj_align( widget_entry[ widget ].label , widget_entry[ widget ].icon_cont, LV_ALIGN_IN_BOTTOM_MID, 0, 0 );
        // create app label
        widget_entry[ widget ].ext_label = lv_label_create( widget_entry[ widget ].icon_cont , NULL );
        mainbar_add_slide_element( widget_entry[ widget ].ext_label);
        lv_obj_reset_style_list( widget_entry[ widget ].ext_label, LV_OBJ_PART_MAIN );
        lv_obj_add_style( widget_entry[ widget ].ext_label, LV_OBJ_PART_MAIN, &iconstyle );
        lv_obj_set_size( widget_entry[ widget ].ext_label, WIDGET_X_SIZE, WIDGET_LABEL_Y_SIZE );
        lv_obj_align( widget_entry[ widget ].ext_label , widget_entry[ widget ].label, LV_ALIGN_OUT_TOP_MID, 0, 0 );
        // create img and indicator
        widget_entry[ widget ].icon_img = lv_imgbtn_create( widget_entry[ widget ].icon_cont , NULL );
        widget_entry[ widget ].icon_indicator = lv_img_create( widget_entry[ widget ].icon_cont, NULL );
        // hide all
        lv_obj_set_hidden( widget_entry[ widget ].icon_cont, true );
        lv_obj_set_hidden( widget_entry[ widget ].icon_img, true );
        lv_obj_set_hidden( widget_entry[ widget ].icon_indicator, true );
        lv_obj_set_hidden( widget_entry[ widget ].label, true );
        lv_obj_set_hidden( widget_entry[ widget ].ext_label, true );
    }

    main_tile_task = lv_task_create( main_tile_update_task, 500, LV_TASK_PRIO_MID, NULL );

    powermgm_register_cb( POWERMGM_WAKEUP , main_tile_powermgm_event_cb, "main tile time update" );
    timesync_register_cb( TIME_SYNC_UPDATE, main_tile_time_update_ebent_cb, "main tile time sync" );
    sensor_register_cb( SENSOR_TEMPERATURE | SENSOR_RELHUMIDITY, main_tile_sensor_event_cb, "main tile sensor" );
    mainbar_add_tile_button_cb( main_tile_num, main_tile_button_event_cb );
    styles_register_cb( STYLE_CHANGE, main_tile_style_event_cb, "main tile style event" );

    maintile_init = true;
}

bool main_tile_style_event_cb( EventBits_t event, void *arg ){
    switch( event ) {
        case STYLE_CHANGE:     style = ws_get_mainbar_style();

                                lv_style_copy( &timestyle, style);
                                lv_style_set_text_font( &timestyle, LV_STATE_DEFAULT, time_font );

                                lv_style_copy( &datestyle, style);
                                lv_style_set_text_font( &datestyle, LV_STATE_DEFAULT, date_font );

                                lv_style_copy( &infostyle, style);
                                lv_style_set_text_font( &infostyle, LV_STATE_DEFAULT, info_font );

                                lv_style_copy( &tempstyle, style);
                                lv_style_set_text_font( &tempstyle, LV_STATE_DEFAULT, temp_font );

                                lv_style_copy( &iconstyle, style);
                                lv_style_set_text_font( &iconstyle, LV_STATE_DEFAULT, icon_font );
                                break;
    }
    return( true );
}

static bool main_tile_sensor_event_cb( EventBits_t event, void *arg ) {
    static float temp = 0.0f;
    static float humidity = 0.0f;

    switch( event ) {
        case SENSOR_TEMPERATURE:        temp = *(float *)arg;
                                        break;
        case SENSOR_RELHUMIDITY:        humidity = *(float*)arg;
                                        break;
    }

    char sensor_str[128] = "";
    snprintf( sensor_str, sizeof( sensor_str ), "temp/humidity: %0.1f°C/%.0f%%", temp, humidity );
    lv_label_set_text( templabel, sensor_str );
    lv_obj_align( templabel, infolabel, LV_ALIGN_OUT_TOP_MID, 0, 0 );

    return( true );
}

static bool main_tile_button_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case BUTTON_RIGHT:  mainbar_jump_to_tilenumber( app_tile_get_tile_num(), LV_ANIM_OFF );
                            mainbar_clear_history();
                            break;
    }
    return( true );
}


bool main_tile_time_update_ebent_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case TIME_SYNC_UPDATE:
            main_tile_update_time( true );
            break;
    }
    return( true );
}

bool main_tile_powermgm_event_cb( EventBits_t event, void *arg ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        return( true );
    }

    switch( event ) {
        case POWERMGM_WAKEUP:
            main_tile_update_time( true );
            break;
    }
    return( true );
}

lv_obj_t *main_tile_register_widget( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        while( true );
    }

    for( int widget = 0 ; widget < MAX_WIDGET_NUM ; widget++ ) {
        if ( widget_entry[ widget ].active == false ) {
            widget_entry[ widget ].active = true;
            lv_obj_set_hidden( widget_entry[ widget ].icon_cont, false );
            main_tile_align_widgets();
            return( widget_entry[ widget ].icon_cont );
        }
    }
    log_e("no more space for a widget");
    return( NULL );
}

icon_t *main_tile_get_free_widget_icon( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        while( true );
    }

    for( int widget = 0 ; widget < MAX_WIDGET_NUM ; widget++ ) {
        if ( widget_entry[ widget ].active == false ) {
            lv_obj_set_hidden( widget_entry[ widget ].icon_cont, false );
            return( &widget_entry[ widget ] );
        }
    }
    log_e("no more space for a widget");
    return( NULL );
}

void main_tile_align_widgets( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        return;
    }

    int active_widgets = 0;
    lv_coord_t xpos = 0;

    for ( int widget = 0 ; widget < MAX_WIDGET_NUM ; widget++ ) {
        if ( widget_entry[ widget ].active )
        active_widgets++;
    }

    if ( active_widgets == 0 ) {
        lv_obj_align( clock_cont, main_cont, LV_ALIGN_CENTER, 0, 0 );
        return;
    };

    lv_obj_align( clock_cont, main_cont, LV_ALIGN_IN_TOP_MID, 0, 0 );

    xpos = 0 - ( ( WIDGET_X_SIZE * active_widgets ) + ( ( active_widgets - 1 ) * WIDGET_X_CLEARENCE ) ) / 2;

    active_widgets = 0;

    for ( int widget = 0 ; widget < MAX_WIDGET_NUM ; widget++ ) {
        if ( widget_entry[ widget ].active ) {
            lv_obj_align( widget_entry[ widget ].icon_cont , main_cont, LV_ALIGN_IN_BOTTOM_MID, xpos + ( WIDGET_X_SIZE * active_widgets ) + ( active_widgets * WIDGET_X_CLEARENCE ) + 32 , - ( ( lv_disp_get_ver_res( NULL ) / 4 ) -32 ) );
            active_widgets++;
        }
    }

}

uint32_t main_tile_get_tile_num( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        while( true );
    }

    return( main_tile_num );
}

void main_tile_hide_stopwatch(void) {
    lv_obj_set_hidden(stopwlabel, true );
}

void main_tile_show_stopwatch(void) {
    lv_obj_set_hidden(stopwlabel, false );
}

void main_tile_update_stopwatch(int stop_min, int stop_sec, int stop_ms)
{
    lv_label_set_text_fmt(stopwlabel, "%02d'%02d'%03d", stop_min, stop_sec, stop_ms); 
}

void main_tile_update_time( bool force ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        return;
    }

    time_t now;
    static time_t last = 0;
    struct tm  info, last_info;
    char time_str[64]="";
    char secs_str[16]="";
    char info_str[64]="";
    /*
     * copy current time into now and convert it local time info
     */
    time( &now );
    localtime_r( &now, &info );
    /*
     * convert last time_t into tm from
     * last check if last equal zero (first run condition)
     */
    if ( last != 0 ) {
        localtime_r( &last, &last_info );
    }
    /*
     * Time:
     * only update while time changes or force ist set
     * Display has a minute resolution
     */
    if ( last == 0 || info.tm_min != last_info.tm_min || info.tm_hour != last_info.tm_hour || info.tm_sec != last_info.tm_sec || force ) {
        timesync_get_current_timestring( time_str, sizeof(time_str) );
        timesync_get_current_secsstring( secs_str, sizeof(secs_str) );
        log_d("renew time: %s", time_str );
        lv_label_set_text( timelabel, time_str );
        lv_label_set_text(shadow_timelabel, lv_label_get_text(timelabel));
        lv_obj_align( timelabel, clock_cont, LV_ALIGN_CENTER, 0, 0 );
        lv_obj_align( shadow_timelabel, clock_cont, LV_ALIGN_CENTER, 3, 3);
        lv_label_set_text( secslabel, secs_str );
        lv_label_set_text_fmt(unixlabel, "%d", (int)time(NULL));
        /*
         * Date:
         * only update while date changes
         */
        if ( last == 0 || info.tm_yday != last_info.tm_yday ) {
            strftime( time_str, sizeof(time_str), "%a %d.%b %Y", &info );
            log_d("renew date: %s", time_str );
            lv_label_set_text( datelabel, time_str );
            //lv_obj_align( datelabel, clock_cont, LV_ALIGN_IN_BOTTOM_MID, 0, 0 );
        }

        //snprintf( info_str, sizeof( info_str ),"battery: %d%%", pmu_get_battery_percent() );
        //lv_label_set_text( infolabel, info_str );
        lv_obj_align( infolabel, datelabel, LV_ALIGN_OUT_TOP_MID, 0, 0 );
        /*
         * Save for next loop
         */
        last = now;
    }
}

void main_tile_update_task( lv_task_t * task ) {
    /*
     * check if maintile alread initialized
     */
    if ( !maintile_init ) {
        log_e("maintile not initialized");
        return;
    }

    main_tile_update_time( false );
}
