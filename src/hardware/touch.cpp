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

#include "motor.h"
#include "display.h"
#include "touch.h"
#include "powermgm.h"
#include "callback.h"

#ifdef NATIVE_64BIT
    #include "utils/logging.h"
    #include "indev/mouse.h"
    #include "indev/mousewheel.h"
#else
    #include <Arduino.h>
    #if defined( M5PAPER )
        #include <M5EPD.h>
    #elif defined( M5CORE2 )
        #include <M5Core2.h>
    #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 )
        #include <TTGO.h>

        volatile bool DRAM_ATTR touch_irq_flag = false;
        portMUX_TYPE DRAM_ATTR Touch_IRQ_Mux = portMUX_INITIALIZER_UNLOCKED;
        void IRAM_ATTR touch_irq( void );

        void IRAM_ATTR touch_irq( void ) {
            /*
            * enter critical section and set interrupt flag
            */
            portENTER_CRITICAL_ISR(&Touch_IRQ_Mux);
            touch_irq_flag = true;
            /*
            * leave critical section
            */
            portEXIT_CRITICAL_ISR(&Touch_IRQ_Mux);
        }

        static SemaphoreHandle_t xSemaphores = NULL;

        bool touch_lock_take( void ) {
            return xSemaphoreTake( xSemaphores, portMAX_DELAY ) == pdTRUE;
        }
        void touch_lock_give( void ) {
            xSemaphoreGive( xSemaphores );
        }
    #elif defined( LILYGO_WATCH_2021 )    
        #include <Wire.h>
        #include <twatch2021_config.h>
        #include <CST8165/CST816S.h>

        CST816S TouchSensor;
    #else
        #error "no hardware driver for touch, please setup minimal drivers ( framebuffer/touch )"
    #endif
#endif

callback_t *touch_callback = NULL;
lv_indev_t *touch_indev = NULL;
bool touched = false;

static bool touch_read(lv_indev_drv_t * drv, lv_indev_data_t*data);
bool touch_powermgm_loop_event_cb( EventBits_t event, void *arg );
bool touch_powermgm_event_cb( EventBits_t event, void *arg );
bool touch_send_event_cb( EventBits_t event, void *arg );

void touch_setup( void ) 
{
#ifdef NATIVE_64BIT
    /**
     * init SDL mouse
     */
    mouse_init();
#else
    #if defined( M5PAPER )
        /**
         * rotate touchscreen
         */
        M5.TP.SetRotation(90);
    #elif defined( M5CORE2 )
        M5.Touch.begin();
    #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 )
        TTGOClass *ttgo = TTGOClass::getWatch();
        /*
        * reset/wakeup touch controller
        */
        #if defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 )
            ttgo->touchWakup();
        #endif
        /*
        * This changes to polling mode.
        * The touch sensor holds the line low for the duration of the touch.
        * The level change can still trigger an interrupt, which we
        * use to start polling, and we don't stop polling till the level is high again.
        */
        ttgo->touch->disableINT();
        detachInterrupt( TOUCH_INT );
        /*
        * This doesn't appear to change anything,
        * the sensor doesn't automatically switch to monitor mode till after 30 seconds
        */
        ttgo->touch->setMonitorTime(0x01);
        /*
        * This is supposed to control how often the sensor checks for touch while in monitor mode
        * The units is ms between checks, so 250 checks only 4 times a second. 
        */
        ttgo->touch->setMonitorPeriod(125);
        /*
        * create semaphore and register interrupt function
        */
        xSemaphores = xSemaphoreCreateMutex();
        attachInterrupt( TOUCH_INT, &touch_irq, FALLING );
    #elif defined( LILYGO_WATCH_2021 )    
        if ( !TouchSensor.begin( Wire, Touch_Res, Touch_Int, CTP_SLAVER_ADDR ) )
            log_e("touch controler failed");
    #endif
#endif
    /**
     * setup lvgl pointer driver
     */
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read;
    lv_indev_drv_register( &indev_drv );
    /*
     * register powermgm callback function
     */
    powermgm_register_cb( POWERMGM_SILENCE_WAKEUP | POWERMGM_STANDBY | POWERMGM_WAKEUP | POWERMGM_ENABLE_INTERRUPTS | POWERMGM_DISABLE_INTERRUPTS , touch_powermgm_event_cb, "touch" );
    powermgm_register_loop_cb( POWERMGM_STANDBY , touch_powermgm_loop_event_cb, "touch powermgm loop" );
}

bool touch_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id ) {
  /*
    * check if an callback table exist, if not allocate a callback table
    */
  if ( touch_callback == NULL ) {
      touch_callback = callback_init( "touch" );
      if ( touch_callback == NULL ) {
          log_e("touch callback alloc failed");
          while(true);
      }
  }
  /*
    * register an callback entry and return them
    */
  return( callback_register( touch_callback, event, callback_func, id ) );
}

bool touch_send_event_cb( EventBits_t event, void *arg ) {
  /*
    * call all callbacks with her event mask
    */
  return( callback_send_no_log( touch_callback, event, arg ) );
}

bool touch_powermgm_loop_event_cb( EventBits_t event, void *arg ) {
    bool retval = false;

    #ifdef NATIVE_64BIT

    #else
        #if defined ( M5CORE2 )
            switch( event ) {
                case POWERMGM_STANDBY:
                    M5.Touch.update();
                    if ( M5.Touch.ispressed() ) {
                        powermgm_set_event( POWERMGM_WAKEUP_REQUEST );
                    }              
                    retval = true;
                    break;
                case POWERMGM_WAKEUP:
                    retval = true;
                    break;
                case POWERMGM_SILENCE_WAKEUP:
                    M5.Touch.update();
                    if ( M5.Touch.ispressed() ) {
                        powermgm_set_event( POWERMGM_WAKEUP_REQUEST );
                    }              
                    retval = true;        
                    break;
            }
        #else
            retval = true;
        #endif
    #endif
    return( retval );
}

bool touch_powermgm_event_cb( EventBits_t event, void *arg ) {
    bool retval = false;

    #ifdef NATIVE_64BIT
        switch( event ) {
            case POWERMGM_STANDBY:              retval = true;
                                                break;
            case POWERMGM_WAKEUP:               retval = true;
                                                break;
            case POWERMGM_SILENCE_WAKEUP:       retval = true;
                                                break;
            case POWERMGM_ENABLE_INTERRUPTS:    retval = true;
                                                break;
            case POWERMGM_DISABLE_INTERRUPTS:   retval = true;
                                                break;
        }
    #else
        #if defined( M5PAPER ) || defined( LILYGO_WATCH_2021 )
            switch( event ) {
                case POWERMGM_STANDBY:          retval = true;
                                                break;
                case POWERMGM_WAKEUP:           retval = true;
                                                break;
                case POWERMGM_SILENCE_WAKEUP:   retval = true;
                                                break;
                case POWERMGM_ENABLE_INTERRUPTS:
                                                retval = true;
                                                break;
                case POWERMGM_DISABLE_INTERRUPTS:
                                                retval = true;
                                                break;
            }
        #elif defined( M5CORE2 )
            switch( event ) {
                case POWERMGM_STANDBY:          /**
                                                 * block standby so is there no another option to handle
                                                 * wakeup by touch. no real button :(
                                                 */
                                                retval = false;
                                                break;
                case POWERMGM_WAKEUP:           retval = true;
                                                break;
                case POWERMGM_SILENCE_WAKEUP:    retval = true;
                                                break;
                case POWERMGM_ENABLE_INTERRUPTS:
                                                retval = true;
                                                break;
                case POWERMGM_DISABLE_INTERRUPTS:
                                                retval = true;
                                                break;
            }
        #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 )
            TTGOClass *ttgo = TTGOClass::getWatch();
            switch( event ) {
                case POWERMGM_STANDBY:          if ( touch_lock_take() ) {
                                                    ttgo->touchToMonitor();
                                                    touch_lock_give();
                                                }
                                                retval = true;
                                                break;
                case POWERMGM_WAKEUP:           if ( touch_lock_take() ) {
                                                    #if defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 )
                                                        ttgo->touchWakup();
                                                    #endif    
                                                    ttgo->touchToMonitor();
                                                    touch_lock_give();
                                                }
                                                retval = true;
                                                break;
                case POWERMGM_SILENCE_WAKEUP:   retval = true;
                                                break;
                case POWERMGM_ENABLE_INTERRUPTS:
                                                attachInterrupt( TOUCH_INT, &touch_irq, FALLING );
                                                retval = true;
                                                break;
                case POWERMGM_DISABLE_INTERRUPTS:
                                                detachInterrupt( TOUCH_INT );
                                                retval = true;
                                                break;
            }
        #endif
    #endif
    return( retval );
}

bool touch_getXY( int16_t &x, int16_t &y ) {
    
    /**
     * disable touch when we are in standby or silence wakeup
     */
    if ( powermgm_get_event( POWERMGM_STANDBY | POWERMGM_SILENCE_WAKEUP ) ) {
        return( false );
    }

    #ifdef NATIVE_64BIT
    #else
        #if defined( M5PAPER )
            if ( M5.TP.avaliable() ) {
                if( !M5.TP.isFingerUp() ) {
                    touched = true;
                    M5.TP.update();
                    tp_finger_t FingerItem = M5.TP.readFinger( 0 );
                    x = FingerItem.x;
                    y = FingerItem.y;
                }
                else {
                    touched = false;
                    return( false );
                }
            }
        #elif defined( M5CORE2 )
            M5.Touch.update();

            if ( M5.Touch.ispressed() ) {
                Point coordinate;
                coordinate = M5.Touch.getPressPoint();            
                x = coordinate.x;
                y = coordinate.y;
                touched = true;
            }
            else {
                touched = false;
                return( false );
            }

        #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 )
            TTGOClass *ttgo = TTGOClass::getWatch();
            static bool touch_press = false;

            // disable touch when we are in standby or silence wakeup
            if ( powermgm_get_event( POWERMGM_STANDBY | POWERMGM_SILENCE_WAKEUP ) ) {
                return( false );
            }
            /*
            * get touchstate from touchcontroller if not taken
            * by other task/thread
            */
            bool getTouchResult = false;
            if ( touch_lock_take() ) {
                getTouchResult = ttgo->getTouch( x, y );
                touch_lock_give();
            }
            /*
            * if touched?
            */
            if ( !getTouchResult ) {
                touch_press = false;
                return( false );
            }
            /*
            * wibe if touched
            */
            if ( !touch_press ) {
                touch_press = true;
                if ( display_get_vibe() )
                    motor_vibe( 3 );
            }
        #elif defined( LILYGO_WATCH_2021 )
            static bool touchState;
            touchState = TouchSensor.TouchInt();

            if ( touchState == Press ) {
                TouchSensor.ReadTouch();
                x = TouchSensor.getX();
                y = TouchSensor.getY();
                touched = true;
            }
            else {
                touched = false;
                return( false );
            }
        #endif
    #endif
    return( true );
}

static bool touch_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
    bool retval = false;
    
    #ifdef NATIVE_64BIT
        retval = mouse_read( drv, data );
    #else
        #if defined( M5PAPER )
            if ( M5.TP.avaliable() ) {
                if( !M5.TP.isFingerUp() ) {
                    M5.TP.update();
                    tp_finger_t FingerItem = M5.TP.readFinger( 0 );
                    data->point.x = FingerItem.x;
                    data->point.y = FingerItem.y;
                    data->state = LV_INDEV_STATE_PR;
                }
                else {
                    M5.TP.update();
                    data->state = LV_INDEV_STATE_REL;
                }
            }
        #elif defined( M5CORE2 )
            static int16_t last_x = 0;
            static int16_t last_y = 0;

            M5.Touch.update();

            if ( M5.Touch.ispressed() ) {
                Point coordinate;
                coordinate = M5.Touch.getPressPoint();
                if ( coordinate.x == -1 || coordinate.y == -1 ) {
                    data->state = LV_INDEV_STATE_REL;
                    data->point.x = last_x;
                    data->point.y = last_y;
                }
                else {
                    data->point.x = coordinate.x;
                    data->point.y = coordinate.y;
                    last_x = coordinate.x;
                    last_y = coordinate.y;
                    data->state = LV_INDEV_STATE_PR;
                }
            }
            else {
                data->state = LV_INDEV_STATE_REL;
            }

        #elif defined( LILYGO_WATCH_2020_V1 ) || defined( LILYGO_WATCH_2020_V2 ) || defined( LILYGO_WATCH_2020_V3 )
            /*
            * We use two flags, one changes in the interrupt handler
            * the other controls whether we poll the sensor,
            * and gets cleared when the level is no longer low,
            * meaning the touch has finished
            */
            portENTER_CRITICAL( &Touch_IRQ_Mux );
            bool temp_touch_irq_flag = touch_irq_flag;
            touch_irq_flag = false;
            portEXIT_CRITICAL( &Touch_IRQ_Mux );
            touched |= temp_touch_irq_flag;
            /*
            * check for an touch interrupt
            */
            data->state = touch_getXY( data->point.x, data->point.y ) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
            touched = digitalRead( TOUCH_INT ) == LOW;
            if ( !touched ) {
                /*
                * Save power by switching to monitor mode now instead of waiting for 30 seconds.
                */
                if ( touch_lock_take() ) {
                    TTGOClass::getWatch()->touchToMonitor();
                    touch_lock_give();
                }
            }
            /*
            * issue https://github.com/sharandac/My-TTGO-Watch/issues/18 fix
            */
            float temp_x = ( data->point.x - ( lv_disp_get_hor_res( NULL ) / 2 ) ) * 1.15;
            float temp_y = ( data->point.y - ( lv_disp_get_ver_res( NULL ) / 2 ) ) * 1.0;
            data->point.x = temp_x + ( lv_disp_get_hor_res( NULL ) / 2 );
            data->point.y = temp_y + ( lv_disp_get_ver_res( NULL ) / 2 );

        #elif defined( LILYGO_WATCH_2021 )
            bool isTouch = TouchSensor.getTouchType();
            data->state = isTouch ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
            TouchSensor.ReadTouch();
            if ( TouchSensor.getX() != 0 && TouchSensor.getY() != 0 ) {
                data->point.x = TouchSensor.getX();
                data->point.y = TouchSensor.getY();
            }
        #endif
    #endif
    /**
     * send touch event
     */
    touch_t touch;
    touch.touched = data->state;
    touch.x_coor = data->point.x;
    touch.y_coor = data->point.y;
    /**
     * discard when callback return true
     */
    if ( touch_send_event_cb( TOUCH_UPDATE, (void*)&touch ) ) {
        data->state = LV_INDEV_STATE_REL;
    }

    return( retval );
}
