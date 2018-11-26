/*
 * (C) Copyright 2007-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "boot0_i.h"

#include <string.h>

#define BOOT_FEL_FLAG  (0x5AA5A55A)

extern const boot0_file_head_t  BT0_head;
static void clear_ZI( void );
static void print_version(void);

extern unsigned int get_fel_flag(void);
extern void show_rtc_reg(void);
extern void clear_fel_flag(void);


/*******************************************************************************
* Function name:		Boot0_C_part
* Function prototype£º	void Boot0_C_part( void )
* Function:				Boot0 (The main flow of the part written in C language)
* Entry parameter:		void
* Return value:			void
* Remarks:
*******************************************************************************/
void Boot0_C_part( void )
{
	__u32 status;
	__s32 dram_size;
	int	index = 0;
	int ddr_aotu_scan = 0;

	__u32 fel_flag;

//	move_RW( );
	clear_ZI( );

	bias_calibration();
#if defined(CONFIG_ARCH_SUN9IW1P1) || defined(CONFIG_ARCH_SUN8IW6P1)
	//do nothing
#else
	timer_init();
#endif

	UART_open( BT0_head.prvt_head.uart_port, (void *)BT0_head.prvt_head.uart_ctrl, 24*1000*1000 );
	if( BT0_head.prvt_head.enable_jtag )
	{
		jtag_init( (normal_gpio_cfg *)BT0_head.prvt_head.jtag_gpio );
	}
	msg("HELLO! BOOT0 is starting!\n");
	print_version();

#ifdef CONFIG_ARCH_SUN7I
	reset_cpux(1);
#endif

	fel_flag = get_fel_flag();
	show_rtc_reg();
	if(fel_flag == BOOT_FEL_FLAG)
	{
		clear_fel_flag();
		msg("eraly jump fel\n");
		pll_reset();
		__msdelay(10);

		jump_to( FEL_BASE );
    }

	mmu_system_init(EGON2_DRAM_BASE, 1 * 1024, EGON2_MMU_BASE);
	mmu_enable();

	ddr_aotu_scan = 0;
//	dram_para_display();
	dram_size = init_DRAM(ddr_aotu_scan, (void *)BT0_head.prvt_head.dram_para);
	if(dram_size)
	{
		//mdfs_save_value();
		msg("dram size =%d\n", dram_size);
	}
	else
	{
		msg("initializing SDRAM Fail.\n");
		mmu_disable( );

		pll_reset();
		jump_to( FEL_BASE );
	}
#if defined(CONFIG_ARCH_SUN9IW1P1)
	__msdelay(100);
#endif

#ifdef CONFIG_ARCH_SUN7I
	check_super_standby_flag();
#endif

#if SYS_STORAGE_MEDIA_TYPE == SYS_STORAGE_MEDIA_NAND_FLASH
	status = load_Boot1_from_nand( );		// Load Boot1
#elif SYS_STORAGE_MEDIA_TYPE == SYS_STORAGE_MEDIA_SPI_NOR_FLASH
	status = load_boot1_from_spinor( );		// Load Boot1
#elif SYS_STORAGE_MEDIA_TYPE == SYS_STORAGE_MEDIA_SD_CARD
	//dram parameter copy
	memcpy((void *)DRAM_PARA_STORE_ADDR, (void *)BT0_head.prvt_head.dram_para, SUNXI_DRAM_PARA_MAX * 4);
	status = load_boot1_from_sdmmc( (char *)BT0_head.prvt_head.storage_data );	// Load Boot1
#else
	#error The storage media of Boot1 has not been defined.
#endif

	msg("Ready to disable icache.\n");

	mmu_disable( );		// disable instruction cache

	if( status == OK )
	{

		// Write all the dram parameters to boot1 before jumping
		set_dram_para((void *)&BT0_head.prvt_head.dram_para, dram_size);
		msg("Jump to second Boot.\n");

		jump_to( UBOOT_BASE );		// If Boot1 is successfully loaded, jump to Boot1 to execute.
	}
	else
	{
//		disable_watch_dog( );		// disable watch dog

		pll_reset();
		msg("Jump to Fel.\n");
		jump_to( FEL_BASE );		// If loading Boot1 fails, give control to FEL
	}
}

/************************************************************************************************************
*
*                                             function
*    name          :
*    parmeters     :
*    return        :
*    note          :
*
************************************************************************************************************/
static void print_version(void)
{
	msg("boot0 version : %s\n", BT0_head.boot_head.platform + 2);
	return;
}

/************************************************************************************************************
*
*                                             function
*    name          :
*    parmeters     :
*    return        :
*    note          :
*
************************************************************************************************************/
static void clear_ZI( void )
{
	__u32 *p32;
	__u32 size;

	extern unsigned char Image$$Boot0_RW_ZI$$ZI$$Base;
	extern unsigned char Image$$Boot0_RW_ZI$$ZI$$Length;

	size = (__u32)  &Image$$Boot0_RW_ZI$$ZI$$Length;
	p32  = (__u32 *)&Image$$Boot0_RW_ZI$$ZI$$Base;

	memset(p32, 0, size);
}
