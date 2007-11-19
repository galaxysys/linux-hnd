/*
 * include/asm-arm/arch-pxa/magician-gpio.h
 * History:
 *
 * 20070225 Philipp Zabel. Complete rewrite based on GAFR/GPDR data known from haret.
 */

#ifndef _MAGICIAN_H_
#define _MAGICIAN_H_

#include <asm/arch/irqs.h>
#include <asm/arch/pxa-regs.h>

/*
 * PXA GPIOs
 */

/* magician specific */

#define GPIO0_MAGICIAN_KEY_POWER		0
#define GPIO9_MAGICIAN_UNKNOWN			9	/* unknown input */
#define GPIO10_MAGICIAN_GSM_IRQ			10
#define GPIO11_MAGICIAN_GSM_OUT1		11	/* SILENCE? */
#define GPIO13_MAGICIAN_CPLD_IRQ		13
#define GPIO18_MAGICIAN_UNKNOWN			18	/* unknown output, high during sleep */
#define GPIO22_MAGICIAN_VIBRA_EN		22
#define GPIO26_MAGICIAN_GSM_POWER		26
#define GPIO27_MAGICIAN_USBC_PUEN		27
#define GPIO30_MAGICIAN_nCHARGE_EN		30
#define GPIO37_MAGICIAN_KEY_PHONE_HANGUP	37
#define GPIO38_MAGICIAN_KEY_CONTACTS		38
#define GPIO40_MAGICIAN_GSM_OUT2		40	/* 0 0 1 */
#define GPIO48_MAGICIAN_UNKNOWN			48	/* unknown output, high during sleep */
#define GPIO56_MAGICIAN_UNKNOWN			56	/* unknown output */
#define GPIO57_MAGICIAN_CAM_RESET		57
#define GPIO83_MAGICIAN_nIR_EN			83
#define GPIO86_MAGICIAN_GSM_RESET		86
#define GPIO87_MAGICIAN_GSM_SELECT		87	/* OUT3 */
#define GPIO90_MAGICIAN_KEY_CALENDAR		90
#define GPIO91_MAGICIAN_KEY_CAMERA		91
#define GPIO93_MAGICIAN_KEY_UP			93
#define GPIO94_MAGICIAN_KEY_DOWN		94
#define GPIO95_MAGICIAN_KEY_LEFT		95
#define GPIO96_MAGICIAN_KEY_RIGHT		96
#define GPIO97_MAGICIAN_KEY_ENTER		97
#define GPIO98_MAGICIAN_KEY_RECORD		98
#define GPIO99_MAGICIAN_HEADPHONE_IN		99
#define GPIO100_MAGICIAN_KEY_VOL_UP		100
#define GPIO101_MAGICIAN_KEY_VOL_DOWN 		101
#define GPIO102_MAGICIAN_KEY_PHONE_LIFT		102
#define GPIO103_MAGICIAN_LED_KP			103
#define GPIO104_MAGICIAN_LCD_POWER_1 		104
#define GPIO105_MAGICIAN_LCD_POWER_2		105
#define GPIO106_MAGICIAN_LCD_POWER_3		106
#define GPIO107_MAGICIAN_DS1WM_IRQ		107
#define GPIO108_MAGICIAN_GSM_READY		108
#define GPIO114_MAGICIAN_UNKNOWN		114	/* unknown output */
#define GPIO115_MAGICIAN_nPEN_IRQ		115
#define GPIO116_MAGICIAN_nCAM_EN		116
#define GPIO119_MAGICIAN_UNKNOWN		119	/* unknown output */
#define GPIO120_MAGICIAN_UNKNOWN		120	/* unknown output */

/* generic, but not defined in pxa-regs.h */

#define GPIO12_CIF_DD_7				12
#define GPIO14_SSPSFRM2				14
#define GPIO17_CIF_DD_6				17
#define GPIO50_CIF_DD_3				50
#define GPIO51_CIF_DD_2				51
#define GPIO52_CIF_DD_4				52
#define GPIO50_CIF_DD_3				50
#define GPIO51_CIF_DD_2				51
#define GPIO52_CIF_DD_4				52
#define GPIO53_CIF_MCLK				53
#define GPIO54_CIF_PCLK				54
#define GPIO55_CIF_DD_1				55
#define GPIO81_CIF_DD_0				81
#define GPIO82_CIF_DD_5				82
#define GPIO84_CIF_FV				84
#define GPIO85_CIF_LV				85
#define GPIO88_SSPRXD2				88
#define GPIO89_SSPTXD2				89
#define GPIO92_MMDAT_0				92

/*
 * GPIO alternate function mode & direction
 */

/* GAFR0_L 0xA2000004 */
/* GPDR0   0x____CC0C */

#define GPIO0_MAGICIAN_KEY_POWER_MD		(0 | GPIO_IN)
//#define GPIO1_RTS_MD				(1 | GPIO_ALT_FN_1_IN)	/* reset */
// 2 OUT (reserved)	SYS_EN
// 3 OUT		PWR_SCL
// 4 OUT		PWR_SDA
// 5 OUT (reserved)	PWR_CAP_0
// 6 IN  (reserved)	PWR_CAP_1
// 7 IN  (reserved)	PWR_CAP_2
// 8 IN  (reserved)	PWR_CAP_3
#define GPIO9_MAGICIAN_UNKNOWN_MD		(9 | GPIO_IN)
#define GPIO10_MAGICIAN_GSM_IRQ_MD		(10 | GPIO_IN)
#define GPIO11_MAGICIAN_GSM_OUT1_MD		(11 | GPIO_OUT)	/* SILENCE? */

#define GPIO12_CIF_DD_7_MD			(12 | GPIO_ALT_FN_2_IN)

#define GPIO13_MAGICIAN_CPLD_IRQ_MD		(13 | GPIO_IN)

#define GPIO14_SSPSFRM2_MD			(14 | GPIO_ALT_FN_2_OUT)
//#define GPIO15_nCS_1_MD			(15 | GPIO_ALT_FN_2_OUT)

/* GAFR0_U 0x491A854A */
/* GPDR0   0xDBFD____ */

//#define GPIO16_PWM0_MD			(16 | GPIO_ALT_FN_2_OUT)	/* backlight */
#define GPIO17_CIF_DD_6_MD			(17 | GPIO_ALT_FN_2_IN)

#define GPIO18_MAGICIAN_UNKNOWN_MD		(18 | GPIO_OUT)

#define GPIO19_SSPSCLK2_MD			(19 | GPIO_ALT_FN_1_OUT)
#define GPIO20_nSDCS_2_MD			(20 | GPIO_ALT_FN_1_OUT)
#define GPIO21_nSDCS_3_MD			(21 | GPIO_ALT_FN_1_OUT)

#define GPIO22_MAGICIAN_VIBRA_EN_MD		(22 | GPIO_OUT)

//#define GPIO23_SCLK_MD			(23 | GPIO_ALT_FN_2_OUT)
//#define GPIO24_SFRM_MD			(24 | GPIO_ALT_FN_2_OUT)
//#define GPIO25_STXD_MD			(25 | GPIO_ALT_FN_2_OUT)	/* sound out */

#define GPIO26_MAGICIAN_GSM_POWER_MD		(26 | GPIO_OUT)
#define GPIO27_MAGICIAN_USBC_PUEN_MD		(27 | GPIO_OUT)

//#define GPIO28_BITCLK_OUT_I2S_MD		(28 | GPIO_ALT_FN_1_OUT)
//#define GPIO29_SDATA_IN_I2S_MD		(29 | GPIO_ALT_FN_2_IN)

#define GPIO30_MAGICIAN_nCHARGE_EN_MD		(30 | GPIO_OUT)

//#define GPIO31_SYNC_I2S_MD			(31 | GPIO_ALT_FN_1_OUT)

/* GAFR1_L 0x6998815A */
/* GPDR1   0x____AB83 */

//#define GPIO32_MMCCLK_MD			(32 | GPIO_ALT_FN_2_OUT)
//#define GPIO33_nCS_5_MD			(33 | GPIO_ALT_FN_2_OUT)
//#define GPIO34_FFRXD_MD			(34 | GPIO_ALT_FN_1_IN)
//#define GPIO35_FFCTS_MD			(35 | GPIO_ALT_FN_1_IN)
//#define GPIO36_FFDCD_MD			(36 | GPIO_ALT_FN_1_IN)

#define GPIO37_MAGICIAN_KEY_PHONE_HANGUP_MD	(37 | GPIO_OUT)
#define GPIO38_MAGICIAN_KEY_CONTACTS_MD		(38 | GPIO_OUT)

//#define GPIO39_FFTXD_MD			(39 | GPIO_ALT_FN_2_OUT)

#define GPIO40_MAGICIAN_GSM_OUT2_MD		(40 | GPIO_OUT)			/* FFDTR? */

//#define GPIO41_FFRTS_MD			(41 | GPIO_ALT_FN_2_OUT)

//#define GPIO42_BTRXD_MD			(42 | GPIO_ALT_FN_1_IN)		/* BTUART: GSM AT */
//#define GPIO43_BTTXD_MD			(43 | GPIO_ALT_FN_2_OUT)
//#define GPIO44_BTCTS_MD			(44 | GPIO_ALT_FN_1_IN)
//#define GPIO45_BTRTS_MD			(45 | GPIO_ALT_FN_2_OUT)
//#define GPIO46_STRXD_MD			(46 | GPIO_ALT_FN_2_IN)		/* STUART: GSM data */
//#define GPIO47_STTXD_MD			(47 | GPIO_ALT_FN_1_OUT)

/* GAFR1_U 0xAAA07958 */
/* GPDR1   0xFF23AB83 */

#define GPIO48_MAGICIAN_UNKNOWN_MD		(48 | GPIO_OUT)

//#define GPIO49_nPWE_MD			(49 | GPIO_ALT_FN_2_OUT)
#define GPIO50_CIF_DD_3_MD			(50 | GPIO_ALT_FN_1_IN)
#define GPIO51_CIF_DD_2_MD			(51 | GPIO_ALT_FN_1_IN)
#define GPIO52_CIF_DD_4_MD			(52 | GPIO_ALT_FN_1_IN)
#define GPIO53_CIF_MCLK_MD			(53 | GPIO_ALT_FN_2_OUT)
#define GPIO54_CIF_PCLK_MD			(54 | GPIO_ALT_FN_3_IN)
#define GPIO55_CIF_DD_1_MD			(55 | GPIO_ALT_FN_1_IN)

#define GPIO56_MAGICIAN_UNKNOWN_MD		(56 | GPIO_OUT)
#define GPIO57_MAGICIAN_CAM_RESET_MD		(57 | GPIO_OUT)

//#define GPIO58_LDD_0_MD			(58 | GPIO_ALT_FN_2_OUT)
//#define GPIO59_LDD_1_MD			(59 | GPIO_ALT_FN_2_OUT)
//#define GPIO60_LDD_2_MD			(60 | GPIO_ALT_FN_2_OUT)
//#define GPIO61_LDD_3_MD			(61 | GPIO_ALT_FN_2_OUT)
//#define GPIO62_LDD_4_MD			(62 | GPIO_ALT_FN_2_OUT)
//#define GPIO63_LDD_5_MD			(63 | GPIO_ALT_FN_2_OUT)

/* GAFR2_L 0xAAAAAAAA */
/* GPDR2   0x____FFFF */

//#define GPIO64_LDD_6_MD			(64 | GPIO_ALT_FN_2_OUT)
//#define GPIO65_LDD_7_MD			(65 | GPIO_ALT_FN_2_OUT)
//#define GPIO66_LDD_8_MD			(66 | GPIO_ALT_FN_2_OUT)
//#define GPIO67_LDD_9_MD			(67 | GPIO_ALT_FN_2_OUT)
//#define GPIO68_LDD_10_MD			(68 | GPIO_ALT_FN_2_OUT)
//#define GPIO69_LDD_11_MD			(69 | GPIO_ALT_FN_2_OUT)
//#define GPIO70_LDD_12_MD			(70 | GPIO_ALT_FN_2_OUT)
//#define GPIO71_LDD_13_MD			(71 | GPIO_ALT_FN_2_OUT)
//#define GPIO72_LDD_14_MD			(72 | GPIO_ALT_FN_2_OUT)
//#define GPIO73_LDD_15_MD			(73 | GPIO_ALT_FN_2_OUT)
//#define GPIO74_LCD_FCLK_MD			(74 | GPIO_ALT_FN_2_OUT)
//#define GPIO75_LCD_LCLK_MD			(75 | GPIO_ALT_FN_2_OUT)
//#define GPIO76_LCD_PCLK_MD			(76 | GPIO_ALT_FN_2_OUT)
//#define GPIO77_LCD_ACBIAS_MD			(77 | GPIO_ALT_FN_2_OUT)
//#define GPIO78_nCS_2_MD			(78 | GPIO_ALT_FN_2_OUT)
//#define GPIO79_nCS_3_MD			(79 | GPIO_ALT_FN_2_OUT)

/* GAFR2_U 0x010E0F3A */
/* GPDR2   0x02C9____ */

//#define GPIO80_nCS_4_MD			(80 | GPIO_ALT_FN_2_OUT)
#define GPIO81_CIF_DD_0_MD			(81 | GPIO_ALT_FN_2_IN)
#define GPIO82_CIF_DD_5_MD			(82 | GPIO_ALT_FN_3_IN

#define GPIO83_MAGICIAN_nIR_EN_MD		(83 | GPIO_OUT)

#define GPIO84_CIF_FV_MD			(84 | GPIO_ALT_FN_3_IN)
#define GPIO85_CIF_LV_MD			(85 | GPIO_ALT_FN_3_IN)

#define GPIO86_MAGICIAN_GSM_RESET_MD		(86 | GPIO_OUT)
#define GPIO87_MAGICIAN_GSM_SELECT_MD		(87 | GPIO_OUT)

#define GPIO88_SSPRXD2_MD			(88 | GPIO_ALT_FN_2_IN)
#define GPIO89_SSPTXD2_MD			(89 | GPIO_ALT_FN_3_OUT)

#define GPIO90_MAGICIAN_KEY_CALENDAR_MD		(90 | GPIO_OUT)
#define GPIO91_MAGICIAN_KEY_CAMERA_MD		(91 | GPIO_OUT)

//#define GPIO92_MMCDAT0_MD			(92 | GPIO_ALT_FN_1_IN)		/* FIXME? different from pxa-regs.h */

#define GPIO93_MAGICIAN_KEY_UP_MD		(93 | GPIO_IN)
#define GPIO94_MAGICIAN_KEY_DOWN_MD		(94 | GPIO_IN)
#define GPIO95_MAGICIAN_KEY_LEFT_MD		(95 | GPIO_IN)

/* GAFR3_L 0x54000000 */
/* GPDR3   0x____1780 */

#define GPIO96_MAGICIAN_KEY_RIGHT_MD		(96 | GPIO_IN)
#define GPIO97_MAGICIAN_KEY_ENTER_MD		(97 | GPIO_IN)
#define GPIO98_MAGICIAN_KEY_RECORD_MD		(98 | GPIO_IN)
#define GPIO99_MAGICIAN_HEADPHONE_IN_MD		(99 | GPIO_IN)
#define GPIO100_MAGICIAN_KEY_VOL_UP_MD		(100 | GPIO_IN)
#define GPIO101_MAGICIAN_KEY_VOL_DOWN_MD 	(101 | GPIO_IN)
#define GPIO102_MAGICIAN_KEY_PHONE_LIFT_MD	(102 | GPIO_IN)
#define GPIO103_MAGICIAN_LED_KP_MD		(103 | GPIO_OUT)
#define GPIO104_MAGICIAN_LCD_POWER_1_MD 	(104 | GPIO_OUT)
#define GPIO105_MAGICIAN_LCD_POWER_2_MD		(105 | GPIO_OUT)
#define GPIO106_MAGICIAN_LCD_POWER_3_MD		(106 | GPIO_OUT)
#define GPIO107_MAGICIAN_DS1WM_IRQ_MD		(107 | GPIO_IN)
#define GPIO108_MAGICIAN_GSM_READY_MD		(108 | GPIO_IN) 		/* out when phone off */

//#define GPIO109_MMCDAT1_MD			(109 | GPIO_ALT_FN_1_IN)	/* FIXME? */
//#define GPIO110_MMCDAT2_MD			(110 | GPIO_ALT_FN_1_IN)	/* those are different */
//#define GPIO110_MMCDAT3_MD			(111 | GPIO_ALT_FN_1_IN)	/* from pxa-regs.h */

/* GAFR3_U 0x00001405 */
/* GPDR3   0x01B6____ */

//#define GPIO112_MMCCMD_MD			(112 | GPIO_ALT_FN_1_IN)	/* FIXME? different from pxa-regs.h */
//#define GPIO113_I2S_SYSCLK_MD			(113 | GPIO_ALT_FN_1_OUT)

#define GPIO114_MAGICIAN_UNKNOWN_MD		(114 | GPIO_OUT)
#define GPIO115_MAGICIAN_nPEN_IRQ_MD		(115 | GPIO_IN)
#define GPIO116_MAGICIAN_nCAM_EN_MD		(116 | GPIO_OUT)

//#define GPIO117_I2CSCL_MD			(117 | GPIO_ALT_FN_1_OUT)
//#define GPIO118_I2CSDA_MD			(118 | GPIO_ALT_FN_1_IN)

#define GPIO119_MAGICIAN_UNKNOWN_MD		(119 | GPIO_OUT) /* 0 */
#define GPIO120_MAGICIAN_UNKNOWN_MD		(120 | GPIO_OUT)

/*
 * CPLD IRQs
 */

#define IRQ_MAGICIAN_SD		(IRQ_BOARD_START + 0)
#define IRQ_MAGICIAN_EP		(IRQ_BOARD_START + 1)
#define IRQ_MAGICIAN_BT		(IRQ_BOARD_START + 2)
#define IRQ_MAGICIAN_AC		(IRQ_BOARD_START + 3)

/*
 * CPLD EGPIOs
 */

#define MAGICIAN_EGPIO_BASE			0x100 /* GPIO_BASE_INCREMENT */
#define MAGICIAN_EGPIO(reg,bit) \
	(MAGICIAN_EGPIO_BASE + 16*reg + bit)

/* output */

#define EGPIO_MAGICIAN_TOPPOLY_POWER		MAGICIAN_EGPIO(0, 2)
#define EGPIO_MAGICIAN_LED_POWER		MAGICIAN_EGPIO(0, 5)
#define EGPIO_MAGICIAN_GSM_RESET		MAGICIAN_EGPIO(0, 6)
#define EGPIO_MAGICIAN_LCD_POWER		MAGICIAN_EGPIO(0, 7)
#define EGPIO_MAGICIAN_SPK_POWER		MAGICIAN_EGPIO(1, 0)
#define EGPIO_MAGICIAN_EP_POWER			MAGICIAN_EGPIO(1, 1)
#define EGPIO_MAGICIAN_IN_SEL0			MAGICIAN_EGPIO(1, 2)
#define EGPIO_MAGICIAN_IN_SEL1			MAGICIAN_EGPIO(1, 3)
#define EGPIO_MAGICIAN_MIC_POWER		MAGICIAN_EGPIO(1, 4)
#define EGPIO_MAGICIAN_CODEC_RESET		MAGICIAN_EGPIO(1, 5)
#define EGPIO_MAGICIAN_CODEC_POWER		MAGICIAN_EGPIO(1, 6)
#define EGPIO_MAGICIAN_BL_POWER			MAGICIAN_EGPIO(1, 7)
#define EGPIO_MAGICIAN_SD_POWER			MAGICIAN_EGPIO(2, 0)
#define EGPIO_MAGICIAN_CARKIT_MIC		MAGICIAN_EGPIO(2, 1)
#define EGPIO_MAGICIAN_UNKNOWN_WAVEDEV_DLL	MAGICIAN_EGPIO(2, 2)
#define EGPIO_MAGICIAN_FLASH_VPP		MAGICIAN_EGPIO(2, 3)
#define EGPIO_MAGICIAN_BL_POWER2		MAGICIAN_EGPIO(2, 4)
#define EGPIO_MAGICIAN_CHARGE_EN		MAGICIAN_EGPIO(2, 5) /* set by wince SleepSetting if cable_state == 5 */
#define EGPIO_MAGICIAN_GSM_POWER		MAGICIAN_EGPIO(2, 7)

/* input */

#define EGPIO_MAGICIAN_CABLE_STATE_AC		MAGICIAN_EGPIO(4, 0)
#define EGPIO_MAGICIAN_CABLE_STATE_USB		MAGICIAN_EGPIO(4, 1)

#define EGPIO_MAGICIAN_BOARD_ID0		MAGICIAN_EGPIO(5, 0)
#define EGPIO_MAGICIAN_BOARD_ID1		MAGICIAN_EGPIO(5, 1)
#define EGPIO_MAGICIAN_BOARD_ID2		MAGICIAN_EGPIO(5, 2)
#define EGPIO_MAGICIAN_LCD_SELECT		MAGICIAN_EGPIO(5, 3)
#define EGPIO_MAGICIAN_nSD_READONLY		MAGICIAN_EGPIO(5, 4)

#define EGPIO_MAGICIAN_EP_INSERT		MAGICIAN_EGPIO(6, 1)

#endif /* _MAGICIAN_H_ */
