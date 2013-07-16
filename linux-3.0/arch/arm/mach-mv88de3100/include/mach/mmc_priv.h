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

#ifndef __ASM_ARM_ARCH_MMC_PRIV_H
#define __ASM_ARM_ARCH_MMC_PRIV_H

#define MV_FLAG_ENABLE_CLOCK_GATING (1<<0)

#define MV_FLAG_CARD_PERMANENT (1 << 1)

#define MV_FLAG_SD_8_BIT_CAPABLE_SLOT (1 << 2)

#define MV_FLAG_MMC_CARD	(1<<3)

#define MV_FLAG_HOST_OFF_CARD_ON	(1<<4)

extern struct platform_device mv88de3100_sdhc_dev0;
extern struct platform_device mv88de3100_sdhc_dev1;
extern struct platform_device mv88de3100_mmc_dev;

struct sdhci_mv_mmc_platdata {
	unsigned int 	flags;
	unsigned int 	clk_delay_cycles;
	unsigned int 	clk_delay_sel;
	bool 		clk_delay_enable;
	unsigned int	ext_cd_gpio;
	bool 		ext_cd_gpio_invert;
	unsigned int	max_speed;
	unsigned int	host_caps;
	unsigned int	quirks;
	unsigned int	pm_caps;
	void		(*set_controller_version)(void);
	void		(*set_sdio_voltage)(int voltage);
	unsigned int	(*get_sdio_tx_hold_delay)(void);
	int		(*get_sdio_pad_voltage)(void);
};

struct sdhci_mv_mmc {
	u8 clk_enable;
	u8 power_mode;
};
#endif
