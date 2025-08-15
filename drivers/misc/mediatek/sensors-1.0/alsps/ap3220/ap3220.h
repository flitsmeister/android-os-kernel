/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 *
 * Filename: ap3220.h
 *
 * Summary:
 *	AP3220 sensor dirver header file.
 *
 * Modification History:
 * Date(dd/mm/yy)	 By		Summary
 * -------- -------- -------------------------------------------------------
 * 09/11/20	Leo	First release for MT6771
 */

/*
 * Definitions for AP3220 als/ps sensor chip.
 */
#ifndef __AP3220_H__
#define __AP3220_H__

#include <linux/ioctl.h>

#define AP3220_SUCCESS						0
#define AP3220_ERR_I2C						-1
#define AP3220_ERR_STATUS					-3
#define AP3220_ERR_SETUP_FAILURE			-4
#define AP3220_ERR_GETGSENSORDATA			-5
#define AP3220_ERR_IDENTIFICATION			-6
/******************************************************************************
 * extern functions
*******************************************************************************/
extern void mt_eint_mask(unsigned int eint_num);
extern void mt_eint_unmask(unsigned int eint_num);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow,
							void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);

#endif
