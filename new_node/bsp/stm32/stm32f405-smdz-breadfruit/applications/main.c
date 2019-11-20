/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "radio_hal.h"
#include "flash.h"


int main(void)
{
		/* W25Q128 ≥ı ºªØ*/
		nor_flash_init();
	
    extern void si446x_for_mcu_init(void);
    si446x_for_mcu_init();
    radio_hal_init();
    radio_software_init();
    return RT_EOK;
}
