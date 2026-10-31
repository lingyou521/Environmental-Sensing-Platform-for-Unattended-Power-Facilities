#include <linux/iio/consumer.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

struct mq2_device { struct device *dev; struct iio_channel *channel; struct miscdevice misc; struct mutex lock; };

static ssize_t mq2_read(struct file *file, char __user *user_buf, size_t count, loff_t *offset)
{
    struct mq2_device *mq2 = file->private_data;
    int raw = 0;
    int ret;
    (void)offset;
    if (mq2 == NULL || count < sizeof(raw)) return -EINVAL;
    mutex_lock(&mq2->lock);
    ret = iio_read_channel_raw(mq2->channel, &raw);
    mutex_unlock(&mq2->lock);
    if (ret < 0) return ret;
    return copy_to_user(user_buf, &raw, sizeof(raw)) ? -EFAULT : sizeof(raw);
}

static int mq2_open(struct inode *inode, struct file *file)
{
    file->private_data = container_of(file->private_data, struct mq2_device, misc);
    return 0;
}

static const struct file_operations mq2_fops = {.owner = THIS_MODULE, .open = mq2_open, .read = mq2_read};

static int mq2_probe(struct platform_device *pdev)
{
    struct mq2_device *mq2 = devm_kzalloc(&pdev->dev, sizeof(*mq2), GFP_KERNEL);
    int ret;
    if (mq2 == NULL) return -ENOMEM;
    mq2->dev = &pdev->dev;
    mutex_init(&mq2->lock);
    mq2->channel = iio_channel_get(&pdev->dev, "mq2");
    if (IS_ERR(mq2->channel)) return PTR_ERR(mq2->channel);
    mq2->misc.minor = MISC_DYNAMIC_MINOR;
    mq2->misc.name = "mq2_misc";
    mq2->misc.fops = &mq2_fops;
    mq2->misc.parent = &pdev->dev;
    ret = misc_register(&mq2->misc);
    if (ret) { iio_channel_release(mq2->channel); return ret; }
    platform_set_drvdata(pdev, mq2);
    return 0;
}

static int mq2_remove(struct platform_device *pdev)
{
    struct mq2_device *mq2 = platform_get_drvdata(pdev);
    misc_deregister(&mq2->misc);
    iio_channel_release(mq2->channel);
    mutex_destroy(&mq2->lock);
    return 0;
}

static const struct of_device_id mq2_match[] = {{.compatible = "power,mq2"}, {}};
MODULE_DEVICE_TABLE(of, mq2_match);
static struct platform_driver mq2_driver = {.probe = mq2_probe, .remove = mq2_remove,
    .driver = {.name = "mq2", .of_match_table = mq2_match}};
module_platform_driver(mq2_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MQ-2 smoke sensor IIO misc driver");
