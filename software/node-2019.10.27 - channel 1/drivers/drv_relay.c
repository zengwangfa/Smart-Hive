/******************************************************************************
* Copyright 2018-2023 zhuangwei@nblink-tech.com
* FileName: 	 drv_relay.c
* Desc:
*
*
* Author: 	 zhuangwei
* Date: 	 2018-12-15
* Notes:
*
* -----------------------------------------------------------------
* Histroy: v1.0   2018-12-15, zhuangwei create this file
*
******************************************************************************/

/*------------------------------- Includes ----------------------------------*/
#define LOG_TAG                        "drv_relay"

#include <rtdevice.h>
#include "drv_relay.h"
#include <elog.h>

/*------------------- Global Definitions and Declarations -------------------*/
typedef struct
{
    char *name;
    rt_base_t pin;
    rt_timer_t timer;
    int status;
}relay_t;

/*---------------------- Constant / Macro Definitions -----------------------*/
static relay_t relay_table[] = {
    {"water", 62, RT_NULL, 0},
    {"feed", 61, RT_NULL, 0},
    {"heat1", 4, RT_NULL, 0},
    {"heat2", 9, RT_NULL, 0}
};
/*----------------------- Type Declarations ---------------------------------*/


/*----------------------- Variable Declarations -----------------------------*/


/*----------------------- Function Prototype --------------------------------*/


/*----------------------- Function  Implement --------------------------------*/
static void timeout_handler(void* parameter)
{
    relay_t *relay_ptr = (relay_t*)parameter;
    
    relay_ptr->status = relay_ptr->status?0:1;
    rt_pin_write(relay_ptr->pin, relay_ptr->status);
    relay_ptr->timer = RT_NULL;
}

int relay_control(relay_dev_t dev, int expect_status, int hold_s)
{
    if(dev >= RELAY_MAX) return -1;
    
    if(relay_table[dev].status == expect_status) return 0;
    if(relay_table[dev].timer)
    {
        if(relay_table[dev].status == expect_status)
            return 0;
        else
            return -1;
    }
    else
    {
        rt_pin_write(relay_table[dev].pin, expect_status);
        
        relay_table[dev].status = expect_status;
        if(hold_s > 0)
        {
            relay_table[dev].timer = rt_timer_create(relay_table[dev].name, \
                timeout_handler, &relay_table[dev], rt_tick_from_millisecond(hold_s*1000), \
                RT_TIMER_FLAG_ONE_SHOT);
            if (relay_table[dev].timer != RT_NULL) rt_timer_start(relay_table[dev].timer);
        }
    }
    
    return 0;
}

static int relay_init(void)
{
    int i;
    
    for(i=0; i<RELAY_MAX; i++)
    {
        rt_pin_mode(relay_table[i].pin, PIN_MODE_OUTPUT);
        rt_pin_write(relay_table[i].pin, relay_table[i].status);
    }
    return 0;
}
INIT_DEVICE_EXPORT(relay_init);
/*---------------------------------------------------------------------------*/