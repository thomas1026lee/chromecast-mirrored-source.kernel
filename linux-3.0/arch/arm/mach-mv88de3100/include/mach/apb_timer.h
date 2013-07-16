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

#ifndef GALOIS_APBT_H
#define GALOIS_APBT_H

/* Langwell DW APB timer registers */
#define APBTMR_N_LOAD_COUNT    0x00
#define APBTMR_N_CURRENT_VALUE 0x04
#define APBTMR_N_CONTROL       0x08
#define APBTMR_N_EOI           0x0c
#define APBTMR_N_INT_STATUS    0x10

#define APBTMRS_INT_STATUS     0xa0
#define APBTMRS_EOI            0xa4
#define APBTMRS_RAW_INT_STATUS 0xa8
#define APBTMRS_COMP_VERSION   0xac
#define APBTMRS_REG_SIZE       0x14

/* register bits */
#define APBTMR_CONTROL_ENABLE  (1<<0)
#define APBTMR_CONTROL_MODE_PERIODIC   (1<<1) /*1: periodic 0:free running */
#define APBTMR_CONTROL_INT     (1<<2)

/* timer: LINUX_TIMER=0, CLOCKSOURCE= 7 */
#define LINUX_TIMER		0
#define GALOIS_DEBUG_TIMER	7
#define GALOIS_CLOCKSOURCE  (GALOIS_DEBUG_TIMER)

#endif
