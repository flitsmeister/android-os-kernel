#include "stk_axes_sim.h"
static uint8_t check_fail_count = 0;
static uint8_t CHECK_COUNT = 3;
static uint8_t sim_status = 0;
static int16_t MAX_VAL = 0;
static int16_t MIN_VAL = 0;
static uint32_t G_VAL = 0;
static int init_success = 0;

typedef enum
{
    /* 12 bits per axis */
    MAX_12_BIT = 2047,
    MIN_12_BIT = -2048,
    /* 16 bits per axis */
    MAX_16_BIT = 32767,
    MIN_16_BIT = -32768
} bit_max_value;

typedef enum
{
    NO_SIM = 0,
    X_SIM = 0x1,
    Y_SIM = 0x2,
    Z_SIM = 0x4,
} sim_state;

static int stk_sqrt(int in)
{
    long root, bit;
    root = 0;
    for (bit = 0x40000000; bit > 0; bit >>= 2)
    {
        long trial = root + bit;
        root >>= 1;
        if (trial <= in)
        {
            root += bit;
            in -= trial;
        }
    }
    return (int)root;
}

void stk_axes_sim_reset()
{
    sim_status = NO_SIM;
    check_fail_count = 0;
}

int stk_axes_sim_init(uint8_t check_count, bit_count bit, sim_range range)
{
    init_success = 1;
    if (check_count > 0)
        CHECK_COUNT = check_count;

    switch (bit)
    {
    case BIT_12:
        MAX_VAL = MAX_12_BIT;
        MIN_VAL = MIN_12_BIT;
        switch (range)
        {
        case SIM_RANGE_2G:
            G_VAL = 1024;
            break;

        case SIM_RANGE_4G:
            G_VAL = 512;
            break;

        case SIM_RANGE_8G:
            G_VAL = 128;
            break;
        default:
            init_success = 0;
            break;
        }
        break;

    case BIT_16:
        MAX_VAL = MAX_16_BIT;
        MIN_VAL = MIN_16_BIT;
        switch (range)
        {
        case SIM_RANGE_2G:
            G_VAL = 16384;
            break;

        case SIM_RANGE_4G:
            G_VAL = 8192;
            break;

        case SIM_RANGE_8G:
            G_VAL = 4096;
            break;
        default:
            init_success = 0;
            break;
        }
        break;
        break;

    default:
        init_success = 0;
        break;
    }

    stk_axes_sim_reset();
    return init_success;
}

static void stk_check_axes_data(int16_t *acc_data)
{
    if (MAX_VAL == acc_data[0] || MIN_VAL == acc_data[0] ||
        MAX_VAL == acc_data[1] || MIN_VAL == acc_data[1] ||
        MAX_VAL == acc_data[2] || MIN_VAL == acc_data[2])
    {
        check_fail_count++;
    }
    else
    {
        check_fail_count = 0;
    }

    if (CHECK_COUNT == check_fail_count)
    {
        if (MAX_VAL == acc_data[0] ||
            MIN_VAL == acc_data[0])
            sim_status |= X_SIM;

        if (MAX_VAL == acc_data[1] ||
            MIN_VAL == acc_data[1])
            sim_status |= Y_SIM;

        if (MAX_VAL == acc_data[2] ||
            MIN_VAL == acc_data[2])
            sim_status |= Z_SIM;

        // check_fail_count = 0;
    }

    return;
}

void stk_axes_sim(int16_t *acc_data)
{
    if (init_success != 1)
        return;

    stk_check_axes_data(acc_data);

    switch (sim_status)
    {
    case X_SIM:
        acc_data[0] = stk_sqrt((G_VAL * G_VAL) - (acc_data[1] * acc_data[1]) - (acc_data[2] * acc_data[2]));
        break;

    case Y_SIM:
        acc_data[1] = stk_sqrt((G_VAL * G_VAL) - (acc_data[0] * acc_data[0]) - (acc_data[2] * acc_data[2]));
        break;

    case Z_SIM:
        acc_data[2] = stk_sqrt((G_VAL * G_VAL) - (acc_data[0] * acc_data[0]) - (acc_data[1] * acc_data[1]));
        break;

    case NO_SIM:
    default:
        // no fail or fail 2 axes
        break;
    }
}
