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

#include <linux/io.h>
#include <mach/galois_platform.h>

#define APB_UART_LCR	0x0C
#define APB_UART_LSR	0x14
#define APB_UART_THR	0x00

static inline void putc(int c)
{
	unsigned int value;
	void __iomem *uart_base = (void __iomem *)APB_UART_INST0_BASE;

	/*
	 * LCR[7]=1, known as DLAB bit, is used to enable rw DLL/DLH registers.
	 * it must be cleared(LCR[7]=0) to allow accessing other registers.
	 */
	value = __raw_readl(uart_base + APB_UART_LCR);
	value &= ~0x00000080;
	__raw_writel(value, uart_base + APB_UART_LCR);

	while (!(__raw_readl(uart_base + APB_UART_LSR) & 0x00000020))
		barrier();

	__raw_writel(c, uart_base + APB_UART_THR);
}

static inline void flush(void)
{
}

/*
 * nothing to do
 */
#define arch_decomp_setup()
#define arch_decomp_wdog()
