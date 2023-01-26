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
#include "config.h"
#include "gui/mainbar/mainbar.h"
#include "gui/widget_styles.h"

#include "gui/statusbar.h"
#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/note_tile/note_tile.h"
#include "gui/mainbar/setup_tile/setup_tile.h"

#ifdef NATIVE_64BIT
    #include "utils/logging.h"
#else
    #include <Arduino.h>
    #ifdef M5PAPER
    #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 )
    #endif
#endif

static bool notetile_init = false;

static lv_obj_t *note_cont = NULL;
static lv_obj_t *notelabel = NULL;

// main 
static lv_obj_t *weathlabel0 = NULL;
static lv_obj_t *weathlabel0text = NULL;
static lv_obj_t *weathlabel0textsm = NULL;
static lv_obj_t *weathlabel0desc = NULL;
static lv_obj_t *weathlabel0xtra = NULL;

//day+1 etc
static lv_obj_t *weathlabel1 = NULL;
static lv_obj_t *weathlabel1text = NULL;
static lv_obj_t *weathlabel2 = NULL;
static lv_obj_t *weathlabel2text = NULL;
static lv_obj_t *weathlabel3 = NULL;
static lv_obj_t *weathlabel3text = NULL;

uint32_t note_tile_num;

static lv_style_t *style;
static lv_style_t notestyle;
static lv_style_t weathstyle;
static lv_style_t miniweathstyle;
static lv_style_t biglabelstyle;
static lv_style_t mediumlabelstyle;
static lv_style_t smalllabelstyle;

LV_FONT_DECLARE(LCD_48px);
LV_FONT_DECLARE(LCD_32px);
LV_FONT_DECLARE(LCD_16px);
LV_FONT_DECLARE(weather_48px);
LV_FONT_DECLARE(weather_72px);

static bool note_tile_button_event_cb( EventBits_t event, void *arg );

void note_tile_setup( void ) {

    if ( notetile_init ) {
        log_e("note tile already init");
        return;
    }

    #if defined( M5PAPER )
        note_tile_num = mainbar_add_tile( 0, 3, "note tile", ws_get_mainbar_style() );
        note_cont = mainbar_get_tile_obj( note_tile_num );
    #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 ) || defined( M5CORE2 )
        note_tile_num = mainbar_add_tile( 0, 1, "note tile", ws_get_mainbar_style() );
        note_cont = mainbar_get_tile_obj( note_tile_num );
    #elif defined( LILYGO_WATCH_2021 )
        note_tile_num = mainbar_add_tile( 0, 1, "note tile", ws_get_mainbar_style() );
        note_cont = mainbar_get_tile_obj( note_tile_num );
    #else
        #error "no note tiles setup"  
    #endif 
    style = ws_get_mainbar_style();

    lv_style_copy( &notestyle, style);
    lv_style_set_text_opa( &notestyle, LV_OBJ_PART_MAIN, LV_OPA_30);
    lv_style_set_text_font( &notestyle, LV_STATE_DEFAULT, &LCD_48px);

    lv_style_copy( &biglabelstyle, style);
    lv_style_set_text_opa( &biglabelstyle, LV_OBJ_PART_MAIN, LV_OPA_80);
    lv_style_set_text_font( &biglabelstyle, LV_STATE_DEFAULT, &LCD_48px);

    lv_style_copy( &mediumlabelstyle, style);
    lv_style_set_text_opa( &mediumlabelstyle, LV_OBJ_PART_MAIN, LV_OPA_80);
    lv_style_set_text_font( &mediumlabelstyle, LV_STATE_DEFAULT, &LCD_32px);

    lv_style_copy( &smalllabelstyle, style);
    lv_style_set_text_opa( &smalllabelstyle, LV_OBJ_PART_MAIN, LV_OPA_80);
    lv_style_set_text_font( &smalllabelstyle, LV_STATE_DEFAULT, &LCD_16px);

    lv_style_copy( &notestyle, style);
    lv_style_set_text_opa( &notestyle, LV_OBJ_PART_MAIN, LV_OPA_30);
    lv_style_set_text_font( &notestyle, LV_STATE_DEFAULT, &LCD_48px);

    notelabel = lv_label_create( note_cont, NULL);
    lv_label_set_text( notelabel, "ROUNDHAY");
    lv_obj_reset_style_list( notelabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( notelabel, LV_OBJ_PART_MAIN, &notestyle );
    lv_obj_align( notelabel, NULL, LV_ALIGN_IN_TOP_MID, 0, (STATUSBAR_HEIGHT + 12));

    lv_style_copy( &weathstyle, style);
    lv_style_set_text_opa( &weathstyle, LV_OBJ_PART_MAIN, LV_OPA_30);
    lv_style_set_text_font( &weathstyle, LV_STATE_DEFAULT, &weather_72px);

    lv_style_copy( &miniweathstyle, style);
    lv_style_set_text_opa( &miniweathstyle, LV_OBJ_PART_MAIN, LV_OPA_30);
    lv_style_set_text_font( &miniweathstyle, LV_STATE_DEFAULT, &weather_48px);

    weathlabel0 = lv_label_create( note_cont, NULL);
    lv_label_set_text( weathlabel0, "\357\200\214");
    lv_obj_reset_style_list( weathlabel0, LV_OBJ_PART_MAIN );
    lv_obj_add_style( weathlabel0, LV_OBJ_PART_MAIN, &weathstyle );
    lv_obj_align( weathlabel0, NULL, LV_ALIGN_IN_LEFT_MID, 0, 10);

    weathlabel0text = lv_label_create( note_cont, NULL);
    lv_label_set_text( weathlabel0text, "30");
    lv_obj_reset_style_list( weathlabel0text, LV_OBJ_PART_MAIN );
    lv_obj_add_style( weathlabel0text, LV_OBJ_PART_MAIN, &biglabelstyle );
    lv_obj_align( weathlabel0text, NULL, LV_ALIGN_CENTER, 50, -18);

    weathlabel0textsm = lv_label_create( note_cont, NULL);
    lv_label_set_text( weathlabel0textsm, "22");
    lv_obj_reset_style_list( weathlabel0textsm, LV_OBJ_PART_MAIN );
    lv_obj_add_style( weathlabel0textsm, LV_OBJ_PART_MAIN, &mediumlabelstyle );
    lv_obj_align( weathlabel0textsm, NULL, LV_ALIGN_CENTER, 99, -30);

    weathlabel0desc = lv_label_create( note_cont, NULL);
    lv_label_set_long_mode(weathlabel0desc, LV_LABEL_LONG_SROLL_CIRC);   
    lv_label_set_anim_speed(weathlabel0desc, 15);
    lv_obj_set_width(weathlabel0desc, 130);  
    lv_label_set_text( weathlabel0desc, "sunshine and showers");
    lv_obj_reset_style_list( weathlabel0desc, LV_OBJ_PART_MAIN );
    lv_obj_add_style( weathlabel0desc, LV_OBJ_PART_MAIN, &smalllabelstyle );
    lv_obj_align( weathlabel0desc, NULL, LV_ALIGN_IN_RIGHT_MID, -5, 15);

    weathlabel0xtra = lv_label_create( note_cont, NULL);
    lv_label_set_long_mode(weathlabel0xtra, LV_LABEL_LONG_SROLL_CIRC);   
    lv_label_set_anim_speed(weathlabel0xtra, 20);
    lv_obj_set_width(weathlabel0xtra, 135); 
    lv_label_set_text( weathlabel0xtra, "suntime: 05:45/20:10 max_wind: 13mph last_sync: 04:20");
    lv_obj_reset_style_list( weathlabel0xtra, LV_OBJ_PART_MAIN );
    lv_obj_add_style( weathlabel0xtra, LV_OBJ_PART_MAIN, &smalllabelstyle );
    lv_obj_align( weathlabel0xtra, NULL, LV_ALIGN_IN_RIGHT_MID, -5, 35);

    weathlabel1 = lv_label_create( note_cont, NULL);
    lv_label_set_text( weathlabel1, "\357\200\215");
    lv_obj_reset_style_list( weathlabel1, LV_OBJ_PART_MAIN );
    lv_obj_add_style( weathlabel1, LV_OBJ_PART_MAIN, &miniweathstyle );
    lv_obj_align( weathlabel1, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, -10);

    weathlabel1text = lv_label_create( note_cont, NULL);
    lv_label_set_text( weathlabel1text, "25 10");
    lv_obj_reset_style_list( weathlabel1text, LV_OBJ_PART_MAIN );
    lv_obj_add_style( weathlabel1text, LV_OBJ_PART_MAIN, &smalllabelstyle );
    lv_obj_align( weathlabel1text, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

    weathlabel2 = lv_label_create( note_cont, NULL);
    lv_label_set_text( weathlabel2, "\357\200\215");
    lv_obj_reset_style_list( weathlabel2, LV_OBJ_PART_MAIN );
    lv_obj_add_style( weathlabel2, LV_OBJ_PART_MAIN, &miniweathstyle );
    lv_obj_align( weathlabel2, NULL, LV_ALIGN_IN_BOTTOM_MID, -30, -10);

    weathlabel2text = lv_label_create( note_cont, NULL);
    lv_label_set_text( weathlabel2text, "21 9");
    lv_obj_reset_style_list( weathlabel2text, LV_OBJ_PART_MAIN );
    lv_obj_add_style( weathlabel2text, LV_OBJ_PART_MAIN, &smalllabelstyle );
    lv_obj_align( weathlabel2text, NULL, LV_ALIGN_IN_BOTTOM_MID, -30, 0);

    weathlabel3 = lv_label_create( note_cont, NULL);
    lv_label_set_text( weathlabel3, "\357\200\214");
    lv_obj_reset_style_list( weathlabel3, LV_OBJ_PART_MAIN );
    lv_obj_add_style( weathlabel3, LV_OBJ_PART_MAIN, &miniweathstyle );
    lv_obj_align( weathlabel3, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -55, -10);

    weathlabel3text = lv_label_create( note_cont, NULL);
    lv_label_set_text( weathlabel3text, "25 11");
    lv_obj_reset_style_list( weathlabel3text, LV_OBJ_PART_MAIN );
    lv_obj_add_style( weathlabel3text, LV_OBJ_PART_MAIN, &smalllabelstyle );
    lv_obj_align( weathlabel3text, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -55, 0);

    mainbar_add_tile_button_cb( note_tile_num, note_tile_button_event_cb );

    notetile_init = true;
}

static bool note_tile_button_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case BUTTON_LEFT:   mainbar_jump_to_tilenumber( setup_get_tile_num(), LV_ANIM_OFF );
                            mainbar_clear_history();
                            break;
    }
    return( true );
}

uint32_t note_tile_get_tile_num( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( !notetile_init ) {
        log_e("maintile not initialized");
        while( true );
    }

    return( note_tile_num );
}
