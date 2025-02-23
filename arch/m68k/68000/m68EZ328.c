/***************************************************************************/

/*
 *  m68EZ328.c - 68EZ328 specific config
 *
 *  Copyright (C) 1993 Hamish Macdonald
 *  Copyright (C) 1999 D. Jeff Dionne
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

/***************************************************************************/

#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/rtc.h>
#include <asm-generic/pgtable.h>
#include <asm-generic/machdep.h>
#include <asm-generic/MC68EZ328.h>
#ifdef CONFIG_UCSIMM
#include <asm-generic/bootstd.h>
#endif

/***************************************************************************/

int m68328_hwclk(int set, struct rtc_time *t);

/***************************************************************************/

void m68ez328_reset(void)
{
  local_irq_disable();
  asm volatile (
    "moveal #0x10c00000, %a0;\n"
    "moveb #0, 0xFFFFF300;\n"
    "moveal 0(%a0), %sp;\n"
    "moveal 4(%a0), %a0;\n"
    "jmp (%a0);\n"
    );
}

/***************************************************************************/

unsigned char *cs8900a_hwaddr;
static int errno;

#ifdef CONFIG_UCSIMM
_bsc0(char *, getserialnum)
_bsc1(unsigned char *, gethwaddr, int, a)
_bsc1(char *, getbenv, char *, a)
#endif

void __init config_BSP(char *command, int len)
{
  unsigned char *p;

  pr_info("68EZ328 DragonBallEZ support (C) 1999 Rt-Control, Inc\n");

#ifdef CONFIG_UCSIMM
  pr_info("uCsimm serial string [%s]\n", getserialnum());
  p = cs8900a_hwaddr = gethwaddr(0);
  pr_info("uCsimm hwaddr %pM\n", p);

  p = getbenv("APPEND");
  if (p) strcpy(p,command);
  else command[0] = 0;
#endif

  mach_sched_init = hw_timer_init;
  mach_hwclk = m68328_hwclk;
  mach_reset = m68ez328_reset;
}

/***************************************************************************/
