/*
 * linux/include/asm-arm/arch-pxa/uncompress.h
 *
 * Author:	Nicolas Pitre
 * Copyright:	(C) 2001 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define FFUART		((volatile unsigned long *)0x40100000)
#define BTUART		((volatile unsigned long *)0x40200000)
#define STUART		((volatile unsigned long *)0x40700000)
#define HWUART		((volatile unsigned long *)0x41600000)

#define UART		FFUART


static inline void putc(char c)
{
/*
#ifdef CONFIG_SERIAL_PXA_CONSOLE
	while (!(UART[5] & 0x20))
		barrier();
	UART[0] = c;
#endif
*/
#if !defined(CONFIG_MACH_PALMZ72) && !defined(CONFIG_MACH_T3XSCALE)
#ifdef CONFIG_LL_PXA_JTAG
__asm__ ("1001: mrc p14, 0, r15, c14, c0, 0\n"
         "bvs 1001b\n"
         "mcr p14, 0, %0, c8, c0, 0\n"
         :
         : "r"(c)
         : "r15"
         );
#else
/*
+       if (machine_is_h1900())
+               return;
+ */
#endif
#endif
}

/*
 * This does not append a newline
 */
static inline void flush(void)
{
}

/*
 * nothing to do
 */
#define arch_decomp_setup()
#define arch_decomp_wdog()