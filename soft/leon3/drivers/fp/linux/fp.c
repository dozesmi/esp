#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "fp.h"

#define DRV_NAME	"fp"

/* <<--regs-->> */
#define FP_FP_LEN_REG 0x48
#define FP_FP_N_REG 0x44
#define FP_FP_VEC_REG 0x40

struct fp_device {
	struct esp_device esp;
};

static struct esp_driver fp_driver;

static struct of_device_id fp_device_ids[] = {
	{
		.name = "SLD_FP",
	},
	{
		.name = "eb_053",
	},
	{
		.compatible = "sld,fp",
	},
	{ },
};

static int fp_devs;

static inline struct fp_device *to_fp(struct esp_device *esp)
{
	return container_of(esp, struct fp_device, esp);
}

static void fp_prep_xfer(struct esp_device *esp, void *arg)
{
	struct fp_access *a = arg;

	/* <<--regs-config-->> */
	iowrite32be(a->fp_len, esp->iomem + FP_FP_LEN_REG);
	iowrite32be(a->fp_n, esp->iomem + FP_FP_N_REG);
	iowrite32be(a->fp_vec, esp->iomem + FP_FP_VEC_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);

}

static bool fp_xfer_input_ok(struct esp_device *esp, void *arg)
{
	/* struct fp_device *fp = to_fp(esp); */
	/* struct fp_access *a = arg; */

	return true;
}

static int fp_probe(struct platform_device *pdev)
{
	struct fp_device *fp;
	struct esp_device *esp;
	int rc;

	fp = kzalloc(sizeof(*fp), GFP_KERNEL);
	if (fp == NULL)
		return -ENOMEM;
	esp = &fp->esp;
	esp->module = THIS_MODULE;
	esp->number = fp_devs;
	esp->driver = &fp_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	fp_devs++;
	return 0;
 err:
	kfree(fp);
	return rc;
}

static int __exit fp_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct fp_device *fp = to_fp(esp);

	esp_device_unregister(esp);
	kfree(fp);
	return 0;
}

static struct esp_driver fp_driver = {
	.plat = {
		.probe		= fp_probe,
		.remove		= fp_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = fp_device_ids,
		},
	},
	.xfer_input_ok	= fp_xfer_input_ok,
	.prep_xfer	= fp_prep_xfer,
	.ioctl_cm	= FP_IOC_ACCESS,
	.arg_size	= sizeof(struct fp_access),
};

static int __init fp_init(void)
{
	return esp_driver_register(&fp_driver);
}

static void __exit fp_exit(void)
{
	esp_driver_unregister(&fp_driver);
}

module_init(fp_init)
module_exit(fp_exit)

MODULE_DEVICE_TABLE(of, fp_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("fp driver");
