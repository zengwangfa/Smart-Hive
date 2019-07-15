/******************************************************************************
* Copyright 2018-2023 zhuangwei@nblink-tech.com
* FileName: 	 drv_hx711.c
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

/*------------------------------- Includes ----------------------------------*/
#define LOG_TAG                        "drv_hx711"

#include <rtdevice.h>
#include "utils.h"
#include "drv_hx711.h"
#include <elog.h>
#include "multi_button.h"
#include "easyflash.h"
#include <stdio.h>
#include <stdlib.h>
/*------------------- Global Definitions and Declarations -------------------*/


/*---------------------- Constant / Macro Definitions -----------------------*/
#define FLASH_INIT_ADDRESS  (4096)	 //起始地址

#define HX711_DT1_PIN               10
#define HX711_SCK1_PIN              11

#define HX711_DT2_PIN               14
#define HX711_SCK2_PIN              15

#define HX711_LED_PIN               55
#define HX711_KEY_PIN               28

#define FLASH_KEY_PIN               29

#define HX711_DT(pin)               rt_pin_read(pin)
#define HX711_SCK_HIGH(pin)         rt_pin_write(pin, PIN_HIGH)
#define HX711_SCK_LOW(pin)          rt_pin_write(pin, PIN_LOW)

#define HX711_LED_ON                rt_pin_write(HX711_LED_PIN, PIN_HIGH)
#define HX711_LED_OFF               rt_pin_write(HX711_LED_PIN, PIN_LOW)


/*----------------------- Type Declarations ---------------------------------*/


/*----------------------- Variable Declarations -----------------------------*/
static struct button btn,btn2;

uint32_t var_min[2]={999999999,999999999};  //比这个值小 记为归一化最小值  -2147483648～+2141483647

float weight = 0;
float side_weight[2];

double Normalized[2]={0}; //归一化系数 float最多精确至 小数后六位，因此用double
rt_int32_t Reverse_Normalized[2] = {0}; //归一化倒数 便于保存于flash

static int key_status = 0;

/*----------------------- Function Prototype --------------------------------*/


/*----------------------- Function Implement --------------------------------*/

int flash_read_normalized(void)
{
    char string[100];

    ef_port_read(FLASH_INIT_ADDRESS+0,(rt_int32_t *)&Reverse_Normalized[0],4);	    
    ef_port_read(FLASH_INIT_ADDRESS+4,(rt_int32_t *)&Reverse_Normalized[1],4);	

    ef_port_read(FLASH_INIT_ADDRESS+8 ,&var_min[0],4);	
    ef_port_read(FLASH_INIT_ADDRESS+12,&var_min[1],4);
    
    Normalized[0] = (double)1 / Reverse_Normalized[0] ;
    Normalized[1] = (double)1 / Reverse_Normalized[1]  ;
    
    //数据打包成string型 
    sprintf(string,"Normalized*100: %f  , %f ",Normalized[0]*100,Normalized[1]*100);
    log_i(string);
    
    sprintf(string,"var_min:%ld  , %ld ",var_min[0],var_min[1]);
    log_i(string);
    return 0;
}


void flash_writer_normalized(void)
{
    ef_port_erase(FLASH_INIT_ADDRESS,4); //先擦后写  擦除的为一个扇区4096 Byte
     
    ef_port_write(FLASH_INIT_ADDRESS+0,(uint32_t *)&Reverse_Normalized[0],4);//从FLASH_INIT_ADDRESS+0个地址处写入数据
    ef_port_write(FLASH_INIT_ADDRESS+4,(uint32_t *)&Reverse_Normalized[1],4);	
    
    ef_port_write(FLASH_INIT_ADDRESS+8 ,&var_min[0],4);	
    ef_port_write(FLASH_INIT_ADDRESS+12,&var_min[1],4);
}

rt_uint32_t hx711_read(int pin_dat, int pin_sck)
{
    unsigned char i;
    unsigned int value;
    
    HX711_SCK_LOW(pin_sck);//使能AD
    value=0;
    
    while(HX711_DT(pin_dat));//AD转换未结束则等待,否则开始读取
    for(i=0; i<24; i++)
    {
        HX711_SCK_HIGH(pin_sck);//SCK 置高(发送脉冲)
        value = value<<1;//下降沿来临时变量Count左移一位,右则补零
        HX711_SCK_LOW(pin_sck);//SCK 拉低
        if(HX711_DT(pin_dat)) value++;
    }
    HX711_SCK_HIGH(pin_sck);
    value = value^0x800000;//第25个脉冲下降沿来时,转换数据完成
    HX711_SCK_LOW(pin_sck);
    
    return value;
}

static void first_hx711_get_ad(rt_int32_t *ad)//初始化快速获取重量
{
    rt_int32_t var0[5];
    rt_int32_t var1[5];
    rt_uint8_t i = 0;
    
    if(!ad) return;
    
    for(i=0; i<5; i++)
    {
         rt_thread_delay(RT_TICK_PER_SECOND/10);//等待100ms采集一次
         var0[i] = hx711_read(HX711_DT1_PIN, HX711_SCK1_PIN);
         var1[i] = hx711_read(HX711_DT2_PIN, HX711_SCK2_PIN);
    }
    ad[0] = Bubble(var0, 5);
    ad[1] = Bubble(var1, 5);
}

static void hx711_get_ad(rt_int32_t *ad)
{
    rt_int32_t var0[5];
    rt_int32_t var1[5];
    uint8_t i = 0;
    
    if(!ad) return;
    
    for(i=0; i<5; i++)
    {
         var0[i] = hx711_read(HX711_DT1_PIN, HX711_SCK1_PIN);
         var1[i] = hx711_read(HX711_DT2_PIN, HX711_SCK2_PIN);
         //rt_kprintf("var: %d  , %d\n",var0[i],var1[i]);
    }

    ad[0] = Bubble(var0, 5);
    ad[1] = Bubble(var1, 5);
    //rt_kprintf("ad: %d  , %d\n",ad[0],ad[1]);
}

void hx711_get_min(void)
{
    rt_int32_t temp_res[2]={0};
    uint8_t j = 5;
      

    while(j--)
    {
         hx711_get_ad(temp_res);
         rt_kprintf("temp_res: %ld  , %ld \n",temp_res[0],temp_res[1]);
         
         if(var_min[0] >= temp_res[0] || var_min[0] < 800000 ) {//采集最小值  若var_min小于800000则 数值异常,var_min替换
            var_min[0] = temp_res[0];
         }
         if(var_min[1] >= temp_res[1] || var_min[1] < 800000 ) { 
            var_min[1] = temp_res[1];
         }
         rt_kprintf("var_min: %ld  , %ld\n",var_min[0],var_min[1]);
    }

    log_i("var_min: %ld, %ld ",var_min[0],var_min[1]);
}

void hx711_normalized(void)
{
    rt_int32_t temp[2]={0};
    
    hx711_get_ad(temp);

    Reverse_Normalized[0]= abs(temp[0]-var_min[0]);
    Reverse_Normalized[1] = abs(temp[1]-var_min[1]);
    
    Normalized[0] =(double) 1/(temp[0]-var_min[0]);
    Normalized[1] =(double) 1/(temp[1]-var_min[1]);
    
    flash_writer_normalized();//写入归一化倒数

    log_i("FLASH Write Reverse_Normalized: %d, %d ",Reverse_Normalized[0],Reverse_Normalized[1]);
}

float hx711_weight_get(void)
{
    float ret;
    
    rt_enter_critical();
    ret = weight;
    rt_exit_critical();
    
    return ret;
}
float hx711_weight_update(void)
{
    rt_int32_t AD_Value[2] = {0};
    static uint8_t count = 0;
    static float last_weight = 0;
    static char ON_OFF = 0;
    while(1)
    {
        if(0 == ON_OFF){
            first_hx711_get_ad(AD_Value);
            ON_OFF = 1;
        }
        else{
            hx711_get_ad(AD_Value);
        }
        side_weight[0] = (float)abs(AD_Value[0]-var_min[0])/abs(Reverse_Normalized[0]);
        side_weight[1] = (float)abs(AD_Value[1]-var_min[1])/abs(Reverse_Normalized[1]);
         
        //rt_kprintf("--- %ld --- %ld ---\n",abs(AD_Value[0]-var_min[0]),abs(AD_Value[1]-var_min[1]));
        if ( ((side_weight[0] > 50 || side_weight[1] > 50) || (side_weight[0] < 0 || side_weight[1] < 0))  && count <= 5) //超过设备最大限制50kg
        {
            count ++;
            //rt_kprintf("count = %d \n",count);
            continue;// 重新读取 最多读取6次 
        } 
        else if(count > 5){ //若重量异常
           count = 0; 
           rt_kprintf("last_weight = %d \n",last_weight);
           return last_weight;//返回上一次的值
        }
        last_weight = (side_weight[0]+side_weight[1])/2;

        return (side_weight[0]+side_weight[1])/2;   //真实重量 
    }
}

void hx711_key_callback(void *btn)
{
    uint32_t btn_event_val; 
    
    btn_event_val = get_button_event((struct button *)btn); 
    
    if(btn_event_val == SINGLE_CLICK)
    {
        switch(key_status)
        {
            case 0://初始化状态
                hx711_get_min();
                HX711_LED_ON;
                key_status = 1;
                log_i("please put 1KG");
                break;
            case 1://已获取最小值
                hx711_normalized();//归一化
                
                HX711_LED_OFF;
                key_status = 0;
                log_i("hx711 normalized");
                break;
            default:
                break;
        }
    }
}
void flash_key_callback(void *btn)
{
    uint32_t btn_event_val; 
    
    btn_event_val = get_button_event((struct button *)btn); 
    
    if(btn_event_val == SINGLE_CLICK)
    {
       ef_port_erase(FLASH_INIT_ADDRESS,4); //擦除FLASH
    }
}

static uint8_t hx711_key_read(void) 
{
    return rt_pin_read(HX711_KEY_PIN); 
}

static uint8_t flash_key_read(void) 
{
    return rt_pin_read(FLASH_KEY_PIN); 
}

void hx711_key_thread_entry(void* p)
{
    int cnt = 200*55;
    
    while(1)
    {
        /* 5ms */
        rt_thread_delay(RT_TICK_PER_SECOND/200); 
        button_ticks(); 
        cnt++;
        if(cnt == 200*60)//60s单片机内 更新重量值
        {
            rt_enter_critical();
            weight = hx711_weight_update();
            rt_exit_critical();
            cnt = 0;
        }
    }
}

static int hx711_init(void)
{
    rt_pin_mode(HX711_DT1_PIN, PIN_MODE_INPUT_PULLUP);  
    rt_pin_mode(HX711_SCK1_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(HX711_SCK1_PIN, PIN_HIGH);

    rt_pin_mode(HX711_DT2_PIN, PIN_MODE_INPUT_PULLUP);  
    rt_pin_mode(HX711_SCK2_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(HX711_SCK2_PIN, PIN_HIGH);
    
    rt_pin_mode(HX711_LED_PIN, PIN_MODE_OUTPUT);
    HX711_LED_OFF;
    
    rt_pin_mode(HX711_KEY_PIN, PIN_MODE_INPUT); 
    button_init(&btn, hx711_key_read, PIN_LOW);
    button_attach(&btn, SINGLE_CLICK, hx711_key_callback);
    button_start (&btn);
    
    rt_pin_mode(FLASH_KEY_PIN, PIN_MODE_INPUT); 
    button_init(&btn2, flash_key_read, PIN_LOW);
    button_attach(&btn2, SINGLE_CLICK, flash_key_callback);
    button_start (&btn2);  
    
    rt_thread_t thread = RT_NULL;
    
    /* Create background ticks thread */
    thread = rt_thread_create("hx711", hx711_key_thread_entry, RT_NULL, 1024, 30, 10);
    if(thread == RT_NULL)
    {
        return RT_ERROR; 
    }

    rt_thread_startup(thread);
    
    return 0;
}
INIT_DEVICE_EXPORT(hx711_init);
/*---------------------------------------------------------------------------*/

/* Get weight 调试用 */
void get_weight_hx711(void)
{
    //数据打包成string型 
    char str[100];
    float temp_weight = 0;
    
    temp_weight = hx711_weight_update();
    
    sprintf(str,"Weight:%f \n",temp_weight);
    rt_kprintf(str);
    
    sprintf(str,"weight1:%f   weight2:%f\n",side_weight[0],side_weight[1]);
    rt_kprintf(str);
}
MSH_CMD_EXPORT(get_weight_hx711,get_weight_hx711[a]);

