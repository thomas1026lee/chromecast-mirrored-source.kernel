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

#ifndef	__GALOIS_PINMUX_HEADER__
#define	__GALOIS_PINMUX_HEADER__

#include <mach/ctypes.h>

#define	PINMUX_IOCTL_READ			0x5100
#define	PINMUX_IOCTL_WRITE			0x5101
#define	PINMUX_IOCTL_SETMOD			0x5102
#define	PINMUX_IOCTL_GETMOD			0x5103
#define	PINMUX_IOCTL_PRINTSTATUS		0x5104

#define	SOC_PINMUX				0
#define	SM_PINMUX				1


#define RA_Gbl_pinMux				0x0000

#define LSb32Gbl_pinMux_gp0			0
#define bGbl_pinMux_gp0 			1
#define MSK32Gbl_pinMux_gp0			0x00000001

#define LSb32Gbl_pinMux_gp1			1
#define bGbl_pinMux_gp1 			2
#define MSK32Gbl_pinMux_gp1			0x00000006

#define LSb32Gbl_pinMux_gp2			3
#define bGbl_pinMux_gp2 			2
#define MSK32Gbl_pinMux_gp2			0x00000018

#define LSb32Gbl_pinMux_gp3			5
#define bGbl_pinMux_gp3 			2
#define MSK32Gbl_pinMux_gp3			0x00000060

#define LSb32Gbl_pinMux_gp4			7
#define bGbl_pinMux_gp4 			2
#define MSK32Gbl_pinMux_gp4			0x00000180

#define LSb32Gbl_pinMux_gp5			9
#define bGbl_pinMux_gp5 			3
#define MSK32Gbl_pinMux_gp5			0x00000E00

#define LSb32Gbl_pinMux_gp6			12
#define bGbl_pinMux_gp6 			2
#define MSK32Gbl_pinMux_gp6			0x00003000

#define LSb32Gbl_pinMux_gp7			14
#define bGbl_pinMux_gp7 			3
#define MSK32Gbl_pinMux_gp7			0x0001C000

#define LSb32Gbl_pinMux_gp8			17
#define bGbl_pinMux_gp8 			3
#define MSK32Gbl_pinMux_gp8			0x000E0000

#define LSb32Gbl_pinMux_gp9			20
#define bGbl_pinMux_gp9 			3
#define MSK32Gbl_pinMux_gp9			0x00700000

#define LSb32Gbl_pinMux_gp10			23
#define bGbl_pinMux_gp10			2
#define MSK32Gbl_pinMux_gp10			0x01800000

#define LSb32Gbl_pinMux_gp11			25
#define bGbl_pinMux_gp11			2
#define MSK32Gbl_pinMux_gp11			0x06000000

#define LSb32Gbl_pinMux_gp12			27
#define bGbl_pinMux_gp12			3
#define MSK32Gbl_pinMux_gp12			0x38000000


#define RA_Gbl_pinMux1				0x0004

#define LSb32Gbl_pinMux_gp13			0
#define bGbl_pinMux_gp13			3
#define MSK32Gbl_pinMux_gp13			0x00000007

#define LSb32Gbl_pinMux_gp14			3
#define bGbl_pinMux_gp14			1
#define MSK32Gbl_pinMux_gp14			0x00000008

#define LSb32Gbl_pinMux_gp15			4
#define bGbl_pinMux_gp15			2
#define MSK32Gbl_pinMux_gp15			0x00000030

#define LSb32Gbl_pinMux_gp16			6
#define bGbl_pinMux_gp16			3
#define MSK32Gbl_pinMux_gp16			0x000001C0

#define LSb32Gbl_pinMux_gp17			9
#define bGbl_pinMux_gp17			3
#define MSK32Gbl_pinMux_gp17			0x00000E00

#define LSb32Gbl_pinMux_gp18			12
#define bGbl_pinMux_gp18			1
#define MSK32Gbl_pinMux_gp18			0x00001000

#define LSb32Gbl_pinMux_gp19			13
#define bGbl_pinMux_gp19			1
#define MSK32Gbl_pinMux_gp19			0x00002000

#define LSb32Gbl_pinMux_gp20			14
#define bGbl_pinMux_gp20			1
#define MSK32Gbl_pinMux_gp20			0x00004000

#define LSb32Gbl_pinMux_gp21			15
#define bGbl_pinMux_gp21			3
#define MSK32Gbl_pinMux_gp21			0x00038000

#define LSb32Gbl_pinMux_gp22			18
#define bGbl_pinMux_gp22			3
#define MSK32Gbl_pinMux_gp22			0x001C0000

#define LSb32Gbl_pinMux_gp23			21
#define bGbl_pinMux_gp23			3
#define MSK32Gbl_pinMux_gp23			0x00E00000

#define LSb32Gbl_pinMux_gp24			24
#define bGbl_pinMux_gp24			2
#define MSK32Gbl_pinMux_gp24			0x03000000

#define LSb32Gbl_pinMux_gp25			26
#define bGbl_pinMux_gp25			2
#define MSK32Gbl_pinMux_gp25			0x0C000000

#define LSb32Gbl_pinMux_gp26			28
#define bGbl_pinMux_gp26			1
#define MSK32Gbl_pinMux_gp26			0x10000000

#define LSb32Gbl_pinMux_gp27			29
#define bGbl_pinMux_gp27			1
#define MSK32Gbl_pinMux_gp27			0x20000000

#define LSb32Gbl_pinMux_gp28			30
#define bGbl_pinMux_gp28			2
#define MSK32Gbl_pinMux_gp28			0xC0000000


#define RA_smSysCtl_SM_GSM_SEL			0x0040


#define LSb32smSysCtl_SM_GSM_SEL_GSM0_SEL	0
#define bsmSysCtl_SM_GSM_SEL_GSM0_SEL		2
#define MSK32smSysCtl_SM_GSM_SEL_GSM0_SEL	0x00000003


#define LSb32smSysCtl_SM_GSM_SEL_GSM1_SEL	2
#define bsmSysCtl_SM_GSM_SEL_GSM1_SEL		2
#define MSK32smSysCtl_SM_GSM_SEL_GSM1_SEL	0x0000000C


#define LSb32smSysCtl_SM_GSM_SEL_GSM2_SEL	4
#define bsmSysCtl_SM_GSM_SEL_GSM2_SEL		2
#define MSK32smSysCtl_SM_GSM_SEL_GSM2_SEL	0x00000030


#define LSb32smSysCtl_SM_GSM_SEL_GSM3_SEL	6
#define bsmSysCtl_SM_GSM_SEL_GSM3_SEL		2
#define MSK32smSysCtl_SM_GSM_SEL_GSM3_SEL	0x000000C0


#define LSb32smSysCtl_SM_GSM_SEL_GSM4_SEL	8
#define bsmSysCtl_SM_GSM_SEL_GSM4_SEL		2
#define MSK32smSysCtl_SM_GSM_SEL_GSM4_SEL	0x00000300


#define LSb32smSysCtl_SM_GSM_SEL_GSM5_SEL	10
#define bsmSysCtl_SM_GSM_SEL_GSM5_SEL		2
#define MSK32smSysCtl_SM_GSM_SEL_GSM5_SEL	0x00000C00


#define LSb32smSysCtl_SM_GSM_SEL_GSM6_SEL	12
#define bsmSysCtl_SM_GSM_SEL_GSM6_SEL		2
#define MSK32smSysCtl_SM_GSM_SEL_GSM6_SEL	0x00003000


#define LSb32smSysCtl_SM_GSM_SEL_GSM7_SEL	14
#define bsmSysCtl_SM_GSM_SEL_GSM7_SEL		1
#define MSK32smSysCtl_SM_GSM_SEL_GSM7_SEL	0x00004000


#define LSb32smSysCtl_SM_GSM_SEL_GSM8_SEL	15
#define bsmSysCtl_SM_GSM_SEL_GSM8_SEL		1
#define MSK32smSysCtl_SM_GSM_SEL_GSM8_SEL	0x00008000


#define LSb32smSysCtl_SM_GSM_SEL_GSM9_SEL	16
#define bsmSysCtl_SM_GSM_SEL_GSM9_SEL		1
#define MSK32smSysCtl_SM_GSM_SEL_GSM9_SEL	0x00010000


#define LSb32smSysCtl_SM_GSM_SEL_GSM10_SEL	17
#define bsmSysCtl_SM_GSM_SEL_GSM10_SEL		1
#define MSK32smSysCtl_SM_GSM_SEL_GSM10_SEL	0x00020000


#define LSb32smSysCtl_SM_GSM_SEL_GSM11_SEL	18
#define bsmSysCtl_SM_GSM_SEL_GSM11_SEL		1
#define MSK32smSysCtl_SM_GSM_SEL_GSM11_SEL	0x00040000


#define w32smSysCtl_SM_GSM_SEL			{\
	UNSG32 uSM_GSM_SEL_GSM0_SEL		: 2;\
	UNSG32 uSM_GSM_SEL_GSM1_SEL		: 2;\
	UNSG32 uSM_GSM_SEL_GSM2_SEL		: 2;\
	UNSG32 uSM_GSM_SEL_GSM3_SEL		: 2;\
	UNSG32 uSM_GSM_SEL_GSM4_SEL		: 2;\
	UNSG32 uSM_GSM_SEL_GSM5_SEL		: 2;\
	UNSG32 uSM_GSM_SEL_GSM6_SEL		: 2;\
	UNSG32 uSM_GSM_SEL_GSM7_SEL		: 1;\
	UNSG32 uSM_GSM_SEL_GSM8_SEL		: 1;\
	UNSG32 uSM_GSM_SEL_GSM9_SEL		: 1;\
	UNSG32 uSM_GSM_SEL_GSM10_SEL		: 1;\
	UNSG32 uSM_GSM_SEL_GSM11_SEL		: 1;\
	UNSG32 RSVDx40_b19			: 13;\
}

#define w32Gbl_pinMux				{\
	UNSG32 upinMux_gp0			: 1;\
	UNSG32 upinMux_gp1			: 2;\
	UNSG32 upinMux_gp2			: 2;\
	UNSG32 upinMux_gp3			: 2;\
	UNSG32 upinMux_gp4			: 2;\
	UNSG32 upinMux_gp5			: 3;\
	UNSG32 upinMux_gp6			: 2;\
	UNSG32 upinMux_gp7			: 3;\
	UNSG32 upinMux_gp8			: 3;\
	UNSG32 upinMux_gp9			: 3;\
	UNSG32 upinMux_gp10			: 2;\
	UNSG32 upinMux_gp11			: 2;\
	UNSG32 upinMux_gp12			: 3;\
	UNSG32 RSVDx0_b30			: 2;\
}

#define w32Gbl_pinMux1				{\
	UNSG32 upinMux_gp13			: 3;\
	UNSG32 upinMux_gp14			: 1;\
	UNSG32 upinMux_gp15			: 2;\
	UNSG32 upinMux_gp16			: 3;\
	UNSG32 upinMux_gp17			: 3;\
	UNSG32 upinMux_gp18			: 1;\
	UNSG32 upinMux_gp19			: 1;\
	UNSG32 upinMux_gp20			: 1;\
	UNSG32 upinMux_gp21			: 3;\
	UNSG32 upinMux_gp22			: 3;\
	UNSG32 upinMux_gp23			: 3;\
	UNSG32 upinMux_gp24			: 2;\
	UNSG32 upinMux_gp25			: 2;\
	UNSG32 upinMux_gp26			: 1;\
	UNSG32 upinMux_gp27			: 1;\
	UNSG32 upinMux_gp28			: 2;\
}


typedef union  T32smSysCtl_SM_GSM_SEL {
	UNSG32 u32;
	struct w32smSysCtl_SM_GSM_SEL;
} T32smSysCtl_SM_GSM_SEL;

typedef union  T32Gbl_pinMux {
	UNSG32 u32;
	struct w32Gbl_pinMux;
} T32Gbl_pinMux;

typedef union  T32Gbl_pinMux1 {
	UNSG32 u32;
	struct w32Gbl_pinMux1;
} T32Gbl_pinMux1;

typedef	struct galois_pinmux_data {
	int owner;
	int group;
	int value;
	char *requster;
} galois_pinmux_data_t;

#endif
