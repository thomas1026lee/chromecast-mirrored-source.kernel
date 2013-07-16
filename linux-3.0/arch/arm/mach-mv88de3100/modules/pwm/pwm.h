/******************************************************************************* 
* Copyright (C) Marvell International Ltd. and its affiliates 
* 
 * Marvell GPL License Option
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File in accordance with the terms and conditions of the General
 * Public License Version 2, June 1991 (the "GPL License"), a copy of which is
 * available along with the File in the license.txt file or by writing to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
 * on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
 * DISCLAIMED.  The GPL License provides additional details about this warranty
 * disclaimer.
 ********************************************************************************/

#ifndef __GALOIS_PWM_H__
#define __GALOIS_PWM_H__

/*
 * define the register offset in each PWM channel
 */
#define ENABLE_REG_OFFSET	0x0
#define CONTROL_REG_OFFSET	0x4
#define DUTY_REG_OFFSET		0x8
#define TERMCNT_REG_OFFSET	0xC

/*
 * ioctl command
 */
#define PWM_IOCTL_READ  0x1234
#define PWM_IOCTL_WRITE 0x4567

typedef struct galois_pwm_rw {
	uint offset;	/* the offset of register in PWM */
	uint data;		/* the value will be read/write from/to register */
} galois_pwm_rw_t;

#endif /* __GALOIS_PWM_H__ */
