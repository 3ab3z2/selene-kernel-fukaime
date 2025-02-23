// SPDX-License-Identifier: GPL-2.0
/*
 * getopt.c
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/string.h>

#include <asm-generic/errno.h>

#include "getopt.h"

/**
 *	ncp_getopt - option parser
 *	@caller: name of the caller, for error messages
 *	@options: the options string
 *	@opts: an array of &struct option entries controlling parser operations
 *	@optopt: output; will contain the current option
 *	@optarg: output; will contain the value (if one exists)
 *	@value: output; may be NULL; will be overwritten with the integer value
 *		of the current argument.
 *
 *	Helper to parse options on the format used by mount ("a=b,c=d,e,f").
 *	Returns opts->val if a matching entry in the 'opts' array is found,
 *	0 when no more tokens are found, -1 if an error is encountered.
 */
int ncp_getopt(const char *caller, char **options, const struct ncp_option *opts,
	       char **optopt, char **optarg, unsigned long *value)
{
	char *token;
	char *val;

	do {
		if ((token = strsep(options, ",")) == NULL)
			return 0;
	} while (*token == '\0');
	if (optopt)
		*optopt = token;

	if ((val = strchr (token, '=')) != NULL) {
		*val++ = 0;
	}
	*optarg = val;
	for (; opts->name; opts++) {
		if (!strcmp(opts->name, token)) {
			if (!val) {
				if (opts->has_arg & OPT_NOPARAM) {
					return opts->val;
				}
				pr_info("%s: the %s option requires an argument\n",
					caller, token);
				return -EINVAL;
			}
			if (opts->has_arg & OPT_INT) {
				int rc = kstrtoul(val, 0, value);

				if (rc) {
					pr_info("%s: invalid numeric value in %s=%s\n",
						caller, token, val);
					return rc;
				}
				return opts->val;
			}
			if (opts->has_arg & OPT_STRING) {
				return opts->val;
			}
			pr_info("%s: unexpected argument %s to the %s option\n",
				caller, val, token);
			return -EINVAL;
		}
	}
	pr_info("%s: Unrecognized mount option %s\n", caller, token);
	return -EOPNOTSUPP;
}
