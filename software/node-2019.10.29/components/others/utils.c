#include "utils.h"

/* current system status */
static system_status_t cur_status = SYSTEM_STATUS_INIT;


void set_system_status(system_status_t status) {
    cur_status = status;
}

system_status_t get_system_status(void) {
    return cur_status;
}

void delay_us(int us)
{
    us *=30;
	while(us--);
}

rt_int32_t Bubble(rt_int32_t *v, rt_int32_t num)
{
	rt_uint8_t i,j;
	rt_int32_t temp,k;
	for(j=0; j<num; j++)
	{
		for(i=0; i<num-1-j; i++)
		{
			if(v[i] > v[i+1])
			{
				temp = v[i];
				v[i] = v[i+1];
				v[i+1] = temp;
			}
		}
	}
    
    if(num%2)
    {
        k = v[num/2+1]; //取中值
    }
    else
    {
        k = (v[num/2]+v[num/2+1])/2; //取中值
    }	
    
	return k;
}