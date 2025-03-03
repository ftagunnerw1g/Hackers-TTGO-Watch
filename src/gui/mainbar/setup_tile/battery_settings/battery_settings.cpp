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
#include "battery_settings.h"
#include "battery_view.h"

#include "gui/mainbar/mainbar.h"
#include "gui/mainbar/setup_tile/setup_tile.h"
#include "gui/statusbar.h"
#include "gui/setup.h"
#include "gui/widget_factory.h"
#include "gui/widget_styles.h"

#include "hardware/display.h"
#include "hardware/motor.h"
#include "hardware/pmu.h"

icon_t *battery_setup_icon = NULL;

lv_obj_t *battery_settings_tile=NULL;
uint32_t battery_settings_tile_num;

lv_obj_t *battery_silence_wakeup_switch = NULL;
lv_obj_t *battery_percent_switch = NULL;
lv_obj_t *battery_experimental_switch = NULL;
lv_obj_t *battery_high_voltage_switch = NULL;
lv_obj_t *battery_logging_switch = NULL;

LV_IMG_DECLARE(battery_icon_64px);
LV_IMG_DECLARE(info_update_16px);

static void enter_battery_setup_event_cb( lv_obj_t * obj, lv_event_t event );
static void exit_battery_setup_event_cb( lv_obj_t * obj, lv_event_t event );
static void battery_silence_wakeup_switch_event_handler( lv_obj_t * obj, lv_event_t event );
static void battery_percent_switch_event_handler( lv_obj_t * obj, lv_event_t event );
static void battery_experimental_switch_event_handler( lv_obj_t * obj, lv_event_t event );
static void battery_high_voltage_switch_event_handler( lv_obj_t * obj, lv_event_t event );
void battery_set_experimental_indicator( void );

void battery_settings_tile_setup( void ) {
    // get an app tile and copy mainstyle
    battery_settings_tile_num = mainbar_add_setup_tile( 1, 1, "battery setup" );
    battery_settings_tile = mainbar_get_tile_obj( battery_settings_tile_num );

    battery_view_tile_setup( battery_settings_tile_num );

    battery_setup_icon = setup_register( "battery", &battery_icon_64px, enter_battery_setup_event_cb );
    setup_hide_indicator( battery_setup_icon );
}

void battery_set_experimental_indicator( void ) {
    if ( pmu_get_experimental_power_save() || pmu_get_calculated_percent() ) {
        setup_set_indicator( battery_setup_icon, ICON_INDICATOR_N );
    }
    else {
        setup_hide_indicator( battery_setup_icon );
    }
}

static void battery_silence_wakeup_switch_event_handler( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_VALUE_CHANGED ): pmu_set_silence_wakeup( lv_switch_get_state( obj ) );
                                        break;
    }
}

static void enter_battery_setup_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mainbar_jump_to_tilenumber( battery_settings_tile_num, LV_ANIM_OFF );
                                        break;
    }

}

static void exit_battery_setup_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mainbar_jump_back();
                                        break;
    }
}
