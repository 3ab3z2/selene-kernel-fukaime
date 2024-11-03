/* SPDX-License-Identifier: GPL-2.0 */
/****************************************************************************/

/*
 *	mcfsim.h -- ColdFire System Integration Module support.
 *
 *	(C) Copyright 1999-2003, Greg Ungerer (gerg@snapgear.com)
 * 	(C) Copyright 2000, Lineo Inc. (www.lineo.com)
 */

/****************************************************************************/
#ifndef	mcfsim_h
#define	mcfsim_h
/****************************************************************************/

/*
 * Include the appropriate ColdFire CPU specific System Integration Module
 * (SIM) definitions.
 */
#if defined(CONFIG_M5206) || defined(CONFIG_M5206e)
#include <asm-generic/m5206sim.h>
#include <asm-generic/mcfintc.h>
#elif defined(CONFIG_M520x)
#include <asm-generic/m520xsim.h>
#elif defined(CONFIG_M523x)
#include <asm-generic/m523xsim.h>
#include <asm-generic/mcfintc.h>
#elif defined(CONFIG_M5249) || defined(CONFIG_M525x)
#include <asm-generic/m525xsim.h>
#include <asm-generic/mcfintc.h>
#elif defined(CONFIG_M527x)
#include <asm-generic/m527xsim.h>
#elif defined(CONFIG_M5272)
#include <asm-generic/m5272sim.h>
#elif defined(CONFIG_M528x)
#include <asm-generic/m528xsim.h>
#elif defined(CONFIG_M5307)
#include <asm-generic/m5307sim.h>
#include <asm-generic/mcfintc.h>
#elif defined(CONFIG_M53xx)
#include <asm-generic/m53xxsim.h>
#elif defined(CONFIG_M5407)
#include <asm-generic/m5407sim.h>
#include <asm-generic/mcfintc.h>
#elif defined(CONFIG_M54xx)
#include <asm-generic/m54xxsim.h>
#elif defined(CONFIG_M5441x)
#include <asm-generic/m5441xsim.h>
#endif

/****************************************************************************/
#endif	/* mcfsim_h */
