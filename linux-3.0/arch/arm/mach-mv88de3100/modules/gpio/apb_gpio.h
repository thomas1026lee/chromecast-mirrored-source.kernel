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

#ifndef __APB_GPIO_H__
#define __APB_GPIO_H__

/*
 * The GPIO inst0 is located at (MEMMAP_APBPERIF_REG_BASE + 0x0C00)
 * i.e. 0xf7f30c00.
 */
#define APB_GPIO_SWPORTA_DR		0x00
#define APB_GPIO_SWPORTA_DDR		0x04
#define APB_GPIO_PORTA_CTL		0x08
#define APB_GPIO_SWPORTB_DR		0x0c
#define APB_GPIO_SWPORTB_DDR		0x10
#define APB_GPIO_PORTB_CTL		0x14
#define APB_GPIO_SWPORTC_DR		0x18
#define APB_GPIO_SWPORTC_DDR		0x1c
#define APB_GPIO_PORTC_CTL		0x20
#define APB_GPIO_SWPORTD_DR		0x24
#define APB_GPIO_SWPORTD_DDR		0x28
#define APB_GPIO_PORTD_CTL		0x2c
#define APB_GPIO_INTEN			0x30
#define APB_GPIO_INTMASK		0x34
#define APB_GPIO_INTTYPE_LEVEL		0x38
#define APB_GPIO_INT_POLARITY		0x3c
#define APB_GPIO_INTSTATUS		0x40
#define APB_GPIO_RAWINTSTATUS		0x44
#define APB_GPIO_DEBOUNCE		0x48
#define APB_GPIO_PORTA_EOI		0x4c
#define APB_GPIO_EXT_PORTA		0x50
#define APB_GPIO_EXT_PORTB		0x54
#define APB_GPIO_EXT_PORTC		0x58
#define APB_GPIO_EXT_PORTD		0x5c
#define APB_GPIO_LS_SYNC		0x60
#define APB_GPIO_ID_CODE		0x64
#define APB_GPIO_RESERVED		0x68
#define APB_GPIO_COMP_VERSION		0x6c

#endif
