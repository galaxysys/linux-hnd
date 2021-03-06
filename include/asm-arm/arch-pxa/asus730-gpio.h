/* 
 * include/asm-arm/arch-pxa/hx4700-gpio.h
 * History:
 *
 * 2004-12-10 Michael Opdenacker. Wrote down GPIO settings as identified by Jamey Hicks.
 *            Reused the h2200-gpio.h file as a template.
 */

#ifndef _A730_GPIO_H_
#define _A730_GPIO_H_

#define A730_GPIO(gpio) (GPIO_NR_A730_ ## gpio)

#define GET_A730_GPIO(gpio) \
	(GPLR(GPIO_NR_A730_ ## gpio) & GPIO_bit(GPIO_NR_A730_ ## gpio))

#define SET_A730_GPIO(gpio, setp) \
do { \
if (setp) \
	GPSR(GPIO_NR_A730_ ## gpio) = GPIO_bit(GPIO_NR_A730_ ## gpio); \
else \
	GPCR(GPIO_NR_A730_ ## gpio) = GPIO_bit(GPIO_NR_A730_ ## gpio); \
} while (0)

#define SET_A730_GPIO_N(gpio, setp) \
do { \
if (setp) \
	GPCR(GPIO_NR_A730_ ## gpio ## _N) = GPIO_bit(GPIO_NR_A730_ ## gpio ## _N); \
else \
	GPSR(GPIO_NR_A730_ ## gpio ## _N) = GPIO_bit(GPIO_NR_A730_ ## gpio ## _N); \
} while (0)

#define A730_IRQ(gpio) \
	IRQ_GPIO(GPIO_NR_A730_ ## gpio)
	
//#define GPIO_NR_A730_KEY_ON			0
#define GPIO_NR_A730_GP_RST_N			1

/*#define GPIO_NR_A730_PWR_SCL			3
#define GPIO_NR_A730_PWR_SDA			4
#define GPIO_NR_A730_PWR_CAP0			5
#define GPIO_NR_A730_PWR_CAP1			6
#define GPIO_NR_A730_PWR_CAP2			7
#define GPIO_NR_A730_PWR_CAP3			8
#define GPIO_NR_A730_CLK_PIO_CPU_13MHz 	9
#define GPIO_NR_A730_CLK_TOUT_32KHz		10
 */
#define GPIO_NR_A730_USB_CABLE_DETECT_N		10
/*
 #define GPIO_NR_A730_CPU_BF_DOOR_N		11
#define GPIO_NR_A730_ASIC3_EXT_INT		12
#define GPIO_NR_A730_W3220_INT			13
#define GPIO_NR_A730_CS1_N			15
*/
#define GPIO_NR_A730_BUTTON_POWER		12
#define GPIO_NR_A730_SD_DETECT_N		13
#define GPIO_NR_A730_BUTTON_RECORD		17

#define GPIO_NR_A730_SD_PWR			18
/*#define GPIO_NR_A730_TOUCHPANEL_SPI_CLK 	19 */
#define GPIO_NR_A730_LCD_EN			19
#define GPIO_NR_A730_USB_PULL_UP		20 
#define GPIO_NR_A730_USB_HOST_EN		21
/*#define GPIO_NR_A730_LCD_RL			22
#define GPIO_NR_A730_SYNAPTICS_SPI_CLK	(23 | GPIO_ALT_FN_2_IN)
#define GPIO_NR_A730_SYNAPTICS_SPI_CS_N	(24 | GPIO_ALT_FN_2_IN)*/
#define GPIO_NR_A730_PENDOWN_IRQ		25 
/*#define GPIO_NR_A730_SYNAPTICS_SPI_DI		(26 | GPIO_ALT_FN_1_IN)*/
#define GPIO_NR_A730_JACK_IRQ			27
//#define IRQ_GPIO_A730_WM9712_IRQ	A730_IRQ(AC97_IRQ)
/*#define GPIO_NR_A730_I2S_BCK			28	* to audio codec *
#define GPIO_NR_A730_I2S_DIN			29	* from audio codec *
#define GPIO_NR_A730_I2S_DOUT			30	* to audio codec *
#define GPIO_NR_A730_I2S_SYNC			31	* to audio codec * */
#define GPIO_NR_A730_RS232_ON			32
/*#define GPIO_NR_A730_CS5_N			33*/
#define GPIO_NR_A730_COM_RXD			34
#define GPIO_NR_A730_COM_CTS			35
#define GPIO_NR_A730_COM_DCD			36
#define GPIO_NR_A730_COM_DSR			37
#define GPIO_NR_A730_COM_RING			38
#define GPIO_NR_A730_COM_TXD			39
#define GPIO_NR_A730_COM_DTR			40
#define GPIO_NR_A730_COM_RTS			41
#define GPIO_NR_A730_BT_RXD			42
#define GPIO_NR_A730_BT_TXD			43
#define GPIO_NR_A730_BT_UART_CTS		44
#define GPIO_NR_A730_BT_UART_RTS		45
/*
#define GPIO_NR_A730_POE_N			48

#define GPIO_NR_A730_PIOR_N			50
#define GPIO_NR_A730_PIOW_N			51
#define GPIO_NR_A730_CPU_BATT_FAULT_N		52

#define GPIO_NR_A730_PCE2_N			54
#define GPIO_NR_A730_PREG_N			55
#define GPIO_NR_A730_PWAIT_N			56
#define GPIO_NR_A730_PIOIS16_N		57
#define GPIO_NR_A730_TOUCHPANEL_IRQ_N		58
#define GPIO_NR_A730_LCD_PC1			59
#define GPIO_NR_A730_CF_RNB			60
#define GPIO_NR_A730_W3220_RESET_N		61
#define GPIO_NR_A730_LCD_RESET_N		62
#define GPIO_NR_A730_CPU_SS_RESET_N		63
*/
#define GPIO_NR_A730_TOUCHPANEL_PEN_PU	65
/*#define GPIO_NR_A730_ASIC3_INT_N		66
#define GPIO_NR_A730_EUART_PS			67

#define GPIO_NR_A730_LCD_SLIN1		70
#define GPIO_NR_A730_ASIC3_RESET_N		71
#define GPIO_NR_A730_CHARGE_EN_N		72
#define GPIO_NR_A730_LCD_UD_1			73

#define GPIO_NR_A730_EARPHONE_DET_N		75*/
#define GPIO_NR_A730_USB_PUEN			76
#define GPIO_NR_A730_BACKLIGHT_EN		77

#define GPIO_NR_A730_PCA9535_IRQ		83
/*
#define GPIO_NR_A730_CS2_N			78
#define GPIO_NR_A730_CS3_N			79
#define GPIO_NR_A730_CS4_N			80
#define GPIO_NR_A730_CPU_GP_RESET_N		81
#define GPIO_NR_A730_EUART_RESET		82
#define GPIO_NR_A730_WLAN_RESET_N		83
#define GPIO_NR_A730_LCD_SQN			84
#define GPIO_NR_A730_PCE1_N			85
#define GPIO_NR_A730_TOUCHPANEL_SPI_DI	86
#define GPIO_NR_A730_TOUCHPANEL_SPI_DO	87
#define GPIO_NR_A730_TOUCHPANEL_SPI_CS_N	88

#define GPIO_NR_A730_USB_HPEN			89				* changed *

#define GPIO_NR_A730_FLASH_VPEN		91
#define GPIO_NR_A730_HP_DRIVER		92
#define GPIO_NR_A730_EUART_INT		93
#define GPIO_NR_A730_KEY_AP3			94
#define GPIO_NR_A730_BATT_OFF			95
#define GPIO_NR_A730_USB_CHARGE_RATE		96
#define GPIO_NR_A730_BL_DETECT_N		97

#define GPIO_NR_A730_KEY_AP1			99
*/
#define GPIO_NR_A730_AC97_SYSCLK		98
#define GPIO_NR_A730_BUTTON_LAUNCHER		94
#define GPIO_NR_A730_BUTTON_CALENDAR		95
#define GPIO_NR_A730_BT_POWER1			97
#define GPIO_NR_A730_BUTTON_TASKS		99
#define GPIO_NR_A730_BUTTON_CONTACTS		100
#define GPIO_NR_A730_SD_RO			103
#define GPIO_NR_A730_BT_POWER2			104
#define GPIO_NR_A730_CF_POWER			105
/*
#define GPIO_NR_A730_SYNAPTICS_POWER_ON	102
#define GPIO_NR_A730_SYNAPTICS_INT		103
#define GPIO_NR_A730_BLUETOOTH_LED		104				* changed *
#define GPIO_NR_A730_IR_ON_N			105
#define GPIO_NR_A730_CPU_BT_RESET_N		106
#define GPIO_NR_A730_SPK_SD_N			107

#define GPIO_NR_A730_CODEC_ON_N		109
#define GPIO_NR_A730_LCD_LVDD_3V3_ON		110
#define GPIO_NR_A730_LCD_AVDD_3V3_ON		111
#define GPIO_NR_A730_LCD_N2V7_7V3_ON		112
#define GPIO_NR_A730_I2S_SYSCLK		113
#define GPIO_NR_A730_CF_RESET			114
#define GPIO_NR_A730_USB2_DREQ		115
#define GPIO_NR_A730_CPU_HW_RESET_N		116
#define GPIO_NR_A730_I2C_SCL			117
#define GPIO_NR_A730_I2C_SDA			118
*/
#endif /* _A730_GPIO_H */
