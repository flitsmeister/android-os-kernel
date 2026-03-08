#include "sitronix_ts.h"
#ifdef ST_REPLACE_TEST_CMD_BY_DISPLAY_ID
#include "sitronix_ts_test_id1.h"
#endif /* ST_REPLACE_TEST_CMD_BY_DISPLAY_ID */
#include <linux/export.h>




//#define SITRONIX_TEST_ST7123
#define SITRONIX_TEST_ST7121P
//#define SITRONIX_TEST_ST7123P
//#define SITRONIX_TEST_ST77921
//#define SITRONIX_TEST_ST7102

#ifdef SITRONIX_TEST_ST7123
# include "sitronix_ts_test_st7123.h"
#elif defined(SITRONIX_TEST_ST7121P)
# include "sitronix_ts_test_st7121p.h"
#elif defined(SITRONIX_TEST_ST7123P)
# include "sitronix_ts_test_st7123p.h"
#elif defined(SITRONIX_TEST_ST77921)
# include "sitronix_ts_test_st77921.h"
#elif defined(SITRONIX_TEST_ST7102)
# include "sitronix_ts_test_st7102.h"
#else
# error "Self-Test not defined!"
#endif

#define TEST_RETRY_MAX	3

//#define SITRONIX_UNIFORMITY_TEST_ENABLED

typedef struct sensing_setting {
	uint8_t	row_cnt;	//stmsg("ROW_CNT : %d\n",tMode[2]);
	uint8_t aa_unit;	//stmsg("AA_UNIT : %d\n",tMode[1]);
	uint8_t	self_unit;	//stmsg("SELF_UNIT : %d\n",tMode[5]);
	uint8_t noise_unit;	//stmsg("NOISE_UNIT : %d\n",tMode[4]);
	uint8_t	cmnc_ch_wr_en;	//stmsg("CMNC_CH_WR_EN : %d\n",tMode[7]);
	uint8_t	key;
} sensing_setting_t;

//static sensing_setting_t	sensing_setting;

//#define __RAW_DATA_DEBUG__

//#define USE_ST_SQRT
#ifdef USE_ST_SQRT
int st_sqrt(int x);
#endif

#ifdef ST_ADDRESS_MODE_WRITE_COMMAND_V2
static int get_cmd_data(AFE_CMD_T *code, uint8_t *data_buf, int data_buf_len)
{
	int len = 0;
	int i;

	//stmsg("%s: %u\n", __func__, max_len);

	if (!code) {
		return 0;
	}

	for (i = 0, len = 0; i < (data_buf_len >> 1); i++) {
		if (code[i].type == 0x02) {
			data_buf[(i*2)] = (uint8_t)((code[i].value & 0xFF00) >> 8);
			data_buf[(i*2) + 1] = (uint8_t)(code[i].value & 0x00FF);
			len += 2;
		} else {
			break;
		}
	}

	return len;
}

#if 0
int print_code(uint8_t *code, int max_len)
{
	int i, j;
	int len = 0;
	int data_offset = 0;

	for (i = 0, j = 1; i < max_len; i++, j++) {
		if (code[i] == 0x53 && code[i+1] == 0x54 && code[i+2] == 0x01) {
			data_offset = i + 6;
			len = get_data_len(&code[data_offset], max_len - data_offset);
		}

		if (i == data_offset) {
			printk("0x%02X,", (len & 0xFF00) >> 8);
			if (j%16 == 0) {
				printk("\\\n");
			}
			j++;
			printk("0x%02X,", (len & 0x00FF));
			if (j%16 == 0) {
				printk("\\\n");
			}
			j++;
			
			data_offset = -1;
		}

		printk("0x%02X,", code[i]);
		if (j%16 == 0)
			printk("\\\n");
	}

	printk("\n");

	return 0;
}
#endif //0

uint8_t cmd_data_buf[1024];
int st_address_mode_hardcode_write_v2(AFE_CMD_T *code, int max_len)		//This is for string macros.
{
	int ret = 0;
	int i;
	int nWriteCommCount=0;
	uint16_t len = 0;
	uint16_t delay = 0;
	uint32_t addr = 0;

	//stmsg("%s: max_len = %u\n", __func__, max_len);

	if (!code) {
		return 0;
	}

	//print_code(code, max_len);

	for (i = 0; i < max_len; i++) {
		switch (code[i].type) {
		case 0x01:
			addr = code[i].value & 0xFFFFFF;
			len = get_cmd_data(&code[i+1], cmd_data_buf, sizeof(cmd_data_buf));
			ret = sitronix_spi_pram_rw(false, addr, cmd_data_buf, NULL, len);
			nWriteCommCount++;
			#if 0
			stmsg("write addr: 0x%06X , len :%x ", addr, len);		
			for (j = 0; j < 2; j++) {
					printk("0x%02X, ", cmd_data_buf[j]);
				}	
			printk("\r\n");
			
			stmsg("write addr: %06X , len :%x ", addr, len);			
			if (len) {
				int j;
				stmsg("write data: ");
				for (j = 0; j < len; j++) {
					printk("0x%02X, ", cmd_data_buf[j]);
				}
				printk("\n");
			}
			#endif //0
			break;
		case 0x02:
			break;
		case 0x03:
			delay = code[i].value;
			//stmsg("delay 0x%x ms \n", delay);
			#if 0
			stmsg("delay 0x%x ms \n", delay);
			#endif //0
			msleep(delay);
			break;
		default:
			i = max_len;
			addr = code[i].value & 0xFFFFFF;
			stmsg("unknown code: 0x%x, Addr:0x%06X\n", code[i].type,addr);
			break;
		}
	}
	stmsg("%s WriteComm Time : %d times\n", __FUNCTION__,nWriteCommCount);
	return ret;
}
#endif //ST_ADDRESS_MODE_WRITE_COMMAND_V2

int st_address_mode_hardcode_write(void *pcode, int max_len)		//This is for binary array.
{
	int ret = 0;
	int index = 0;
	uint16_t key = 0;
	uint16_t len = 0;
	uint32_t addr = 0;
	uint8_t *code = (uint8_t*)pcode;

	if (!code) {
		return 0;
	}

#ifdef ST_ADDRESS_MODE_WRITE_COMMAND_V2
	ret = st_address_mode_hardcode_write_v2((AFE_CMD_T*)pcode, max_len);
	return ret;
#endif //ST_ADDRESS_MODE_WRITE_COMMAND_V2

	while (index < max_len - 1 ) {
		key = code[index] << 8 | code[index + 1];		
		if (key == 0x5354){
			index += 2;
			switch(code[index]) {
				case 0x01:
					index += 1;
					addr = code[index] << 16 | code[index + 1] << 8 | code[index + 2];
					len = code[index + 3] << 8 | code[index + 4];
					index += 5;
					ret = sitronix_spi_pram_rw(false, addr, code + index, NULL, len);
					index += len;
					//stmsg("write addr: %x , len :%x \n", addr, len);
					break;
				case 0x02:
					index += 1;
					len = code[index] << 8 | code[index + 1];
					//stmsg("delay 0x%x ms \n", len);
					msleep(len);
					index += 2;
					break;
				default:
					index = max_len;
					stmsg("unknown code: 0x%x\n", code[index]);
					break;
			}
		} else {
			index ++;
		}
	}
	return ret;
}

bool st_get_test_enable(int col, int row)
{
	int index = col * 4 + row / 8;
	unsigned char mask = 0x80 >> (row % 8);
	
	if( index < sizeof(test_disable_sensor))
		return !(test_disable_sensor[index] & mask);
	else
		return false;
}

#ifdef ST_SELFTEST_LOG_FILE
bool sitonix_createlogfileok = true; // FIH self test open log file success or not

void sitronix_vfswrite(struct file *filp, char *data1, int str_len, loff_t *ppos)
{
	if(sitonix_createlogfileok) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 14, 13)
		vfs_write(filp, data1, str_len, ppos);
#else
		kernel_write(filp, data1, str_len, ppos);
#endif
	}
}
#endif /* ST_SELFTEST_LOG_FILE */

#ifdef ST_SELFTEST_LOG_FILE
int st_open_Delta_test(char func, int skipcol, struct file *filp,loff_t *ppos)
#else
int st_open_Delta_test(char func, int skipcol)
#endif
{
	unsigned char cmd[0x08];
	int tMode[8];
	int read_len;
	int ret = 0;
	unsigned char *raw_buf;
	signed short *rawI;
	signed short *nMuxOnRaw;
	signed short *nMuxOffRaw;
	int frameCounter;
	int retryCounter;
	int max_retry = 3;
	unsigned char raw_dat_rd_on[2] = { 0x02, 0x00 };
	unsigned char raw_dat_rd_off[2] = { 0x00, 0x00 };
	unsigned char raw_header[18];
	unsigned char frame_counter = 0;
	int total_error = 0;
	int error_count = 0;
	//int count_frame = 1;
	int buf_index , aa_index;
	int nMax = -9999,nMin = 9999;
	int col , row, RawType,nRealRow,nRealCol;//,nMuxOnRaw[16*32]={0},nMuxOffRaw[16*32]={0};
#ifdef ST_SELFTEST_LOG_FILE
	char data1[150];
#endif
	
	/* ROW_CNT */
	sitronix_spi_pram_rw(true, 0xF04A, NULL, cmd, 2);
	tMode[2] = cmd[0]&0x3F;	
	nRealRow = gts->ts_dev_info.y_chs;
	tMode[2] = 32;
	/* CMNC_CH_WR_EN */
	tMode[7] = cmd[1]&0x01;
	tMode[7] = 0x00;
	/* AA_UNIT */
	sitronix_spi_pram_rw(true, 0xF024, NULL, cmd, 2);
	tMode[1] = (cmd[0]&0xF0)>>3;
	nRealCol = tMode[1];
	//tMode[1] = 16;
	nRealCol = gts->ts_dev_info.x_chs;//tMode[1];
	tMode[1] = gts->ts_dev_info.x_chs;
	/* SELF_UNIT */		
	tMode[5] = (cmd[0]&0xC)>>1;

	/* NOISE_UNIT */
	//tMode[4] = (cmd[0]&0x3)<<2;
	tMode[4] = 0x00;	//for ST7102

	/* KEY */
	tMode[3] = 0;

	stmsg("ROW_CNT : %d, RealRow : %d\n",tMode[2],nRealRow);
	stmsg("AA_UNIT : %d, RealCol : %d\n",tMode[1],nRealCol);
	stmsg("SELF_UNIT : %d\n",tMode[5]);
	stmsg("NOISE_UNIT : %d\n",tMode[4]);
	stmsg("CMNC_CH_WR_EN : %d\n",tMode[7]);

	read_len = 16*32*2;//(tMode[2]+tMode[7]) * (tMode[1] + tMode[5] + tMode[4] + skipcol) *2;
	raw_buf = kmalloc(read_len*2, GFP_KERNEL);

	rawI = (signed short *)kmalloc((16*32) * sizeof(short), GFP_KERNEL);	
	nMuxOnRaw = (signed short *)kmalloc((16*32) * sizeof(short), GFP_KERNEL);	
	nMuxOffRaw = (signed short *)kmalloc((16*32) * sizeof(short), GFP_KERNEL);	

	//Ignore
	frameCounter = 0;
	retryCounter = 0;

	while (ST_SELFTEST_IGNORE_FRAME > 0 && retryCounter++ < max_retry)
	{
		msleep(10);
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_on, NULL, 2);
		sitronix_spi_pram_rw(true, 0xF180, NULL, raw_header, 18);

		stmsg("header %x \n",raw_header[1]);
		if(frame_counter != raw_header[1])
		{
			frameCounter++;
			retryCounter = 0;
			frame_counter = raw_header[1];
		}else{
		//	ret = st_address_mode_hardcode_write(test_cmd_open_mux_on, sizeof(test_cmd_open_mux_on));			
		}

		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);

		if (frameCounter >= ST_SELFTEST_IGNORE_FRAME + 1)
			break;
	}
/*
	if( retryCounter >=  max_retry)
	{
		if(func == 0) {
			sterr("st open test fail ,  can't wait IRQ \n");
#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 50, "st open test fail ,  can't wait IRQ\n");
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
		} 
		ret = -1;
	//	goto st_open_short_test_finish;
	}
*/	
	//raw
	frameCounter = 0;
	retryCounter = 0;

	//collect Mux On Raw
	//while (count_frame > 0 && retryCounter++ < max_retry)
	//{
	//Mux On Start	
		stmsg("MuxOn Raw \n");
		msleep(10);
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_on, NULL, 2);
		sitronix_spi_pram_rw(true, 0xF180, NULL, raw_header, 18);		
		//if(frame_counter != raw_header[1])
		{
			frameCounter++;
			retryCounter = 0;
			frame_counter = raw_header[1];

			sitronix_spi_pram_rw(true, 0xD000, NULL, raw_buf, read_len*2);
			sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);

			aa_index = (tMode[1] / 2) * (tMode[2] + tMode[7]) *2;
			buf_index = (skipcol) * (tMode[2] + tMode[7]) *2;
			stmsg("aa_index = %d, buf_index = %d\n",aa_index,buf_index);
			if (skipcol != 0)
				for(col = 0; col < aa_index; col++)
					raw_buf[aa_index + col ] = raw_buf[aa_index + col + buf_index];

			buf_index = 0;
			aa_index = 0;

			//AA A
			for (col = 0; col < tMode[1]; col++)
			{
				error_count = 0;
				for (row = 0; row < tMode[2]; row++)
				{
					rawI[row] = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);
					nMuxOnRaw[col*tMode[2]+row] = rawI[row];
					buf_index +=2 ;
					//snprintf(data1, 130, "RawI[%2d,%2d]=[%4d],RawP[%4d], RawD[%4d]\n",col,row,rawI[row],rawP[row],rawD[row]);			
					//sitronix_vfswrite(filp, data1, strlen(data1), ppos);	
				}
			}
		}
	//}
	//collect Mux Off Raw and calcu Delta
	stmsg("MuxOff Raw \n");
	//sitronix_ts_reset_device(gts);
	//ret = st_address_mode_hardcode_write(test_flash_afe_df, sizeof(test_flash_afe_df));
	if (ret < 0)
		return ret;
	
	//ret = st_address_mode_hardcode_write(test_cmd_open_MuxOff, sizeof(test_cmd_open_MuxOff));
	
	stmsg("======ST7102 Open Test Write Mux off Code Start \r\n=====");
	#ifdef SITRONIX_TEST_ST7102
	ret = st_address_mode_hardcode_write_v2(test_cmd_open_MuxOff, ARRAY_SIZE(test_cmd_open_MuxOff));	
	ret = st_address_mode_hardcode_write_v2(test_cmd_channel_mapping, ARRAY_SIZE(test_cmd_channel_mapping));	
	#endif
	stmsg("======ST7102 Open Test Write Mux off Code End \r\n=====");
	msleep(10);
	frameCounter = 0;
	retryCounter = 0;
	
	while (ST_SELFTEST_IGNORE_FRAME > 0 && retryCounter++ < max_retry)
	{
		msleep(10);
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_on, NULL, 2);
		sitronix_spi_pram_rw(true, 0xF180, NULL, raw_header, 18);

		stmsg("header %x \n",raw_header[1]);
		if(frame_counter != raw_header[1])
		{
			frameCounter++;
			retryCounter = 0;
			frame_counter = raw_header[1];
		}else{
	//		ret = st_address_mode_hardcode_write(test_cmd_open_mux_off, sizeof(test_cmd_open_mux_off));
		}

		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);

		if (frameCounter >= ST_SELFTEST_IGNORE_FRAME + 1)
			break;
	}
	
	frameCounter = 0;
	retryCounter = 0;
	
	//while (count_frame > 0 && retryCounter++ < max_retry)
	//{
		msleep(10);
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_on, NULL, 2);
		sitronix_spi_pram_rw(true, 0xF180, NULL, raw_header, 18);
		//if(frame_counter != raw_header[1])
		{
			frameCounter++;
			retryCounter = 0;
			frame_counter = raw_header[1];

			sitronix_spi_pram_rw(true, 0xD000, NULL, raw_buf, read_len*2);
			sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);

			aa_index = (tMode[1] / 2) * (tMode[2] + tMode[7]) *2;
			buf_index= (skipcol) * (tMode[2] + tMode[7]) *2;

			if (skipcol != 0)
				for(col = 0; col < aa_index; col++)
					raw_buf[aa_index + col ] = raw_buf[aa_index + col + buf_index];

			buf_index = 0;
			aa_index = 0;

			//AA A
			for (col = 0; col < tMode[1]; col++)
			{
				error_count = 0;
				for (row = 0; row < tMode[2]; row++)
				{
					aa_index = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);
					nMuxOffRaw[col*tMode[2]+row] = aa_index;					
					//snprintf(data1, 130, "RawI[%2d,%2d]=[%4d],RawP[%4d], RawD[%4d] Index[%5d]\n",col,row,nMuxOnRaw[col*tMode[2]+row],nMuxOffRaw[col*tMode[2]+row],nMuxOnRaw[col*tMode[2]+row]-nMuxOffRaw[col*tMode[2]+row],col*tMode[2]+row);			
					//sitronix_vfswrite(filp, data1, strlen(data1), ppos);									
					if((col < nRealCol) && (row<nRealRow )){
						//if(st_get_test_enable(col, row) && (nMuxOnRaw[col*tMode[2]+row]-nMuxOffRaw[col*tMode[2]+row] < gts->self_test_open_min))
						if(st_get_test_enable(col, row) && (nMuxOnRaw[col*tMode[2]+row] < gts->self_test_open_min))
							error_count++;
						if(nMin>(nMuxOnRaw[col*tMode[2]+row]))	nMin =  (nMuxOnRaw[col*tMode[2]+row]);
						if(nMax<(nMuxOnRaw[col*tMode[2]+row]))	nMax = (nMuxOnRaw[col*tMode[2]+row]);
					}
					buf_index +=2 ;
				}

				if(error_count > ST_SELFTEST_ADJUST_COUNT)
				{
					for (row = 0; row < tMode[2] ; row++)
					{
						if((col<nRealCol)&&(row<nRealRow)){
							//if(st_get_test_enable(col, row) && (nMuxOnRaw[col*tMode[2]+row]-nMuxOffRaw[col*tMode[2]+row] < gts->self_test_open_min ))
							if(st_get_test_enable(col, row) && (nMuxOnRaw[col*tMode[2]+row] < gts->self_test_open_min ))
							{
								total_error++;
								sterr("sensor (%2d,%2d) RAW (%4d) out of range (%d ~ %d) in open test\n" , col, row, nMuxOnRaw[col*tMode[2]+row]-nMuxOffRaw[col*tMode[2]+row], gts->self_test_open_min, gts->self_test_open_max);
	#ifdef ST_SELFTEST_LOG_FILE
								snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) out of range (%d ~ %d) in open test\n", col, row, nMuxOnRaw[col*tMode[2]+row]-nMuxOffRaw[col*tMode[2]+row], gts->self_test_open_min, gts->self_test_open_max);
								sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	#endif
							}
						}
					}
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

			//SE A
			for (col = 0; col < tMode[5] / 2; col++)
			{
				for (row = 0; row < tMode[2]; row++)
				{
					buf_index +=2 ; 
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

		
#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 100, "[OPEN RAW Data start]\n");			
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
			buf_index = 0;
			aa_index = 0;
			//AA A
			for(RawType =2; RawType<3;RawType++){
				snprintf(data1, 100, "\n\n ");
				sitronix_vfswrite(filp, data1, strlen(data1), ppos);
				for (col = 0; col < tMode[1]; col++)
				{
					for (row = 0; row < tMode[2]; row++)
					{
						if(RawType ==0 )
							aa_index = nMuxOnRaw[col*tMode[2]+row];
						else if (RawType == 1)
							aa_index = nMuxOffRaw[col*tMode[2]+row];
						else if(RawType == 2)
							//aa_index = nMuxOnRaw[col*tMode[2]+row]-nMuxOffRaw[col*tMode[2]+row];
							aa_index = nMuxOnRaw[col*tMode[2]+row];						
	#ifdef ST_SELFTEST_LOG_FILE
						if (row == (tMode[2] - 1) )
							snprintf(data1, 100, "%6d \n ", aa_index);
						else
							snprintf(data1, 100, "%6d ", aa_index);
						sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	#endif
					}

				}
			}

#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 100, "[RAW Data end]        (Min/Max: %4d / %4d)\n",nMin,nMax);
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif			
		}
		//else
		//{
		//	sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);
		//}

		//if (frameCounter >= count_frame)
			//break;
	//}


//st_open_short_test_finish:

	kfree(rawI);
	kfree(nMuxOnRaw);
	kfree(nMuxOffRaw);
	



	if ( ret < 0)
		return ret;
	else if(total_error == 0)
		return 0;
	else
		return total_error;
}
#ifdef ST_SELFTEST_LOG_FILE
int st7102_test_open(struct file *filp,loff_t *ppos)
#else
int st7102_test_open(void)
#endif
{	
	
	int ret = 0;  
	stmsg("======ST7102 Open Test=====\r\n");
	//sitronix_ts_reset_device(gts);
	//ret = st_address_mode_hardcode_write(test_cmd_normal, sizeof(test_cmd_normal));
	//stmsg("======ST7102 Open Test Write Mux On Code Start \r\n=====");
	#ifdef ST_ADDRESS_MODE_WRITE_COMMAND_V2
	ret = st_address_mode_hardcode_write_v2(test_cmd_open_mux_on, ARRAY_SIZE(test_cmd_open_mux_on));	
	#else	
	ret = st_address_mode_hardcode_write(test_cmd_open_mux_on, sizeof(test_cmd_open_mux_on));
	#endif
	//stmsg("======ST7102 Open Test Write Mux On Code End \r\n=====");
	if (ret < 0)
		return ret;
	
#ifdef ST_SELFTEST_LOG_FILE
	ret = st_open_Delta_test(0,0,filp,ppos);	//for MuxOn Off use
#else
	ret = st_open_Delta_test(0,0);	//for MuxOn Off use
#endif

	return ret;
}


#ifdef ST_SELFTEST_LOG_FILE
int st_Normal_test(char func, int skipcol, struct file *filp,loff_t *ppos)
#else
int st_Normal_test(char func, int skipcol)
#endif
{
	unsigned char cmd[0x08];
	int tMode[8];
	int read_len;
	int ret = 0;
	unsigned char *raw_buf;
	signed short *rawI;
	signed short *nMuxOnRaw;
	int frameCounter;
	int retryCounter;
	int max_retry = 5;
	unsigned char raw_dat_rd_on[2] = { 0x02, 0x00 };
	unsigned char raw_dat_rd_off[2] = { 0x00, 0x00 };
	unsigned char raw_header[18];
	unsigned char frame_counter = 0;
	int total_error = 0;
	int error_count = 0;
	//int count_frame = 1;
	int buf_index , aa_index;
	int col , row, RawType,nRealRow,nRealCol;//,nMuxOnRaw[16*32]={0},nMuxOffRaw[16*32]={0};
#ifdef ST_SELFTEST_LOG_FILE
	char data1[150];
#endif

	/* ROW_CNT */
	sitronix_spi_pram_rw(true, 0xF04A, NULL, cmd, 2);
	tMode[2] = cmd[0]&0x3F;	
	nRealRow = gts->ts_dev_info.y_chs;
	tMode[2] = 32;
	/* CMNC_CH_WR_EN */
	tMode[7] = cmd[1]&0x01;
	tMode[7] = 0x00;
	/* AA_UNIT */
	sitronix_spi_pram_rw(true, 0xF024, NULL, cmd, 2);
	tMode[1] = (cmd[0]&0xF0)>>3;
	nRealCol = gts->ts_dev_info.x_chs;//tMode[1];
	tMode[1] = gts->ts_dev_info.x_chs;
	/* SELF_UNIT */		
	tMode[5] = (cmd[0]&0xC)>>1;

	/* NOISE_UNIT */
	//tMode[4] = (cmd[0]&0x3)<<2;
	tMode[4] = 0x00;	//for ST7102

	/* KEY */
	tMode[3] = 0;

	stmsg("ROW_CNT : %d, RealRow : %d\n",tMode[2],nRealRow);
	stmsg("AA_UNIT : %d, RealCol : %d\n",tMode[1],nRealCol);
	stmsg("SELF_UNIT : %d\n",tMode[5]);
	stmsg("NOISE_UNIT : %d\n",tMode[4]);
	stmsg("CMNC_CH_WR_EN : %d\n",tMode[7]);

	read_len = 19*32*2;//(tMode[2]+tMode[7]) * (tMode[1] + tMode[5] + tMode[4] + skipcol) *2;
	raw_buf = kmalloc(read_len*2, GFP_KERNEL);

	rawI = (signed short *)kmalloc((19*32) * sizeof(short), GFP_KERNEL);	
	nMuxOnRaw = (signed short *)kmalloc((19*32) * sizeof(short), GFP_KERNEL);	
		
	//Ignore
	frameCounter = 0;
	retryCounter = 0;
	//ST71XX_EnterAddressMode();

	while (ST_SELFTEST_IGNORE_FRAME > 0 && retryCounter++ < max_retry)
	{
		//msleep(10);
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_on, NULL, 2);
		sitronix_spi_pram_rw(true, 0xF180, NULL, raw_header, 18);

		stmsg("header %x \n",raw_header[1]);
		if(frame_counter != raw_header[1])
		{
			frameCounter++;
			retryCounter = 0;
			frame_counter = raw_header[1];
		}else{
			/*
			ST71XX_EnterAddressMode();
			ret = st_address_mode_hardcode_write(test_cmd_normal, sizeof(test_cmd_normal));
			sitronix_spi_pram_rw(true, 0xF000, NULL, raw_header, 18);		
			if(raw_header[1]==0x00){
				for(i=0;i<10;i++){
					msleep(100);
					ret = st_address_mode_hardcode_write(test_cmd_normal, sizeof(test_cmd_normal));
					sitronix_spi_pram_rw(true, 0xF000, NULL, raw_header, 18);				
					stmsg("0xF000 =  0x%02X\n",raw_header[1]);
					if(raw_header[1]==0x02){
						break;
					}
				}
				
				
			}
				*/
		}
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);

		if (frameCounter >= ST_SELFTEST_IGNORE_FRAME + 1)
			break;
	}
	
	//raw
	frameCounter = 0;
	retryCounter = 0;
	frame_counter = 0;


	stmsg("Normal Raw \n");
	msleep(10);
	sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_on, NULL, 2);
	sitronix_spi_pram_rw(true, 0xF180, NULL, raw_header, 18);
	//if(frame_counter != raw_header[1])
	{
		frameCounter++;
		retryCounter = 0;
		frame_counter = raw_header[1];

		sitronix_spi_pram_rw(true, 0xD000, NULL, raw_buf, read_len*2);
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);

		aa_index = (tMode[1] / 2) * (tMode[2] + tMode[7]) *2;
		buf_index = (skipcol) * (tMode[2] + tMode[7]) *2;
		stmsg("aa_index = %d, buf_index = %d\n",aa_index,buf_index);
		if (skipcol != 0)
			for(col = 0; col < aa_index; col++)
				raw_buf[aa_index + col ] = raw_buf[aa_index + col + buf_index];

		buf_index = 0;
		aa_index = 0;

		//AA A
		for (col = 0; col < tMode[1]; col++)
		{
			error_count = 0;
			for (row = 0; row < tMode[2]; row++)
			{
				rawI[row] = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);
				nMuxOnRaw[col*tMode[2]+row] = rawI[row];
				buf_index +=2 ;
				//snprintf(data1, 130, "RawI[%2d,%2d]=[%4d],RawP[%4d], RawD[%4d]\n",col,row,rawI[row],rawP[row],rawD[row]);			
				//sitronix_vfswrite(filp, data1, strlen(data1), ppos);	
			}
		}
	}
	//if(frame_counter != raw_header[1])
	{
		frameCounter++;
		retryCounter = 0;
		frame_counter = raw_header[1];
		aa_index = (tMode[1] / 2) * (tMode[2] + tMode[7]) *2;
		buf_index= (skipcol) * (tMode[2] + tMode[7]) *2;
		
		//AA A
		for (col = 0; col < tMode[1]; col++)
		{
			error_count = 0;
			for (row = 0; row < tMode[2]; row++)
			{								
				if((col <nRealCol ) && (row<nRealRow ))
					//if(skipNodeArray[col][row]!=0x01){
						if(st_get_test_enable(col, row) &&( (nMuxOnRaw[col*tMode[2]+row] < gts->self_test_normal_min) || (nMuxOnRaw[col*tMode[2]+row] > gts->self_test_normal_max)))
							error_count++;
					//}
											
				buf_index +=2 ;
			}

			if(error_count > ST_SELFTEST_ADJUST_COUNT)
			{
				for (row = 0; row < tMode[2] ; row++)
				{
					if((col<nRealCol)&&(row<nRealRow)){
						//if(skipNodeArray[col][row]!=0x01){
							if(st_get_test_enable(col, row) &&( (nMuxOnRaw[col*tMode[2]+row] < gts->self_test_normal_min) || (nMuxOnRaw[col*tMode[2]+row] > gts->self_test_normal_max)))
							{
								total_error++;								
	#ifdef ST_SELFTEST_LOG_FILE
								snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) out of range (%d ~ %d) in normal test\n", col, row, nMuxOnRaw[col*tMode[2]+row], gts->self_test_normal_min, gts->self_test_normal_max);
								sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	#endif
							}
						//}
					}
				}
			}

			if (tMode[7] != 0)
				buf_index += 2;
		}

		//SE A
		for (col = 0; col < tMode[5] / 2; col++)
		{
			for (row = 0; row < tMode[2]; row++)
			{
				buf_index +=2 ; 
			}

			if (tMode[7] != 0)
				buf_index += 2;
		}

	
#ifdef ST_SELFTEST_LOG_FILE
		snprintf(data1, 100, "[Normal RAW Data start]\n");			
		sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
		buf_index = 0;
		aa_index = 0;
		//AA A
		for(RawType =0; RawType<1;RawType++){
			snprintf(data1, 100, "\n\n ");
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
			for (col = 0; col < tMode[1]; col++)
			{
				for (row = 0; row < tMode[2]; row++)
				{
					if(RawType ==0 )
						aa_index = nMuxOnRaw[col*tMode[2]+row];
					else if (RawType == 1)
						aa_index = nMuxOnRaw[col*tMode[2]+row];
					else if(RawType == 2)
						aa_index = nMuxOnRaw[col*tMode[2]+row];
#ifdef ST_SELFTEST_LOG_FILE
					if (row == (tMode[2] - 1) )
						snprintf(data1, 100, "%6d \n ", aa_index);
					else
						snprintf(data1, 100, "%6d ", aa_index);
					sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
				}

			}
		}

#ifdef ST_SELFTEST_LOG_FILE
		snprintf(data1, 100, "[Normal Raw Data end]\n");
		sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif			
	}
	

	//if (frameCounter >= count_frame)
		//break;
//}


//st_open_short_test_finish:

	kfree(rawI);
	kfree(nMuxOnRaw);

	if ( ret < 0)
		return ret;
	else if(total_error == 0)
		return 0;
	else
		return total_error;
}


#ifdef ST_SELFTEST_LOG_FILE
int st7102_test_normal(struct file *filp,loff_t *ppos)
#else
int st7102_test_normal(void)
#endif
{

	int ret = 0,i=0;

	//sitronix_ts_reset_device(gts);
#ifdef SITRONIX_TP_WITH_FLASH
	ret = st_address_mode_hardcode_write(test_flash_afe_df, sizeof(test_flash_afe_df));
	if (ret < 0)
		return ret;
#endif /* SITRONIX_TP_WITH_FLASH */
	
	msleep(10); 	
	#ifdef ST_ADDRESS_MODE_WRITE_COMMAND_V2
	ret = st_address_mode_hardcode_write_v2(test_cmd_normal_rawdata, ARRAY_SIZE(test_cmd_normal_rawdata));
	#else
	ret = st_address_mode_hardcode_write(test_cmd_normal_rawdata, sizeof(test_cmd_normal_rawdata));
	#endif
	if (ret < 0)
		return ret;
	for(i=0;i<TEST_RETRY_MAX;i++)
	{
#ifdef ST_SELFTEST_LOG_FILE
	ret = st_Normal_test(0,0,filp,ppos);	
#else
	ret = st_Normal_test(0,0,filp,ppos);	
#endif
		if(ret ==0 ) break;
	}
	return ret;
}


#ifdef ST_SELFTEST_LOG_FILE
int st7102_short_test(char func, int skipcol, struct file *filp,loff_t *ppos)
#else
int st7102_short_test(char func, int skipcol)
#endif
{
	unsigned char cmd[0x08];
	int tMode[8];
	int read_len;
	int ret = 0,i=0;
	unsigned char *raw_buf;
	signed short *rawI;
	//signed short *rawP;
	int frameCounter;
	int retryCounter;
	int max_retry = 5;
	unsigned char raw_dat_rd_on[2] = { 0x02, 0x00 };
	unsigned char raw_dat_rd_off[2] = { 0x00, 0x00 };
	unsigned char raw_header[18];
	unsigned char frame_counter = 0;
	int total_error = 0;
	int error_count = 0;
	//int count_frame = 1;
	int buf_index , aa_index;
	int col , row,nRealRow,nRealCol;
#ifdef ST_SELFTEST_LOG_FILE
	char data1[150];
#endif
	//int percentage = 0;	
	//int *rawS; //Raw data STD for STD test
	//int raw_avg;	
	//unsigned long sqrt;

	/* ROW_CNT */
	sitronix_spi_pram_rw(true, 0xF04A, NULL, cmd, 2);

	/* ROW_CNT */
	sitronix_spi_pram_rw(true, 0xF04A, NULL, cmd, 2);
	tMode[2] = cmd[0]&0x3F;	
	nRealRow = gts->ts_dev_info.y_chs;
	tMode[2] = 32;
	/* CMNC_CH_WR_EN */
	tMode[7] = cmd[1]&0x01;
	tMode[7] = 0x00;
	/* AA_UNIT */
	sitronix_spi_pram_rw(true, 0xF024, NULL, cmd, 2);
	tMode[1] = (cmd[0]&0xF0)>>3;	
	tMode[1] = 16;
	nRealCol = tMode[1];
	/* SELF_UNIT */		
	tMode[5] = (cmd[0]&0xC)>>1;

	/* NOISE_UNIT */
	//tMode[4] = (cmd[0]&0x3)<<2;
	tMode[4] = 0x00;	//for ST7102

	/* KEY */
	tMode[3] = 0;
	/* CMNC_CH_WR_EN */
	tMode[7] = cmd[1]&0x01;
	tMode[7] = 0x00;
	/* AA_UNIT */
	sitronix_spi_pram_rw(true, 0xF024, NULL, cmd, 2);
	//tMode[1] = (cmd[0]&0xF0)>>3;
	if((func ==1)||(func ==2)){
		tMode[1] = tMode[1]+3;
		//nRealCol = tMode[1];
	}
	
	/* SELF_UNIT */		
	tMode[5] = (cmd[0]&0xC)>>1;

	/* NOISE_UNIT */
	//tMode[4] = (cmd[0]&0x3)<<2;
	tMode[4] = 0x00;	//for ST7102

	/* KEY */
	tMode[3] = 0;
	//tMode[1] = 16;
	//tMode[2] = 24;
	stmsg("Short Test \n"); 
	stmsg("ROW_CNT : %d, RealRow : %d\n",tMode[2],nRealRow);
	stmsg("AA_UNIT : %d, RealCol : %d\n",tMode[1],nRealCol);
	stmsg("SELF_UNIT : %d\n",tMode[5]);
	stmsg("NOISE_UNIT : %d\n",tMode[4]);
	stmsg("CMNC_CH_WR_EN : %d\n",tMode[7]);

	read_len = 21*32*4;
	raw_buf = kmalloc(read_len, GFP_KERNEL);
	rawI = (signed short *)kmalloc((read_len) * sizeof(short), GFP_KERNEL);	
	
	//Ignore
	frameCounter = 0;
	retryCounter = 0;
	sitronix_ts_reset_device(gts);	
	if(func == 1){ //odd
		#ifdef ST_ADDRESS_MODE_WRITE_COMMAND_V2
		ret = st_address_mode_hardcode_write_v2(test_cmd_short_odd, ARRAY_SIZE(test_cmd_short_odd));
		#else
		ret = st_address_mode_hardcode_write(test_cmd_short_odd, sizeof(test_cmd_short_odd));
		#endif
	}else if(func == 2){ //even
		#ifdef ST_ADDRESS_MODE_WRITE_COMMAND_V2
		ret = st_address_mode_hardcode_write_v2(test_cmd_short_even, ARRAY_SIZE(test_cmd_short_even));
		#else
		ret = st_address_mode_hardcode_write(test_cmd_short_even, sizeof(test_cmd_short_even));
		#endif
		sitronix_spi_pram_rw(true, 0xF000, NULL, raw_header, 18);
		stmsg("0xF000 =  0x%02X\n",raw_header[1]);
		if(raw_header[1]==0x00){
			for(i=0;i<10;i++){
				msleep(100);
				#ifdef ST_ADDRESS_MODE_WRITE_COMMAND_V2
				ret = st_address_mode_hardcode_write_v2(test_cmd_short_even, ARRAY_SIZE(test_cmd_short_even));
				#else
				ret = st_address_mode_hardcode_write(test_cmd_short_even, sizeof(test_cmd_short_even));
				#endif
				sitronix_spi_pram_rw(true, 0xF000, NULL, raw_header, 18);				
				stmsg("0xF000 =  0x%02X\n",raw_header[1]);
				if(raw_header[1]==0x02){
					break;
				}
			}				
		}
	}
	if (ret < 0)
		return ret;
	//msleep(100);

	
	while (ST_SELFTEST_IGNORE_FRAME > 0 && retryCounter++ < max_retry)
	{
		msleep(20);
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_on, NULL, 2);
		sitronix_spi_pram_rw(true, 0xF180, NULL, raw_header, 18);

		stmsg("header %x \n",raw_header[1]);
		if(frame_counter != raw_header[1])
		{
			frameCounter++;
			retryCounter = 0;
			frame_counter = raw_header[1];
		}else{
			//sitronix_ts_reset_device(gts);	
			
		}
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);

		if (frameCounter >= ST_SELFTEST_IGNORE_FRAME + 1)
				break;
	}
	

	//raw
	frameCounter = 0;
	retryCounter = 0;

	//while (count_frame > 0 && retryCounter++ < max_retry)
	//{
		msleep(10);
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_on, NULL, 2);
		sitronix_spi_pram_rw(true, 0xF180, NULL, raw_header, 18);
		//if(frame_counter != raw_header[1])
		{
			frameCounter++;
			retryCounter = 0;
			frame_counter = raw_header[1];

			sitronix_spi_pram_rw(true, 0xD000, NULL, raw_buf, read_len);
			sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);

			aa_index = (tMode[1] / 2) * (tMode[2] + tMode[7]) *2;
			buf_index = (skipcol) * (tMode[2] + tMode[7]) *2;

			if (skipcol != 0)
				for(col = 0; col < aa_index; col++)
					raw_buf[aa_index + col ] = raw_buf[aa_index + col + buf_index];

			buf_index = 0;
			aa_index = 0;

			//AA A
			for (col = 0; col < tMode[1] / 2; col++)
			{
				error_count = 0;
				for (row = 0; row < tMode[2]; row++)
				{
					rawI[row] = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);
					//stmsg("sensor (%2d,%2d) RAW (%4d) \n" , col, row, rawI[row]);
					
					if(func == 1) // short odd
					{
						if(col!=8 && col!=16 && col!=17)
						{
							if(row %2 == 0 && rawI[row] > gts->self_test_short_max)
								error_count++;
						}
					}
					else if(func == 2) //short even
					{
						if(col!=8 && col!=16 && col!=17)
						{
							if(row %2 == 1 && rawI[row] > gts->self_test_short_max)
								error_count++;
						}
					}
					buf_index +=2 ;
				}

				if(error_count > ST_SELFTEST_ADJUST_COUNT)
				{
					for (row = 0; row < tMode[2]; row++)
					{
						if(func == 1)
						{
							if(col!=8 && col!=16 && col!=17)
							{
								if(row %2 == 0 && rawI[row] > gts->self_test_short_max) {
									total_error++;
									//sterr("sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_odd test\n" , col, row, rawI[row], gts->self_test_short_max);
	#ifdef ST_SELFTEST_LOG_FILE
									snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_odd test\n", col, row, rawI[row], gts->self_test_short_max);
									sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	#endif
								}
							}
						}
						else if(func == 2 )
						{
							if(col!=8 && col!=16 && col!=17)
							{
								if(row %2 == 1 && rawI[row] > gts->self_test_short_max) {
									total_error++;
									//sterr("sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_even test\n" , col, row, rawI[row], gts->self_test_short_max);
	#ifdef ST_SELFTEST_LOG_FILE
									snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_even test\n", col, row, rawI[row], gts->self_test_short_max);
									sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	#endif
								}
							}
						}
					}
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

			//SE A
			for (col = 0; col < tMode[5] / 2; col++)
			{
				for (row = 0; row < tMode[2]; row++)
				{
					buf_index +=2 ; 
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

			//AA B
			for (col = tMode[1] / 2; col < tMode[1]; col++)
			{
				error_count = 0;
				for (row = 0; row < tMode[2]; row++)
				{
					rawI[row] = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);
					//stmsg("sensor (%2d,%2d) RAW (%4d) \n" , col, row, rawI[row]);
					if(func == 1) // short odd
					{
						if(col!=8 && col!=16 && col!=17)
						{
							if(row %2 == 0 && rawI[row] > gts->self_test_short_max)
								error_count++;
						}
					}
					else if(func == 2) //short even
					{
						if(col!=8 && col!=16 && col!=17)
						{
							if(row %2 == 1 && rawI[row] > gts->self_test_short_max)
								error_count++;
						}
					}					
					buf_index +=2 ;
				}

				if(error_count > ST_SELFTEST_ADJUST_COUNT)
				{
					for (row = 0; row < tMode[2]; row++)
					{
						if(func == 1)
						{
							if(col!=8 && col!=16 && col!=17)
							{
								if(row %2 == 0 && rawI[row] > gts->self_test_short_max) {
									total_error++;
									sterr("sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_odd test\n" , col, row, rawI[row], gts->self_test_short_max);
	#ifdef ST_SELFTEST_LOG_FILE
									snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_odd test\n", col, row, rawI[row], gts->self_test_short_max);
									sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	#endif
								}
							}
						}
						else if(func == 2 )
						{
							if(col!=8 && col!=16 && col!=17)
							{
								if(row %2 == 1 && rawI[row] > gts->self_test_short_max) {
									total_error++;
									sterr("sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_even test\n" , col, row, rawI[row], gts->self_test_short_max);
	#ifdef ST_SELFTEST_LOG_FILE
									snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_even test\n", col, row, rawI[row], gts->self_test_short_max);
									sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	#endif
								}
							}
						}
					}
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

			//SE B
			for (col = tMode[5] / 2; col < tMode[5] ; col++)
			{
				for (row = 0; row < tMode[2]; row++)
				{
					buf_index +=2 ; 
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

			if ( func == 2 || func == 1 )
			{
#ifdef ST_SELFTEST_LOG_FILE
				 if(func == 1)
					snprintf(data1, 100, "[SHORT_ODD RAW Data start]\n");
				else if(func == 2)
					snprintf(data1, 100, "[SHORT_EVEN RAW Data start]\n");				
				sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
				buf_index = 0;
				aa_index = 0;
				//AA A
				for (col = 0; col < tMode[1] / 2; col++)
				{
					for (row = 0; row < tMode[2]; row++)
					{
						rawI[row] = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);	
#ifdef ST_SELFTEST_LOG_FILE
						if (row == tMode[2] - 1 )
							snprintf(data1, 100, "%6d \n", rawI[row]);
						else
							snprintf(data1, 100, "%6d ", rawI[row]);
						if(func==1 || func==2){
							if(col!=8 && col!=16 && col!=17)
							{
								sitronix_vfswrite(filp, data1, strlen(data1), ppos);
							}
						}else
						sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
						buf_index +=2;
					}

					if (tMode[7] != 0)
						buf_index += 2;
				}
				//SE A
				for (col = 0; col < tMode[5] / 2; col++)
				{
					for (row = 0; row < tMode[2]; row++)
					{
						buf_index +=2;
					}

					if (tMode[7] != 0)
						buf_index += 2;
				}
				//AA B
				for (col = tMode[1] / 2; col < tMode[1]; col++)
				{
					for (row = 0; row < tMode[2]; row++)
					{
						rawI[row] = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);
#ifdef ST_SELFTEST_LOG_FILE
						if (row == tMode[2] - 1 ) {
							snprintf(data1, 100, "%6d \n", rawI[row]);
						} else {
							snprintf(data1, 100, "%6d ", rawI[row]);
							if(func==1 || func==2){
								if(col!=8 && col!=16 && col!=17)
								{
									sitronix_vfswrite(filp, data1, strlen(data1), ppos);
								}
							}else{
								sitronix_vfswrite(filp, data1, strlen(data1), ppos);
							}
#endif
							buf_index +=2 ;
						}
					}
					if (tMode[7] != 0)
						buf_index += 2;
				}

#ifdef ST_SELFTEST_LOG_FILE
				snprintf(data1, 100, "[RAW Data end]\n");
				sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
			}
		}
		//else
		//{
	//		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);
//		}

		//if (frameCounter >= count_frame)
		//	break;
//	}

//st_open_short_test_finish:

	kfree(rawI);
	kfree(raw_buf);
	
	if ( ret < 0)
		return ret;
	else if(total_error == 0)
		return 0;
	else
		return total_error;
}




#ifdef ST_SELFTEST_LOG_FILE
int st_open_short_test(char func, int skipcol, struct file *filp,loff_t *ppos)
#else
int st_open_short_test(char func, int skipcol)
#endif
{
	unsigned char cmd[0x08];
	int tMode[8];
	int read_len;
	int ret = 0;
	unsigned char *raw_buf = NULL;
	signed short *rawI = NULL;
	signed short *rawP = NULL;
	int frameCounter;
	int retryCounter;
	int max_retry = 50; //5;
	unsigned char raw_dat_rd_on[2] = { 0x02, 0x00 };
	unsigned char raw_dat_rd_off[2] = { 0x00, 0x00 };
	unsigned char raw_header[18];
	unsigned char frame_counter = 0;
	int total_error = 0;
	int error_count = 0;
	int count_frame = 1;
	int buf_index , aa_index;
	int col , row;
#ifdef ST_SELFTEST_LOG_FILE
	char data1[150];
#endif
	int percentage = 0;	
	int *rawS = NULL;	//Raw data STD for STD test
	int raw_avg;	
	unsigned long sqrt;
	int min = 0xFFF0, max = -0xFFF0;
	signed short rawdata;
#ifdef __RAW_DATA_DEBUG__
	//int i, j, k;
#endif //__RAW_DATA_DEBUG__


	/* ROW_CNT */
	sitronix_spi_pram_rw(true, 0xF04A, NULL, cmd, 2);
	tMode[2] = cmd[0]&0x3F;

	/* CMNC_CH_WR_EN */
	tMode[7] = cmd[1]&0x01;

	/* AA_UNIT */
	sitronix_spi_pram_rw(true, 0xF024, NULL, cmd, 2);
	tMode[1] = (cmd[0]&0xF0)>>3;

	/* SELF_UNIT */		
	tMode[5] = (cmd[0]&0xC)>>1;

	/* NOISE_UNIT */
	tMode[4] = (cmd[0]&0x3)<<2;

	/* KEY */
	tMode[3] = 0;

	stmsg("ROW_CNT : %d\n",tMode[2]);
	stmsg("AA_UNIT : %d\n",tMode[1]);
	stmsg("SELF_UNIT : %d\n",tMode[5]);
	stmsg("NOISE_UNIT : %d\n",tMode[4]);
	stmsg("CMNC_CH_WR_EN : %d\n",tMode[7]);

	read_len = (tMode[2] + tMode[7]) * (tMode[1] + tMode[5] + tMode[4] + skipcol) * 2;
	raw_buf = (unsigned char*)kmalloc(read_len, GFP_KERNEL);
	rawI = (signed short *)kmalloc((tMode[2]) * sizeof(short), GFP_KERNEL);	

	//Ignore
	frameCounter = 0;
	retryCounter = 0;

	while (ST_SELFTEST_IGNORE_FRAME > 0 && retryCounter++ < max_retry)
	{
		msleep(10);
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_on, NULL, 2);
		sitronix_spi_pram_rw(true, 0xF180, NULL, raw_header, 18);

#ifdef __RAW_DATA_DEBUG__
#if 0
		// Read Rawdata.
		memset(raw_buf, 0, read_len);
		sitronix_spi_pram_rw(true, 0xD000, NULL, raw_buf, read_len);
		for (i = 0, j = 0; j < (tMode[1] + tMode[5] + tMode[4] + skipcol); j++) {
			stmsg("AA_UNIT[%d]: ", j);
			for (k = 0; k < (tMode[2] + tMode[7]); i++, k++) {
				short raw = (short)(raw_buf[i*2] << 8 | raw_buf[i*2 + 1]);
				printk("%d ", raw);
			}
			printk(" (i=%d, k=%d)\n", i, k);
		}
#endif //0
#endif //__RAW_DATA_DEBUG__

		stmsg("header %x \n",raw_header[1]);
		if(frame_counter != raw_header[1])
		{
			frameCounter++;
			retryCounter = 0;
			frame_counter = raw_header[1];
		}

		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);

		if (frameCounter >= ST_SELFTEST_IGNORE_FRAME + 1)
			break;
	}

	if( retryCounter >=  max_retry)
	{
		if(func == 0) {
			sterr("st open test fail ,  can't wait IRQ \n");
#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 50, "st open test fail ,  can't wait IRQ\n");
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
		} else if(func == 1 || func == 2) {
			sterr("st short test fail ,  can't wait IRQ \n");
#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 50, "st short test fail ,  can't wait IRQ \n");
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
		} else if(func == 3) {
			sterr("st uniformity test fail ,  can't wait IRQ \n");
#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 50, "st uniformity test fail ,  can't wait IRQ \n");
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
		} else if(func == 4) {
			sterr("st STD test fail ,  can't wait IRQ \n");
#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 50, "st STD test fail ,  can't wait IRQ \n");
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
		}
		ret = -1;
		goto st_open_short_test_finish;
	}

	//uniformity test
	if( func == 3) {
		rawP = (signed short *)kmalloc((tMode[2]) * sizeof(short), GFP_KERNEL);
	}
	
	//STD test
	if (func == 4) {
		retryCounter = 0;
		count_frame = ST_SELFTEST_STD_FRAME_CNT;
		rawS = (int *)kmalloc((tMode[2] * tMode[1]) * sizeof(int), GFP_KERNEL);
		while (retryCounter++ < max_retry) {
			rawP = (signed short *)kmalloc((tMode[2] * tMode[1] * ST_SELFTEST_STD_FRAME_CNT) * sizeof(short), GFP_KERNEL);
			if ( rawP != NULL)
				break;
		}
		
		if( retryCounter >=  max_retry) {
			sterr("st STD test fail for can not kmalloc\n");
#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 50, "st STD test fail for can not kmalloc\n");
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
			ret = -1;
			goto st_open_short_test_finish;
		}
	}

	//raw
	frameCounter = 0;
	retryCounter = 0;

	while (count_frame > 0 && retryCounter++ < max_retry)
	{
		msleep(10);
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_on, NULL, 2);
		sitronix_spi_pram_rw(true, 0xF180, NULL, raw_header, 18);
		if(frame_counter != raw_header[1])
		{
			frameCounter++;
			retryCounter = 0;
			frame_counter = raw_header[1];

			sitronix_spi_pram_rw(true, 0xD000, NULL, raw_buf, read_len);
			sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);

			aa_index = (tMode[1] / 2) * (tMode[2] + tMode[7]) *2;
			buf_index = (skipcol) * (tMode[2] + tMode[7]) *2;

			if (skipcol != 0)
				for(col = 0; col < aa_index; col++)
					raw_buf[aa_index + col ] = raw_buf[aa_index + col + buf_index];

			buf_index = 0;
			aa_index = 0;

			//AA A
			for (col = 0; col < tMode[1] / 2; col++)
			{
				error_count = 0;
				for (row = 0; row < tMode[2]; row++)
				{
					rawI[row] = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);
					//stmsg("sensor (%2d,%2d) RAW (%4d) \n" , col, row, rawI[row]);
					if(func == 3) // uniformity
					{
						//judge
						if (sizeof(golden_buf) > 0) {
							if( sizeof(golden_buf) > buf_index + 1)
								rawP[row] = (signed short)((golden_buf[buf_index]) + (golden_buf[buf_index + 1]<<8));
							else
								rawP[row] = rawI[row];

							rawI[row] = rawI[row] + gts->self_test_uniformity_shift;
							if(rawP[row] < 1)
								percentage = 100;
							else
								percentage = rawI[row] * 100 / rawP[row];							
							if(st_get_test_enable(col, row) && (percentage < gts->self_test_uniformity_min || percentage > gts->self_test_uniformity_max))
								error_count++;
						}
					}
					else if(func == 1)	//short odd
					{
						if (row % 2 == 0) {
							rawdata = rawI[row];
							if (rawdata > gts->self_test_short_max)
								error_count++;

							if (rawdata < min)
								min = (int)rawdata;
							if (rawdata > max)
								max = (int)rawdata;
						}
					}
					else if(func == 2)	//short even
					{
						if (row % 2 == 1) {
							rawdata = rawI[row];
							if (rawdata > gts->self_test_short_max)
								error_count++;

							if (rawdata < min)
								min = (int)rawdata;
							if (rawdata > max)
								max = (int)rawdata;
						}
					}
					else if(func == 4)
					{
						percentage = ((frameCounter - 1) * tMode[1] * tMode[2]) + (col * tMode[2]) + row;
						rawP[percentage] = rawI[row];						
						//stmsg("sensor (%2d,%2d) , index %2d RAW (%4d) \n" , col, row, percentage, rawI[row]);
					}
					else //open
					{
						if(st_get_test_enable(col, row) && (rawI[row] < gts->self_test_open_min || rawI[row] > gts->self_test_open_max))
							error_count++;
					}

					buf_index +=2 ;
				}

				if(error_count > ST_SELFTEST_ADJUST_COUNT)
				{
					for (row = 0; row < tMode[2]; row++)
					{
						if(func == 3)
						{	
							if(rawP[row] < 1)
								percentage = 100;
							else
								percentage = rawI[row] * 100 / rawP[row];
							if(st_get_test_enable(col, row) && (percentage < gts->self_test_uniformity_min || percentage > gts->self_test_uniformity_max))
							{
								total_error++;
								sterr("sensor (%2d,%2d) RAW (%3d%%) out of percentage (%d%% ~ %d%%) in uniformity test\n" , col, row, percentage, gts->self_test_uniformity_min , gts->self_test_uniformity_max);
#ifdef ST_SELFTEST_LOG_FILE
								snprintf(data1, 150, "sensor (%2d,%2d) RAW (%3d%%) out of percentage (%d%% ~ %d%%) in uniformity test\n" , col, row, percentage, gts->self_test_uniformity_min , gts->self_test_uniformity_max);
								sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
							}
						}
						else if(func == 1)	//short odd
						{
							if(row %2 == 0 && rawI[row] > gts->self_test_short_max) {
								total_error++;
								sterr("sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_odd test\n" , col, row, rawI[row], gts->self_test_short_max);
#ifdef ST_SELFTEST_LOG_FILE
								snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_odd test\n", col, row, rawI[row], gts->self_test_short_max);
								sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
							}
						}
						else if(func == 2)	//short even
						{
							if(row %2 == 1 && rawI[row] > gts->self_test_short_max) {
								total_error++;
								sterr("sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_even test\n" , col, row, rawI[row], gts->self_test_short_max);
#ifdef ST_SELFTEST_LOG_FILE
								snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_even test\n", col, row, rawI[row], gts->self_test_short_max);
								sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
							}
						}
						else if(st_get_test_enable(col, row) && (rawI[row] < gts->self_test_open_min || rawI[row] > gts->self_test_open_max))
						{
							total_error++;
							sterr("sensor (%2d,%2d) RAW (%4d) out of range (%d ~ %d) in open test\n" , col, row, rawI[row], gts->self_test_open_min, gts->self_test_open_max);
#ifdef ST_SELFTEST_LOG_FILE
							snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) out of range (%d ~ %d) in open test\n", col, row, rawI[row], gts->self_test_open_min, gts->self_test_open_max);
							sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
						}
					}
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

			//SE A
			for (col = 0; col < tMode[5] / 2; col++)
			{
				for (row = 0; row < tMode[2]; row++)
				{
					buf_index +=2 ; 
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

			//AA B
			for (col = tMode[1] / 2; col < tMode[1]; col++)
			{
				error_count = 0;
				for (row = 0; row < tMode[2]; row++)
				{
					rawI[row] = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);
					//stmsg("sensor (%2d,%2d) RAW (%4d) \n" , col, row, rawI[row]);
					if(func == 3) // uniformity
					{
						//judge
						if (sizeof(golden_buf) > 0) {
							if( sizeof(golden_buf) > buf_index + 1)
								rawP[row] = (signed short)((golden_buf[buf_index]) + (golden_buf[buf_index + 1]<<8));
							else
								rawP[row] = rawI[row];
							
							rawI[row] = rawI[row] + gts->self_test_uniformity_shift;
							if(rawP[row] < 1)
								percentage = 100;
							else
								percentage = rawI[row] * 100 / rawP[row];
							if(st_get_test_enable(col, row) && (percentage < gts->self_test_uniformity_min || percentage > gts->self_test_uniformity_max))
								error_count++;
						}
					}
					else if(func == 1)	//short odd
					{
						if (row % 2 == 0) {
							rawdata = rawI[row];
							if (rawdata > gts->self_test_short_max)
								error_count++;

							if (rawdata < min)
								min = (int)rawdata;
							if (rawdata > max)
								max = (int)rawdata;
						}
					}
					else if(func == 2)	//short even
					{
						if (row % 2 == 1) {
							rawdata = rawI[row];
							if (rawdata > gts->self_test_short_max)
								error_count++;

							if (rawdata < min)
								min = (int)rawdata;
							if (rawdata > max)
								max = (int)rawdata;
						}
					}
					else if(func == 4)
					{
						percentage = ((frameCounter - 1) * tMode[1] * tMode[2]) + (col * tMode[2]) + row;
						rawP[percentage] = rawI[row];
					}
					else //open
					{
						if(st_get_test_enable(col, row) && (rawI[row] < gts->self_test_open_min || rawI[row] > gts->self_test_open_max))
							error_count++;
					}

					buf_index +=2 ;
				}

				if(error_count > ST_SELFTEST_ADJUST_COUNT)
				{
					for (row = 0; row < tMode[2]; row++)
					{
						if(func == 3)
						{
							if(rawP[row] < 1)
								percentage = 100;
							else
								percentage = rawI[row] * 100 / rawP[row];
							if(st_get_test_enable(col, row) && (percentage < gts->self_test_uniformity_min || percentage > gts->self_test_uniformity_max))
							{
								total_error++;
								sterr("sensor (%2d,%2d) RAW (%3d%%) out of percentage (%d%% ~ %d%%) in uniformity test\n" , col, row, percentage, gts->self_test_uniformity_min , gts->self_test_uniformity_max);
#ifdef ST_SELFTEST_LOG_FILE
								snprintf(data1, 150, "sensor (%2d,%2d) RAW (%3d%%) out of percentage (%d%% ~ %d%%) in uniformity test\n" , col, row, percentage, gts->self_test_uniformity_min , gts->self_test_uniformity_max);
								sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
							}
						}
						else if(func == 1)	//short odd
						{
							if(row %2 == 0 && rawI[row] > gts->self_test_short_max) {
								total_error++;
								sterr("sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_odd test\n" , col, row, rawI[row], gts->self_test_short_max);
#ifdef ST_SELFTEST_LOG_FILE
								snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_odd test\n", col, row, rawI[row], gts->self_test_short_max);
								sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
							}
						}
						else if(func == 2)	//short even
						{
							if(row %2 == 1 && rawI[row] > gts->self_test_short_max) {
								total_error++;
								sterr("sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_even test\n" , col, row, rawI[row], gts->self_test_short_max);
#ifdef ST_SELFTEST_LOG_FILE
								snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) > standard value (%d) in short_even test\n", col, row, rawI[row], gts->self_test_short_max);
								sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
							}
						}
						else if(st_get_test_enable(col, row) && (rawI[row] < gts->self_test_open_min || rawI[row] > gts->self_test_open_max))
						{
							total_error++;
							sterr("sensor (%2d,%2d) RAW (%4d) out of range (%d ~ %d) in open test\n" , col, row, rawI[row], gts->self_test_open_min, gts->self_test_open_max);
#ifdef ST_SELFTEST_LOG_FILE
							snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) out of range (%d ~ %d) in open test\n", col, row, rawI[row], gts->self_test_open_min, gts->self_test_open_max);
							sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
						}
					}
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

			//SE B
			for (col = tMode[5] / 2; col < tMode[5] ; col++)
			{
				for (row = 0; row < tMode[2]; row++)
				{
					buf_index +=2 ; 
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

			if (func == 3 || func == 2 || func == 1 || func == 0)
			{
#ifdef ST_SELFTEST_LOG_FILE
				if (func == 0) {
					snprintf(data1, 100, "[OPEN RAW Data start]\n");
				} else if(func == 1) {
					snprintf(data1, 100, "[SHORT_ODD RAW Data start]        (Min/Max: %d / %d)\n", min, max);
				} else if(func == 2) {
					snprintf(data1, 100, "[SHORT_EVEN RAW Data start]        (Min/Max: %d / %d)\n", min, max);
				} else {
					snprintf(data1, 100, "[UNIFORMITY RAW Data start]\n");
				}
				sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
				buf_index = 0;
				aa_index = 0;
				//AA A
				for (col = 0; col < tMode[1] / 2; col++)
				{
					for (row = 0; row < tMode[2]; row++)
					{
						rawI[row] = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);	
#ifdef ST_SELFTEST_LOG_FILE
						if (row == tMode[2] - 1 )
							snprintf(data1, 100, "%6d \n", rawI[row]);
						else
							snprintf(data1, 100, "%6d ", rawI[row]);
						sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
						buf_index +=2;
					}

					if (tMode[7] != 0)
						buf_index += 2;
				}
				//SE A
				for (col = 0; col < tMode[5] / 2; col++)
				{
					for (row = 0; row < tMode[2]; row++)
					{
						buf_index +=2;
					}

					if (tMode[7] != 0)
						buf_index += 2;
				}
				//AA B
				for (col = tMode[1] / 2; col < tMode[1]; col++)
				{
					for (row = 0; row < tMode[2]; row++)
					{
						rawI[row] = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);
#ifdef ST_SELFTEST_LOG_FILE
						if (row == tMode[2] - 1 )
							snprintf(data1, 100, "%6d \n", rawI[row]);
						else
							snprintf(data1, 100, "%6d ", rawI[row]);
						sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
						buf_index +=2 ;
					}
					if (tMode[7] != 0)
						buf_index += 2;
				}

#ifdef ST_SELFTEST_LOG_FILE
				snprintf(data1, 100, "[RAW Data end]\n");
				sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
			}
		}
		else
		{
			sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);
		}

		if (frameCounter >= count_frame)
			break;
	}

	if (func == 4) {
#ifdef ST_SELFTEST_LOG_FILE
		snprintf(data1, 100, "[STD start]\n");
		sitronix_vfswrite(filp, data1, strlen(data1), ppos);		
#endif
		for (col = 0; col < tMode[1]; col++)
		{
			for (row = 0; row < tMode[2]; row++)
			{
				aa_index = col * tMode[2] + row;
				percentage = 0;
				for (frameCounter = 0 ; frameCounter < ST_SELFTEST_STD_FRAME_CNT ; frameCounter++) {
					buf_index = frameCounter * tMode[1] * tMode[2];
					percentage += (int)rawP[buf_index + aa_index];
				}

				raw_avg = percentage * 10 / ST_SELFTEST_STD_FRAME_CNT;

				rawS[aa_index] = 0;
				for (frameCounter = 0 ; frameCounter < ST_SELFTEST_STD_FRAME_CNT ; frameCounter++) {					
					buf_index = frameCounter * tMode[1] * tMode[2];
					if( raw_avg >= (int)(rawP[buf_index + aa_index] * 10))
						percentage = raw_avg - (int)(rawP[buf_index + aa_index] * 10);
					else
						percentage = (int)(rawP[buf_index + aa_index] * 10) - raw_avg;
					if (percentage > ST_SELFTEST_STD_CALCULATE_LIMIT)
						percentage = ST_SELFTEST_STD_CALCULATE_LIMIT;
					rawS[aa_index] += percentage * percentage;
				}
				rawS[aa_index] = rawS[aa_index] / ST_SELFTEST_STD_FRAME_CNT;		
#ifndef USE_ST_SQRT		
				sqrt = int_sqrt((unsigned long) rawS[aa_index]);
#else
				sqrt = st_sqrt(rawS[aa_index]);
#endif	//end of USE_ST_SQRT

#ifdef ST_SELFTEST_LOG_FILE
				if (row == tMode[2] - 1 )
					snprintf(data1, 100, "%4ld.%1ld \n", sqrt/10, sqrt%10); //snprintf(data1, 100, "%6d \n", rawS[aa_index]);
				else
					snprintf(data1, 100, "%4ld.%1ld ", sqrt/10, sqrt%10);
				sitronix_vfswrite(filp, data1, strlen(data1), ppos);

				if (sqrt < min)
					min = (int)sqrt;
				if ((int)sqrt > max)
					max = (int)sqrt;
#endif
			}
		}
		
		for (col = 0; col < tMode[1]; col++)
		{
			for (row = 0; row < tMode[2]; row++)
			{
				aa_index = col * tMode[2] + row;
				if(st_get_test_enable(col, row) && (rawS[aa_index] > gts->self_test_std_square100_max)) {
					total_error++;
#ifndef USE_ST_SQRT	
					sqrt = int_sqrt((unsigned long) rawS[aa_index]);
#else
					sqrt = st_sqrt(rawS[aa_index]);
#endif//end of USE_ST_SQRT
					sterr("sensor (%2d,%2d) STD (%4ld.%1ld) > standard value (%4d.%1d) in STD test\n" , col, row, sqrt/10, sqrt%10, gts->self_test_std_max/10, gts->self_test_std_max%10);
#ifdef ST_SELFTEST_LOG_FILE
					snprintf(data1, 150, "sensor (%2d,%2d) STD (%4ld.%1ld) > standard value (%4d.%1d) in STD test\n" , col, row, sqrt/10, sqrt%10, gts->self_test_std_max/10, gts->self_test_std_max%10);
					sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
				}
			}
		}
#ifdef ST_SELFTEST_LOG_FILE
		snprintf(data1, 100, "[STD end]        (Min/Max: %4ld.%1ld / %4ld.%1ld)\n",
				(unsigned long)min/10, (unsigned long)min%10,  (unsigned long)max/10, (unsigned long)max%10);
		sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
	}

st_open_short_test_finish:

	if (rawI)
		kfree(rawI);

	if (raw_buf)
		kfree(raw_buf);

	if (rawP)
		kfree(rawP);

	if (rawS)
		kfree(rawS);

	if ( ret < 0)
		return ret;
	else if (total_error == 0)
		return 0;
	else
		return total_error;
}

#if 0
int st_get_afe_sensing_settings(sensing_setting_t *sensing_setting)
{
	unsigned char cmd[0x08];

	/* ROW_CNT */
	sitronix_spi_pram_rw(true, 0xF04A, NULL, cmd, 2);
	sensing_setting->row_cnt = cmd[0] & 0x3F;		//tMode[2] = cmd[0]&0x3F;

	/* CMNC_CH_WR_EN */
	sensing_setting->cmnc_ch_wr_en = cmd[1] & 0x01;		//tMode[7] = cmd[1]&0x01;

	/* AA_UNIT */
	sitronix_spi_pram_rw(true, 0xF024, NULL, cmd, 2);
	sensing_setting->aa_unit = (cmd[0] & 0xF0) >> 3;	//tMode[1] = (cmd[0]&0xF0)>>3;

	/* SELF_UNIT */		
	sensing_setting->self_unit = (cmd[0] & 0x0C) >> 1;	//tMode[5] = (cmd[0]&0xC)>>1;

	/* NOISE_UNIT */
	sensing_setting->noise_unit = (cmd[0] & 0x03) << 2;	//tMode[4] = (cmd[0]&0x3)<<2;

	/* KEY */
	sensing_setting->key = 0;				//tMode[3] = 0;

	stmsg("ROW_CNT : %d\n", sensing_setting->row_cnt);
	stmsg("AA_UNIT : %d\n", sensing_setting->aa_unit);
	stmsg("SELF_UNIT : %d\n", sensing_setting->self_unit);
	stmsg("NOISE_UNIT : %d\n", sensing_setting->noise_unit);
	stmsg("CMNC_CH_WR_EN : %d\n", sensing_setting->cmnc_ch_wr_en);

	return 0;
}
#endif //0

#ifdef ST_SELFTEST_LOG_FILE
signed short *st_get_aa_rawdata(uint8_t *raw_cnt, uint8_t *aa_unit, int skipcol, struct file *filp, loff_t *ppos)
#else //ST_SELFTEST_LOG_FILE
signed short *st_get_aa_rawdata(uint8_t *raw_cnt, uint8_t *aa_unit, int skipcol)
#endif //ST_SELFTEST_LOG_FILE
{
	unsigned char cmd[0x08];
	int tMode[8];
	int read_len;
	unsigned char *raw_buf = NULL;
	signed short *rawI = NULL;
	int frameCounter;
	int retryCounter;
	int max_retry = 50; //5;
	unsigned char raw_dat_rd_on[2] = { 0x02, 0x00 };
	unsigned char raw_dat_rd_off[2] = { 0x00, 0x00 };
	unsigned char raw_header[18];
	unsigned char frame_counter = 0;
	int buf_index , aa_index;
	int col , row;
#ifdef ST_SELFTEST_LOG_FILE
	char data1[150];
#endif
#ifdef __RAW_DATA_DEBUG__
	int i, j, k;
#endif //__RAW_DATA_DEBUG__


	/* ROW_CNT */
	sitronix_spi_pram_rw(true, 0xF04A, NULL, cmd, 2);
	tMode[2] = cmd[0]&0x3F;
	*raw_cnt = tMode[2];

	/* CMNC_CH_WR_EN */
	tMode[7] = cmd[1]&0x01;

	/* AA_UNIT */
	sitronix_spi_pram_rw(true, 0xF024, NULL, cmd, 2);
	tMode[1] = (cmd[0]&0xF0)>>3;
	*aa_unit = tMode[1];

	/* SELF_UNIT */		
	tMode[5] = (cmd[0]&0xC)>>1;

	/* NOISE_UNIT */
	tMode[4] = (cmd[0]&0x3)<<2;

	/* KEY */
	tMode[3] = 0;

	stmsg("ROW_CNT : %d\n",tMode[2]);
	stmsg("AA_UNIT : %d\n",tMode[1]);
	stmsg("SELF_UNIT : %d\n",tMode[5]);
	stmsg("NOISE_UNIT : %d\n",tMode[4]);
	stmsg("CMNC_CH_WR_EN : %d\n",tMode[7]);

	read_len = (tMode[2] + tMode[7]) * (tMode[1] + tMode[5] + tMode[4] + skipcol) * 2;
	raw_buf = (unsigned char*)kmalloc(read_len, GFP_KERNEL);
	rawI = (signed short *)kmalloc((tMode[1] * tMode[2] * sizeof(short)), GFP_KERNEL);

	//Ignore
	frameCounter = 0;
	retryCounter = 0;

	while (ST_SELFTEST_IGNORE_FRAME > 0 && retryCounter++ < max_retry)
	{
		msleep(10);
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_on, NULL, 2);
		sitronix_spi_pram_rw(true, 0xF180, NULL, raw_header, 18);

#ifdef __RAW_DATA_DEBUG__
		// Read Rawdata.
		memset(raw_buf, 0, read_len);
		sitronix_spi_pram_rw(true, 0xD000, NULL, raw_buf, read_len);
		for (i = 0, j = 0; j < (tMode[1] + tMode[5] + tMode[4] + skipcol); j++) {
			stmsg("AA_UNIT[%d]: ", j);
			for (k = 0; k < (tMode[2] + tMode[7]); i++, k++) {
				short raw = (short)(raw_buf[i*2] << 8 | raw_buf[i*2 + 1]);
				printk("%d ", raw);
			}
			printk(" (i=%d, k=%d)\n", i, k);
		}
#endif //__RAW_DATA_DEBUG__

		stmsg("header %x \n",raw_header[1]);
		if(frame_counter != raw_header[1])
		{
			frameCounter++;
			retryCounter = 0;
			frame_counter = raw_header[1];
		}

		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);

		if (frameCounter >= ST_SELFTEST_IGNORE_FRAME + 1)
			break;
	}

	if( retryCounter >=  max_retry)
	{
		sterr("st open test fail ,  can't wait IRQ \n");
#ifdef ST_SELFTEST_LOG_FILE
		snprintf(data1, 50, "st open test fail ,  can't wait IRQ\n");
		sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
		goto rawdata_retry_err;
	}

	//raw
	frameCounter = 0;
	retryCounter = 0;

	while (retryCounter++ < max_retry)
	{
		msleep(10);
		sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_on, NULL, 2);
		sitronix_spi_pram_rw(true, 0xF180, NULL, raw_header, 18);
		if(frame_counter != raw_header[1])
		{
			frameCounter++;
			retryCounter = 0;
			frame_counter = raw_header[1];

			sitronix_spi_pram_rw(true, 0xD000, NULL, raw_buf, read_len);
			sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);

			aa_index = (tMode[1] / 2) * (tMode[2] + tMode[7]) *2;
			buf_index = (skipcol) * (tMode[2] + tMode[7]) *2;

			if (skipcol != 0)
				for(col = 0; col < aa_index; col++)
					raw_buf[aa_index + col ] = raw_buf[aa_index + col + buf_index];

			buf_index = 0;
			aa_index = 0;

			//AA A
			for (col = 0; col < tMode[1] / 2; col++)
			{
				for (row = 0; row < tMode[2]; row++)
				{
					rawI[aa_index++] = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);
					//stmsg("sensor (%2d,%2d) RAW (%4d) \n" , col, row, rawI[row]);

					buf_index +=2 ;
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

			//SE A
			for (col = 0; col < tMode[5] / 2; col++)
			{
				for (row = 0; row < tMode[2]; row++)
				{
					buf_index +=2 ; 
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

			//AA B
			for (col = tMode[1] / 2; col < tMode[1]; col++)
			{
				for (row = 0; row < tMode[2]; row++)
				{
					rawI[aa_index++] = (signed short)((raw_buf[buf_index] << 8) + raw_buf[buf_index + 1]);
					//stmsg("sensor (%2d,%2d) RAW (%4d) \n" , col, row, rawI[row]);

					buf_index +=2 ;
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}

			//SE B
			for (col = tMode[5] / 2; col < tMode[5] ; col++)
			{
				for (row = 0; row < tMode[2]; row++)
				{
					buf_index +=2 ; 
				}

				if (tMode[7] != 0)
					buf_index += 2;
			}
			break;
		} else {
			sitronix_spi_pram_rw(false, 0xF004, raw_dat_rd_off, NULL, 2);
		}
	}

rawdata_retry_err:

	if (raw_buf)
		kfree(raw_buf);

	if (retryCounter >= max_retry) {
		return NULL;
	} else {
		return rawI;
	}
}

#ifdef ST_SELFTEST_LOG_FILE
int st_test_open(struct file *filp,loff_t *ppos)
#else
int st_test_open(void)
#endif
{
	int ret = 0;
 
	sitronix_ts_reset_device(gts);
#if defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)
	ret = st_address_mode_hardcode_write(test_flash_afe_df, ARRAY_SIZE(test_flash_afe_df));
	if (ret < 0)
		return ret;

	ret = st_address_mode_hardcode_write(test_cmd_open, ARRAY_SIZE(test_cmd_open));	//Open test without Mux On/Off.
	if (ret < 0)
		return ret;
#endif //defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)

	ret = st_address_mode_hardcode_write(test_cmd_channel_mapping, ARRAY_SIZE(test_cmd_channel_mapping));
	if (ret < 0)
		return ret;

	msleep(100);

#ifdef ST_SELFTEST_LOG_FILE
	ret = st_open_short_test(0, 0, filp, ppos);
#else
	ret = st_open_short_test(0, 0);
#endif
	return ret;
}

#ifdef ST_SELFTEST_LOG_FILE
int normal_rawdata_check(uint8_t raw_cnt, uint8_t aa_unit, signed short *normal_rawdata, struct file *filp, loff_t *ppos)
#else
int normal_rawdata_check(uint8_t raw_cnt, uint8_t aa_unit, signed short *normal_rawdata)
#endif
{
	int col, row, raw_index;
	signed short rawdata;
	int total_error = 0;
#ifdef ST_SELFTEST_LOG_FILE
	char data1[150];
	int i, j;
	int min = 0xFFF0, max = -0xFFF0;
#endif	//ST_SELFTEST_LOG_FILE

#ifdef __RAW_DATA_DEBUG__
	stmsg("NORMAL_RAW:\n");
	for (raw_index = 0, i = 0; i < aa_unit; i++) {
		stmsg("AA_UNIT[%d]: ", i);
		for (j = 0; j < raw_cnt; j++) {
			printk("%d ", normal_rawdata[raw_index++]);
		}
		printk("\n");
	}
#endif //__RAW_DATA_DEBUG__

#ifdef ST_SELFTEST_LOG_FILE
	snprintf(data1, 100, "[NORMAL RAW Data start]\n");
	sitronix_vfswrite(filp, data1, strlen(data1), ppos);

	for (raw_index = 0, i = 0; i < aa_unit; i++) {
		for (j = 0; j < raw_cnt; j++) {
			rawdata = normal_rawdata[raw_index];
			snprintf(data1, 100, "%6d ", rawdata);
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
			raw_index++;
			if (rawdata < min)
				min = (int)rawdata;
			if (rawdata > max)
				max = (int)rawdata;
		}
		snprintf(data1, 100, "\n");
		sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	}

	snprintf(data1, 100, "[NORMAL RAW Data end]        (Min/Max: %d / %d)\n", min, max);
	sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif //ST_SELFTEST_LOG_FILE

	for (raw_index = 0, col = 0; col < aa_unit; col++) {
		for (row = 0; row < raw_cnt; row++) {
			rawdata = normal_rawdata[raw_index];
			if(st_get_test_enable(col, row) && (rawdata < gts->self_test_normal_min || rawdata > gts->self_test_normal_max))
			{
				total_error++;
				sterr("sensor (%2d,%2d) RAW (%4d) out of range (%d ~ %d) in normal rawdata test.\n",
						col, row, rawdata, gts->self_test_normal_min, gts->self_test_normal_max);
#ifdef ST_SELFTEST_LOG_FILE
				snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) out of range (%d ~ %d) in normal rawdata test.\n",
						col, row, rawdata, gts->self_test_normal_min, gts->self_test_normal_max);
				sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif	//ST_SELFTEST_LOG_FILE
			}
			raw_index++;
		}
	}

	return total_error;
}

#ifdef ST_SELFTEST_LOG_FILE
int open_mux_on_off_check_delta(uint8_t raw_cnt, uint8_t aa_unit,
		signed short *mux_on_raw, signed short *mux_off_raw, struct file *filp, loff_t *ppos)
#else
int open_mux_on_off_check_delta(uint8_t raw_cnt, uint8_t aa_unit, signed short *mux_on_raw, signed short *mux_off_raw)
#endif
{
	int col, row, raw_index;
	signed short delta;
	int total_error = 0;
#ifdef ST_SELFTEST_LOG_FILE
	char data1[150];
	int i, j;
	int min = 0xFFF0, max = -0xFFF0;
#endif	//ST_SELFTEST_LOG_FILE

#ifdef __RAW_DATA_DEBUG__
	stmsg("MUX_ON_RAW:\n");
	for (raw_index = 0, i = 0; i < aa_unit; i++) {
		stmsg("AA_UNIT[%d]: ", i);
		for (j = 0; j < raw_cnt; j++) {
			printk("%d ", mux_on_raw[raw_index++]);
		}
		printk("\n");
	}

	stmsg("MUX_OFF_RAW:\n");
	for (raw_index = 0, i = 0; i < aa_unit; i++) {
		stmsg("AA_UNIT[%d]: ", i);
		for (j = 0; j < raw_cnt; j++) {
			printk("%d ", mux_off_raw[raw_index++]);
		}
		printk("\n");
	}

	stmsg("MUX_ON_OFF_DELTA:\n");
	for (raw_index = 0, i = 0; i < aa_unit; i++) {
		stmsg("AA_UNIT[%d]: ", i);
		for (j = 0; j < raw_cnt; j++) {
			delta = mux_on_raw[raw_index] - mux_off_raw[raw_index];
			printk("%d ", delta);
			raw_index++;
		}
		printk("\n");
	}
#endif //__RAW_DATA_DEBUG__

#ifdef ST_SELFTEST_LOG_FILE
	snprintf(data1, 100, "[OPEN RAW Data start]\n");
	sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	for (raw_index = 0, i = 0; i < aa_unit; i++) {
		for (j = 0; j < raw_cnt; j++) {
			delta = mux_on_raw[raw_index] ;
			snprintf(data1, 100, "%6d ", delta);
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
			raw_index++;
			if (delta < min)
				min = (int)delta;
			if (delta > max)
				max = (int)delta;
		}
		snprintf(data1, 100, "\n");
		sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	}
	snprintf(data1, 100, "\n");
	sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	for (raw_index = 0, i = 0; i < aa_unit; i++) {
		for (j = 0; j < raw_cnt; j++) {
			delta = mux_off_raw[raw_index];
			snprintf(data1, 100, "%6d ", delta);
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
			raw_index++;
			if (delta < min)
				min = (int)delta;
			if (delta > max)
				max = (int)delta;
		}
		snprintf(data1, 100, "\n");
		sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	}
	snprintf(data1, 100, "\n");
	sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	for (raw_index = 0, i = 0; i < aa_unit; i++) {
		for (j = 0; j < raw_cnt; j++) {
			delta = mux_on_raw[raw_index] - mux_off_raw[raw_index];
			snprintf(data1, 100, "%6d ", delta);
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
			raw_index++;
			if (delta < min)
				min = (int)delta;
			if (delta > max)
				max = (int)delta;
		}
		snprintf(data1, 100, "\n");
		sitronix_vfswrite(filp, data1, strlen(data1), ppos);
	}

	snprintf(data1, 100, "[OPEN RAW Data end]        (Min/Max: %d / %d)\n", min, max);
	sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif //ST_SELFTEST_LOG_FILE

	for (raw_index = 0, col = 0; col < aa_unit; col++) {
		for (row = 0; row < raw_cnt; row++) {
			delta = mux_on_raw[raw_index] - mux_off_raw[raw_index];
			if(st_get_test_enable(col, row) && (delta < gts->self_test_open_mux_on_off_min))
			{
				total_error++;
				sterr("sensor (%2d,%2d) RAW (%4d) is less than min value(< %d) in open mux on/off test.\n",
						col, row, delta, gts->self_test_open_mux_on_off_min);
#ifdef ST_SELFTEST_LOG_FILE
				snprintf(data1, 100, "sensor (%2d,%2d) RAW (%4d) is less than min value(< %d) in open mux on/off test.\n",
						col, row, delta, gts->self_test_open_mux_on_off_min);
				sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif	//ST_SELFTEST_LOG_FILE
			}
			raw_index++;
		}
	}

	return total_error;
}


#ifdef ST_SELFTEST_LOG_FILE
int st_test_normal_rawdata(struct file *filp, loff_t *ppos)
#else
int st_test_normal_rawdata(void)
#endif
{
	int ret = 0;
	signed short *normal_rawdata = NULL;
	uint8_t raw_cnt, aa_unit;

	if (test_cmd_normal_rawdata == NULL) {
		sterr("Normal rawdata test fail! (test_cmd_normal_rawdata==NULL)\n");
		return -1;
	}

	sitronix_ts_reset_device(gts);
#if defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)
	if (test_flash_afe_df != NULL) {
		ret = st_address_mode_hardcode_write(test_flash_afe_df, ARRAY_SIZE(test_flash_afe_df));
		if (ret < 0)
			goto normal_rawdata_err;
	}
#endif //defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)
	#ifdef ST_ADDRESS_MODE_WRITE_COMMAND_V2
	ret = st_address_mode_hardcode_write_v2(test_cmd_normal_rawdata, ARRAY_SIZE(test_cmd_normal_rawdata));
	#else
	ret = st_address_mode_hardcode_write(test_cmd_normal_rawdata, sizeof(test_cmd_normal_rawdata));
	#endif

	if (ret < 0)
		goto normal_rawdata_err;

	ret = st_address_mode_hardcode_write(test_cmd_channel_mapping, ARRAY_SIZE(test_cmd_channel_mapping));
	if (ret < 0)
		goto normal_rawdata_err;

	stmsg("Normal Raw\n");
#ifdef ST_SELFTEST_LOG_FILE
	normal_rawdata = st_get_aa_rawdata(&raw_cnt, &aa_unit, 0, filp, ppos);
#else //ST_SELFTEST_LOG_FILE
	normal_rawdata = st_get_aa_rawdata(&raw_cnt, &aa_unit, 0);
#endif //ST_SELFTEST_LOG_FILE

	if (!normal_rawdata) {
		sterr("Normal rawdata test fail! (normal_rawdata==NULL)\n");
		ret = -1;
		goto normal_rawdata_err;
	}

#ifdef ST_SELFTEST_LOG_FILE
	ret = normal_rawdata_check(raw_cnt, aa_unit, normal_rawdata, filp, ppos);
#else
	ret = normal_rawdata_check(raw_cnt, aa_unit, normal_rawdata);
#endif
	if (ret < 0)
		goto normal_rawdata_err;

normal_rawdata_err:

	if (normal_rawdata)
		kfree(normal_rawdata);

	msleep(100);

	return ret;
}


#ifdef ST_SELFTEST_LOG_FILE
int st_test_open_mux_on_off(struct file *filp, loff_t *ppos)
#else
int st_test_open_mux_on_off(void)
#endif
{
	int ret = 0;
	signed short *mux_on_raw = NULL;
	signed short *mux_off_raw = NULL;
	uint8_t raw_cnt, aa_unit;

#if  defined(SITRONIX_TEST_ST77921)	// for ST77921 Open test.
#ifdef ST_SELFTEST_LOG_FILE
	ret = st_test_open(filp, ppos);
#else
	ret = st_test_open();
#endif
	msleep(100);
	return ret;
#endif //defined(SITRONIX_TEST_ST77921)

	sitronix_ts_reset_device(gts);

	ret = st_address_mode_hardcode_write(test_cmd_open_mux_on, ARRAY_SIZE(test_cmd_open_mux_on));
	if (ret < 0)
		goto open_mux_on_off_err;

	ret = st_address_mode_hardcode_write(test_cmd_channel_mapping, ARRAY_SIZE(test_cmd_channel_mapping));
	if (ret < 0)
		goto open_mux_on_off_err;

	stmsg("MuxOn Raw\n");
#ifdef ST_SELFTEST_LOG_FILE
	mux_on_raw = st_get_aa_rawdata(&raw_cnt, &aa_unit, 0, filp, ppos);
#else //ST_SELFTEST_LOG_FILE
	mux_on_raw = st_get_aa_rawdata(&raw_cnt, &aa_unit, 0);
#endif //ST_SELFTEST_LOG_FILE

	if (!mux_on_raw) {
		sterr("Open test fail! (mux_on_raw==NULL)\n");
		ret = -1;
		goto open_mux_on_off_err;
	}


	ret = st_address_mode_hardcode_write_v2(test_cmd_open_MuxOff, ARRAY_SIZE(test_cmd_open_MuxOff));
	if (ret < 0)
		goto open_mux_on_off_err;

	ret = st_address_mode_hardcode_write_v2(test_cmd_channel_mapping, ARRAY_SIZE(test_cmd_channel_mapping));
	if (ret < 0)
		goto open_mux_on_off_err;

	stmsg("MuxOff Raw\n");
#ifdef ST_SELFTEST_LOG_FILE
	mux_off_raw = st_get_aa_rawdata(&raw_cnt, &aa_unit, 0, filp, ppos);
#else //ST_SELFTEST_LOG_FILE
	mux_off_raw = st_get_aa_rawdata(&raw_cnt, &aa_unit, 0);
#endif //ST_SELFTEST_LOG_FILE

	if (!mux_off_raw) {
		sterr("Open test fail! (mux_off_raw==NULL)\n");
		ret = -1;
		goto open_mux_on_off_err;
	}

#ifdef ST_SELFTEST_LOG_FILE
	ret = open_mux_on_off_check_delta(raw_cnt, aa_unit, mux_on_raw, mux_off_raw, filp, ppos);
#else
	ret = open_mux_on_off_check_delta(raw_cnt, aa_unit, mux_on_raw, mux_off_raw);
#endif
	if (ret < 0)
		goto open_mux_on_off_err;

open_mux_on_off_err:

	if (mux_on_raw)
		kfree(mux_on_raw);

	if (mux_off_raw)
		kfree(mux_off_raw);

	msleep(100);

	return ret;
}

#ifdef ST_SELFTEST_LOG_FILE
int st_test_short_even(struct file *filp,loff_t *ppos)
#else
int st_test_short_even(void)
#endif
{
	int ret = 0,i=0;

	sitronix_ts_reset_device(gts);
#if defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)
	ret = st_address_mode_hardcode_write(test_flash_afe_df, ARRAY_SIZE(test_flash_afe_df));
	if (ret < 0)
		return ret;
#endif //defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)
	#ifdef ST_ADDRESS_MODE_WRITE_COMMAND_V2
	ret = st_address_mode_hardcode_write_v2(test_cmd_short_even, ARRAY_SIZE(test_cmd_short_even));
	#else
	ret = st_address_mode_hardcode_write(test_cmd_short_even, sizeof(test_cmd_short_even));
	#endif
	if (ret < 0)
		return ret;

	msleep(100);
	for(i=0;i<TEST_RETRY_MAX;i++)
	{
#ifdef ST_SELFTEST_LOG_FILE
#ifdef SITRONIX_TEST_ST7102
	ret = st7102_short_test(2, ST_SELFTEST_SKIP_COLS, filp, ppos);
#else
	ret = st_open_short_test(2, ST_SELFTEST_SKIP_COLS, filp, ppos);
#endif
#else
	ret = st_open_short_test(2, ST_SELFTEST_SKIP_COLS);
#endif
		if(ret == 0)	 break;
	}
	return ret;
}

#ifdef ST_SELFTEST_LOG_FILE
int st_test_short_odd(struct file *filp,loff_t *ppos)
#else
int st_test_short_odd(void)
#endif
{
	int ret = 0,i=0;

	sitronix_ts_reset_device(gts);
#if defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)
	ret = st_address_mode_hardcode_write(test_flash_afe_df, ARRAY_SIZE(test_flash_afe_df));
	if (ret < 0)
		return ret;
#endif //defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)

	ret = st_address_mode_hardcode_write(test_cmd_short_odd, ARRAY_SIZE(test_cmd_short_odd));
	if (ret < 0)
		return ret;

	msleep(100);
	for(i=0;i<TEST_RETRY_MAX;i++)
	{
#ifdef ST_SELFTEST_LOG_FILE
#ifdef SITRONIX_TEST_ST7102
	ret = st7102_short_test(1, ST_SELFTEST_SKIP_COLS, filp, ppos);
#else
	ret = st_open_short_test(1, ST_SELFTEST_SKIP_COLS, filp, ppos);
#endif
#else
	ret = st_open_short_test(1, ST_SELFTEST_SKIP_COLS);
#endif
		if(ret == 0) break;
	}
	return ret;
}

#ifdef ST_SELFTEST_LOG_FILE
int st_test_uniformity(struct file *filp,loff_t *ppos)
#else
int st_test_uniformity(void)
#endif
{
	int ret = 0;

	sitronix_ts_reset_device(gts);
/*
#if defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)
	ret = st_address_mode_hardcode_write(test_flash_afe_df, ARRAY_SIZE(test_flash_afe_df));
	if (ret < 0)
		return ret;
#endif //defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)

	ret = st_address_mode_hardcode_write(test_cmd_uniformity, ARRAY_SIZE(test_cmd_uniformity));
	if (ret < 0)
		return ret;
#ifndef SITRONIX_TEST_ST7102
	ret = st_address_mode_hardcode_write(test_cmd_channel_mapping, ARRAY_SIZE(test_cmd_channel_mapping));
	if (ret < 0)
		return ret;

	msleep(100);
#endif
#ifdef ST_SELFTEST_LOG_FILE
	ret = st_open_short_test(3, 0, filp, ppos);
#else
	ret = st_open_short_test(3, 0);
#endif
*/
	return ret;
}

#ifdef ST_SELFTEST_LOG_FILE
int st_test_std(struct file *filp,loff_t *ppos)
#else
int st_test_std(void)
#endif
{
	int ret = 0,i=0;

	sitronix_ts_reset_device(gts);
#if defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)
	ret = st_address_mode_hardcode_write(test_flash_afe_df, ARRAY_SIZE(test_flash_afe_df));
	if (ret < 0)
		return ret;
#endif //defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)

	ret = st_address_mode_hardcode_write(test_cmd_std, ARRAY_SIZE(test_cmd_std));
	if (ret < 0)
		return ret;

	ret = st_address_mode_hardcode_write(test_cmd_channel_mapping, ARRAY_SIZE(test_cmd_channel_mapping));
	if (ret < 0)
		return ret;

	msleep(100);

	for(i=0;i<TEST_RETRY_MAX;i++)
	{
#ifdef ST_SELFTEST_LOG_FILE
	ret = st_open_short_test(4, 0, filp, ppos);
#else
	ret = st_open_short_test(4, 0);
#endif
		if(ret == 0 ) break;
	}
	return ret;
}


#ifdef ST_SELFTEST_LOG_FILE
void st_record_ic_info(struct file *filp,loff_t *pos)
{
	int ret = 0;
	char data1[255];

	ret = sitronix_ts_get_device_info(gts);
	if (ret < 0 ) {
		snprintf(data1, 50, "sitronix_ts_get_device_info failed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), pos);
	}
	snprintf(data1, 100, "%s\n",SITRONIX_TP_DRIVER_VERSION);
	sitronix_vfswrite(filp, data1, strlen(data1), pos);
	snprintf(data1, 50, "Chip ID = %02X\n", gts->ts_dev_info.chip_id);
	sitronix_vfswrite(filp, data1, strlen(data1), pos);
	snprintf(data1, 50, "FW Verison = %02X\n", gts->ts_dev_info.fw_version);
	sitronix_vfswrite(filp, data1, strlen(data1), pos);
	snprintf(data1, 50, "FW Revision = %02X %02X %02X %02X\n", gts->ts_dev_info.fw_revision[0], gts->ts_dev_info.fw_revision[1], gts->ts_dev_info.fw_revision[2], gts->ts_dev_info.fw_revision[3]);
	sitronix_vfswrite(filp, data1, strlen(data1), pos);
	snprintf(data1, 50, "Resolution = %d x %d\n", gts->ts_dev_info.x_res, gts->ts_dev_info.y_res);
	sitronix_vfswrite(filp, data1, strlen(data1), pos);
	snprintf(data1, 50, "Channels = %d x %d\n", gts->ts_dev_info.x_chs, gts->ts_dev_info.y_chs);
	sitronix_vfswrite(filp, data1, strlen(data1), pos);
	snprintf(data1, 50, "Max touches = %d\n", gts->ts_dev_info.max_touches);
	sitronix_vfswrite(filp, data1, strlen(data1), pos);
	snprintf(data1, 50, "Misc. Info = 0x%X\n", gts->ts_dev_info.misc_info);
	sitronix_vfswrite(filp, data1, strlen(data1), pos);

	ret = sitronix_get_ic_sfrver();
	if (ret < 0 ) {
		snprintf(data1, 50, "sitronix_get_ic_sfrver failed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), pos);
	} else {
		snprintf(data1, 50, "IC SFR VER = 0x%X\n", ret);
		sitronix_vfswrite(filp, data1, strlen(data1), pos);
	}
}
#endif


#define TEST_FW_BUF_LEN			80
#define TEST_FW_FRAME_SKIP		10
#define TEST_FW_FRAME_CNT		10
#define TEST_FW_N_STD			64	//8^2
#ifdef ST_SELFTEST_LOG_FILE
int st_test_fw(struct file *filp,loff_t *ppos)
#else
int st_test_fw(void)
#endif
{	
	int testFailCnt = 0;
	int ret = 0;
	uint8_t *buf = NULL;
	int *rawFrame = NULL;
	int *rawRec = NULL;
	int *rawStd = NULL;
	int lastSenCnt = 0;
	int checkCnt = 0;
	int i, rawCnt, retry, retrymax, ix, iy, datasize, frameNum;
	int *rawP = NULL;
	int tmpsVal;
#ifdef ST_SELFTEST_LOG_FILE
	char data1[150];
	
	snprintf(data1, 100, "[Test Firmware Start]\n");
	sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
	stmsg("[Test Firmware Start]\n");


	buf = kmalloc(TEST_FW_BUF_LEN, GFP_KERNEL);

	//fw info
	if(gts->ts_dev_info.x_chs <= 0 || gts->ts_dev_info.y_chs <= 0){
#ifdef ST_SELFTEST_LOG_FILE
		snprintf(data1, 100, "Failed : Invalid Channel Number = %d x %d\n",
			gts->ts_dev_info.x_chs, gts->ts_dev_info.y_chs);
		sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
		stmsg("Failed : Invalid Channel Number = %d x %d\n", 
				gts->ts_dev_info.x_chs, gts->ts_dev_info.y_chs);

			testFailCnt++;
	}

	rawCnt = (gts->ts_dev_info.x_chs + gts->ts_dev_info.n_chs) * gts->ts_dev_info.y_chs;
	rawFrame =  (int *)kmalloc( rawCnt* sizeof(int), GFP_KERNEL);
	rawStd =  (int *)kmalloc( rawCnt* sizeof(int), GFP_KERNEL);
	rawRec = (int *)kmalloc( (rawCnt* sizeof(int) * TEST_FW_FRAME_CNT), GFP_KERNEL);

	//ic status
	memset(buf, 0, TEST_FW_BUF_LEN);
	ret = sitronix_ts_reg_read(gts, STATUS_REG, buf, 1);
	if(ret < 0){
#ifdef ST_SELFTEST_LOG_FILE
		snprintf(data1, 100, "Error : Read Status register error!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
		sterr("%s: Read Status register error!(%d)\n", __func__, ret);
		testFailCnt++;
	}
	else{
		if((buf[0] & 0xF0) > 0){
#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 100, "Failed : IC Status Error Code = %02X\n", ((buf[0] & 0xF0) >> 4));
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif	
			stmsg("Failed : IC Status Error Code = %02X\n", ((buf[0] & 0xF0) >> 4));

			testFailCnt++;
		}
	}
	
	//sensing count
	for(i = 0 ; i < 5 ; i++){
		memset(buf, 0, TEST_FW_BUF_LEN);
		ret = sitronix_ts_reg_read(gts, SENSING_COUNTER_H, buf, 2);
		if(ret < 0){
#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 100, "Error : Read sensing counter error!\n");
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
			sterr("%s: Read sensing counter error!(%d)\n", __func__, ret);
			testFailCnt++;
		}
		else{
			//stmsg("sensing counter = %02X %02X\n", buf[0], buf[1]);
			if(lastSenCnt != (int)(buf[0] << 8 | buf[1])){
				lastSenCnt = (int)(buf[0] << 8 | buf[1]);
				checkCnt = 0;
			}
			else{
				checkCnt++;
				if(checkCnt >= 3){
#ifdef ST_SELFTEST_LOG_FILE
					snprintf(data1, 100, "Failed : sensing counter error\n");
					sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
					stmsg("Failed : sensing counter error.\n");
					testFailCnt++;
					break;
				}
			}
		}
		msleep(10);
	}
	//touch valid bit
	for(i = 0 ; i < 5 ; i++){
		memset(buf, 0, sizeof(TEST_FW_BUF_LEN));
		ret = sitronix_ts_reg_read(gts, TOUCH_INFO, buf, TEST_FW_BUF_LEN);
		if(ret < 0){
#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 100, "Error : Read touch info error\n");
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
			sterr("%s: Read touch info error!(%d)\n", __func__, ret);		
			testFailCnt++;
		}
		else{
			//stmsg("X0H = %02X\n", buf[4]);
			if((buf[4] & 0x80) != 0){
#ifdef ST_SELFTEST_LOG_FILE
				snprintf(data1, 100, "Failed : touch detected\n");
				sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
				stmsg("Failed : touch detected!\n");
				testFailCnt++;
				break;
			}
		}
		msleep(10);
	}

	
	//stay in Active mode
	memset(buf, 0, TEST_FW_BUF_LEN);
	buf[0] = 0x01;
	buf[1] = 0x06;
	buf[2] = 0x01;
	buf[5] = 0x01;
	buf[7] = 0x26;
	sitronix_ts_reg_write(gts, CMDIO_PORT, buf, 8);

	memset(buf, 0, TEST_FW_BUF_LEN);
	buf[0] = 0x01;
	sitronix_ts_reg_write(gts, CMDIO_CONTROL, buf, 1);
	msleep(50);
	//dist should not be 0 by line
	frameNum = 0;
	retry = 0;
	retrymax = (gts->ts_dev_info.x_chs + gts->ts_dev_info.n_chs) * 2;
	//stmsg("Do Dist test\n");
	while(retry < retrymax){	//wait for header
		ret = sitronix_ts_reg_read(gts, DATA_OUTPUT_BUFFER, buf, TEST_FW_BUF_LEN);
		if(ret < 0){
#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 100, "Error : Wait Dist. error\n");
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
			sterr("%s:Wait Dist. error!(%d)\n", __func__, ret);

			testFailCnt++;
			goto st_test_fw_end;
		}
		if(buf[0] == 0x10)	//Header
		{
			break;
		}
		else{
			retry++;
			if(retry >= retrymax){
#ifdef ST_SELFTEST_LOG_FILE
				snprintf(data1, 100, "Error : Read Dist Header error\n");
				sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
				sterr("Error : Read Dist Header error!\n");
				testFailCnt++;
				goto st_test_fw_end;
			}
		}
	}//end of wait header
	frameNum = 0;
	retry = 0;
	while( (frameNum < (TEST_FW_FRAME_SKIP + TEST_FW_FRAME_CNT)) && retry < retrymax){
		
		ret = sitronix_ts_reg_read(gts, DATA_OUTPUT_BUFFER, buf, TEST_FW_BUF_LEN);
		//stmsg("Do Dist test step 2 type = 0x%02X\n", buf[0]);
		if(ret < 0){
#ifdef ST_SELFTEST_LOG_FILE
			snprintf(data1, 100, "Error : Read Dist error\n");
			sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
			sterr("%s: Read Dist. error!(%d)\n", __func__, ret);
			testFailCnt++;
			goto st_test_fw_end;
		} 
		if(buf[0] == 0x10)
		{	//header
			if(frameNum >= TEST_FW_FRAME_SKIP){
				//verify data
				//printk("Frame 0x%02X ======= \n", frameNum);
				//for(ix = 0; ix < (gts->ts_dev_info.x_chs + gts->ts_dev_info.n_chs); ix++){
				for(ix = 0; ix < (gts->ts_dev_info.x_chs); ix++){
					checkCnt = 0;
					for(iy = 0 ; iy < gts->ts_dev_info.y_chs; iy++){
		
						//printk("%3d ", rawFrame[ix * gts->ts_dev_info.y_chs + iy]);
						if(rawFrame[ix * gts->ts_dev_info.y_chs + iy] == 0){
							checkCnt++;
						}
					}
					//printk("\n");
					if(checkCnt == gts->ts_dev_info.y_chs){
#ifdef ST_SELFTEST_LOG_FILE
						snprintf(data1, 100, "Failed : Dist of X%02d are 0!\n", ix);
						sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
						stmsg("Failed : Dist of X%02d are 0!\n", ix);
						testFailCnt++;
					} 
				}
				rawP = &(rawRec[ rawCnt*(frameNum-TEST_FW_FRAME_SKIP) ]);
				memcpy(rawP, rawFrame, rawCnt* sizeof(int));
				//memcpy(rawRec+((rawCnt* sizeof(short)*(frameNum-TEST_FW_FRAME_SKIP))), rawFrame, rawCnt* sizeof(short));
				//printk("===== =====\n");
				if(testFailCnt > 0){
					goto st_test_fw_end;
				}
			}
			retry = 0;
			frameNum++;
			memset(rawFrame, 0, rawCnt* sizeof(int));
			if(frameNum >= (TEST_FW_FRAME_SKIP + TEST_FW_FRAME_CNT)){
				break;
			}
		}//endof header
		else if(buf[0] == 0x93){	//AA
			ix = buf[2];
			iy = buf[3];
			datasize =  (buf[1] - 3) / 2; 
			for(i = 0; i < datasize; i++){   
				rawFrame[ix * gts->ts_dev_info.y_chs + iy + i] = (signed short)((buf[2 + i*2 + 2] << 8) | (buf[2+ i*2 + 3]));
			}
			retry = 0;
		}
		else if(buf[0] == 0x95){	//noise
			ix = buf[2] + gts->ts_dev_info.x_chs;
			iy = buf[3];
			datasize =  (buf[1] - 3) / 2; 
			for(i = 0; i < datasize; i++){   
				rawFrame[ix * gts->ts_dev_info.y_chs + iy + i] = (signed short)((buf[2 + i*2 + 2] << 8) | (buf[2+ i*2 + 3]));
			}
			retry = 0;
		}
		//else if(buf[0] == 0x00){	//not ready
		//	retry++;
		//}
		else{	//unexpected data, include 0x00 data not ready
			retry++;
			if(buf[0] == 0x00){
				msleep(10);
			}
			if(retry >= retrymax){
#ifdef ST_SELFTEST_LOG_FILE
				snprintf(data1, 100, "Error : Read Dist. timeout!\n");
				sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif	
				sterr("%s: Read Dist. timeout!\n", __func__);
				testFailCnt++;
				goto st_test_fw_end;
			}
		}
	}//end of read dist

	//std of noise line0/1 < 8
	memset(rawFrame, 0, rawCnt* sizeof(int));
	memset(rawStd, 0, rawCnt* sizeof(int));
	for(i = 0 ; i < TEST_FW_FRAME_CNT; i++){
		rawP = &(rawRec[ rawCnt * i]);
		for(ix = 0; ix < (gts->ts_dev_info.x_chs + gts->ts_dev_info.n_chs); ix++){
			for(iy = 0 ; iy < gts->ts_dev_info.y_chs; iy++){
				rawFrame[ix * gts->ts_dev_info.y_chs + iy] += rawP[ix * gts->ts_dev_info.y_chs + iy];
				tmpsVal = 0;
			}
		}
	}//end of sum

	for(ix = 0; ix < (gts->ts_dev_info.x_chs + gts->ts_dev_info.n_chs); ix++){
		for(iy = 0 ; iy < gts->ts_dev_info.y_chs; iy++){
			rawFrame[ix * gts->ts_dev_info.y_chs + iy] = rawFrame[ix * gts->ts_dev_info.y_chs + iy] / TEST_FW_FRAME_CNT;
		}
	}//end of mean
	for(i = 0 ; i < TEST_FW_FRAME_CNT; i++){
		rawP = &(rawRec[ rawCnt * i]);
		for(ix = 0; ix < (gts->ts_dev_info.x_chs + gts->ts_dev_info.n_chs); ix++){
			for(iy = 0 ; iy < gts->ts_dev_info.y_chs; iy++){
				tmpsVal = rawP[ix * gts->ts_dev_info.y_chs + iy] - rawFrame[ix * gts->ts_dev_info.y_chs + iy];
				rawStd[ix * gts->ts_dev_info.y_chs + iy] += (tmpsVal*tmpsVal);
			}
		}
	}//end of sum of diffence square
	//printk("=== STD ===== \n");
	for(ix = 0; ix < (gts->ts_dev_info.x_chs + gts->ts_dev_info.n_chs); ix++){
		for(iy = 0 ; iy < gts->ts_dev_info.y_chs; iy++){
			rawStd[ix * gts->ts_dev_info.y_chs + iy] = rawStd[ix * gts->ts_dev_info.y_chs + iy] / TEST_FW_FRAME_CNT;
			//printk("%3d ", rawStd[ix * gts->ts_dev_info.y_chs + iy]);

			if(ix == gts->ts_dev_info.x_chs || ix == (gts->ts_dev_info.x_chs+1)){	//verify noise 0/1 line std
				if(rawStd[ix * gts->ts_dev_info.y_chs + iy] > TEST_FW_N_STD){
#ifdef ST_SELFTEST_LOG_FILE
					snprintf(data1, 100, "Failed : STD^2 of Noise(%2d, %2d) = %d", 
							ix, iy, rawStd[ix * gts->ts_dev_info.y_chs + iy]);
					sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
					stmsg("Failed : STD^2 of Noise(%2d, %2d) = %d", 
						ix, iy, rawStd[ix * gts->ts_dev_info.y_chs + iy] );
					testFailCnt++;
				}
			}
		}
		//printk("\n");
	}//end of STD square
	//printk("==========\n");

st_test_fw_end:
#ifdef ST_SELFTEST_LOG_FILE
	snprintf(data1, 100, "[Test Firmware End]\n");
	sitronix_vfswrite(filp, data1, strlen(data1), ppos);
#endif
	stmsg("[Test Firmware End]\n");

	if (buf)
		kfree(buf);
	if (rawFrame)
		kfree(rawFrame);
	if (rawRec)
		kfree(rawRec);
	if (rawStd)
		kfree(rawStd);

	return (testFailCnt>0)?1:0;
}//end of st_test_fw


/*
return of st_self_test
/ ret = 0 : test success
/ ret < 0 : test failed with error
/ ret > 0 : test failed with N sensors :
*/
int st_self_test(void)
{
	int ret = 0, i=0;
	int result_normal_rawdata = 0;
	int result_open = 0;
	int result_short_odd = 0;
	int result_short_even = 0;
	int result_uniformity = 0;
	int result_std = 0;
	int result_fw = 0;
	char ic_position[2];
#ifdef ST_SELFTEST_LOG_FILE
	struct file *filp;
	char data1[50];
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,18,0)
	mm_segment_t fs;
#endif
	loff_t pos;

	filp = filp_open(ST_SELFTEST_LOG_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	sitonix_createlogfileok = true;
	if (IS_ERR(filp))
	{
		sitonix_createlogfileok = false;
		sterr("ST open %s error...\n", ST_SELFTEST_LOG_PATH);
		//return -1;
	}
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
	fs = get_fs();
	set_fs(KERNEL_DS);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5,18,0)
    fs = force_uaccess_begin();
#else
	//do nothing
#endif

	pos = 0;

	st_record_ic_info(filp, &pos);

#endif	
	//get IC position
	sitronix_get_ic_position(ic_position);
	stmsg("IC position X,Y = (%03d,%03d)\n", (((ic_position[0] & 0x1) <<4) | ic_position[1]),(ic_position[0] >> 4));
#ifdef ST_SELFTEST_LOG_FILE
	snprintf(data1, 50, "IC position X,Y = (%03d,%03d)\n", (((ic_position[0] & 0x1) <<4) | ic_position[1]),(ic_position[0] >> 4));
	sitronix_vfswrite(filp, data1, strlen(data1), &pos);

#if 0
	//fw test
	result_fw = st_test_fw(filp, &pos);
	if( result_fw == 0) {
		stmsg("Test Firmware successed!\n");
		snprintf(data1, 50, "Test Firmware successed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	} else {
		stmsg("Test Firmware failed!\n");
		snprintf(data1, 50, "Test Firmware failed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	}
#endif //0
	result_open = result_short_even = result_short_odd = result_std = result_normal_rawdata = 0;
	//open test(mux on/off)
	for(i=0;i<TEST_RETRY_MAX;i++)
	{
#ifdef SITRONIX_TEST_ST7102
	result_open = st7102_test_open(filp, &pos);
#else
	result_open = st_test_open_mux_on_off(filp, &pos);
#endif
		if(result_open == 0)	break;
	}
	if( result_open == 0) {
		stmsg("Test open successed!\n");
		snprintf(data1, 50, "Test open successed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	} else {
		stmsg("Test open failed!\n");
		snprintf(data1, 50, "Test open failed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	}

	//short test even
	result_short_even = st_test_short_even(filp, &pos);
	if( result_short_even == 0) {
		stmsg("Test short_even successed!\n");
		snprintf(data1, 50, "Test short_even successed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	} else {
		stmsg("Test short_even failed!\n");
		snprintf(data1, 50, "Test short_even failed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	}

	//short test odd
	result_short_odd = st_test_short_odd(filp, &pos);
	if( result_short_odd == 0) {
		stmsg("Test short_odd successed!\n");
		snprintf(data1, 50, "Test short_odd successed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	} else {
		stmsg("Test short_odd failed! %d\n",result_short_odd);
		snprintf(data1, 50, "Test short_odd failed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	}

#ifdef SITRONIX_UNIFORMITY_TEST_ENABLED
	//uniformity test
	result_uniformity = st_test_uniformity(filp, &pos);
	if( result_uniformity == 0) {
		stmsg("Test uniformity successed!\n");
		snprintf(data1, 50, "Test uniformity successed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	} else {
		stmsg("Test uniformity failed!\n");
		snprintf(data1, 50, "Test uniformity failed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	}
#endif //SITRONIX_UNIFORMITY_TEST_ENABLED

	//STD test
	result_std = st_test_std(filp, &pos);
	if( result_std == 0) {
		stmsg("Test STD successed!\n");
		snprintf(data1, 50, "Test STD successed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	} else {
		stmsg("Test STD failed!\n");
		snprintf(data1, 50, "Test STD failed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	}

	//normal rawdata test
#ifdef SITRONIX_TEST_ST7102
	result_normal_rawdata = st7102_test_normal(filp, &pos);
#else
	result_normal_rawdata = st_test_normal_rawdata(filp, &pos);
#endif
	if( result_normal_rawdata == 0) {
		stmsg("Test normal rawdata successed!\n");
		snprintf(data1, 50, "Test normal rawdata successed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	} else {
		stmsg("Test normal rawdata failed!\n");
		snprintf(data1, 50, "Test normal rawdata failed!\n");
		sitronix_vfswrite(filp, data1, strlen(data1), &pos);
	}


#else

#if 0
	//firmware test
	result_fw = st_test_fw();
	if( result_fw == 0)
		stmsg("Test Firmware successed!\n");
	else
		stmsg("Test Firmware failed!\n");
#endif //0

	//normal rawdata test
	result_normal_rawdata = st_test_normal_rawdata();
	if( result_normal_rawdata == 0)
		stmsg("Test normal rawdata successed!\n");
	else
		stmsg("Test normal rawdata failed!\n");

	//open test(mux on/off)
	result_open = st_test_open_mux_on_off();
	if( result_open == 0)
		stmsg("Test open successed!\n");
	else
		stmsg("Test open failed!\n");
	//short test odd
	result_short_odd = st_test_short_odd();
	if( result_short_odd == 0)
		stmsg("Test short_odd successed!\n");
	else
		stmsg("Test short_odd failed!\n");	
	//short test even
	result_short_even = st_test_short_even();
	if( result_short_even == 0)
		stmsg("Test short_even successed!\n");
	else
		stmsg("Test short_even failed!\n");

#ifdef SITRONIX_UNIFORMITY_TEST_ENABLED
	//uniformity test
	result_uniformity = st_test_uniformity();
	if( result_uniformity == 0)
		stmsg("Test uniformity successed!\n");
	else
		stmsg("Test uniformity failed!\n");
#endif //SITRONIX_UNIFORMITY_TEST_ENABLED

	//STD test
	result_std = st_test_std();
	if( result_std == 0)
		stmsg("Test STD successed!\n");
	else
		stmsg("Test STD failed!\n");

#endif
#ifdef SITRONIX_TEST_ST7102	
	ret = st_address_mode_hardcode_write(EnterSleepOut, ARRAY_SIZE(EnterSleepOut));
#endif
	if ( result_open != 0 || result_short_odd != 0 || result_short_even != 0 || result_uniformity != 0 || result_std != 0 || result_fw != 0)
		ret = -1;
	else
		ret = result_open + result_short_odd + result_short_even + result_uniformity + result_std + result_fw;

#ifdef ST_SELFTEST_LOG_FILE

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
	set_fs(fs);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5,18,0)
    force_uaccess_end(fs);
#else
	//do nothing
#endif


	if(sitonix_createlogfileok) {
		filp_close(filp, NULL);
		stmsg("Test log file : %s\n", ST_SELFTEST_LOG_PATH);
	}
#endif

	return ret;
}

void sitronix_set_default_test_criteria(void)
{
	gts->self_test_normal_min = ST_SELFTEST_NORMAL_MIN;
	gts->self_test_normal_max = ST_SELFTEST_NORMAL_MAX;
	gts->self_test_short_max = ST_SELFTEST_SHORT_MAX;
	gts->self_test_open_min = ST_SELFTEST_OPEN_MIN;
	gts->self_test_open_max = ST_SELFTEST_OPEN_MAX;
	gts->self_test_open_mux_on_off_min = ST_SELFTEST_OPEN_MUX_ON_OFF_MIN;
	gts->self_test_uniformity_shift = ST_SELFTEST_UNIFORMITY_SHIFT;
	gts->self_test_uniformity_min = ST_SELFTEST_UNIFORMITY_MIN;
	gts->self_test_uniformity_max = ST_SELFTEST_UNIFORMITY_MAX;
	gts->self_test_std_max = ST_SELFTEST_STD_MAX;
	
	sitronix_request_test_criteria(ST_SELFTEST_INI_PATH);

	gts->self_test_std_square100_max = gts->self_test_std_max * gts->self_test_std_max;
}

void sitronix_replace_test_cmd(unsigned char *id)
{
#ifdef ST_REPLACE_TEST_CMD_BY_DISPLAY_ID
	if(id[0] == test_id_1[0] && id[1] == test_id_1[1] && id[2] == test_id_1[2]) {
		//do replace here
		stmsg("find display id = %X %X %X , replace test_cmd_*_id1 to test_cmd_*  \n",test_id_1[0], test_id_1[1], test_id_1[2]);

		if (test_cmd_normal_rawdata != NULL)
			memcpy(test_cmd_normal_data, test_cmd_open_normal_rawdata_id1, SITRONIX_TEST_CMD_NORMAL_MAX_LEN);

#if defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)
		if (test_cmd_open != NULL)
			memcpy(test_cmd_open , test_cmd_open_id1, SITRONIX_TEST_CMD_OPEN_MAX_LEN);
#endif //defined(SITRONIX_TEST_ST7123) || defined(SITRONIX_TEST_ST77921)

		if (test_cmd_open_mux_on != NULL)
			memcpy(test_cmd_open_mux_on , test_cmd_open_mux_on_id1, SITRONIX_TEST_CMD_OPEN_MAX_LEN);

		if (test_cmd_open_mux_off != NULL)
			memcpy(test_cmd_open_mux_off , test_cmd_open_mux_off_id1, SITRONIX_TEST_CMD_OPEN_MAX_LEN);

		memcpy(test_cmd_short_odd , test_cmd_short_odd_id1, SITRONIX_TEST_CMD_SHORT_ODD_MAX_LEN);
		memcpy(test_cmd_short_even , test_cmd_short_even_id1, SITRONIX_TEST_CMD_SHORT_EVEN_MAX_LEN);
		memcpy(test_cmd_uniformity , test_cmd_uniformity_id1, SITRONIX_TEST_CMD_UNIFORMITY_MAX_LEN);
		memcpy(test_cmd_std , test_cmd_std_id1, SITRONIX_TEST_CMD_STD_MAX_LEN);
		memcpy(golden_buf , golden_buf_id1, sizeof(golden_buf));
		
		gts->self_test_normal_min = ST_SELFTEST_NORMAL_MIN_ID1;
		gts->self_test_normal_max = ST_SELFTEST_NORMAL_MAX_ID1;
		gts->self_test_short_max = ST_SELFTEST_SHORT_MAX_ID1;
		gts->self_test_open_min = ST_SELFTEST_OPEN_MIN_ID1;
		gts->self_test_open_max = ST_SELFTEST_OPEN_MAX_ID1;
		gts->self_test_open_mux_on_off_min = ST_SELFTEST_OPEN_MUX_ON_OFF_MIN_ID1;
		gts->self_test_uniformity_shift = ST_SELFTEST_UNIFORMITY_SHIFT_ID1;
		gts->self_test_uniformity_min = ST_SELFTEST_UNIFORMITY_MIN_ID1;
		gts->self_test_uniformity_max = ST_SELFTEST_UNIFORMITY_MAX_ID1;
		gts->self_test_std_max = ST_SELFTEST_STD_MAX_ID1;

		sitronix_request_test_criteria(ST_SELFTEST_INI_PATH_ID1);

		gts->self_test_std_square100_max = gts->self_test_std_max * gts->self_test_std_max;
	}
	
#endif	/* ST_REPLACE_TEST_CMD_BY_DISPLAY_ID */
}

int sitronix_request_test_criteria(const char *name){
#ifdef ST_REQUEST_SELF_TEST_INI	
	u8 *tmp = NULL;
	u8 *buff = NULL;
	int ret = 0;
	const struct firmware *fw = NULL;

	ret = request_firmware(&fw, name, &gts->pdev->dev);
	if (ret == 0) {
		stmsg("selftesst INI request(%s) success\n", name);
		buff = kzalloc(fw->size, GFP_KERNEL);
		//stmsg("INI size is:%d\n", fw->size);
		if (!buff) {
			sterr("selftest INI buffer kzalloc fail\n");
			return -EPERM;
		}
		memcpy(buff, fw->data, fw->size);
		release_firmware(fw);

		/* self_test_normal_min */
		tmp = strstr(buff, "self_test_normal_min=");
		if (tmp != NULL) {
			ret = sscanf(tmp, "self_test_normal_min=%d", &gts->self_test_normal_min);
			if (ret < 0)
				sterr("%s read self_test_normal_min error.\n", __func__);
			//stmsg("self_test_normal_min = %d\n", gts->self_test_normal_min);
		}
		/* self_test_normal_max */
		tmp = strstr(buff, "self_test_normal_max=");
		if (tmp != NULL) {
			ret = sscanf(tmp, "self_test_normal_max=%d", &gts->self_test_normal_max);
			if (ret < 0)
				sterr("%s read self_test_normal_max error.\n", __func__);
			//stmsg("self_test_normal_max = %d\n", gts->self_test_normal_max);
		}

		/* self_test_short_max */
		tmp = strstr(buff, "self_test_short_max=");
		if (tmp != NULL) {
			ret = sscanf(tmp, "self_test_short_max=%d", &gts->self_test_short_max);
			if (ret < 0)
				sterr("%s read self_test_short_max error.\n", __func__);
			//stmsg("self_test_short_max = %d\n", gts->self_test_short_max);
		}
		/* self_test_open_min */
		tmp = strstr(buff, "self_test_open_min=");
		if (tmp != NULL) {
			ret = sscanf(tmp, "self_test_open_min=%d", &gts->self_test_open_min);
			if (ret < 0)
				sterr("%s read self_test_open_min error.\n", __func__);
			//stmsg("self_test_open_min = %d\n", gts->self_test_open_min);
		}
		/* self_test_open_max */
		tmp = strstr(buff, "self_test_open_max=");
		if (tmp != NULL) {
			ret = sscanf(tmp, "self_test_open_max=%d", &gts->self_test_open_max);
			if (ret < 0)
				sterr("%s read self_test_open_max error.\n", __func__);
			//stmsg("self_test_open_max = %d\n", gts->self_test_open_max);
		}
		/* self_test_open_mux_on_off_min */
		tmp = strstr(buff, "self_test_open_mux_on_off_min=");
		if (tmp != NULL) {
			ret = sscanf(tmp, "self_test_open_mux_on_off_min=%d", &gts->self_test_open_mux_on_off_min);
			if (ret < 0)
				sterr("%s read self_test_open_mux_on_off_min error.\n", __func__);
			//stmsg("self_test_open_mux_on_off_min = %d\n", gts->self_test_open_mux_on_off_min);
		}
		/* self_test_uniformity_shift */
		tmp = strstr(buff, "self_test_uniformity_shift=");
		if (tmp != NULL) {
			ret = sscanf(tmp, "self_test_uniformity_shift=%d", &gts->self_test_uniformity_shift);
			if (ret < 0)
				sterr("%s read self_test_uniformity_shift error.\n", __func__);
			//stmsg("self_test_uniformity_shift = %d\n", gts->self_test_uniformity_shift);
		}
		/* self_test_uniformity_min */
		tmp = strstr(buff, "self_test_uniformity_min=");
		if (tmp != NULL) {
			ret = sscanf(tmp, "self_test_uniformity_min=%d", &gts->self_test_uniformity_min);
			if (ret < 0)
				sterr("%s read self_test_uniformity_min error.\n", __func__);
			//stmsg("self_test_uniformity_min = %d\n", gts->self_test_uniformity_min);
		}
		/* self_test_uniformity_max */
		tmp = strstr(buff, "self_test_uniformity_max=");
		if (tmp != NULL) {
			ret = sscanf(tmp, "self_test_uniformity_max=%d", &gts->self_test_uniformity_max);
			if (ret < 0)
				sterr("%s read self_test_uniformity_max error.\n", __func__);
			//stmsg("self_test_uniformity_max = %d\n", gts->self_test_uniformity_max);
		}
		/* self_test_std_max */
		tmp = strstr(buff, "self_test_std_max=");
		if (tmp != NULL) {
			ret = sscanf(tmp, "self_test_std_max=%d", &gts->self_test_std_max);
			if (ret < 0)
				sterr("%s read self_test_std_max error.\n", __func__);
			//stmsg("self_test_std_max = %d\n", gts->self_test_std_max);
		}
		kfree(buff);
	} else {
		sterr("firmware request(%s) fail,ret=%d", name, ret);
	}
#endif //ST_REQUEST_SELF_TEST_INI
	return 0;
}

//[CC]FIH factory functions
bool sitonix_resultselftest = false; // FIH self test result


int sitronix_selftest_result_read(void)
{
    int num_read_chars = 0;
    if(sitonix_resultselftest)
        num_read_chars = 0;
    else
        num_read_chars = 1;
    return num_read_chars;
}

#ifdef USE_ST_SQRT
int st_sqrt(int x) {
	int l , h, mid, sqrt;
	if (x <= 1) {
        return x;
    }
    l = 1;
	h = x;
    while (l <= h) {
        mid = l + (h - l) / 2;
        sqrt = x / mid;
        if (sqrt == mid) {
            return mid;
        } else if (mid > sqrt) {
            h = mid - 1;
        } else {
            l = mid + 1;
        }
    }
    return h;
}
#endif
