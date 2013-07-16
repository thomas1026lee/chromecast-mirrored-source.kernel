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

/*
 * CONFIGURATIONS
 */

#ifndef ASSERT
#   define ASSERT(x)                /* {if (!(x))  while (1);} */
#endif /* ASSERT */

#define DO_TRACE_LOG                (0)

#if DO_TRACE_LOG
int printf(const char *fmt, ...);
#define TRACE_LOG               printf
#else
#define TRACE_LOG(...)
#endif

/* We use random seed to make it work for all chip that requires randomization.
 */
#define RANDOM_SEED

/*
 * INCLUDES
 */
#include "prbs.h"
#include "nand_randomizer.h"

/*
 * CONSTANTS
 */
#ifndef TRUE
#define TRUE				(1)
#endif

#ifndef FALSE
#define FALSE				(0)
#endif

#ifndef NULL
#define NULL				(0)
#endif


#define NAND_ID_MAX_SIZE                (8)
#define NAND_DEFAULT_OOB_SIZE		(32)

#define IS_POWER_OF_2(x)                ( !( (x) & ((x)-1) ) )
#define ARRAYSIZE(a)                    (sizeof(a)/sizeof(*a))

#define PAGE_START_POS_ALIGNED_BYTES    (4)
#define PAGE_START_POS_MASK(cycle_len)	                                \
	((cycle_len) - 1) & (~(PAGE_START_POS_ALIGNED_BYTES - 1))

/** NAND randomizer types.
 *
 * We only support SAMSUNG PRBS-15 at present.
 * @sa SAMSUNG Recommendation for a randomizer v0.1
 */
enum mv_nand_randomizer_type_e {
	MV_NAND_RANDOMIZER_FROM_MEMORY,     /** Use the input buffer directly   */
	MV_NAND_RANDOMIZER_SAMSUNG_PRBS15,  /** Generate by PRBS-15 with seeds
                                             *  defined by SAMSUNG.
                                             */
	MV_NAND_RANDOMIZER_TYPE_MAX
};

/*
 * TYPES
 */

/** NAND Randomizer chip information.
 */
struct nand_randomized_chip_info_s {
	int chip_id_len;
	unsigned char chip_id[NAND_ID_MAX_SIZE];
	unsigned int block_size;
	unsigned int page_size;
	unsigned int spare_size;
	enum mv_nand_randomizer_type_e randomizer_type;
	unsigned int randomizer_buffer_length;
};


/** NAND Randomizer.
 */
struct mv_nand_randomizer_s {
	int          chip_randomized;
	struct nand_randomized_chip_info_s *chip_info;

	unsigned int block_size;
	unsigned int page_size;
	unsigned int oob_size;

	unsigned int page_per_block;
	unsigned int block_shift;
	unsigned int page_mask;

	unsigned int random_data_buffer_length;
	unsigned char *p_random_data_buffer;
};

/*
 * VARIABLES
 */
static const struct nand_randomized_chip_info_s g_nand_randomized_chip_list[] = {
	/* SAMSUNG K9GBG08U0A, 32Gb */
	{
		6,
		{0xEC, 0xD7, 0x94, 0x7A, 0x54, 0x43},
		8192 * 128,
		8192,
		640,
		MV_NAND_RANDOMIZER_SAMSUNG_PRBS15,
		4096
	}
	/* SAMSUNG K9GBG08U0B, 32Gb */
	,{
		6,
		{0xEC, 0xD7, 0x94, 0x7E, 0x64, 0x44},
		8192 * 128,
		8192,
		1024,
		MV_NAND_RANDOMIZER_SAMSUNG_PRBS15,
		4096
	}
	/* TOSHIBA TC58NVG4D2HTA00, 16Gb */
	,{
		6,
		{0x98, 0xD5, 0x84, 0x32, 0x72, 0x56},
		8192 * 128,
		8192,
		640,
		MV_NAND_RANDOMIZER_SAMSUNG_PRBS15,
		4096
	}
	/* SAMSUNG K9LCG08U0A/K9HDG08U1A, 64Gb/128Gb
	 * K9HDG08U1A consist 2 pieces of K9LCG08U0A, 1 CE(Chip Enable) pin
	 * for 1 piece.
	 */
	,{
		6,
		{0xEC, 0xDE, 0xD5, 0x7A, 0x58, 0x43},
		8192 * 128,
		8192,
		640,
		MV_NAND_RANDOMIZER_SAMSUNG_PRBS15,
		4096
	}
	/* MICRON MT29F32G08CBxBx/MT29F64G08CxxBx, 32Gb/64Gb */
	,{
		5,
		{0x2C, 0x68, 0x04, 0x46, 0x89},
		4096 * 256,
		4096,
		224,
		MV_NAND_RANDOMIZER_SAMSUNG_PRBS15,
		4096
	}
	/* MICRON MT29F16G08CBACA, 16Gb */
	,{
		5,
		{0x2C, 0x48, 0x04, 0x4A, 0xA5},
		4096 * 256,
		4096,
		224,
		MV_NAND_RANDOMIZER_SAMSUNG_PRBS15,
		4096
	}
	/* HYNIX H27UBG8T2BTR, 32Gb */
	,{
		6,
		{0xAD, 0xDE, 0x94, 0xD2, 0x04, 0x43},
		8192 * 256,
		8192,
		448,
		MV_NAND_RANDOMIZER_SAMSUNG_PRBS15,
		4096
	}
};

/* Samsung randomizer.
 */

#define NAND_RANDOMIZER_SEED_SAMSUNG                (0x576A)

/* Global randomizer. */
static struct mv_nand_randomizer_s g_nand_randomizer;


/*
 * PRIVATE FUNCTIONS
 */

#define DUMP_8_BYTES(name, b)							\
	if (b) {                                                                \
		TRACE_LOG("%s(0x%08X): %02X %02X %02X %02X %02X %02X %02X %02X\n", \
		name, (int)(b),                                                 \
		(b)[0], (b)[1], (b)[2], (b)[3], (b)[4], (b)[5], (b)[6], (b)[7]);\
	}


/** Get shift of x.
 */
static unsigned int get_shift(unsigned int x)
{
	unsigned int s = 0;
	ASSERT(x > 0 && IS_POWER_OF_2(x));

	for (s=0; s<sizeof(x)*8; s++) {
		if ((1 << s) == x) {
			break;
		}
	}
	return s;
}

/** Get the random data by PRBS generator
 *
 */
static unsigned int gen_prbs15_random_data_samsung(unsigned char *p_random_data_buffer,
						   unsigned int   random_data_length)
{
	unsigned short seed = NAND_RANDOMIZER_SEED_SAMSUNG;

	ASSERT(p_random_data_buffer);
	ASSERT(random_data_length);

	prbs15_gen(PRBS_POLYNOMIAL_DEFAULT,
		   seed,
		   p_random_data_buffer,
		   random_data_length,
		   FALSE);
	return random_data_length;
}

/** Randomize data by XOR way.
 *
 * For each byte, *p_dest = *p_random_data ^ *p_src.
 *
 * @param p_random_data             Random data.
 * @param p_src                     Source data.
 * @param p_dst                     Buffer to save the randomized data.
 * @param length                    Length of the data to randomize.
 *
 * @return void
 *
 * @sa mv_nand_randomizer_init(), mv_nand_randomizer_get_page().
 */
static void randomize_by_xor(const unsigned char *p_random_data,
			     const unsigned char *p_src,
			     unsigned char       *p_dst,
			     int                  length)
{
	int i;
	const int aligned_mask = sizeof(unsigned int) - 1;
	ASSERT(p_random_data);
	ASSERT(p_src);
	ASSERT(p_dst);
	ASSERT(length > 0);

	/* if it's 4-byte aligned, then we use a faster way to do so            */
	if ((0 == ((int)p_random_data & aligned_mask)) &&
	    (0 == ((int)p_src & aligned_mask)) &&
	    (0 == ((int)p_dst & aligned_mask)) &&
	    (0 == (length & aligned_mask))) {
		const unsigned int *p_random_data_32 = (const unsigned int *)p_random_data;
		const unsigned int *p_src_32 = (const unsigned int *)p_src;
		unsigned int *p_dst_32 = (unsigned int *)p_dst;
		int length_32 = length/sizeof(*p_random_data_32);

		for (i=0; i<length_32; i++) {
			p_dst_32[i] = p_src_32[i] ^ p_random_data_32[i];
		}
	} else {
		for (i=0; i<length; i++) {
			p_dst[i] = p_src[i] ^ p_random_data[i];
		}
	}
}

/** It will randomize the data in ring.
 *
 * @param p_random_data             Random data.
 * @param random_data_length        Length of the random data.
 * @param p_src                     Source data.
 * @param p_dst                     Buffer to save the randomized data.
 * @param length                    Length of the data to randomize, must not
 *                                  be larger than a page_random_data_length.
 * @param start                     Start position in the page to randomize.
 *
 * @return void
 *
 * @sa randomize_by_xor()
 */
static void randomize_by_xor_ring(const unsigned char *p_random_data,
				  unsigned int         random_data_length,
				  const unsigned char *p_src,
				  unsigned char       *p_dst,
				  unsigned int         length,
				  unsigned int         start)
{
	unsigned int randomized_length = 0;
	unsigned int offset = start;

	/* make sure offset is in a page */
	while (offset >= random_data_length) {
		offset -= random_data_length;
	}

	while (randomized_length < length) {
		unsigned int processing_length = random_data_length - offset;
		if (processing_length > length - randomized_length) {
			processing_length = length - randomized_length;
		}

		if (offset != 0) {
			DUMP_8_BYTES("RANDOM ", p_random_data + offset);
		}

		randomize_by_xor(p_random_data + offset,
				 p_src + randomized_length,
				 p_dst + randomized_length,
				 processing_length);
		offset = 0;
		randomized_length += processing_length;
	}
}

/** Get the randomized chip info.
 *
 * @param p_chip_id             Chip ID data.
 * @param chip_id_len           Chip ID length.
 *
 * @retval  NULL                Chip is unrandomized.
 * @retval !NULL                Chip is randomized, return the chip info.
 *
 * @sa
 */
static const struct nand_randomized_chip_info_s*
mv_nand_get_chip_randomized_info(const unsigned char *p_chip_id,
				 int                  chip_id_len)
{
	const struct nand_randomized_chip_info_s *p_found_chip_info = NULL;
	int chip_index;

	/* If no chip_id input, then default is enabled	*/
	if (NULL == p_chip_id || chip_id_len <= 0) {
		TRACE_LOG("no chip_id input, use default randomizer.......\n");
		return &g_nand_randomized_chip_list[0];
	}

	for (chip_index=0; chip_index < ARRAYSIZE(g_nand_randomized_chip_list); chip_index++) {
		const struct nand_randomized_chip_info_s * p_chip_info =
			&g_nand_randomized_chip_list[chip_index];
		int compare_len = chip_id_len;
		int found = TRUE;
		int i;

		if ((0 == compare_len) ||
		    (compare_len > p_chip_info->chip_id_len)) {
			compare_len = p_chip_info->chip_id_len;
		}

		/* we don't use memcmp() or strncmp() to make the function get better
		 * compatibility.
		 */
		for (i=0; i<compare_len; i++) {
			if (p_chip_id[i] != p_chip_info->chip_id[i]) {
				found = FALSE;
				break;
			}
		}

		if (found) {
			p_found_chip_info = p_chip_info;
			break;
		}
	}

	TRACE_LOG("mv_nand_chip_randomized(%02X %02X %02X %02X %02X %02X): !!!%s!!!\n",
		  p_chip_id[0], p_chip_id[1], p_chip_id[2], p_chip_id[3],
		  p_chip_id[4], p_chip_id[5],
		  p_found_chip_info ? "RANDOMIZED" : "UNRANDOMIZED");

	return p_found_chip_info;
}

/** Check whether a block is randomized.
 *
 * @param block_index               Block index to check.
 *
 * @retval 0                        Chip is not randomized.
 * @retval !0                       Chip is randomized.
 *
 * @sa
 */
static int mv_nand_block_randomized(unsigned int block_index)
{
	/* Modify it to support other blocks */
	const unsigned int unrandomized_block_table[] = {
		/* block 0    */0,
		/* bootloader */1, 2, 3, 4, 5, 6, 7, 8
	};
	int i;
	int randomized = TRUE;

	for (i=0; i<ARRAYSIZE(unrandomized_block_table); i++) {
		if (block_index == unrandomized_block_table[i]) {
			randomized = FALSE;
			break;
		}
	}
	return randomized;
}

/** Get the start random data pos for a page.
 *
 * @param page_addr                 Address of a page.
 *
 * @return unsigned int             Start pos in random data for the page.
 *
 * @sa
 */
static unsigned int mv_nand_get_page_start(unsigned int page_addr)
{
	struct mv_nand_randomizer_s * p_nr = &g_nand_randomizer;
	unsigned int page_index_in_block = page_addr & p_nr->page_mask;

	unsigned int start;
	/* 2 for each pos need 2 bytes
	 * we don't use pos = page_addr * 2; because LEAPimg need repeat the seed
	 * in each block, for the which block is bad is unexpected.
	 */
	unsigned int pos = page_index_in_block * 2;
	pos = pos & (p_nr->random_data_buffer_length - 1);

	start = (p_nr->p_random_data_buffer[pos + 1] << 8) +
		p_nr->p_random_data_buffer[pos];

	start &= PAGE_START_POS_MASK(p_nr->random_data_buffer_length);
	/* & p_nr->random_data_buffer_length is to make sure it won't exceed random
	 * data length.
	 * & (~0x3) is to make sure it's 32-bit aligned
	 */

	return start;
}

/** The function to do real randomization.
 *
 * It will be called by all randomization functions.
 *
 * @param page_addr                 Address of the page (start from 0).
 * @param p_src                     Source data.
 * @param p_dst                     Buffer to save the randomized data.
 * @param length                    Length of the data to randomize, must not
 *                                  be larger than a page_random_data_length.
 * @param offset_in_page            Start position in the page to randomize.
 *
 * @return void
 *
 * @sa
 */
static void mv_nand_do_randomize(unsigned int         page_addr,
				 const unsigned char *p_src,
				 unsigned char       *p_dst,
				 unsigned int         length,
				 unsigned int         offset_in_page)
{
	struct mv_nand_randomizer_s * p_nr = &g_nand_randomizer;

	int start = mv_nand_get_page_start(page_addr);
	randomize_by_xor_ring(p_nr->p_random_data_buffer,
			      p_nr->random_data_buffer_length,
			      p_src,
			      p_dst,
			      length,
			      start + offset_in_page);
}
/*
 * PUBLIC FUNCTIONS
 */

int mv_nand_chip_randomized(const unsigned char *p_chip_id,
			    int                  chip_id_len,
			    unsigned int        *p_randomizer_buffer_length)
{
	const struct nand_randomized_chip_info_s *p_randomized_chip_info = NULL;
	p_randomized_chip_info = mv_nand_get_chip_randomized_info(p_chip_id, chip_id_len);

	if (p_randomizer_buffer_length) {
		*p_randomizer_buffer_length = 0;
	}

	if (!p_randomized_chip_info) {
		return FALSE;
	}

	if (p_randomizer_buffer_length) {
		*p_randomizer_buffer_length = p_randomized_chip_info->randomizer_buffer_length;
	}
	return TRUE;
}

int mv_nand_randomizer_init(const unsigned char *p_chip_id,
			    int                  chip_id_len,
			    unsigned int         block_size,
			    unsigned int         page_size,
			    unsigned int         oob_size,
			    unsigned char       *p_randomizer_buffer,
			    unsigned int         randomizer_buffer_length)
{
	struct mv_nand_randomizer_s *p_nr = &g_nand_randomizer;
	const struct nand_randomized_chip_info_s *p_randomized_chip_info = NULL;

	ASSERT(IS_POWER_OF_2(page_size));
	ASSERT(IS_POWER_OF_2(block_size));

	TRACE_LOG("mv_nand_randomizer_init(block_size=%d, page_size=%d, oob_size=%d, buf_len=%d)\n",
		  block_size, page_size, oob_size, randomizer_buffer_length);

	if (NULL == p_randomizer_buffer || 0 == randomizer_buffer_length) {
		mv_nand_chip_randomized(p_chip_id, chip_id_len, &randomizer_buffer_length);
		return randomizer_buffer_length;
	}

	p_nr->p_random_data_buffer      = p_randomizer_buffer;

	/* we initialize randomizer ASAP, to make sure the the randomizer can be initialized
	 * as unrandomized first.
	 */
	/*memset(p_nr, 0, sizeof(*p_nr));*/
	p_nr->chip_randomized           = FALSE;
	p_nr->chip_info                 = NULL;

	p_randomized_chip_info = mv_nand_get_chip_randomized_info(p_chip_id, chip_id_len);

	if (NULL == p_randomized_chip_info) {
		return 0;
	}

	if (0 == block_size) {
		block_size = p_randomized_chip_info->block_size;
	}

	if (0 == page_size) {
		page_size = p_randomized_chip_info->page_size;
	}

	if (0 == oob_size) {
		oob_size = NAND_DEFAULT_OOB_SIZE;
	}

	p_nr->block_size                = block_size;
	p_nr->page_size                 = page_size;
	p_nr->oob_size                  = oob_size;

	/* calculate the masks and shifts */
	p_nr->page_per_block = block_size / page_size;
	p_nr->page_mask = p_nr->page_per_block - 1;
	p_nr->block_shift = get_shift(p_nr->page_per_block);

	if (randomizer_buffer_length < p_randomized_chip_info->randomizer_buffer_length) {
		return -1;
	} else {
		randomizer_buffer_length = p_randomized_chip_info->randomizer_buffer_length;
	}

	if (MV_NAND_RANDOMIZER_SAMSUNG_PRBS15 == p_randomized_chip_info->randomizer_type) {
		gen_prbs15_random_data_samsung(p_randomizer_buffer,
					       randomizer_buffer_length);
	}

	ASSERT(IS_POWER_OF_2(randomizer_buffer_length));
	p_nr->chip_randomized = TRUE;
	p_nr->random_data_buffer_length = randomizer_buffer_length;
	return randomizer_buffer_length;
}

unsigned int mv_nand_randomizer_randomize(unsigned int         page_addr,
					  const unsigned char *p_src,
					  unsigned char       *p_dst,
					  unsigned int         length,
					  unsigned int         offset_in_page)
{
	const struct mv_nand_randomizer_s *p_nr = &g_nand_randomizer;
	unsigned int block_index = page_addr >> p_nr->block_shift;
	/*unsigned int page_index = page_addr & p_nr->page_mask;*/

	int need_set_bad_marker = FALSE;
	unsigned char bad_marker = 0;
	unsigned int bad_marker_pos = 0;
	ASSERT(p_src);
	ASSERT(p_dst);
	ASSERT(length > 0);

	if (!p_nr->chip_randomized || !mv_nand_block_randomized(block_index)) {
		return 0;
	}

	if ((offset_in_page <= p_nr->page_size) &&
	    ((offset_in_page + length) > p_nr->page_size)) {
		need_set_bad_marker = TRUE;
		bad_marker_pos = p_nr->page_size - offset_in_page;
		bad_marker = p_src[bad_marker_pos];
	}

	mv_nand_do_randomize(page_addr, p_src, p_dst, length, offset_in_page);

	if (need_set_bad_marker) {
		p_dst[bad_marker_pos] = bad_marker;
	}

	return length;
}

unsigned int mv_nand_randomizer_randomize_page(unsigned int         page_addr,
					       const unsigned char *p_data_src,
					       const unsigned char *p_oob_src,
					       unsigned char       *p_data_dst,
					       unsigned char       *p_oob_dst)
{
	const struct mv_nand_randomizer_s *p_nr = &g_nand_randomizer;
	unsigned int block_index = page_addr >> p_nr->block_shift;
#if DO_TRACE_LOG
	unsigned int page_index = page_addr & p_nr->page_mask;
#endif /* DO_TRACE_LOG */
	unsigned int randomized_length = 0;

	if (!p_nr->chip_randomized || !mv_nand_block_randomized(block_index)) {
		TRACE_LOG("nand_randomize_page(0x%08X): UNRONDOMIZED\n", page_addr);
		return 0;
	}

	TRACE_LOG("mv_nand_randomizer_randomize_page(0x%08X): block %d, page %d, !!! RANDOMIZED !!!.\n",
		  page_addr, block_index, page_index);

	if (p_data_src) {
		/* randomize page data  */
		DUMP_8_BYTES("DATA_SRC", p_data_src);
		mv_nand_do_randomize(page_addr, p_data_src, p_data_dst, p_nr->page_size, 0);
		randomized_length += p_nr->page_size;
		DUMP_8_BYTES("DATA_DST", p_data_dst);
	}

	if (p_oob_src) {
		unsigned char bad_marker = p_oob_src[0];
		/* randomize oob data   */
		DUMP_8_BYTES("OOB_SRC", p_oob_src);

		mv_nand_do_randomize(page_addr, p_oob_src, p_oob_dst, p_nr->oob_size, p_nr->page_size);
		/* we don't randomize the bad block marker  */
		p_oob_dst[0] = bad_marker;
		randomized_length += p_nr->oob_size;
		DUMP_8_BYTES("OOB_DST", p_oob_dst);
	}

	return randomized_length;
}

