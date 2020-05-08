#ifndef _FP_H_
#define _FP_H_

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#ifndef __user
#define __user
#endif
#endif /* __KERNEL__ */

#include <esp.h>
#include <esp_accelerator.h>

struct fp_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned fp_len;
	unsigned fp_n;
	unsigned fp_vec;
	unsigned src_offset;
	unsigned dst_offset;
};

#define FP_IOC_ACCESS	_IOW ('S', 0, struct fp_access)

#endif /* _FP_H_ */
