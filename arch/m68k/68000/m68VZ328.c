/***************************************************************************/

/*
 *  m68VZ328.c - 68VZ328 specific config
 *
 *  Copyright (C) 1993 Hamish Macdonald
 *  Copyright (C) 1999 D. Jeff Dionne
 *  Copyright (C) 2001 Georges Menie, Ken Desmet
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

/***************************************************************************/

#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/kd.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/rtc.h>

#include <asm-generic/pgtable.h>
#include <asm-generic/machdep.h>
#include <asm-generic/MC68VZ328.h>
#include <asm-generic/bootstd.h>

#ifdef CONFIG_INIT_LCD
#include "bootlogo-vz.h"
#endif

/***************************************************************************/

int m68328_hwclk(int set, struct rtc_time *t);

/***************************************************************************/
/*                        Init Drangon Engine hardware                     */
/***************************************************************************/
#if defined(CONFIG_DRAGEN2)

static void m68vz328_reset(void)
{
	local_irq_disable();

#ifdef CONFIG_INIT_LCD
	PBDATA |= 0x20;				/* disable CCFL light */
	PKDATA |= 0x4;				/* disable LCD controller */
	LCKCON = 0;
#endif

	__asm__ __volatile__(
		"reset\n\t"
		"moveal #0x04000000, %a0\n\t"
		"moveal 0(%a0), %sp\n\t"
		"moveal 4(%a0), %a0\n\t"
		"jmp (%a0)"
	);
}

static void __init init_hardware(char *command, int size)
{
#ifdef CONFIG_DIRECT_IO_ACCESS
	SCR = 0x10;					/* allow user access to internal registers */
#endif

	/* CSGB Init */
	CSGBB = 0x4000;
	CSB = 0x1a1;

	/* CS8900 init */
	/* PK3: hardware sleep function pin, active low */
	PKSEL |= PK(3);				/* select pin as I/O */
	PKDIR |= PK(3);				/* select pin as output */
	PKDATA |= PK(3);			/* set pin high */

	/* PF5: hardware reset function pin, active high */
	PFSEL |= PF(5);				/* select pin as I/O */
	PFDIR |= PF(5);				/* select pin as output */
	PFDATA &= ~PF(5);			/* set pin low */

	/* cs8900 hardware reset */
	PFDATA |= PF(5);
	{ int i; for (i = 0; i < 32000; ++i); }
	PFDATA &= ~PF(5);

	/* INT1 enable (cs8900 IRQ) */
	PDPOL &= ~PD(1);			/* active high signal */
	PDIQEG &= ~PD(1);
	PDIRQEN |= PD(1);			/* IRQ enabled */

#ifdef CONFIG_INIT_LCD
	/* initialize LCD controller */
	LSSA = (long) screen_bits;
	LVPW = 0x14;
	LXMAX = 0x140;
	LYMAX = 0xef;
	LRRA = 0;
	LPXCD = 3;
	LPICF = 0x08;
	LPOLCF = 0;
	LCKCON = 0x80;
	PCPDEN = 0xff;
	PCSEL = 0;

	/* Enable LCD controller */
	PKDIR |= 0x4;
	PKSEL |= 0x4;
	PKDATA &= ~0x4;

	/* Enable CCFL backlighting circuit */
	PBDIR |= 0x20;
	PBSEL |= 0x20;
	PBDATA &= ~0x20;

	/* contrast control register */
	PFDIR |= 0x1;
	PFSEL &= ~0x1;
	PWMR = 0x037F;
#endif
}

/***************************************************************************/
/*                      Init RT-Control uCdimm hardware                    */
/***************************************************************************/
#elif defined(CONFIG_UCDIMM)

static void m68vz328_reset(void)
{
	local_irq_disable();
	asm volatile (
		"moveal #0x10c00000, %a0;\n\t"
		"moveb #0, 0xFFFFF300;\n\t"
		"moveal 0(%a0), %sp;\n\t"
		"moveal 4(%a0), %a0;\n\t"
		"jmp (%a0);\n"
	);
}

unsigned char *cs8900a_hwaddr;
static int errno;

_bsc0(char *, getserialnum)
_bsc1(unsigned char *, gethwaddr, int, a)
_bsc1(char *, getbenv, char *, a)

static void __init init_hardware(char *command, int size)
{
	char *p;

	pr_info("uCdimm serial string [%s]\n", getserialnum());
	p = cs8900a_hwaddr = gethwaddr(0);
	pr_info("uCdimm hwaddr %pM\n", p);
	p = getbenv("APPEND");
	if (p)
		strcpy(p, command);
	else
		command[0] = 0;
}

/***************************************************************************/
#else

static void m68vz328_reset(void)
{
}

static void __init init_hardware(char *command, int size)
{
}

/***************************************************************************/
#endif
/***************************************************************************/

void __init config_BSP(char *command, int size)
{
	pr_info("68VZ328 DragonBallVZ support (c) 2001 Lineo, Inc.\n");

	init_hardware(command, size);

	mach_sched_init = hw_timer_init;
	mach_hwclk = m68328_hwclk;
	mach_reset = m68vz328_reset;
}

/***************************************************************************/
