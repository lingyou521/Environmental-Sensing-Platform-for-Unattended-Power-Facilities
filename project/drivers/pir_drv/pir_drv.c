#include <linux/gpio/consumer.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

struct pir_device { struct gpio_desc *gpio; struct miscdevice misc; };

static ssize_t pir_read(struct file *file, char __user *user_buf, size_t count, loff_t *offset)
{
    struct pir_device *pir = file->private_data;
    int value;
    (void)offset;
    if (pir == NULL || count < sizeof(value)) return -EINVAL;
    value = gpiod_get_value_cansleep(pir->gpio) ? 1 : 0;
    return copy_to_user(user_buf, &value, sizeof(value)) ? -EFAULT : sizeof(value);
}

static int pir_open(struct inode *inode, struct file *file)
{
    file->private_data = container_of(file->private_data, struct pir_device, misc);
    return 0;
}

static const struct file_operations pir_fops = {.owner = THIS_MODULE, .open = pir_open, .read = pir_read};

static int pir_probe(struct platform_device *pdev)
{
    struct pir_device *pir = devm_kzalloc(&pdev->dev, sizeof(*pir), GFP_KERNEL);
    if (pir == NULL) return -ENOMEM;
    pir->gpio = devm_gpiod_get(&pdev->dev, "pir", GPIOD_IN);
    if (IS_ERR(pir->gpio)) return PTR_ERR(pir->gpio);
    pir->misc.minor = MISC_DYNAMIC_MINOR;
    pir->misc.name = "pir_misc";
    pir->misc.fops = &pir_fops;
    pir->misc.parent = &pdev->dev;
    platform_set_drvdata(pdev, pir);
    return misc_register(&pir->misc);
}

static int pir_remove(struct platform_device *pdev)
{
    struct pir_device *pir = platform_get_drvdata(pdev);
    misc_deregister(&pir->misc);
    return 0;
}

static const struct of_device_id pir_match[] = {{.compatible = "power,pir-sensor"}, {}};
MODULE_DEVICE_TABLE(of, pir_match);
static struct platform_driver pir_driver = {.probe = pir_probe, .remove = pir_remove,
    .driver = {.name = "power-pir", .of_match_table = pir_match}};
module_platform_driver(pir_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PIR human presence GPIO misc driver");
