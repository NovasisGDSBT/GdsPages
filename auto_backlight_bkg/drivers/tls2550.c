#include "../main.h"

int set_tls2550_device(int file)
{
    if (ioctl(file,I2C_SLAVE,TSL2550_I2CADDR>>1) < 0)
    {
        printf("Failed to acquire bus access and/or talk to tls2550.\n");
        system("echo 1 > /tmp/ext_ambientlight_fault");
        return 1;
    }
    return 0;
}

unsigned char set_tls2550_powerup(int file)
{
    buf[0] = 0x03;  /* Send powerup command */
    if (write(file,buf,1) != 1)
    {
        printf("%s : i2c bus write1 failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_ambientlight_fault");
        return 0;
    }
    buf[0] = 0x18;  /* Standard mode */
    if (write(file,buf,1) != 1)
    {
        printf("%s : i2c bus write2 failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_ambientlight_fault");
        return 0;
    }
    buf[0] = 0;
    if (read(file,buf,1) != 1)
    {
        printf("%s : i2c bus read failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_ambientlight_fault");
        return 0;
    }
    return buf[0];
}

unsigned char read_tls2550_adcs(int file,unsigned char command)
{
    buf[0] = command;  /* Set pointer to temperature register */
    if (write(file,buf,1) != 1)
    {
        printf("%s : i2c bus write3 failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_ambientlight_fault");
        return 0;
    }
    if (read(file,buf,1) != 1)
    {
        printf("%s : i2c bus read failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_ambientlight_fault");
        return 0;
    }
    return buf[0];
}

unsigned char set_tls2550_powerdown(int file)
{
    buf[0] = 0x00;  /* Send powerdown command */
    if (write(file,buf,1) != 1)
    {
        printf("%s : i2c bus write4 failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_ambientlight_fault");
        return 0;
    }
    buf[0] = 0;
    if (read(file,buf,1) != 1)
    {
        printf("%s : i2c bus read failed\n",__FUNCTION__);
        system("echo 1 > /tmp/ext_ambientlight_fault");
        return 0;
    }
    return buf[0];
}

unsigned int TSL2550_chord[8] = {0, 16, 49, 115, 247, 511, 1039, 2095};
unsigned int TSL2550_step_value[8]={1,2,4,8,16,32,64,128};

int tls2550_get_light(int file)
{
unsigned char   ch0;
unsigned int    chord_ch0,step_ch0;
unsigned int    adc0_count;

    ch0 = read_tls2550_adcs(file,0x43);
    if (( ch0 & 0x80) == 0x0)
    {
        return -1;
        system("echo 1 > /tmp/ext_ambientlight_fault");
    }
    chord_ch0 = ((ch0 & 0x7f) >> 4) & 0xff;
    step_ch0 = ch0 & 0x0f;
    adc0_count = TSL2550_chord[chord_ch0] + TSL2550_step_value[chord_ch0] * step_ch0;
    if ( adc0_count > 50 )
    {
        system("echo 0 > /tmp/ext_ambientlight_fault");
        return adc0_count;
    }
    system("echo 1 > /tmp/ext_ambientlight_fault");
    return 0;
}


