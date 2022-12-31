#include "lvgl.h"
#include "gui/gui.h"

#include "hardware/hardware.h"
#include "hardware/powermgm.h"

#if defined ( LILYGO_WATCH_2020_V2 )
#include "app/gps_status/gps_status.h"
#include "app/osmmap/osmmap_app.h"
#endif

#include "app/stopwatch/stopwatch_app.h"
#include "app/alarm_clock/alarm_clock.h"

#include "app/bluebox/bluebox_app.h"
#include "app/netscan/netscan_app.h"
#include "app/ping/ping_app.h"
#include "app/sshclient/sshclient_app.h"
#include "app/subnet/subnet_app.h"
#include "app/iplookup/iplookup_app.h"
#include "app/wireless/wireless_app.h"
#include "app/wifimon/wifimon_app.h"
#include "app/debug/debug_app.h"

#if defined( NATIVE_64BIT )
    /**
     * for non arduino
     */                 
    void setup( void );
    void loop( void );

    int main( void ) {
        setup();
        while( 1 ) { loop(); };
        return( 0 );
    }
#endif // NATIVE_64BIT

void setup() {
    /**
     * hardware setup
     */
    hardware_setup();
    /**
     * gui setup
     */
    gui_setup();
    /**
     * apps here
     */
#if defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V3 )
    bluebox_app_setup();
#endif 
    netscan_app_setup();
    subnet_app_setup();
    ping_app_setup();
    iplookup_app_setup();
    sshclient_app_setup();
    wireless_app_setup();
    wifimon_app_setup();
    debug_app_setup();
    stopwatch_app_setup();
    alarm_clock_setup();

#if defined ( LILYGO_WATCH_2020_V2 )
    gps_status_setup();
    osmmap_app_setup();
#endif

    /**
     * post hardware setup
     */
    hardware_post_setup();
}

void loop(){
    powermgm_loop();
}
