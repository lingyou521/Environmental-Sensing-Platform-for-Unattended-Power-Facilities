#include <linux/gpio/consumer.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

struct door_device { struct gpio_desc *gpio; struct miscdevice misc; };

static ssize_t door_read(struct file *file, char __user *user_buf, size_t count, loff_t *offset)
{
    struct door_device *door = file->private_data;
    int value;
    (void)offset;
    if (door == NULL || count < sizeof(value)) return -EINVAL;
    value = gpiod_get_value_cansleep(door->gpio) ? 1 : 0;
    return copy_to_user(user_buf, &value, sizeof(value)) ? -EFAULT : sizeof(value);
}

static int door_open(struct inode *inode, struct file *file)
{
    file->private_data = container_of(file->private_data, struct door_device, misc);
    return 0;
}

static const struct file_operations door_fops = {.owner = THIS_MODULE, .open = door_open, .read = door_read};

static int door_probe(struct platform_device *pdev)
{
    struct door_device *door = devm_kzalloc(&pdev->dev, sizeof(*door), GFP_KERNEL);
    if (door == NULL) return -ENOMEM;
    door->gpio = devm_gpiod_get(&pdev->dev, "door", GPIOD_IN);
    if (IS_ERR(door->gpio)) return PTR_ERR(door->gpio);
    door->misc.minor = MISC_DYNAMIC_MINOR;
    door->misc.name = "door_misc";
    door->misc.fops = &door_fops;
    door->misc.parent = &pdev->dev;
    platform_set_drvdata(pdev, door);
    return misc_register(&door->misc);
}

static int door_remove(struct platform_device *pdev)
{
    struct door_device *door = platform_get_drvdata(pdev);
    misc_deregister(&door->misc);
    return 0;
}

static const struct of_device_id door_match[] = {{.compatible = "power,door-sensor"}, {}};
MODULE_DEVICE_TABLE(of, door_match);
static struct platform_driver door_driver = {.probe = door_probe, .remove = door_remove,
    .driver = {.name = "power-door", .of_match_table = door_match}};
module_platform_driver(door_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Door magnet GPIO misc driver");
