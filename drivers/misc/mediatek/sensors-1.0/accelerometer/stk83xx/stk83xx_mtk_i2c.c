#include "stk83xx_mtk_i2c.h"

#ifdef CONFIG_MID_ITEMS_SUPPORT
#include <mt-plat/items.h>
#endif

struct attribute_group stk_attribute_accel_group;
static struct stk_data *gStk = NULL;

#define STK_DRI_GET_DATA(ddri) \
    dev_get_drvdata((struct device *)container_of(ddri, struct device, driver))
int stk_init_flag = 0;

/*
 * @brief: Read all register (0x0 ~ 0x3F)
 *
 * @param[in/out] stk: struct stk_data *
 * @param[out] show_buffer: record all register value
 *
 * @return: buffer length or fail
 *          positive value: return buffer length
 *          -1: Fail
 */
static int stk_show_all_reg(stk_data *stk, char *show_buffer)
{
    unsigned short reg;
    int len = 0;
    int err = 0;
    u8 data = 0;

    if (NULL == show_buffer)
        return -1;

    for (reg = 0; reg <= 0x3F; reg++)
    {
        err = STK_REG_READ(stk, reg, &data);

        if (err < 0)
        {
            len = -1;
            goto exit;
        }

        if (0 >= (PAGE_SIZE - len))
        {
            STK_ACC_ERR("print string out of PAGE_SIZE");
            goto exit;
        }

        len += scnprintf(show_buffer + len, PAGE_SIZE - len,
                         "[0x%2X]=0x%2X, ", reg, data);

        if (4 == reg % 5)
        {
            len += scnprintf(show_buffer + len, PAGE_SIZE - len,
                             "\n");
        }
    }

    len += scnprintf(show_buffer + len, PAGE_SIZE - len, "\n");
exit:
    return len;
}

/**
 * @brief: Get power status
 *          Send 0 or 1 to userspace.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t enable_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;
    char en;
    en = atomic_read(&stk->enabled);
    return scnprintf(buf, PAGE_SIZE, "%d\n", en);
}

/**
 * @brief: Set power status
 *          Get 0 or 1 from userspace, then set stk832x power status.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 * @param[in] count: size_t
 *
 * @return: ssize_t
 */
static ssize_t enable_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct stk_data *stk = gStk;
    unsigned int data;
    int error;
    error = kstrtouint(buf, 10, &data);

    if (error)
    {
        STK_ACC_ERR("kstrtoul failed, error=%d", error);
        return error;
    }

    if ((1 == data) || (0 == data))
        stk_set_enable(stk, data);
    else
        STK_ACC_ERR("invalid argument, en=%d", data);

    return count;
}

/**
 * @brief: Get accel data
 *          Send accel data to userspce.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t value_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;
    bool enable = true;

    if (!atomic_read(&stk->enabled))
    {
        stk_set_enable(stk, 1);
        enable = false;
    }

    stk_read_accel_data(stk);

    if (!enable)
        stk_set_enable(stk, 0);

    return scnprintf(buf, PAGE_SIZE, "%hd %hd %hd\n",
                     stk->xyz[0], stk->xyz[1], stk->xyz[2]);
}

/**
 * @brief: Get delay value in usec
 *          Send delay in usec to userspce.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t delay_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;
    return scnprintf(buf, PAGE_SIZE, "%lld\n", (long long)stk_get_delay(stk) * 1000);
}

/**
 * @brief: Set delay value in usec
 *          Get delay value in usec from userspace, then write to register.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 * @param[in] count: size_t
 *
 * @return: ssize_t
 */
static ssize_t delay_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct stk_data *stk = gStk;
    long long data;
    int err;
    err = kstrtoll(buf, 10, &data);

    if (err)
    {
        STK_ACC_ERR("kstrtoul failed, error=%d", err);
        return err;
    }

    stk_set_delay(stk, (int)data / 1000);
    return count;
}

/**
 * @brief: Get offset value
 *          Send X/Y/Z offset value to userspace.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t offset_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;
    u8 offset[3] = {0, 0, 0};
    stk_get_offset(stk, offset);
    return scnprintf(buf, PAGE_SIZE, "0x%X 0x%X 0x%X\n",
                     offset[0], offset[1], offset[2]);
}

/**
 * @brief: Set offset value
 *          Get X/Y/Z offset value from userspace, then write to register.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 * @param[in] count: size_t
 *
 * @return: ssize_t
 */
static ssize_t offset_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct stk_data *stk = gStk;
    char *token[10];
    u8 r_offset[3];
    int err, data, i;

    for (i = 0; i < 3; i++)
        token[i] = strsep((char **)&buf, " ");

    err = kstrtoint(token[0], 16, &data);

    if (err)
    {
        STK_ACC_ERR("kstrtoint failed, error=%d", err);
        return err;
    }

    r_offset[0] = (u8)data;
    err = kstrtoint(token[1], 16, &data);

    if (err)
    {
        STK_ACC_ERR("kstrtoint failed, error=%d", err);
        return err;
    }

    r_offset[1] = (u8)data;
    err = kstrtoint(token[2], 16, &data);

    if (err)
    {
        STK_ACC_ERR("kstrtoint failed, error=%d", err);
        return err;
    }

    r_offset[2] = (u8)data;
    STK_ACC_LOG("offset=0x%X, 0x%X, 0x%X", r_offset[0], r_offset[1], r_offset[2]);
    stk_set_offset(stk, r_offset);
    return count;
}

/**
 * @brief: Register writting
 *          Get address and content from userspace, then write to register.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 * @param[in] count: size_t
 *
 * @return: ssize_t
 */
static ssize_t send_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct stk_data *stk = gStk;
    char *token[10];
    int addr, cmd, err, i;
    bool enable = false;

    for (i = 0; i < 2; i++)
        token[i] = strsep((char **)&buf, " ");

    err = kstrtoint(token[0], 16, &addr);

    if (err)
    {
        STK_ACC_ERR("kstrtoint failed, err=%d", err);
        return err;
    }

    err = kstrtoint(token[1], 16, &cmd);

    if (err)
    {
        STK_ACC_ERR("kstrtoint failed, err=%d", err);
        return err;
    }

    STK_ACC_LOG("write reg[0x%X]=0x%X", addr, cmd);

    if (!atomic_read(&stk->enabled))
        stk_set_enable(stk, 1);
    else
        enable = true;

    if (STK_REG_WRITE(stk, (u8)addr, (u8)cmd))
    {
        err = -1;
        goto exit;
    }

exit:

    if (!enable)
        stk_set_enable(stk, 0);

    if (err)
        return -1;

    return count;
}

/**
 * @brief: Read stk_data.recv(from stk_recv_store), then send to userspace.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t recv_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;
    return scnprintf(buf, PAGE_SIZE, "0x%X\n", stk->recv);
}

/**
 * @brief: Get the read address from userspace, then store the result to
 *          stk_data.recv.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 * @param[in] count: size_t
 *
 * @return: ssize_t
 */
static ssize_t recv_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct stk_data *stk = gStk;
    int addr, err;
    bool enable = false;
    err = kstrtoint(buf, 16, &addr);

    if (err)
    {
        STK_ACC_ERR("kstrtoint failed, error=%d", err);
        return err;
    }

    if (!atomic_read(&stk->enabled))
        stk_set_enable(stk, 1);
    else
        enable = true;

    err = STK_REG_READ(stk, (u8)addr, &stk->recv);

    if (err < 0)
    {
        goto exit;
    }

    STK_ACC_LOG("read reg[0x%X]=0x%X", addr, stk->recv);
exit:

    if (!enable)
        stk_set_enable(stk, 0);

    if (err)
        return -1;

    return count;
}

/**
 * @brief: Read all register value, then send result to userspace.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t allreg_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;
    int result;
    result = stk_show_all_reg(stk, buf);

    if (0 > result)
        return result;

    return (ssize_t)result;
}

/**
 * @brief: Check PID, then send chip number to userspace.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t chipinfo_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;

    switch (stk->pid)
    {
        case STK8BA50_R_ID:
            return scnprintf(buf, PAGE_SIZE, "stk8ba50-r\n");

        case STK8BA53_ID:
            return scnprintf(buf, PAGE_SIZE, "stk8ba53\n");

        case STK8323_ID:
            return scnprintf(buf, PAGE_SIZE, "stk8321/8323\n");

        case STK8327_ID:
            return scnprintf(buf, PAGE_SIZE, "stk8327\n");

        case STK8329_ID:
            return scnprintf(buf, PAGE_SIZE, "stk8329\n");

        default:
            return scnprintf(buf, PAGE_SIZE, "unknown\n");
    }

    return scnprintf(buf, PAGE_SIZE, "unknown\n");
}

/**
 * @brief: Read FIFO data, then send to userspace.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t fifo_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;
    u8 fifo_wm = 0;
    u8 frame_unit = 0;
    int fifo_len, len = 0, err = 0;

    if (!stk->fifo)
    {
        return scnprintf(buf, PAGE_SIZE, "No fifo data\n");
    }

    err = STK_REG_READ(stk, STK83XX_REG_FIFOSTS, &fifo_wm);

    if (err < 0)
        return scnprintf(buf, PAGE_SIZE, "fail to read FIFO cnt\n");

    fifo_wm &= STK83XX_FIFOSTS_FIFO_FRAME_CNT_MASK;

    if (0 == fifo_wm)
        return scnprintf(buf, PAGE_SIZE, "no fifo data yet\n");

    err = STK_REG_READ(stk, STK83XX_REG_CFG2, &frame_unit);

    if (err < 0)
        return scnprintf(buf, PAGE_SIZE, "fail to read FIFO\n");

    frame_unit &= STK83XX_CFG2_FIFO_DATA_SEL_MASK;

    if (0 == frame_unit)
        fifo_len = fifo_wm * 6; /* xyz * 2 bytes/axis */
    else
        fifo_len = fifo_wm * 2; /* single axis * 2 bytes/axis */

    {
        u8 *fifo = NULL;
        int i;
        /* vzalloc: allocate memory and set to zero. */
        fifo = vzalloc(sizeof(u8) * fifo_len);

        if (!fifo)
        {
            STK_ACC_ERR("memory allocation error");
            return scnprintf(buf, PAGE_SIZE, "fail to read FIFO\n");
        }

        stk_fifo_reading(stk, fifo, fifo_len);

        for (i = 0; i < fifo_wm; i++)
        {
            if (0 == frame_unit)
            {
                s16 x, y, z;
                x = fifo[i * 6 + 1] << 8 | fifo[i * 6];
                x >>= 4;
                y = fifo[i * 6 + 3] << 8 | fifo[i * 6 + 2];
                y >>= 4;
                z = fifo[i * 6 + 5] << 8 | fifo[i * 6 + 4];
                z >>= 4;
                len += scnprintf(buf + len, PAGE_SIZE - len,
                                 "%dth x:%d, y:%d, z:%d\n", i, x, y, z);
            }
            else
            {
                s16 xyz;
                xyz = fifo[i * 2 + 1] << 8 | fifo[i * 2];
                xyz >>= 4;
                len += scnprintf(buf + len, PAGE_SIZE - len,
                                 "%dth fifo:%d\n", i, xyz);
            }

            if ( 0 >= (PAGE_SIZE - len))
            {
                STK_ACC_ERR("print string out of PAGE_SIZE");
                break;
            }
        }

        vfree(fifo);
    }
    return len;
}

/**
 * @brief: Read water mark from userspace, then send to register.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 * @param[in] count: size_t
 *
 * @return: ssize_t
 */
static ssize_t fifo_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct stk_data *stk = gStk;
    int wm, err;

    if (!stk->fifo)
    {
        STK_ACC_ERR("not support fifo");
        return count;
    }

    err = kstrtoint(buf, 10, &wm);

    if (err)
    {
        STK_ACC_ERR("kstrtoint failed, error=%d", err);
        return err;
    }

    if (stk_change_fifo_status(stk, (u8)wm))
    {
        return -1;
    }

    return count;
}

/**
 * @brief: Show self-test result.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t selftest_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;
    u8 result = atomic_read(&stk->selftest);

    if (STK_SELFTEST_RESULT_NA == result)
        return scnprintf(buf, PAGE_SIZE, "No result\n");

    if (STK_SELFTEST_RESULT_RUNNING == result)
        return scnprintf(buf, PAGE_SIZE, "selftest is running\n");
    else if (STK_SELFTEST_RESULT_NO_ERROR == result)
        return scnprintf(buf, PAGE_SIZE, "No error\n");
    else
        return scnprintf(buf, PAGE_SIZE, "Error code:0x%2X\n", result);
}

/**
 * @brief: Do self-test.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 * @param[in] count: size_t
 *
 * @return: ssize_t
 */
static ssize_t selftest_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct stk_data *stk = gStk;
    stk_selftest(stk);
    return count;
}

/**
 * @brief: Get range value
 *          Send range to userspce.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t range_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;

    switch (stk->sensitivity)
    {
        //2G , for 16it , 12 bit
        // 2^16 / 2x2 , 2^12/2x2
        case 16384:
        case 1024:
            return scnprintf(buf, PAGE_SIZE, "2\n");

        //4G , for 16it , 12 bit,10bit
        //2^16 / 2x4 , 2^12/2x4, 2^10/2x4
        case 8192:
        case 512:
        case 128:
            return scnprintf(buf, PAGE_SIZE, "4\n");

        //8G , for 16it ,10bit
        //2^16 / 2x8 , 2^10/2x8
        case 4096:
        case 64:
            return scnprintf(buf, PAGE_SIZE, "8\n");

        //2G, 10bit  ,8G,12bit
        //2^10/2x2 , 2^12/2x8
        case 256:
            if (stk->pid == STK8BA50_R_ID)
                return scnprintf(buf, PAGE_SIZE, "2\n");
            else
                return scnprintf(buf, PAGE_SIZE, "8\n");

        //16G , for 16it
        //2^16 / 2x16
        case 2048:
            return scnprintf(buf, PAGE_SIZE, "16\n");

        default:
            return scnprintf(buf, PAGE_SIZE, "unkown\n" );
    }

    return scnprintf(buf, PAGE_SIZE, "unkown\n");
}

/**
 * @brief: Set range value
 *         Get range value from userspace, then write to register.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 * @param[in] count: size_t
 *
 * @return: ssize_t
 */
static ssize_t range_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct stk_data *stk = gStk;
    long long data;
    int err;
    stk_rangesel range;
    err = kstrtoll(buf, 10, &data);

    if (err)
    {
        STK_ACC_ERR("kstrtoul failed, error=%d", err);
        return err;
    }

    if (stk->pid != STK8327_ID)
    {
        if (data == 16)
        {
            STK_ACC_LOG(" This chip not support 16G,auto switch to 8G");
            data = 8;
        }
    }

    switch (data)
    {
        case 2:
        default:
            range = STK83XX_RANGESEL_2G;
            break;

        case 4:
            range = STK83XX_RANGESEL_4G;
            break;

        case 8:
            range = STK83XX_RANGESEL_8G;
            break;

        case 16:
            range = STK83XX_RANGESEL_16G;
            break;
    }

    stk_range_selection(stk, range);
    return count;
}

#ifdef STK_CALI
/**
 * @brief: Get calibration status
 *          Send calibration status to userspace.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t cali_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;
    int result = atomic_read(&stk->cali_status);
#ifdef STK_CALI_FILE

    if (STK_K_RUNNING != result)
    {
        stk_get_cali(stk);
    }

#endif /* STK_CALI_FILE */
    return scnprintf(buf, PAGE_SIZE, "0x%X\n", result);
}

/**
 * @brief: Trigger to calculate calibration data
 *          Get 1 from userspace, then start to calculate calibration data.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 * @param[in] count: size_t
 *
 * @return: ssize_t
 */
static ssize_t cali_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct stk_data *stk = gStk;

    if (sysfs_streq(buf, "1"))
    {
        stk_set_cali(stk);
    }
    else
    {
        STK_ACC_ERR("invalid value %d", *buf);
        return -EINVAL;
    }

    return count;
}
#endif /* STK_CALI */

#ifdef STK_HW_STEP_COUNTER
/**
 * @brief: Read step counter data, then send to userspace.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t stepcount_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;

    if (STK8323_ID != stk->pid)
    {
        STK_ACC_ERR("not support step counter");
        return scnprintf(buf, PAGE_SIZE, "Not support\n");
    }

    stk_read_step_data(stk);
    return scnprintf(buf, PAGE_SIZE, "%d\n", stk->steps);
}

/**
 * @brief: Read step counter settins from userspace, then send to register.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 * @param[in] count: size_t
 *
 * @return: ssize_t
 */
static ssize_t stepcount_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct stk_data *stk = gStk;
    int step, err;

    if (STK8323_ID != stk->pid)
    {
        STK_ACC_ERR("not support step counter");
        return count;
    }

    err = kstrtoint(buf, 10, &step);

    if (err)
    {
        STK_ACC_ERR("kstrtoint failed, err=%d", err);
        return err;
    }

    if (step)
        stk_turn_step_counter(stk, true);
    else
        stk_turn_step_counter(stk, false);

    return count;
}
#endif /* STK_HW_STEP_COUNTER */

#ifdef STK_FIR
/**
 * @brief: Get FIR parameter, then send to userspace.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 *
 * @return: ssize_t
 */
static ssize_t firlen_show(struct device_driver *ddri, char *buf)
{
    struct stk_data *stk = gStk;
    int len = stk->data_log_len;

    if (len)
    {
        STK_ACC_LOG("FIR count=%2d, idx=%2d", stk->data_log.count, stk->data_log.idx);
        STK_ACC_LOG("sum = [\t%d \t%d \t%d]",
                    stk->data_log.sum[0], stk->data_log.sum[1], stk->data_log.sum[2]);
        STK_ACC_LOG("avg = [\t%d \t%d \t%d]",
                    stk->data_log.sum[0] / len, stk->data_log.sum[1] / len, stk->data_log.sum[2] / len);
    }

    return scnprintf(buf, PAGE_SIZE, "%d\n", len);
}

/**
 * @brief: Get FIR length from userspace, then write to stk_data.fir_len.
 *
 * @param[in] ddri: struct device_driver *
 * @param[in/out] buf: char *
 * @param[in] count: size_t
 *
 * @return: ssize_t
 */
static ssize_t firlen_store(struct device_driver *ddri, const char *buf, size_t count)
{
    struct stk_data *stk = gStk;
    int firlen, err;
    err = kstrtoint(buf, 10, &firlen);

    if (err)
    {
        STK_ACC_ERR("kstrtoint failed, error=%d", err);
        return err;
    }

    if (STK_DATA_LOG_LEN_MAX < firlen)
    {
        STK_ACC_ERR("maximum FIR length is %d", STK_DATA_LOG_LEN_MAX);
    }
    else
    {
        memset(&stk->data_log, 0, sizeof(struct data_log));
        stk->data_log_len =  firlen;
    }

    return count;
}
#endif /* STK_FIR */

static DRIVER_ATTR_RW(enable);
static DRIVER_ATTR_RO(value);
static DRIVER_ATTR_RW(delay);
static DRIVER_ATTR_RW(offset);
static DRIVER_ATTR_WO(send);
static DRIVER_ATTR_RW(recv);
static DRIVER_ATTR_RO(allreg);
static DRIVER_ATTR_RO(chipinfo);
static DRIVER_ATTR_RW(fifo);
static DRIVER_ATTR_RW(selftest);
static DRIVER_ATTR_RW(range);
#ifdef STK_CALI
    static DRIVER_ATTR_RW(cali);
#endif /* STK_CALI */
#ifdef STK_HW_STEP_COUNTER
    static DRIVER_ATTR_RW(stepcount);
#endif /* STK_HW_STEP_COUNTER */
#ifdef STK_FIR
    static DRIVER_ATTR_RW(firlen);
#endif /* STK_FIR */

static struct driver_attribute *stk_attr_list[] =
{
    &driver_attr_enable,
    &driver_attr_value,
    &driver_attr_delay,
    &driver_attr_offset,
    &driver_attr_send,
    &driver_attr_recv,
    &driver_attr_allreg,
    &driver_attr_chipinfo,
    &driver_attr_fifo,
    &driver_attr_selftest,
    &driver_attr_range,
#ifdef STK_CALI
    &driver_attr_cali,
#endif /* STK_CALI */
#ifdef STK_HW_STEP_COUNTER
    &driver_attr_stepcount,
#endif /* STK_HW_STEP_COUNTER */
#ifdef STK_FIR
    &driver_attr_firlen,
#endif /* STK_FIR */
};

static int gsensor_get_data(int *x, int *y, int *z, int *status)
{
    stk83xx_wrapper *stk_wrapper = container_of(gStk, stk83xx_wrapper, stk);
    struct stk_data *stk = &stk_wrapper->stk;
    int x_data, y_data, z_data, err;
    int acc[3] = {0};
    char buff[256];
    stk_read_accel_data(stk);
    sprintf(buff, "%04x %04x %04x", stk->xyz[0], stk->xyz[1], stk->xyz[2]);
    /* remap coordinate */
    acc[stk_wrapper->cvt.map[0]] = stk_wrapper->cvt.sign[0] * stk->xyz[0];
    acc[stk_wrapper->cvt.map[1]] = stk_wrapper->cvt.sign[1] * stk->xyz[1];
    acc[stk_wrapper->cvt.map[2]] = stk_wrapper->cvt.sign[2] * stk->xyz[2];
    /* Output for mg */
    x_data = (acc[0] * GRAVITY_EARTH_1000 ) / stk->sensitivity;
    y_data = (acc[1] * GRAVITY_EARTH_1000 ) / stk->sensitivity;
    z_data = (acc[2] * GRAVITY_EARTH_1000 ) / stk->sensitivity;
//    STK_ACC_LOG("report x = %d, y = %d, z = %d\n", acc[0], acc[1], acc[2]);
    sprintf(buff, "%04x %04x %04x", x_data, y_data, z_data);
    err = sscanf(buff, "%x %x %x", x, y, z);

    if (3 != err)
    {
        STK_ACC_ERR("Invalid argument");
        return -EINVAL;
    }

    *status = SENSOR_STATUS_ACCURACY_MEDIUM;
    return 0;
}


/**
 * @brief:
 */
static int stk_create_attr(struct device_driver *driver)
{
    int err = 0;
    int i, num = (int)(ARRAY_SIZE(stk_attr_list));

    if (NULL == driver)
    {
        STK_ACC_ERR("Cannot find driver");
        return -EINVAL;
    }

    for (i = 0; i < num; i++)
    {
        err = driver_create_file(driver, stk_attr_list[i]);

        if (err)
        {
            STK_ACC_ERR("driver_create_file (%s) = %d",
                        stk_attr_list[i]->attr.name, err);
            break;
        }
    }

    return err;
}

static void stk_remove_attr(struct device_driver *driver)
{
    int i, num = (int)(ARRAY_SIZE(stk_attr_list));

    if (NULL == driver)
    {
        STK_ACC_ERR("Cannot find driver");
        return;
    }

    for (i = 0; i < num; i++)
    {
        driver_remove_file(driver, stk_attr_list[i]);
    }
}

static int stk_readCalibration(int *dat)
{
    struct stk_data *stk = gStk;
    STK_ACC_LOG("ori x:%d, y:%d, z:%d", stk->cali_sw[0], stk->cali_sw[1], stk->cali_sw[2]);
    dat[0] = stk->cali_sw[0];
    dat[1] = stk->cali_sw[1];
    dat[2] = stk->cali_sw[2];
    return 0;
}

static int stk_writeCalibration(int *dat)
{
    struct stk_data *stk = gStk;
    int err = 0;
    int cali[3];
    err = stk_readCalibration(cali);
    STK_ACC_LOG("raw cali_sw[%d][%d][%d] dat[%d][%d][%d]",
                cali[0], cali[1], cali[2], dat[0], dat[1], dat[2]);
    cali[0] += dat[0];
    cali[1] += dat[1];
    cali[2] += dat[2];
    stk->cali_sw[0] = cali[0];
    stk->cali_sw[1] = cali[1];
    stk->cali_sw[2] = cali[2];
    STK_ACC_LOG("new cali_sw[%d][%d][%d]",
                stk->cali_sw[0], stk->cali_sw[1], stk->cali_sw[2]);
    mdelay(1);
    return err;
}

/**
 * @brief: Open data rerport to HAL.
 *      refer: drivers/misc/mediatek/accelerometer/inc/accel.h
 */
static int gsensor_open_report_data(int open)
{
    /* TODO. should queuq work to report event if  is_report_input_direct=true */
    return 0;
}

/**
 * @brief: Only enable not report event to HAL.
 *      refer: drivers/misc/mediatek/accelerometer/inc/accel.h
 */
static int gsensor_enable_nodata(int en)
{
    struct stk_data *stk = gStk;

    if (en)
    {
        stk_set_enable(stk, 1);
        atomic_set(&stk->enabled, 1);
    }
    else
    {
        stk_set_enable(stk, 0);
        atomic_set(&stk->enabled, 0);
    }

    STK_ACC_LOG("enabled is %d", en);
    return 0;
}

/**
 * @brief:
 */
static int gsensor_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
    STK_ACC_FUN();
    return 0;
}

/**
 * @brief:
 */
static int gsensor_flush(void)
{
    STK_ACC_FUN();
    //    return -1; /* error */
    return acc_flush_report();
}

/**
 * @brief:
 */
static int gsensor_set_delay(u64 delay_ns)
{
    struct stk_data *stk = gStk;
    STK_ACC_LOG("delay= %d ms", (int)delay_ns / 1000);
    stk_set_delay(stk, (int)delay_ns / 1000);
    return 0;
}

/* struct accel_factory_fops */
static int stk_factory_enable_sensor(bool enable, int64_t sample_ms)
{
    int en = (true == enable ? 1 : 0);

    if (gsensor_enable_nodata(en))
    {
        STK_ACC_ERR("enable sensor failed");
        return -1;
    }

    return 0;
}


/* struct accel_factory_fops */
static int stk_factory_get_data(int32_t data[3], int *status)
{
    return gsensor_get_data(&data[0], &data[1], &data[2], status);
}

/* struct accel_factory_fops */
static int stk_factory_get_raw_data(int32_t data[3])
{
    struct stk_data *stk = gStk;
    stk_read_accel_rawdata(stk);
    data[0] = (int32_t)stk->xyz[0];
    data[1] = (int32_t)stk->xyz[1];
    data[2] = (int32_t)stk->xyz[2];
    return 0;
}

/* struct accel_factory_fops */
static int stk_factory_enable_cali(void)
{
#ifdef STK_CALI
    struct stk_data *stk = gStk;
    stk_set_cali(stk);
#endif /* STK_CALI */
    return 0;
}

/* struct accel_factory_fops */
static int stk_factory_clear_cali(void)
{
    struct stk_data *stk = gStk;
#ifdef STK_CALI
    stk_reset_cali(stk);
#endif /* STK_CALI */
    memset(stk->cali_sw, 0x0, sizeof(stk->cali_sw));
    return 0;
}

/* struct accel_factory_fops */
static int stk_factory_set_cali(int32_t data[3])
{
    int err = 0;
    struct stk_data *stk = gStk;
    int cali[3] = {0, 0, 0};
#ifdef STK_CALI_FILE
    u8 xyz[3] = {0, 0, 0};
    atomic_set(&stk->cali_status, STK_K_RUNNING);
#endif /* STK_CALI_FILE */
    cali[0] = data[0] * stk->sensitivity / GRAVITY_EARTH_1000;
    cali[1] = data[1] * stk->sensitivity / GRAVITY_EARTH_1000;
    cali[2] = data[2] * stk->sensitivity / GRAVITY_EARTH_1000;
    STK_ACC_LOG("new x:%d, y:%d, z:%d", cali[0], cali[1], cali[2]);
#ifdef STK_CALI_FILE
    xyz[0] = (u8)cali[0];
    xyz[1] = (u8)cali[1];
    xyz[2] = (u8)cali[2];
    /* write cali to file */
    err = stk_write_cali_to_file(stk, xyz, STK_K_SUCCESS_FILE);

    if (err)
    {
        STK_ACC_ERR("failed to stk_write_cali_to_file, err=%d", err);
        return -1;
    }

#endif /* STK_CALI_FILE */
    err = stk_writeCalibration(cali);

    if (err)
    {
        STK_ACC_ERR("stk_writeCalibration failed!");
        return -1;
    }

#ifdef STK_CALI
    atomic_set(&stk->cali_status, STK_K_SUCCESS_FILE);
#endif /* STK_CALI */
    return 0;
}

/* struct accel_factory_fops */
static int stk_factory_get_cali(int32_t data[3])
{
    struct stk_data *stk = gStk;
    data[0] = (int32_t)(stk->cali_sw[0] * GRAVITY_EARTH_1000 / stk->sensitivity);
    data[1] = (int32_t)(stk->cali_sw[1] * GRAVITY_EARTH_1000 / stk->sensitivity);
    data[2] = (int32_t)(stk->cali_sw[2] * GRAVITY_EARTH_1000 / stk->sensitivity);
    STK_ACC_LOG("x:%d, y:%d, z:%d", data[0], data[1], data[2]);
    return 0;
}

/* struct accel_factory_fops */
static int stk_factory_do_self_test(void)
{
    struct stk_data *stk = gStk;
    stk_selftest(stk);

    if (STK_SELFTEST_RESULT_NO_ERROR == atomic_read(&stk->selftest))
        return 0;
    else
        return -1;
}

static struct accel_factory_fops stk_factory_fops =
{
    .enable_sensor = stk_factory_enable_sensor,
    .get_data = stk_factory_get_data,
    .get_raw_data = stk_factory_get_raw_data,
    .enable_calibration = stk_factory_enable_cali,
    .clear_cali = stk_factory_clear_cali,
    .set_cali = stk_factory_set_cali,
    .get_cali = stk_factory_get_cali,
    .do_self_test = stk_factory_do_self_test,
};

static struct accel_factory_public stk_factory_device =
{
    .gain = 1,
    .sensitivity = 1,
    .fops = &stk_factory_fops,
};


/*
 * @brief:
 *
 * @param[in/out] stk: struct stk_data *
 *
 * @return:
 *      0: Success
 *      others: Fail
 */
static int stk_init_mtk(stk83xx_wrapper *stk_wrapper)
{
    int err = 0;
    struct acc_control_path stk_acc_control_path = { 0 };
    struct acc_data_path stk_acc_data_path = { 0 };
    err = stk_create_attr(&stk_acc_init_info.platform_diver_addr->driver);

    if (err)
    {
        STK_ACC_ERR("Fail in stk_create_attr, err=%d", err);
        return -1;
    }

    stk_acc_control_path.is_use_common_factory = false;
    err = accel_factory_device_register(&stk_factory_device);

    if (err)
    {
        STK_ACC_ERR("Fail in accel_factory_device_register, err=%d", err);
        accel_factory_device_deregister(&stk_factory_device);
        return -1;
    }

    stk_acc_control_path.open_report_data = gsensor_open_report_data;
    stk_acc_control_path.enable_nodata = gsensor_enable_nodata;
    stk_acc_control_path.is_support_batch = false;
    stk_acc_control_path.batch = gsensor_batch;
    stk_acc_control_path.flush = gsensor_flush;
    stk_acc_control_path.set_delay = gsensor_set_delay;
    stk_acc_control_path.is_report_input_direct = false;
    err = acc_register_control_path(&stk_acc_control_path);

    if (err)
    {
        STK_ACC_ERR("acc_register_control_path fail");
        accel_factory_device_deregister(&stk_factory_device);
        stk_remove_attr(&stk_acc_init_info.platform_diver_addr->driver);
        return -1;
    }

    stk_acc_data_path.get_data = gsensor_get_data;
    stk_acc_data_path.vender_div = 1000;
    err = acc_register_data_path(&stk_acc_data_path);

    if (err)
    {
        STK_ACC_ERR("acc_register_data_path fail");
        accel_factory_device_deregister(&stk_factory_device);
        stk_remove_attr(&stk_acc_init_info.platform_diver_addr->driver);
        return -1;
    }

    return 0;
}

/*
 * @brief: Exit mtk related settings safely.
 *
 * @param[in/out] stk: struct stk_data *
 */
static void stk_exit_mtk(struct stk83xx_wrapper *stk_wrapper)
{
    accel_factory_device_deregister(&stk_factory_device);
    stk_remove_attr(&stk_acc_init_info.platform_diver_addr->driver);
}

void read_data_callback(struct stk_data *stk)
{
    int ii = 0;
    s16 coor_trans[3] = {0};
    STK_ACC_LOG("xyz before coordinate trans %d %d %d with direction:%d",
                stk->xyz[0], stk->xyz[1], stk->xyz[2], stk->direction);

    for (ii = 0; ii < 3; ii++)
    {
        coor_trans[0] += stk->xyz[ii] * coordinate_trans[stk->direction][0][ii];
        coor_trans[1] += stk->xyz[ii] * coordinate_trans[stk->direction][1][ii];
        coor_trans[2] += stk->xyz[ii] * coordinate_trans[stk->direction][2][ii];
    }

    stk->xyz[0] = coor_trans[0];
    stk->xyz[1] = coor_trans[1];
    stk->xyz[2] = coor_trans[2];
    STK_ACC_LOG("xyz after coordinate trans %d %d %d",
                stk->xyz[0], stk->xyz[1], stk->xyz[2]);
}

/**
 * @brief: Probe function for i2c_driver.
 *
 * @param[in] client: struct i2c_client *
 * @param[in] stk_bus_ops: const struct stk_bus_ops *
 *
 * @return: Success or fail
 *          0: Success
 *          others: Fail
 */
int stk_i2c_probe(struct i2c_client *client, struct common_function *common_fn)
{
    int err = 0;
#ifdef STK_INTERRUPT_MODE
    u32 ints[2] = {0, 0};
#endif // STK_INTERRUPT_MODE
#ifdef CONFIG_MID_ITEMS_SUPPORT
	int orient=0;
#endif
    stk83xx_wrapper *stk_wrapper;
    struct stk_data *stk;
    STK_ACC_LOG("VERSION_STK83XX: 0x%x ", VERSION_STK83XX);

    if (NULL == client)
    {
        return -ENOMEM;
    }
    else if (NULL == common_fn)
    {
        STK_ACC_ERR("cannot get common function. EXIT");
        return -EIO;
    }

    /* kzalloc: allocate memory and set to zero. */
    stk_wrapper = kzalloc(sizeof(stk83xx_wrapper), GFP_KERNEL);

    if (!stk_wrapper)
    {
        STK_ACC_ERR("memory allocation error");
        return -ENOMEM;
    }

    stk = &stk_wrapper->stk;

    if (!stk)
    {
        printk(KERN_ERR "%s: failed to allocate stk83xx_wrapper\n", __func__);
        return -ENOMEM;
    }

    STK_ACC_LOG("init data");
    gStk = stk;
    stk_wrapper->i2c_mgr.client = client;
    stk_wrapper->i2c_mgr.addr_type = ADDR_8BIT;
    stk->bops   = common_fn->bops;
    stk->tops   = common_fn->tops;
    stk->gops   = common_fn->gops;
    stk->sops   = common_fn->sops;
    stk->read_data_cb = NULL;
    stk->accel_report_cb = NULL;
    stk->read_fifo_cb = NULL;
    stk->fifo_report_cb = NULL;
    i2c_set_clientdata(client, stk_wrapper);
    mutex_init(&stk_wrapper->i2c_mgr.lock);
    stk->bus_idx = stk->bops->init(&stk_wrapper->i2c_mgr);
    STK_ACC_ERR("bus_idx = %d\n", stk->bus_idx);

    if (stk->bus_idx < 0)
    {
        STK_ACC_ERR("bus_idx < 0");
        goto err_free_mem;
    }

    err = get_accel_dts_func(client->dev.of_node, &stk_wrapper->hw);
	client->addr = 0x18;
#ifdef STK_INTERRUPT_MODE
    of_property_read_u32_array(client->dev.of_node, "interrupts", ints, ARRAY_SIZE(ints));
    STK_ACC_ERR("int[0] = %d, int[1] = %d \n", ints[0], ints[1]);
    stk->gpio_info.int_pin = ints[0];
#endif // STK_INTERRUPT_MODE

    if (err)
    {
        STK_ACC_ERR("DTS info fail");
        goto err_free_mem;
    }
    stk_register_queue(stk);
#ifdef CONFIG_MID_ITEMS_SUPPORT
	orient = item_integer("sensor.accelerometer.bma.orientation",0);
	orient = orient < 0 ? 0 : orient;
	stk_wrapper->hw.direction = orient;
	printk("stk83xx=%d\n", stk_wrapper->hw.direction);
#endif
    /* direction */
    err = hwmsen_get_convert(stk_wrapper->hw.direction, &stk_wrapper->cvt);

    if (err)
    {
        STK_ACC_ERR("Invalid direction: %d", stk_wrapper->hw.direction);
        goto err_free_mem;
    }

    err = stk_get_pid(stk);

    if (err)
        goto err_free_mem;

    STK_ACC_LOG("PID 0x%x", stk->pid);
    stk_data_initialize(stk);

    if (stk_reg_init(stk, STK83XX_RANGESEL_DEF, stk->sr_no))
    {
        STK_ACC_ERR("stk83xx initialization failed");
        goto err_free_mem;
    }

    if (stk_init_mtk(stk_wrapper))
    {
        STK_ACC_ERR("stk_init_mtk failed");
        goto err_free_mem;
    }

    STK_ACC_LOG("Success");
    stk_init_flag = 0;
    return 0;
    //stk_exit_mtk(stk);
err_free_mem:
    stk->bops->remove(&stk_wrapper->i2c_mgr);
    mutex_destroy(&stk_wrapper->i2c_mgr.lock);
    kfree(stk_wrapper);
    stk_init_flag = -1;
    return err;
}

/*
 * @brief: Remove function for i2c_driver.
 *
 * @param[in] client: struct i2c_client *
 *
 * @return: 0
 */
int stk_i2c_remove(struct i2c_client *client)
{
    stk83xx_wrapper *stk_wrapper = i2c_get_clientdata(client);
    struct stk_data *stk = &stk_wrapper->stk;
    stk_exit_mtk(stk_wrapper);
    stk->bops->remove(&stk_wrapper->i2c_mgr);
    mutex_destroy(&stk_wrapper->i2c_mgr.lock);
    kfree(stk_wrapper);
    stk_init_flag = -1;
    return 0;
}

/*
 * @brief: Suspend function for dev_pm_ops.
 *
 * @param[in] dev: struct device *
 *
 * @return: 0
 */
int stk83xx_suspend(struct device *dev)
{
    stk83xx_wrapper *stk_wrapper = dev_get_drvdata(dev);
    struct stk_data *stk = &stk_wrapper->stk;

    if (atomic_read(&stk->enabled))
    {
        stk_set_enable(stk, 0);
        stk->temp_enable = true;
    }
    else
        stk->temp_enable = false;

    return 0;
}

/*
 * @brief: Resume function for dev_pm_ops.
 *
 * @param[in] dev: struct device *
 *
 * @return: 0
 */
int stk83xx_resume(struct device *dev)
{
    stk83xx_wrapper *stk_wrapper = dev_get_drvdata(dev);
    struct stk_data *stk = &stk_wrapper->stk;

    if (stk->temp_enable)
        stk_set_enable(stk, 1);

    stk->temp_enable = false;
    return 0;
}
