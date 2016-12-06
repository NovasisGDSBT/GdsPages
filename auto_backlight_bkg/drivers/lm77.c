#include "../main.h"

void set_lm77_device(int file)
{
    if (ioctl(file,I2C_SLAVE,LM77_I2CADDR>>1) < 0)
    {
        printf("Failed to acquire bus access and/or talk to lm77.\n");
        system("echo 1 > /tmp/ext_temp_fault");
        return;
    }
    system("echo 0 > /tmp/ext_temp_fault");
}

short read_lm77_read_temperature(int file)
{
    buf[0] = 0x00;  /* Set pointer to temperature register */
    if (write(file,buf,1) != 1)
    {
        printf("%s : i2c bus write failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_temp_fault");
        return 0;
    }
    if (read(file,buf,2) != 2)
    {
        printf("%s : i2c bus read failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_temp_fault");
        return 0;
    }
    system("echo 0 > /tmp/ext_temp_fault");
    return (buf[0]<<8) | buf[1];
}

int read_lm77_read_configuration(int file)
{
    buf[0] = 0x01;  /* Set pointer to configuration register */
    if (write(file,buf,1) != 1)
    {
        printf("%s : i2c bus write failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_temp_fault");
        return 0;
    }
    if (read(file,buf,1) != 1)
    {
        printf("%s : i2c bus read failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_temp_fault");
        return 0;
    }
    system("echo 0 > /tmp/ext_temp_fault");
    return buf[0];
}
