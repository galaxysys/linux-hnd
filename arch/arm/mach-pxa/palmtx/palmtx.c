/*
 * $Id$
 * 
 * Hardware definitions for Palm TX
 *
 * Based on: palmld.c from Alex Osborne
 *
 * Authors: Alex Osborne <bobofdoom@gmail.com>
 *          Cristiano P. <cristianop AT users DOT sourceforge DOT net>  (adapted for Palm TX)
 * 	    Jan Herman <2hp@seznam.cz>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * (find more info at www.hackndev.com) 
 * 
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/corgi_bl.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <asm/arch/audio.h>
#include <asm/arch/hardware.h>
#include <asm/arch/mmc.h>
#include <asm/arch/pxafb.h>
#include <asm/arch/pxa-dmabounce.h>
#include <asm/arch/pxa-regs.h>
#include <asm/arch/udc.h>
#include <asm/arch/irda.h>
#include <asm/arch/serial.h>
#include <asm/arch/bitfield.h>
#include <asm/arch/pxa27x_keyboard.h>
#include <asm/arch/palmtx-init.h>
#include <asm/arch/palmtx-gpio.h>

#include "../generic.h"

#define DEBUG

/**************************
 * SD/MMC card controller *
 **************************/

static int palmtx_mci_init(struct device *dev, irqreturn_t (*palmtx_detect_int)(int, void *), void *data)
{
	int err;
	
	/**
	 * Setup GPIOs for MMC/SD card controller.
	 */
	pxa_gpio_mode(GPIO32_MMCCLK_MD);
	pxa_gpio_mode(GPIO92_MMCDAT0_MD);
	pxa_gpio_mode(GPIO109_MMCDAT1_MD);
	pxa_gpio_mode(GPIO110_MMCDAT2_MD);
	pxa_gpio_mode(GPIO111_MMCDAT3_MD);
	pxa_gpio_mode(GPIO112_MMCCMD_MD);
		
	/**
	 * Setup an interrupt for detecting card insert/remove events
	 */
	set_irq_type(IRQ_GPIO_PALMTX_SD_DETECT_N, IRQT_BOTHEDGE);
	err = request_irq(IRQ_GPIO_PALMTX_SD_DETECT_N, palmtx_detect_int,
			SA_INTERRUPT, "SD/MMC card detect", data);
			
	if(err) {
		printk(KERN_ERR "palmtx_mci_init: cannot request SD/MMC card detect IRQ\n");
		return -1;
	}
	
	printk("palmtx_mci_init: irq registered\n");
	
	return 0;
}


static void palmtx_mci_exit(struct device *dev, void *data)
{
	free_irq(IRQ_GPIO_PALMTX_SD_DETECT_N, data);
}


static struct pxamci_platform_data palmtx_mci_platform_data = {
	.ocr_mask	= MMC_VDD_32_33|MMC_VDD_33_34,
	.init 		= palmtx_mci_init,
	/* .setpower 	= palmtx_mci_setpower,	*/
	.exit		= palmtx_mci_exit,

};


/**********
 * Keypad *
 **********/
 
static struct pxa27x_keyboard_platform_data palmtx_kbd_data = {
	.nr_rows = 4,
	.nr_cols = 3,
	.keycodes = {
		{	/* row 0 */
			KEY_F8,
			KEY_F9,
			KEY_ENTER,
		},
		{	/* row 1 */
			KEY_F10,
			KEY_F11,
			KEY_F12,
		},
		{	/* row 2 */
			KEY_UP,
			-1,
			KEY_DOWN,
		},
		{
			/* row 3 */
			KEY_RIGHT,
			-1,
			KEY_LEFT,
		},

	},
	.gpio_modes = {
		 GPIO_NR_PALMTX_KP_MKIN0_MD,
		 GPIO_NR_PALMTX_KP_MKIN1_MD,
		 GPIO_NR_PALMTX_KP_MKIN2_MD,
		 GPIO_NR_PALMTX_KP_MKIN3_MD,
		 GPIO_NR_PALMTX_KP_MKOUT0_MD,
		 GPIO_NR_PALMTX_KP_MKOUT1_MD,
		 GPIO_NR_PALMTX_KP_MKOUT2_MD,
	 },
};

static struct platform_device palmtx_keypad = {
        .name   = "pxa27x-keyboard",
        .id     = -1,
	.dev	=  {
		.platform_data	= &palmtx_kbd_data,
	},
};

/**************
 * LCD Border *
 **************/

struct platform_device palmtx_border = {
        .name = "palmtx-border",
        .id = -1,
        .dev = {
            .platform_data = NULL,
        },
};

/*************
 * Backlight *
 *************/

static void palmtx_bl_power(int on)
{
    if(GET_PALMTX_GPIO(BL_POWER)!=on) {
    	SET_PALMTX_GPIO(LCD_POWER, on);
    	SET_PALMTX_GPIO(BL_POWER, on);
    	pxa_set_cken(CKEN0_PWM0, on);
    	pxa_set_cken(CKEN1_PWM1, on);
    	mdelay(50);
    }
}

static void palmtx_set_bl_intensity(int intensity)
{
    palmtx_bl_power(intensity ? 1 : 0);
    if(intensity) {
        PWM_CTRL0   = 0x7;
        PWM_PERVAL0 = PALMTX_PERIOD;
        PWM_PWDUTY0 = intensity;
    }
}

static struct corgibl_machinfo palmtx_bl_machinfo = {
    .max_intensity      = PALMTX_MAX_INTENSITY,
    .default_intensity  = PALMTX_MAX_INTENSITY,
    .set_bl_intensity   = palmtx_set_bl_intensity,
    .limit_mask         = PALMTX_LIMIT_MASK,
};

static struct platform_device palmtx_backlight = {
    .name       = "corgi-bl",
    .id         = 0,
    .dev        = {
            .platform_data = &palmtx_bl_machinfo,
    },
};


/********
 * IRDA *
 ********/

static void palmtx_irda_transceiver_mode(struct device *dev, int mode)
{
        unsigned long flags;

        local_irq_save(flags);

        if (mode & IR_SIRMODE){
		printk (KERN_INFO "IrDA: setting mode to SIR\n");
	} 
	else if (mode & IR_FIRMODE){
		printk (KERN_INFO "IrDA: setting mode to FIR\n");
	}
	if (mode & IR_OFF){
		printk (KERN_INFO "IrDA: turning OFF\n");
		SET_GPIO(GPIO_NR_PALMTX_IR_DISABLE, 1);
	}
	else {
		printk (KERN_INFO "IrDA: turning ON\n");
		SET_GPIO(GPIO_NR_PALMTX_IR_DISABLE, 0);
		SET_GPIO(GPIO_NR_PALMTX_ICP_TXD_MD, 1);
		mdelay(30);
		SET_GPIO(GPIO_NR_PALMTX_ICP_TXD_MD, 0);	
	}

        local_irq_restore(flags);
}


static struct pxaficp_platform_data palmtx_ficp_platform_data = {
        .transceiver_cap  = IR_SIRMODE | IR_FIRMODE | IR_OFF,
        .transceiver_mode = palmtx_irda_transceiver_mode,
};

/**************
 *  Bluetooth *
 **************/

void bcm2035_bt_reset(int on)
{
	printk(KERN_NOTICE "Switch BT reset %d\n", on);
	SET_PALMTX_GPIO( BT_RESET, on ? 1 : 0 );
}
EXPORT_SYMBOL(bcm2035_bt_reset);

void bcm2035_bt_power(int on)
{
	printk(KERN_NOTICE "Switch BT power %d\n", on ? 1 : 0);
	SET_PALMTX_GPIO( BT_POWER, on ? 1 : 0 );
}
EXPORT_SYMBOL(bcm2035_bt_power);


struct bcm2035_bt_funcs {
	void (*configure) ( int state );
};

static struct bcm2035_bt_funcs bt_funcs;

static void
bcm2035_bt_configure( int state )
{
	if (bt_funcs.configure != NULL)
		bt_funcs.configure( state );
}

static struct platform_pxa_serial_funcs bcm2035_pxa_bt_funcs = {
	.configure = bcm2035_bt_configure,
};

static struct platform_device bcm2035_bt = {
	.name = "bcm2035-bt",
	.id = -1,
	.dev = {
		.platform_data = &bt_funcs,
	},
};

/*******
 * USB *
 *******/
 
static int palmtx_udc_is_connected (void){
	int ret = !(GET_GPIO(GPIO_NR_PALMTX_USB_DETECT));
	if (ret)
		printk (KERN_INFO "palmtx_udc: device detected [USB_DETECT: %d]\n",ret);
	else
		printk (KERN_INFO "palmtx_udc: no device detected [USB_DETECT: %d]\n",ret);
		
	return ret;
}


static void palmtx_udc_command (int cmd){
	
	switch (cmd) {
		case PXA2XX_UDC_CMD_DISCONNECT:
			SET_GPIO(GPIO_NR_PALMTX_USB_PULLUP, 0);
			SET_GPIO(GPIO_NR_PALMTX_USB_POWER, 0);
			printk(KERN_INFO "palmtx_udc: got command PXA2XX_UDC_CMD_DISCONNECT\n");
			break;
		case PXA2XX_UDC_CMD_CONNECT:
			SET_GPIO(GPIO_NR_PALMTX_USB_POWER, 1);
			SET_GPIO(GPIO_NR_PALMTX_USB_PULLUP, 1);
			printk(KERN_INFO "palmtx_udc: got command PXA2XX_UDC_CMD_CONNECT\n");
			break;
	default:
			printk("palmtx_udc: unknown command '%d'\n", cmd);
	}
}


static struct pxa2xx_udc_mach_info palmtx_udc_mach_info __initdata = {
	.udc_is_connected	= palmtx_udc_is_connected,
	.udc_command		= palmtx_udc_command,
};



/*************************
 * AC97 audio controller *
 *************************/

static pxa2xx_audio_ops_t palmtx_audio_ops = {
	/*
	.startup  = palmtx_audio_startup,
	.shutdown = mst_audio_shutdown,
	.suspend  = mst_audio_suspend,
	.resume   = mst_audio_resume,
	*/
};

static struct platform_device palmtx_ac97 = {
	.name	= "pxa2xx-ac97",
	.id	= -1,
	.dev	= {
	    .platform_data = &palmtx_audio_ops
	},
};

/********************
 * Power Management *
 ********************/

struct platform_device palmtx_pm = {
	.name = "palmtx-pm",
	.id = -1,
	.dev = {
		.platform_data = NULL,
	},
};

static struct platform_device *devices[] __initdata = {
         &palmtx_keypad, 
	 &palmtx_ac97,
	 &palmtx_pm,
	 &palmtx_backlight,
	 &bcm2035_bt,
	 &palmtx_border,
};

/***************
 * framebuffer *
 ***************/
 
/* 
 * framebuffer settings as from palmos: 
 *
 * LCCR1: 0x1f030d3f	[ BLW=0x1f, ELW=0x3, HSW=0x3, PPL=0x13f ]
 * LCCR2: 0x070801df	[ BFW=0x7,  EFW=0x8, VSW=0x0, LPP=0x1df ]
 *
 * the fields marked with * are custom values that seem to fix a
 * persistent buzz of the TS present with palmos too
 *
 */

static struct pxafb_mode_info palmtx_lcd_modes[] = {
    {
	.pixclock		= 0,	
	.xres			= 320,	// PPL + 1
	.yres			= 480,	// LPP + 1
	.bpp			= 16,	// BPP

					// linux    palmos
	.left_margin		= 32,	// BLW + 1  (BLW=31) 
	.right_margin		= 1,	// ELW + 1  (ELW=3)  *
	.upper_margin		= 7,	// BFW + 1  (BFW=7)  *
	.lower_margin		= 1,	// EFW + 1  (EFW=8)  *

	.hsync_len              = 4,	// HSW + 1  (HSW=3)
	.vsync_len              = 1,	// VSW + 1  (VSW=0)
    },
};

static struct pxafb_mach_info palmtx_lcd_screen = {
	.modes                  = palmtx_lcd_modes,
	.num_modes              = ARRAY_SIZE(palmtx_lcd_modes),
	.lccr0			= PALMTX_INIT_LCD_LLC0,
	.lccr3			= PALMTX_INIT_LCD_LLC3,
	.pxafb_backlight_power	= NULL,
};

static struct map_desc palmtx_io_desc[] __initdata = {
	{
		.virtual	= 0xf0000000,
		.pfn		= __phys_to_pfn(0x28000000),
		.length		= 0x00100000,
		.type		= MT_DEVICE
	},
};


static void __init palmtx_map_io(void)
{
	pxa_map_io();
	iotable_init(palmtx_io_desc, ARRAY_SIZE(palmtx_io_desc));
}

/****************
 * Init Machine *
 ****************/

static void __init palmtx_init(void)
{
	// disable primary codec interrupt to prevent WM9712 constantly interrupting the CPU
	// and preventing the boot process to complete (Thanx Alex & Shadowmite!)
	GCR &= ~GCR_PRIRDY_IEN;

	// configure AC97's GPIOs
	pxa_gpio_mode(GPIO28_BITCLK_AC97_MD);
	pxa_gpio_mode(GPIO29_SDATA_IN_AC97_MD);
	pxa_gpio_mode(GPIO30_SDATA_OUT_AC97_MD);
	pxa_gpio_mode(GPIO31_SYNC_AC97_MD);                                 
	
	set_pxa_fb_info   ( &palmtx_lcd_screen );
	pxa_set_btuart_info(&bcm2035_pxa_bt_funcs);
	pxa_set_mci_info  ( &palmtx_mci_platform_data );
	pxa_set_udc_info  ( &palmtx_udc_mach_info );
	pxa_set_ficp_info ( &palmtx_ficp_platform_data );	

	platform_add_devices( devices, ARRAY_SIZE(devices) );
}



MACHINE_START(XSCALE_PALMTX, "Palm TX")
	.phys_io	= PALMTX_PHYS_IO_START,
	.io_pg_offst	= io_p2v(0x40000000),
	.boot_params	= 0xa0000100,
	.map_io         = palmtx_map_io,
	.init_irq	= pxa_init_irq,
	.timer		= &pxa_timer,
	.init_machine	= palmtx_init
MACHINE_END
