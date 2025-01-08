/*
* Copyright 2023 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/


/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include "lvgl.h"
#include "custom.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static float temp=0.0,hum=0.0;
static int light=0;
static lv_timer_t *timer1;
/**
 * Create a demo application
 */
 void lv_custom_set_temp(float data)
 {
    temp=data;
 }
 void lv_custom_set_hum(float data)
 {
    hum=data;
 }

 void lv_custom_set_light(int data)
 {
    light=data;
 }

 int lv_custom_get_light(void)
 {
   return light;
 }

void timer1_call_back(lv_timer_t *arg)
{
    lv_ui *p=&guider_ui;
    uint32_t index=lv_tabview_get_tab_act(p->screen_1_tabview_1);
    char str[10];
    switch(index){
      case 0: {
         snprintf(str,sizeof(str),"%.2f ",temp);
         lv_label_set_text(p->screen_1_label_1,str);
         break;
      }
      case 1: {
         snprintf(str,sizeof(str),"%.2f",hum);
         lv_label_set_text(p->screen_1_label_2,str);
         break;
      }
      case 2: {
         if(light){
            int32_t value=lv_slider_get_value(guider_ui.screen_1_slider_1);
            lv_led_set_brightness(guider_ui.screen_1_led_1, value);
         }else{
            lv_led_off(p->screen_1_led_1);
         }

         break;
      }
    }
}

 void screen1_loaded_func(void)
 { 
    lv_ui*p=&guider_ui;
    timer1 = lv_timer_create(timer1_call_back,500,NULL);
    lv_led_off(p->screen_1_led_1); 
    lv_label_set_text(p->screen_1_label_2,"0.00");
    lv_label_set_text(p->screen_1_label_1,"0.00");
    lv_timer_ready(timer1);
 }

void custom_init(lv_ui *ui)
{
    /* Add your codes here */
}

