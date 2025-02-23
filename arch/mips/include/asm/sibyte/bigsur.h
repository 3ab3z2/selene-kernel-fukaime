/*
 * Copyright (C) 2000,2001,2002,2003,2004 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef __ASM_SIBYTE_BIGSUR_H
#define __ASM_SIBYTE_BIGSUR_H

#include <asm-generic/sibyte/sb1250.h>
#include <asm-generic/sibyte/bcm1480_int.h>

#ifdef CONFIG_SIBYTE_BIGSUR
#define SIBYTE_BOARD_NAME "BCM91x80A/B (BigSur)"
#define SIBYTE_HAVE_PCMCIA 1
#define SIBYTE_HAVE_IDE	   1
#endif

/* Generic bus chip selects */
#define LEDS_CS		3
#define LEDS_PHYS	0x100a0000

#ifdef SIBYTE_HAVE_IDE
#define IDE_CS		4
#define IDE_PHYS	0x100b0000
#define K_GPIO_GB_IDE	4
#define K_INT_GB_IDE	(K_INT_GPIO_0 + K_GPIO_GB_IDE)
#endif

#ifdef SIBYTE_HAVE_PCMCIA
#define PCMCIA_CS	6
#define PCMCIA_PHYS	0x11000000
#define K_GPIO_PC_READY 9
#define K_INT_PC_READY	(K_INT_GPIO_0 + K_GPIO_PC_READY)
#endif

#endif /* __ASM_SIBYTE_BIGSUR_H */
