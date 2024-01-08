/****************************************************************************
 *   TTGO Watch 2020 Silver box 
 *                 linuxthor   
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

#include "silverbox_app.h"
#include "silverbox_app_main.h"

#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/mainbar.h"
#include "gui/statusbar.h"
#include "gui/widget_factory.h"
#include "gui/widget_styles.h"

#include "utils/alloc.h"
#include "hardware/sound.h"
#include "hardware/blectl.h"

lv_obj_t *silverbox_app_main_tile;
lv_obj_t *silver_label; 

int silver_held = 0;

static lv_style_t *style;
static lv_style_t silver_labelstyle;
lv_style_t silverbox_app_main_style;

LV_FONT_DECLARE(LCD_32px);
lv_font_t *silver_label_font = &LCD_32px;

lv_task_t * _silverbox_app_task;

char *silver_outsound; 

void sound_dtmf_task_run(char *str)
{
    int x;
    
    x = strlen(str) + 1;
    silver_outsound = (char *)MALLOC(x);
    memset(silver_outsound, 0, x);
    strncpy(silver_outsound,str,strlen(str));

    xTaskCreate     (   dtmf_app_task,                                  /* Function to implement the task */
                        "DTMF Task",                                    /* Name of the task */
                        2048,                                           /* Stack size in words */
     (void *)silver_outsound,                                           /* Task input parameter */
                           1,                                           /* Priority of the task */
                        NULL);
}

void silverplay_handler(char *str)
{
    sound_dtmf_task_run(str);
}

void silverlabel_update(const char *str)
{
    if(!silver_held)
    {
        lv_label_set_text(silver_label, str);
        silverplay_handler(lv_label_get_text(silver_label));
        return; 
    }
    lv_label_ins_text(silver_label, LV_LABEL_POS_LAST, str); 
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
                silverlabel_update("1");
                break; 
            case '2':
                silverlabel_update("2");
                break; 
            case '3':
                silverlabel_update("3");
                break; 
            case '4':
                silverlabel_update("4");
                break; 
            case '5':
                silverlabel_update("5");
                break; 
            case '6':
                silverlabel_update("6");
                break; 
            case '7':
                silverlabel_update("7");
                break; 
            case '8':
                silverlabel_update("8");
                break; 
            case '9': 
                silverlabel_update("9");
                break; 
            case '0':
                silverlabel_update("0");
                break;
            case 'A':
                silverlabel_update("A"); 
                break; 
            case 'B':
                silverlabel_update("B");
                break; 
            case 'C':
                silverlabel_update("C");
                break; 
            case 'D':
                silverlabel_update("D");
                break; 
            case '*':
                silverlabel_update("*");
                break; 
            case '#':
                silverlabel_update("#");
                break; 

            // Hold
            case 'H':
                silver_held = !silver_held;
                if(silver_held == 0)
                {
                    int len = strlen(lv_label_get_text(silver_label)); 
                    if(len >= 1)
                      silverplay_handler(lv_label_get_text(silver_label));    
                } 
                break; 
            // Exit
            case 'E':
                mainbar_jump_to_maintile( LV_ANIM_OFF );
                break;
        }
    }
}

static const char * btnm_map[] = {"1", "2", "3", "A", "\n",
                                  "4", "5", "6", "B", "\n",
                                  "7", "8", "9", "C", "\n", 
                                  "*", "0", "#", "D", "\n",
                                  "Exit", "Hold", ""
                                 };

void silverbox_app_main_setup( uint32_t tile_num ) 
{
    silverbox_app_main_tile = mainbar_get_tile_obj( tile_num );
    lv_style_copy( &silverbox_app_main_style, APP_STYLE );

    lv_style_copy( &silver_labelstyle, ws_get_mainbar_style());
    lv_style_set_text_font( &silver_labelstyle, LV_STATE_DEFAULT, silver_label_font );
    lv_style_set_outline_width(&silver_labelstyle, LV_STATE_DEFAULT, 2);
    lv_style_set_outline_color(&silver_labelstyle, LV_STATE_DEFAULT, LV_COLOR_RED);
    lv_style_set_outline_pad(&silver_labelstyle, LV_STATE_DEFAULT, 8);

    silver_label = lv_label_create(silverbox_app_main_tile, NULL);
    lv_obj_align(silver_label, silverbox_app_main_tile, LV_ALIGN_IN_TOP_LEFT, 10, 15);
    lv_obj_add_style( silver_label, LV_OBJ_PART_MAIN, &silver_labelstyle ); 
    lv_label_set_long_mode(silver_label, LV_LABEL_LONG_CROP);
    lv_obj_set_width(silver_label, 220);
    lv_label_set_text(silver_label, "");

    lv_obj_t * btnmsilv = lv_btnmatrix_create(silverbox_app_main_tile, NULL);
    lv_btnmatrix_set_map(btnmsilv, btnm_map);
    lv_btnmatrix_set_btn_ctrl(btnmsilv, 17, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_obj_set_size(btnmsilv, 240,185);
    lv_obj_align(btnmsilv, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_obj_set_event_cb(btnmsilv, event_handler);
}


