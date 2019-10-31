/******************************************************************************
* Copyright 2018-2023 zhuangwei@nblink-tech.com
* FileName: 	 drv_relay.h
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
#ifndef _DRV_RELAY_H_
#define _DRV_RELAY_H_

/*------------------------------- Includes ----------------------------------*/
#include <rtthread.h>


/*----------------------------- Global Typedefs ------------------------------*/
typedef enum
{
    RELAY_WATER = 0,
    RELAY_FEED,
    RELAY_HEAT1,
    RELAY_HEAT2,
    RELAY_MAX
}relay_dev_t;


/*----------------------------- Global Defines -----------------------------*/


/*----------------------------- External Variables --------------------------*/


/*------------------------ Global Function Prototypes -----------------------*/
int relay_control(relay_dev_t dev, int expect_status, int hold_s);


#endif // _DRV_RELAY_H_
