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
#include <asm/hardware/gic.h>
#define HAVE_GET_IRQNR_PREAMBLE
#include <asm/hardware/entry-macro-gic.S>

	.macro  disable_fiq
	.endm

	.macro  get_irqnr_preamble, base, tmp
	ldr	\base, =MEMMAP_GIC_CPU_BASE
	.endm

	.macro  arch_ret_to_user, tmp1, tmp2
	.endm
