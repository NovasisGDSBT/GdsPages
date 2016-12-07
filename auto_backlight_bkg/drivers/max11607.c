#include "../main.h"

unsigned char max11607_setup(int file)
{
    buf[0] = MAX11607_REG_SETUP | MAX11607_SETUP_X;  /* Setup = 1, ref=vdd , unipolar, no reset  */
    buf[1] = MAX11607_REG_SETUP | MAX11607_SETUP_RST | MAX11607_SETUP_X;  /* Setup = 1, ref=vdd , unipolar, no reset  */
    buf[2] = MAX11607_REG_CONFIG | MAX11607_CONFIG_SGL | MAX11607_CONFIG_CS1;  /* Setup = 0 : Config, 0,1 and 2 registers scan, single ended  */
    if (write(file,buf,3) != 3)
    {
        printf("%s : i2c bus write failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_backlight_fault");
        return 0;
    }
    system("echo 0 > /tmp/ext_backlight_fault");
    return buf[0];
}

void max11607_get_values(int file)
{
int i;
#define     LIMIT   6
    for(i=0;i<LIMIT;i++)
        buf[i] = 0;

    if (read(file,buf,LIMIT) != LIMIT)
    {
        printf("%s : i2c bus read failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_backlight_fault");
        return;
    }
    for(i=0;i<LIMIT;i+=2)
        buf[i] &= 0x03;
    system("echo 0 > /tmp/ext_backlight_fault");
}

int max11607_get_light(void)
{
    return (buf[0]<<8) | buf[1];
}

int max11607_get_voltage24(void)
{
    return (buf[2]<<8) | buf[3];
}

int max11607_get_voltage12(void)
{
    return (buf[4]<<8) | buf[5];
}

void set_max11607_device(int file)
{
    if (ioctl(file,I2C_SLAVE,MAX11607_I2CADDR>>1) < 0)
    {
        printf("Failed to acquire bus access and/or talk to tls2550.\n");
        system("echo 1 > /tmp/ext_backlight_fault");
        return;
    }
    system("echo 0 > /tmp/ext_backlight_fault");
}


