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

#include <mach/galois_platform.h>

#define _SM_APB_REG_BASE		(MEMMAP_SM_REG_BASE + 0x40000)
#define _SM_APB_UART0_BASE		(_SM_APB_REG_BASE + 0x9000)

	.macro	addruart, rp, rv
	ldr	\rp, =_SM_APB_UART0_BASE
	ldr	\rv, =_SM_APB_UART0_BASE	@ virtual = physical
	.endm

#define UART_SHIFT	2
#include <asm/hardware/debug-8250.S>
