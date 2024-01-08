#include "config.h"
#include <TTGO.h>

#include "silverbox_app.h"
#include "silverbox_app_main.h"

#include "gui/mainbar/mainbar.h"
#include "gui/statusbar.h"
#include "gui/app.h"

uint32_t silverbox_app_main_tile_num;
icon_t *silverbox_app = NULL;

LV_IMG_DECLARE(silverbox_app_64px);
LV_IMG_DECLARE(info_1_16px);

static void enter_silverbox_app_event_cb( lv_obj_t * obj, lv_event_t event );

/*
 * setup routine for silverbox app
 */
void silverbox_app_setup( void ) {
    silverbox_app_main_tile_num = mainbar_add_app_tile( 1, 1, "silverbox" );
    silverbox_app = app_register( "silver box", &silverbox_app_64px, enter_silverbox_app_event_cb );
    silverbox_app_main_setup( silverbox_app_main_tile_num );
}

/*
 *
 */
uint32_t silverbox_app_get_app_main_tile_num( void ) {
    return( silverbox_app_main_tile_num );
}

/*
 *
 */
static void enter_silverbox_app_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       statusbar_hide( true );
                                        app_hide_indicator( silverbox_app );
                                        mainbar_jump_to_tilenumber( silverbox_app_main_tile_num, LV_ANIM_OFF );
                                        break;
    }    
}

