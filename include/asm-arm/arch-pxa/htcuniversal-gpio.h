/* 
 * include/asm-arm/arch-pxa/htcuniversal-gpio.h
 * History:
 *
 * 2004-12-10 Michael Opdenacker. Wrote down GPIO settings as identified by Jamey Hicks.
 *            Reused the h2200-gpio.h file as a template.
 */

#ifndef _HTCUNIVERSAL_GPIO_H_
#define _HTCUNIVERSAL_GPIO_H_

#include <asm/arch/pxa-regs.h>

#define GET_HTCUNIVERSAL_GPIO(gpio) \
	(GPLR(GPIO_NR_HTCUNIVERSAL_ ## gpio) & GPIO_bit(GPIO_NR_HTCUNIVERSAL_ ## gpio))

#define SET_HTCUNIVERSAL_GPIO(gpio, setp) \
do { \
if (setp) \
	GPSR(GPIO_NR_HTCUNIVERSAL_ ## gpio) = GPIO_bit(GPIO_NR_HTCUNIVERSAL_ ## gpio); \
else \
	GPCR(GPIO_NR_HTCUNIVERSAL_ ## gpio) = GPIO_bit(GPIO_NR_HTCUNIVERSAL_ ## gpio); \
} while (0)

#define SET_HTCUNIVERSAL_GPIO_N(gpio, setp) \
do { \
if (setp) \
	GPCR(GPIO_NR_HTCUNIVERSAL_ ## gpio ## _N) = GPIO_bit(GPIO_NR_HTCUNIVERSAL_ ## gpio ## _N); \
else \
	GPSR(GPIO_NR_HTCUNIVERSAL_ ## gpio ## _N) = GPIO_bit(GPIO_NR_HTCUNIVERSAL_ ## gpio ## _N); \
} while (0)

#define HTCUNIVERSAL_IRQ(gpio) \
	IRQ_GPIO(GPIO_NR_HTCUNIVERSAL_ ## gpio)
	
#define GPIO_NR_HTCUNIVERSAL_KEY_ON_N			0
#define GPIO_NR_HTCUNIVERSAL_GP_RST_N			1

#define GPIO_NR_HTCUNIVERSAL_USB_DET			9
#define GPIO_NR_HTCUNIVERSAL_POWER_DET			10

#define GPIO_NR_HTCUNIVERSAL_CIF_DD7			12
#define GPIO_NR_HTCUNIVERSAL_ASIC3_SDIO_INT_N		13
#define GPIO_NR_HTCUNIVERSAL_ASIC3_EXT_INT		14
#define GPIO_NR_HTCUNIVERSAL_CS1_N			15

#define GPIO_NR_HTCUNIVERSAL_CIF_DD6			17
#define GPIO_NR_HTCUNIVERSAL_RDY			18

#define GPIO_NR_HTCUNIVERSAL_PHONE_START		19

#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT7			22
#define GPIO_NR_HTCUNIVERSAL_SPI_CLK			23
#define GPIO_NR_HTCUNIVERSAL_SPI_FRM			24
#define GPIO_NR_HTCUNIVERSAL_SPI_DO			25
#define GPIO_NR_HTCUNIVERSAL_SPI_DI			26

#define GPIO_NR_HTCUNIVERSAL_CODEC_ON			27
#define GPIO_NR_HTCUNIVERSAL_I2S_BCK			28
#define GPIO_NR_HTCUNIVERSAL_I2S_DIN			29
#define GPIO_NR_HTCUNIVERSAL_I2S_DOUT			30
#define GPIO_NR_HTCUNIVERSAL_I2S_SYNC			31

#define GPIO_NR_HTCUNIVERSAL_RS232_ON			32
#define GPIO_NR_HTCUNIVERSAL_CS5_N			33

#define GPIO_NR_HTCUNIVERSAL_PHONE_RXD			34
#define GPIO_NR_HTCUNIVERSAL_PHONE_UART_CTS		35
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN7			36
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN3			37
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN4			38
#define GPIO_NR_HTCUNIVERSAL_PHONE_TXD			39
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT6			40
#define GPIO_NR_HTCUNIVERSAL_PHONE_UART_RTS		41
#define GPIO_NR_HTCUNIVERSAL_BT_RXD			42
#define GPIO_NR_HTCUNIVERSAL_BT_TXD			43
#define GPIO_NR_HTCUNIVERSAL_BT_UART_CTS		44
#define GPIO_NR_HTCUNIVERSAL_BT_UART_RTS		45

#define GPIO_NR_HTCUNIVERSAL_SIR_RXD			42
#define GPIO_NR_HTCUNIVERSAL_SIR_TXD			43

#define GPIO_NR_HTCUNIVERSAL_POE_N			48
#define GPIO_NR_HTCUNIVERSAL_PWE_N			49
#define GPIO_NR_HTCUNIVERSAL_CIF_DD3			50
#define GPIO_NR_HTCUNIVERSAL_CIF_DD2			51
#define GPIO_NR_HTCUNIVERSAL_CIF_DD4			52

#define GPIO_NR_HTCUNIVERSAL_CIF_MCLK			53
#define GPIO_NR_HTCUNIVERSAL_CIF_PCLK			54
#define GPIO_NR_HTCUNIVERSAL_CIF_DD1			55

#define GPIO_NR_HTCUNIVERSAL_LDD0			58
#define GPIO_NR_HTCUNIVERSAL_LDD1			59
#define GPIO_NR_HTCUNIVERSAL_LDD2			60
#define GPIO_NR_HTCUNIVERSAL_LDD3			61
#define GPIO_NR_HTCUNIVERSAL_LDD4			62
#define GPIO_NR_HTCUNIVERSAL_LDD5			63
#define GPIO_NR_HTCUNIVERSAL_LDD6			64
#define GPIO_NR_HTCUNIVERSAL_LDD7			65
#define GPIO_NR_HTCUNIVERSAL_LDD8			66
#define GPIO_NR_HTCUNIVERSAL_LDD9			67
#define GPIO_NR_HTCUNIVERSAL_LDD10			68
#define GPIO_NR_HTCUNIVERSAL_LDD11			69
#define GPIO_NR_HTCUNIVERSAL_LDD12			70
#define GPIO_NR_HTCUNIVERSAL_LDD13			71
#define GPIO_NR_HTCUNIVERSAL_LDD14			72
#define GPIO_NR_HTCUNIVERSAL_LDD15			73

#define GPIO_NR_HTCUNIVERSAL_LFCLK_RD			74
#define GPIO_NR_HTCUNIVERSAL_LFCLK_A0			75
#define GPIO_NR_HTCUNIVERSAL_LFCLK_WR			76
#define GPIO_NR_HTCUNIVERSAL_LBIAS			77

#define GPIO_NR_HTCUNIVERSAL_CS2_N			78
#define GPIO_NR_HTCUNIVERSAL_CS3_N			79
#define GPIO_NR_HTCUNIVERSAL_CS4_N			80
#define GPIO_NR_HTCUNIVERSAL_CIF_DD0			81
#define GPIO_NR_HTCUNIVERSAL_CIF_DD5			82

#define GPIO_NR_HTCUNIVERSAL_CIF_LV			84
#define GPIO_NR_HTCUNIVERSAL_CIF_FV			85

#define GPIO_NR_HTCUNIVERSAL_LCD1			86
#define GPIO_NR_HTCUNIVERSAL_LCD2			87

#define GPIO_NR_HTCUNIVERSAL_KP_MKIN5			90
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN6			91

#define GPIO_NR_HTCUNIVERSAL_DREQ1			97

#define GPIO_NR_HTCUNIVERSAL_PHONE_RESET		98

#define GPIO_NR_HTCUNIVERSAL_KP_MKIN0			100
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN1			101
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN2			102
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT0			103
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT1			104
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT2			105
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT3			106
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT4			107
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT5			108

#define GPIO_NR_HTCUNIVERSAL_PHONE_UNKNOWN 		109
#define GPIO_NR_HTCUNIVERSAL_PHONE_OFF			110

#define GPIO_NR_HTCUNIVERSAL_USB_PUEN			112
#define GPIO_NR_HTCUNIVERSAL_I2S_SYSCLK			113

#define GPIO_NR_HTCUNIVERSAL_PWM_OUT1			115

#define GPIO_NR_HTCUNIVERSAL_I2C_SCL			117
#define GPIO_NR_HTCUNIVERSAL_I2C_SDA			118

#if 0
#define GPIO_NR_HTCUNIVERSAL_CPU_BATT_FAULT_N
#define GPIO_NR_HTCUNIVERSAL_ASIC3_RESET_N
#define GPIO_NR_HTCUNIVERSAL_CHARGE_EN_N
#define GPIO_NR_HTCUNIVERSAL_FLASH_VPEN
#define GPIO_NR_HTCUNIVERSAL_BATT_OFF
#define GPIO_NR_HTCUNIVERSAL_USB_CHARGE_RATE
#define GPIO_NR_HTCUNIVERSAL_BL_DETECT_N
#define GPIO_NR_HTCUNIVERSAL_CPU_HW_RESET_N
#endif


#define GPIO_NR_HTCUNIVERSAL_TOUCHSCREEN_SPI_CLK_MD	(23 | GPIO_ALT_FN_2_OUT)
#define GPIO_NR_HTCUNIVERSAL_TOUCHSCREEN_SPI_FRM_MD	(24 | GPIO_ALT_FN_2_OUT)
#define GPIO_NR_HTCUNIVERSAL_TOUCHSCREEN_SPI_DO_MD	(25 | GPIO_ALT_FN_2_OUT)
#define GPIO_NR_HTCUNIVERSAL_TOUCHSCREEN_SPI_DI_MD	(26 | GPIO_ALT_FN_1_IN)

#define GPIO_NR_HTCUNIVERSAL_I2S_BCK_MD			(28 | GPIO_ALT_FN_1_OUT)
#define GPIO_NR_HTCUNIVERSAL_I2S_DIN_MD			(29 | GPIO_ALT_FN_2_IN)
#define GPIO_NR_HTCUNIVERSAL_I2S_DOUT_MD		(30 | GPIO_ALT_FN_1_OUT)
#define GPIO_NR_HTCUNIVERSAL_I2S_SYNC_MD		(31 | GPIO_ALT_FN_1_OUT)

#define GPIO_NR_HTCUNIVERSAL_PHONE_RXD_MD		(34 | GPIO_ALT_FN_1_IN)
#define GPIO_NR_HTCUNIVERSAL_PHONE_UART_CTS_MD		(35 | GPIO_ALT_FN_1_IN)

#define GPIO_NR_HTCUNIVERSAL_PHONE_TXD_MD		(39 | GPIO_ALT_FN_2_OUT)
#define GPIO_NR_HTCUNIVERSAL_PHONE_UART_RTS_MD		(41 | GPIO_ALT_FN_2_OUT)

#define GPIO_NR_HTCUNIVERSAL_BT_RXD_MD			(42 | GPIO_ALT_FN_1_IN)
#define GPIO_NR_HTCUNIVERSAL_BT_TXD_MD			(43 | GPIO_ALT_FN_2_OUT)
#define GPIO_NR_HTCUNIVERSAL_BT_UART_CTS_MD		(44 | GPIO_ALT_FN_1_IN)
#define GPIO_NR_HTCUNIVERSAL_BT_UART_RTS_MD		(45 | GPIO_ALT_FN_2_OUT)

#define GPIO_NR_HTCUNIVERSAL_SIR_RXD_MD			(46 | GPIO_ALT_FN_2_IN)
#define GPIO_NR_HTCUNIVERSAL_SIR_TXD_MD			(47 | GPIO_ALT_FN_1_OUT)

#define GPIO_NR_HTCUNIVERSAL_POE_N_MD			(48 | GPIO_ALT_FN_2_OUT | GPIO_DFLT_HIGH)
#define GPIO_NR_HTCUNIVERSAL_PWE_N_MD			(49 | GPIO_ALT_FN_2_OUT | GPIO_DFLT_HIGH)

#define GPIO_NR_HTCUNIVERSAL_KP_MKIN0_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKIN0 | GPIO_ALT_FN_1_IN)
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN1_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKIN1 | GPIO_ALT_FN_1_IN)
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN2_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKIN2 | GPIO_ALT_FN_1_IN)
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN3_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKIN3 | GPIO_ALT_FN_3_IN)
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN4_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKIN4 | GPIO_ALT_FN_2_IN)
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN5_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKIN5 | GPIO_ALT_FN_1_IN)
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN6_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKIN6 | GPIO_ALT_FN_1_IN)
#define GPIO_NR_HTCUNIVERSAL_KP_MKIN7_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKIN7 | GPIO_ALT_FN_3_IN)

#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT0_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKOUT0 | GPIO_ALT_FN_2_OUT)
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT1_MD		(GPIO_NR_HTCUNIVERSAL_KP_MKOUT1 | GPIO_ALT_FN_2_OUT)
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT2_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKOUT2 | GPIO_ALT_FN_2_OUT)
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT3_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKOUT3 | GPIO_ALT_FN_2_OUT)
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT4_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKOUT4 | GPIO_ALT_FN_2_OUT)
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT5_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKOUT5 | GPIO_ALT_FN_2_OUT)
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT6_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKOUT6 | GPIO_ALT_FN_1_OUT)
#define GPIO_NR_HTCUNIVERSAL_KP_MKOUT7_MD 		(GPIO_NR_HTCUNIVERSAL_KP_MKOUT7 | GPIO_ALT_FN_1_OUT)


#define GPIO_NR_HTCUNIVERSAL_I2S_SYSCLK_MD		(113 | GPIO_ALT_FN_1_OUT)

#define GPIO_NR_HTCUNIVERSAL_PWM1OUT_MD			(115 | GPIO_ALT_FN_3_OUT)

#define GPIO_NR_HTCUNIVERSAL_I2C_SCL_MD			(117 | GPIO_ALT_FN_1_OUT)
#define GPIO_NR_HTCUNIVERSAL_I2C_SDA_MD			(118 | GPIO_ALT_FN_1_OUT)

#endif /* _HTCUNIVERSAL_GPIO_H */