#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"

typedef int64_t token_t;

/* <<--params-def-->> */
#define FP_LEN 64
#define FP_N 1
#define FP_VEC 100

/* <<--params-->> */
const int32_t fp_len = FP_LEN;
const int32_t fp_n = FP_N;
const int32_t fp_vec = FP_VEC;

#define NACC 1

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "fp.0",
		.type = fp,
		/* <<--descriptor-->> */
		.desc.fp_desc.fp_len = FP_LEN,
		.desc.fp_desc.fp_n = FP_N,
		.desc.fp_desc.fp_vec = FP_VEC,
		.desc.fp_desc.src_offset = 0,
		.desc.fp_desc.dst_offset = 0,
		.desc.fp_desc.esp.coherence = ACC_COH_NONE,
		.desc.fp_desc.esp.p2p_store = 0,
		.desc.fp_desc.esp.p2p_nsrcs = 0,
		.desc.fp_desc.esp.p2p_srcs = {"", "", "", ""},
	}
};

#endif /* __ESP_CFG_000_H__ */
