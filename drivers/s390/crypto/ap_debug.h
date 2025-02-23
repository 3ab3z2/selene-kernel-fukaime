/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  Copyright IBM Corp. 2016
 *  Author(s): Harald Freudenberger <freude@de.ibm.com>
 */
#ifndef AP_DEBUG_H
#define AP_DEBUG_H

#include <asm-generic/debug.h>

#define DBF_ERR		3	/* error conditions   */
#define DBF_WARN	4	/* warning conditions */
#define DBF_INFO	5	/* informational      */
#define DBF_DEBUG	6	/* for debugging only */

#define RC2ERR(rc) ((rc) ? DBF_ERR : DBF_INFO)
#define RC2WARN(rc) ((rc) ? DBF_WARN : DBF_INFO)

#define DBF_MAX_SPRINTF_ARGS 5

#define AP_DBF(...)					\
	debug_sprintf_event(ap_dbf_info, ##__VA_ARGS__)

extern debug_info_t *ap_dbf_info;

int ap_debug_init(void);
void ap_debug_exit(void);

#endif /* AP_DEBUG_H */
