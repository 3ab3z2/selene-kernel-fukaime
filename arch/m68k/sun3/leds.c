// SPDX-License-Identifier: GPL-2.0
#include <asm-generic/contregs.h>
#include <asm-generic/sun3mmu.h>
#include <asm-generic/io.h>

void sun3_leds(unsigned char byte)
{
	unsigned char dfc;

	GET_DFC(dfc);
	SET_DFC(FC_CONTROL);
	SET_CONTROL_BYTE(AC_LEDS, byte);
	SET_DFC(dfc);
}
