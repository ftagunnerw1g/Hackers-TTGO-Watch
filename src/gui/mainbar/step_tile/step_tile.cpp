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
#include "gui/mainbar/step_tile/step_tile.h"
#include "gui/mainbar/setup_tile/setup_tile.h"

#ifdef NATIVE_64BIT
    #include "utils/logging.h"
#else
    #include <Arduino.h>
    #ifdef M5PAPER
    #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 )
    #endif
#endif

static bool steptile_init = false;

static lv_obj_t *step_cont = NULL;
static lv_obj_t *steplabel = NULL;

uint32_t step_tile_num;

static lv_style_t *style;
static lv_style_t stepstyle;

LV_FONT_DECLARE(LCD_48px);
LV_FONT_DECLARE(LCD_32px);
LV_FONT_DECLARE(LCD_16px);

static bool step_tile_button_event_cb( EventBits_t event, void *arg );

void step_tile_setup( void ) {

    if ( steptile_init ) {
        log_e("step tile already init");
        return;
    }

    #if defined( M5PAPER )
        step_tile_num = mainbar_add_tile( 0, 4, "step tile", ws_get_mainbar_style() );
        step_cont = mainbar_get_tile_obj( step_tile_num );
    #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 ) || defined( M5CORE2 )
        step_tile_num = mainbar_add_tile( 0, 2, "step tile", ws_get_mainbar_style() );
        step_cont = mainbar_get_tile_obj( step_tile_num );
    #elif defined( LILYGO_WATCH_2021 )
        step_tile_num = mainbar_add_tile( 0, 2, "step tile", ws_get_mainbar_style() );
        step_cont = mainbar_get_tile_obj( step_tile_num );
    #else
        #error "no step tiles setup"  
    #endif 
    style = ws_get_mainbar_style();

    lv_style_copy( &stepstyle, style);
    lv_style_set_text_opa( &stepstyle, LV_OBJ_PART_MAIN, LV_OPA_40);
    lv_style_set_text_font( &stepstyle, LV_STATE_DEFAULT, &LCD_48px);

    steplabel = lv_label_create( step_cont, NULL);
    lv_label_set_text( steplabel, "123456");
    lv_obj_reset_style_list( steplabel, LV_OBJ_PART_MAIN );
    lv_obj_add_style( steplabel, LV_OBJ_PART_MAIN, &stepstyle );
    lv_obj_align( steplabel, NULL, LV_ALIGN_IN_LEFT_MID, 0, 0);

    mainbar_add_tile_button_cb( step_tile_num, step_tile_button_event_cb );

    steptile_init = true;
}

static bool step_tile_button_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case BUTTON_LEFT:   mainbar_jump_to_tilenumber( setup_get_tile_num(), LV_ANIM_OFF );
                            mainbar_clear_history();
                            break;
    }
    return( true );
}

uint32_t step_tile_get_tile_num( void ) {
    /*
     * check if maintile alread initialized
     */
    if ( !steptile_init ) {
        log_e("maintile not initialized");
        while( true );
    }

    return( step_tile_num );
}
