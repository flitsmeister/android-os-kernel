/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (C) 2021 MediaTek Inc.
*/

#ifndef _MT_PMIC_IPI_H_
#define _MT_PMIC_IPI_H_

void pmic_ipi_write_test(void);

#ifdef CONFIG_MTK_EXTBUCK
enum { /* must compatible with sspm EXTBUCK ID */
	EXTBUCK_ID_GPU,
	EXTBUCK_ID_PROC2,
	EXTBUCK_ID_PROC1,
};

int extbuck_ipi_enable(unsigned char buck_id, unsigned char en);
#else
int extbuck_ipi_enable(unsigned char buck_id, unsigned char en)
{
	return -1;
}
#endif /* CONFIG_MTK_EXTBUCK */

unsigned int pmic_ipi_read_interface(unsigned int RegNum,
				     unsigned int *val,
				     unsigned int MASK,
				     unsigned int SHIFT,
				     unsigned char lock);
unsigned int pmic_ipi_config_interface(unsigned int RegNum,
				       unsigned int val,
				       unsigned int MASK,
				       unsigned int SHIFT,
				       unsigned char lock);

unsigned int pmic_regulator_profiling(unsigned char type);

unsigned int sub_pmic_ipi_interface(unsigned int type, unsigned int ctrl);

unsigned int mt6311_ipi_set_mode(unsigned char mode);

#endif /* _MT_PMIC_IPI_H_*/

