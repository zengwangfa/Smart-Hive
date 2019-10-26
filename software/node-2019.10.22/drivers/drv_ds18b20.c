/******************************************************************************
* Copyright 2018-2023 zhuangwei@nblink-tech.com
* FileName: 	 drv_ds18b20.c
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
#include <rtdevice.h>
#include "utils.h"
#include "drv_ds18b20.h"

/*------------------- Global Definitions and Declarations -------------------*/


/*---------------------- Constant / Macro Definitions -----------------------*/
#define DS18B20_PIN             54

/*----------------------- Type Declarations ---------------------------------*/


/*----------------------- Variable Declarations -----------------------------*/
static int temp_value = 0;

/*----------------------- Function Prototype --------------------------------*/


/*----------------------- Function Implement --------------------------------*/
/******************************************************************************
* Name: 	 ds18b20_rst
*
* Desc: 	 复位DS18B20
* Param:     无
* Return:    
* Global:
* Note:
* Author: 	 zhuangwei
* -------------------------------------
* Log: 	 2018/12/15, Create this function by zhuangwei
 ******************************************************************************/
static rt_uint8_t ds18b20_rst(void)	   
{
    rt_pin_mode(DS18B20_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(DS18B20_PIN, PIN_LOW);
    delay_us(750);
    rt_pin_write(DS18B20_PIN, PIN_HIGH);
	delay_us(20);
    rt_pin_mode(DS18B20_PIN, PIN_MODE_INPUT);
    delay_us(100);
    return rt_pin_read(DS18B20_PIN);
}

/******************************************************************************
* Name: 	 ds18b20_read_byte
*
* Desc: 	 读取一字节数据
* Param:     无
* Return:    DS18B20一字节数据
* Global:
* Note:
* Author: 	 zhuangwei
* -------------------------------------
* Log: 	 2018/12/15, Create this function by zhuangwei
 ******************************************************************************/
static rt_uint8_t ds18b20_read_byte(void)    // read one byte
{        
    rt_uint8_t i;
    rt_uint8_t data = 0;

    for (i=0; i<8; i++) 
    {
        rt_pin_mode(DS18B20_PIN, PIN_MODE_OUTPUT);
        rt_pin_write(DS18B20_PIN, PIN_LOW);
        delay_us(2);
        rt_pin_write(DS18B20_PIN, PIN_HIGH);
        rt_pin_mode(DS18B20_PIN, PIN_MODE_INPUT);
        delay_us(12);
        data >>=  1;
        if(rt_pin_read(DS18B20_PIN))
        {
            data |= 0x80;
        }
        delay_us(50);
        rt_pin_mode(DS18B20_PIN, PIN_MODE_OUTPUT);
        rt_pin_write(DS18B20_PIN, PIN_HIGH);
    }						    
    return data;
}

/******************************************************************************
* Name: 	 ds18b20_write_byte
*
* Desc: 	 写一字节数据
* Param:     要写的数据
* Return:    无
* Global:
* Note:
* Author: 	 zhuangwei
* -------------------------------------
* Log: 	 2018/12/15, Create this function by zhuangwei
 ******************************************************************************/
static void ds18b20_write_byte(rt_uint8_t data)     
{             
    rt_uint8_t i,tmp;
    
	rt_pin_mode(DS18B20_PIN, PIN_MODE_OUTPUT);
    for (i=1; i<=8; i++) 
    {
        tmp = data&0x01;
        data = data>>1;
        if(tmp)
        {
            rt_pin_write(DS18B20_PIN, PIN_LOW);
            delay_us(5);
            rt_pin_write(DS18B20_PIN, PIN_HIGH);
            delay_us(60);
        }
        else 
        {
            rt_pin_write(DS18B20_PIN, PIN_LOW);
            delay_us(60);
            rt_pin_write(DS18B20_PIN, PIN_HIGH);
            delay_us(5);
        }
    }
}

/******************************************************************************
* Name: 	 ds18b20_get_temp
*
* Desc: 	 获取ds18b20温度值
* Param:     无
* Return:    温度值
* Global:
* Note:
* Author: 	 zhuangwei
* -------------------------------------
* Log: 	 2018/12/15, Create this function by zhuangwei
 ******************************************************************************/
static int ds18b20_get_value(void)
{
    rt_uint8_t TL,TH;
    rt_uint16_t temp;
    float ret;
    
    if(ds18b20_rst()) return 0;
    delay_us(100);
    ds18b20_write_byte(0xCC);//skip rom
    ds18b20_write_byte(0x44);//convert
    //rt_thread_delay(1);
    if(ds18b20_rst()) return 0;
    delay_us(100);
    ds18b20_write_byte(0xCC);// skip rom
    ds18b20_write_byte(0xBE);
    delay_us(10);
    TL = ds18b20_read_byte();
    TH = ds18b20_read_byte();
	    	  
    temp = (TH<<8)|TL;
    if(temp&0xf000)
    {
        ret = (~temp+1)*0.0625;
    }
    else
    {
        ret = temp*0.0625;
    }
    
    return (int)ret;
}

int ds18b20_get_temp(void)
{
    int ret;
    
    rt_enter_critical();
    ret = temp_value;
    rt_exit_critical();
    
    return ret;
}


static void ds18b20(void)
{
    rt_uint8_t count;
    rt_uint16_t temp;
    
    for(count=1; count<=3; count++)
    {     
        temp =ds18b20_get_temp();  
        rt_kprintf("temperature : %d\r\n", temp);
        rt_thread_mdelay(500);
    }
}
MSH_CMD_EXPORT(ds18b20, ds18b20 test);

void first_get_temperature(void)
{
    char j = 0;
    rt_int32_t value[5] = {0};
      /* 500ms */
    rt_thread_delay(RT_TICK_PER_SECOND/2); 
    rt_enter_critical();
    for(j=0; j<5; j++)
    {
        rt_thread_delay(10);
        value[j] = ds18b20_get_value();
    }
    
    temp_value = Bubble(value, 5);
    rt_exit_critical();  
}
rt_int32_t value[5] = {0};
void ds18b20_thread_entry(void* p)
{
    char i = 0;
    char ON_OFF = 0;  //自锁开关

    rt_int32_t last_temp = 0;
    
    while(1)
    {

        if(ON_OFF == 0){
            first_get_temperature();//初始化快速获取温度值
            ON_OFF = 1;//自锁开关置1
        }
        else{//而后每间隔5*10=50s 更新一次温度值，以防延时打断通讯
          
            rt_thread_delay(RT_TICK_PER_SECOND*60);//等待0.2s采集一次【该为测试 ，真实为10s】

            value[i++] = ds18b20_get_value();
            if(i >= 5){//采集5次
                temp_value = Bubble(value, 5);
                i = 0; 
                
                if(temp_value > -10 && temp_value < 55){ //如果采集的为较为正确数值
                    last_temp = temp_value;              //保存上一次温度值
                }
                else if(abs(temp_value-last_temp)>5 && last_temp != 0 )//如果上一次与这一次温度差为5度 且  上一次温度不为0
                {
                    temp_value = last_temp;//当前值替换成 上一次温度值
                }
                //rt_kprintf("temp:$%d\r\n",temp_value);

            }

        }
            
        
             
    }
}
//void ds18b20_thread_entry(void* p)//
//{
//    int i = 0;
//    int value[5] = {0};
//    static rt_int32_t last_temp = 0;
//    
//    while(1)
//    {
//        /* 500ms */
//        rt_thread_delay(RT_TICK_PER_SECOND/2); 
//        rt_enter_critical();
//        for(i=0; i<5; i++)
//        {
//            rt_thread_delay(1);
//            value[i] = ds18b20_get_value();
//        }
//
//        temp_value = Bubble((rt_int32_t *)value, 5);
//        if(temp_value > -20 && temp_value < 70){ //如果采集的为较为正确数值
//            last_temp = temp_value;              //保存上一次温度值
//        }
//        else {
//            temp_value = last_temp;//否则，当前值替换成 上一次温度值
//        }
//        rt_exit_critical();      
//    }
//}

static int ds18b20_init(void)
{
    rt_thread_t thread = RT_NULL;
    
    /* Create background ticks thread */
    thread = rt_thread_create("ds18b20", ds18b20_thread_entry, RT_NULL, 1024, 30, 10);
    if(thread == RT_NULL)
    {
        return RT_ERROR; 
    }
    rt_thread_startup(thread);
    
    return 0;
}
INIT_DEVICE_EXPORT(ds18b20_init);
/*---------------------------------------------------------------------------*/