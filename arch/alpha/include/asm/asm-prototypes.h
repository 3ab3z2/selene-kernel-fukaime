#include <linux/spinlock.h>

#include <asm-generic/checksum.h>
#include <asm-generic/console.h>
#include <asm-generic/page.h>
#include <asm-generic/string.h>
#include <asm-generic/uaccess.h>

#include <asm-generic/asm-prototypes.h>

extern void __divl(void);
extern void __reml(void);
extern void __divq(void);
extern void __remq(void);
extern void __divlu(void);
extern void __remlu(void);
extern void __divqu(void);
extern void __remqu(void);
