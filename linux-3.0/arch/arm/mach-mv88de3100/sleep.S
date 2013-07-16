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

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/system.h>

	.text

ENTRY(mv88de3100_cpu_suspend)
	stmfd	sp!, {r4 - r12, lr}	@ save registers on stack

	mov	r1, r0
	ldr	r3, =resume_with_mmu	@ resume function
	bl	cpu_suspend
	bl	EnterStandby_2
20:
	nop
	b	20b

	.align 5
resume_with_mmu:
	ldmfd	sp!, {r4 - r12, pc}		@ return to caller

	.data
	.align
ENTRY(mv88de3100_cpu_resume)
	b	cpu_resume
ENDPROC(mv88de3100_cpu_resume)
