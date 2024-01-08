/****************************************************************************
 *   TTGO Watch 2020 Bluebox 
 *                 linuxthor   
 *      RIP Onkel Dittemeyer
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
#include <TTGO.h>

#include "bluebox_app.h"
#include "bluebox_app_main.h"

#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/mainbar.h"
#include "gui/statusbar.h"
#include "gui/widget_factory.h"
#include "gui/widget_styles.h"

#include "utils/alloc.h"
#include "hardware/sound.h"
#include "hardware/blectl.h"

lv_obj_t *bluebox_app_main_tile;
lv_obj_t *label; 

int held = 0;

static lv_style_t labelstyle;
lv_style_t bluebox_app_main_style;

LV_FONT_DECLARE(LCD_32px);
lv_font_t *label_font = &LCD_32px;

lv_task_t * _bluebox_app_task;

char *outsound; 

void sound_mf_task_run(char *str)
{
    int x;
    
    x = strlen(str) + 1;
    outsound =  (char *)MALLOC(x);
    memset(outsound, 0, x);
    strncpy(outsound,str,strlen(str));

    xTaskCreate     (   mf_app_task,                                    /* Function to implement the task */
                        "MF Task",                                      /* Name of the task */
                        2048,                                           /* Stack size in words */
            (void *)outsound,                                           /* Task input parameter */
                           1,                                           /* Priority of the task */
                        NULL);
}

void replay_handler(char *str)
{
    sound_mf_task_run(str);
}

void label_update(const char *str)
{
    if(!held)
    {
        lv_label_set_text(label, str);
        replay_handler(lv_label_get_text(label));
        return; 
    }
    lv_label_ins_text(label, LV_LABEL_POS_LAST, str); 
    return; 
}

static void event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) 
     {
        const char * txt = lv_btnmatrix_get_active_btn_text(obj);

        switch(txt[0]) 
        {
            case '1':
                label_update("1");
                break; 
            case '2':
                label_update("2");
                break; 
            case '3':
                label_update("3");
                break; 
            case '4':
                label_update("4");
                break; 
            case '5':
                label_update("5");
                break; 
            case '6':
                label_update("6");
                break; 
            case '7':
                label_update("7");
                break; 
            case '8':
                label_update("8");
                break; 
            case '9': 
                label_update("9");
                break; 
            case '0':
                label_update("0");
                break; 
            // KP
            case 'K':
                label_update("\\");
                break;
            // ST
            case 'S':
                label_update("/");
                break; 
            // " 2600 "
            case ' ':
                label_update("^");
                break;
            // Hold
            case 'H':
                held = !held;
                if(held == 0)
                {
                    int len = strlen(lv_label_get_text(label)); 
                    if(len >= 1)
                        replay_handler(lv_label_get_text(label));    
                } 
                //lv_label_set_text(label, "");
                break; 
            // Exit
            case 'E':
                mainbar_jump_to_maintile( LV_ANIM_OFF );
                break;
        }
    }
}

static const char * btnm_map[] = {"1", "2", "3", "\n",
                                  "4", "5", "6", "\n",
                                  "7", "8", "9", "\n", 
                                  "KP","0", "ST","\n",
                                  " 2600 ", "Hold", "\n", 
                                  "Exit", ""
                                 };

void bluebox_app_main_setup( uint32_t tile_num ) 
{
    bluebox_app_main_tile = mainbar_get_tile_obj( tile_num );
    lv_style_copy( &bluebox_app_main_style, APP_STYLE );

    lv_style_copy( &labelstyle, ws_get_mainbar_style());
    lv_style_set_text_font( &labelstyle, LV_STATE_DEFAULT, label_font );
    lv_style_set_outline_width(&labelstyle, LV_STATE_DEFAULT, 2);
    lv_style_set_outline_color(&labelstyle, LV_STATE_DEFAULT, LV_COLOR_BLUE);
    lv_style_set_outline_pad(&labelstyle, LV_STATE_DEFAULT, 8);

    label = lv_label_create(bluebox_app_main_tile, NULL);
    lv_obj_align(label, bluebox_app_main_tile, LV_ALIGN_IN_TOP_LEFT, 10, 15);
    lv_obj_add_style( label, LV_OBJ_PART_MAIN, &labelstyle ); 
    lv_label_set_long_mode(label, LV_LABEL_LONG_CROP);
    lv_obj_set_width(label, 220);
    lv_label_set_text(label, "");
    //lv_label_set_text(label, "READY");

    lv_obj_t * btnm1 = lv_btnmatrix_create(bluebox_app_main_tile, NULL);
    lv_btnmatrix_set_map(btnm1, btnm_map);
    lv_btnmatrix_set_btn_ctrl(btnm1, 13, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_obj_set_size(btnm1, 240,185);
    lv_obj_align(btnm1, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_obj_set_event_cb(btnm1, event_handler);
}


