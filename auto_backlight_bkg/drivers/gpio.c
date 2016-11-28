#include "../main.h"


int set_bit_out_init(int out_bit, int value)
{
char    command[64];

    if ( value > 1 )
        return 0;

    sprintf(command,"echo %d > /sys/class/gpio/export",out_bit);
    system(command);

    sprintf(command,"echo out > /sys/class/gpio/gpio%d/direction",out_bit);
    system(command);

    sprintf(command,"echo %d > /sys/class/gpio/gpio%d/value",value,out_bit);
    system(command);

    return 1;
}

int set_bit_in_init(int out_bit)
{
char    command[64];

    sprintf(command,"echo %d > /sys/class/gpio/export",out_bit);
    system(command);

    sprintf(command,"echo in > /sys/class/gpio/gpio%d/direction",out_bit);
    system(command);

    return 1;
}

int set_bit_level(int out_bit, int value)
{
char    command[64];
    sprintf(command,"echo %d > /sys/class/gpio/gpio%d/value",value,out_bit);
    system(command);
    return 1;
}

int get_bit_level(int in_bit)
{
char    command[64];
    sprintf(command,"cat /sys/class/gpio/gpio%d/value",in_bit);
    system(command);
    return 1;
}


void set_eth(int val)
{
    set_bit_level(ETH_SWITCH,val);
}

void set_inputs(void)
{
    set_bit_in_init(CFG_BIT0);
    set_bit_in_init(CFG_BIT1);
    set_bit_in_init(CFG_BIT2);
    set_bit_in_init(CFG_BIT3);
    set_bit_in_init(BACKLIGHT_FAULT);
}

void set_outputs(void)
{
    set_bit_out_init(SW_FAULT,1);
    set_bit_out_init(URL_COM,1);
    set_bit_out_init(OVERTEMP,1);
    set_bit_out_init(PANEL_LIGHT_FAIL,1);
    set_bit_out_init(ETH_SWITCH,1); // ... or you will loose the connection, stupid ...!
}

void gpio_init(void)
{
    set_inputs();
    set_outputs();
}
