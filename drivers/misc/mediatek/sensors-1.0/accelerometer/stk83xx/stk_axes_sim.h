//#include "stdio.h"
#include "common_define.h"

typedef enum
{
    BIT_12 = 0,
    BIT_16 = 1
} bit_count;

//mapping register
typedef enum
{
    SIM_RANGE_2G = 0x03,
    SIM_RANGE_4G = 0x05,
	SIM_RANGE_8G = 0x0C
} sim_range;

void stk_axes_sim(int16_t *acc_data);
void stk_axes_sim_reset(void);
int stk_axes_sim_init(uint8_t check_count, bit_count bit, sim_range range);