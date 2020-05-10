#include "libesp.h"
#include "cfg.h"
#include <string.h>

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned size;

/* User-defined code */
static int validate_buffer(token_t *out, token_t *gold)
{
	int i;
	int j;
	unsigned errors = 0;

	for (i = 0; i < fp_n; i++)
		for (j = 0; j < fp_vec; j++)
			if (gold[i * out_words_adj + j] != out[i * out_words_adj + j])
				errors++;

	return errors;
}


/* User-defined code */
static void init_buffer(token_t *in, token_t * gold, double * double_in, double * double_gold)
{
	int i;
	int j;
	int k;

	for (i = 0; i < fp_n; i++)
        for (j = 0; j < fp_len * fp_vec; j++)
            if(j%2 == 0)
                double_in[i * in_words_adj + j] = -j/10;
            else
                double_in[i * in_words_adj + j] = j;

    //to convert bits from ieee standard double to an int type
    memcpy((void *)in, (void *)double_in, in_size);

    // Compute golden output
    for (i = 0; i < fp_n; i++)
        for (j = 0; j < fp_vec; j++) {
            double_gold[i * out_words_adj + j] = 0;
            for (k = 0; k < fp_len; k++) {
                double_gold[i * out_words_adj + j] += double_in[i * in_words_adj + j * fp_len + k];
            }
        }

    memcpy((void *)gold, (void *)double_gold, out_size);
}


/* User-defined code */
static void init_parameters()
{
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = fp_len * fp_vec;
		out_words_adj = fp_vec;
	} else {
		in_words_adj = round_up(fp_len * fp_vec, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(fp_vec, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj * (fp_n);
	out_len =  out_words_adj * (fp_n);
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset = in_len;
	size = (out_offset * sizeof(token_t)) + out_size;
}


int main(int argc, char **argv)
{
	int errors;

	token_t *gold;
	token_t *buf;
	double *double_gold;
	double *double_buf;

	init_parameters();

	buf = (token_t *) esp_alloc(size);
	gold = malloc(out_size);
	double_buf = malloc(size);
	double_gold = malloc(out_size);

	init_buffer(buf, gold, double_buf, double_gold);

	printf("\n====== %s ======\n\n", cfg_000[0].devname);
	/* <<--print-params-->> */
	printf("  .fp_len = %d\n", fp_len);
	printf("  .fp_n = %d\n", fp_n);
	printf("  .fp_vec = %d\n", fp_vec);
	printf("\n  ** START **\n");

	esp_run(cfg_000, NACC);

	printf("\n  ** DONE **\n");

	errors = validate_buffer(&buf[out_offset], gold);

	free(gold);
	free(double_buf);
	free(double_gold);
	esp_cleanup();

	if (!errors)
		printf("+ Test PASSED\n");
	else
		printf("+ Test FAILED\n");

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
}
