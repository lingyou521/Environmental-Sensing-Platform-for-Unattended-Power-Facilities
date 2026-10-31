#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>

struct sht30_user_sample {
    __u16 temperature_raw;
    __u16 humidity_raw;
};

static struct i2c_client *sht30_client;
static DEFINE_MUTEX(sht30_lock);

static u8 sht30_crc8(const u8 *data)
{
    u8 crc = 0xff;
    int bit;
    int i;
    for (i = 0; i < 2; i++) {
        crc ^= data[i];
        for (bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (u8)((crc << 1) ^ 0x31) : (u8)(crc << 1);
        }
    }
    return crc;
}

static ssize_t sht30_read(struct file *file, char __user *user_buf,
                          size_t count, loff_t *offset)
{
    struct i2c_msg messages[2];
    u8 command[2] = {0x24, 0x00};
    u8 response[6] = {0};
    struct sht30_user_sample sample;
    int ret;

    (void)file;
    (void)offset;
    if (count < sizeof(sample) || sht30_client == NULL) {
        return -EINVAL;
    }

    messages[0].addr = sht30_client->addr;
    messages[0].flags = 0;
    messages[0].len = sizeof(command);
    messages[0].buf = command;
    messages[1].addr = sht30_client->addr;
    messages[1].flags = I2C_M_RD;
    messages[1].len = sizeof(response);
    messages[1].buf = response;

    mutex_lock(&sht30_lock);
    ret = i2c_transfer(sht30_client->adapter, messages, 2);
    mutex_unlock(&sht30_lock);
    if (ret != 2) {
        return ret < 0 ? ret : -EIO;
    }
    if (sht30_crc8(response) != response[2] || sht30_crc8(response + 3) != response[5]) {
        return -EBADMSG;
    }
    sample.temperature_raw = (__u16)((response[0] << 8) | response[1]);
    sample.humidity_raw = (__u16)((response[3] << 8) | response[4]);
    return copy_to_user(user_buf, &sample, sizeof(sample)) ? -EFAULT : sizeof(sample);
}

static const struct file_operations sht30_fops = {
    .owner = THIS_MODULE,
    .read = sht30_read,
};

static struct miscdevice sht30_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "sht30_misc",
    .fops = &sht30_fops,
};

static int sht30_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    (void)id;
    sht30_client = client;
    return misc_register(&sht30_misc);
}

static int sht30_remove(struct i2c_client *client)
{
    (void)client;
    misc_deregister(&sht30_misc);
    sht30_client = NULL;
    return 0;
}

static const struct i2c_device_id sht30_ids[] = {{"sht30", 0}, {}};
MODULE_DEVICE_TABLE(i2c, sht30_ids);
static const struct of_device_id sht30_of_match[] = {{.compatible = "power,sht30"}, {}};
MODULE_DEVICE_TABLE(of, sht30_of_match);
static struct i2c_driver sht30_driver = {
    .probe = sht30_probe,
    .remove = sht30_remove,
    .driver = {.name = "sht30", .of_match_table = sht30_of_match},
    .id_table = sht30_ids,
};
module_i2c_driver(sht30_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SHT30 I2C temperature and humidity misc driver");
