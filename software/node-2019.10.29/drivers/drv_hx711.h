/******************************************************************************
* Copyright 2018-2023 zhuangwei@nblink-tech.com
* FileName: 	 drv_hx711.h
* Desc:
*
*
* Author: 	 zhuangwei
* Date: 	 2018-12-19
* Notes:
*
* -----------------------------------------------------------------
* Histroy: v1.0   2018-12-19, zhuangwei create this file
*
******************************************************************************/
#ifndef _DRV_HX711_H_
#define _DRV_HX711_H_

/*------------------------------- Includes ----------------------------------*/
#include <rtthread.h>


/*----------------------------- Global Typedefs ------------------------------*/



/*----------------------------- Global Defines -----------------------------*/


/*----------------------------- External Variables --------------------------*/


/*------------------------ Global Function Prototypes -----------------------*/
float hx711_weight_get(void);

/* Get weight µ˜ ‘”√ */
void get_weight_hx711(void);
#endif // _DRV_HX711_H_
