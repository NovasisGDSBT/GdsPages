#include "main.h"

#define AUTOBACKLIGHT_FILE_NAME     "/tmp/autobacklight_enable"
//#define DEBUG       1
unsigned char advalues[8];
unsigned char lvds_ptr[256];
unsigned char buf[32] = {0};
int           read_buf[10];

int open_i2cdev(void)
{
int file;

    if ((file = open(I2CDEV,O_RDWR)) < 0)
    {
        printf("Failed to open the bus.");
        system("echo 1 > /tmp/ext_ambientlight_fault");
        system("echo 1 > /tmp/ext_temp_fault");
        return 0;
    }
    return file;
}

void close_i2cdev(int file)
{
    close(file);
}

void store_file(char *name,char *var_name,int val)
{
FILE *fp;
    fp = fopen(name,"w");
    if ( fp == NULL)
    {
        printf("Error opening file %s\n",name);
        exit (0);
    }
    fprintf(fp,"%s=%d\n",var_name,val);
    fclose(fp);
}

int check_autobacklight_enable(void)
{
FILE  *filp;

    filp = fopen(AUTOBACKLIGHT_FILE_NAME,"r");
    if( filp == NULL )
        return 1;
    fclose(filp);
    return 0;
}

int get_processor_temp(void)
{
FILE  *filp;
unsigned int processor_temp;
    filp = fopen("/sys/class/thermal/thermal_zone0/temp","r");
    if( filp == NULL )
        exit(0);
    fscanf(filp,"%d",&processor_temp);
    fclose(filp);
    processor_temp /= 1000;
    return processor_temp;

}

int main()
{
int     file,k,ambientlight_value,backlight_sensor_value,carrier_temp,voltage12,voltage24;
unsigned int processor_temp,backlight_max_at_max_light;
FILE    *fp;
char   cmd[256];

    light_value = 0;
    k = 0;
    gpio_init();
    //system("/tmp/www/cgi-bin/find_lvds");
    fp = fopen("/tmp/www/cgi-bin/lvds_device","r");
    if ( fp )
    {
        fscanf(fp,"%s",lvds_ptr);
        fclose(fp);
    }
    system("cat /tmp/backlight_limits | grep BACKLIGHT_MAX_AT_MAXLIGHT | sed 's/BACKLIGHT_MAX_AT_MAXLIGHT=//g' > /tmp/var");
    fp = fopen("/tmp/var","r");
    if ( fp )
    {
        fscanf(fp,"%d",&backlight_max_at_max_light);
        printf("%d\n",backlight_max_at_max_light);
        fclose(fp);
    }
    else
        backlight_max_at_max_light=7;

    system("echo 0 >  /sys/class/graphics/fb0/blank");
    sprintf(cmd,"echo %d > %s/brightness",backlight_max_at_max_light,lvds_ptr);
    printf("%s\n",cmd);
    system(cmd);

    file = open_i2cdev();
    if ( file != 0)
    {
        while(1)
        {
            set_max11607_device(file);
            max11607_setup(file);
            max11607_get_values(file);
            /* 3.2 mV / bit */
            voltage24 = (((max11607_get_voltage24() * 32)/10)*96)/10;
            voltage12 = (((max11607_get_voltage12() * 32)/10)*96)/20;
            ambientlight_value = max11607_get_light();
            #ifdef DEBUG
            printf("voltage24 : %d\n", voltage24);
            printf("voltage12 : %d\n", voltage12);
            printf("ambientlight_value : %d\n", ambientlight_value);
            #endif
            store_file("/tmp/voltage24_value","VOLTAGE24",voltage24);
            store_file("/tmp/voltage12_value","VOLTAGE12",voltage12);

            if ( check_autobacklight_enable() == 0 )
            {
                light_value += ambientlight_value;
                if ( k > 9 )
                {
                    k = 0;
                    light_value /= 10;
                    #ifdef DEBUG
                    printf("light_value : %d\n", light_value);
                    #endif
                    ambientlight_value = bkg_algo();
                    light_value = 0;
                }
            }
            #ifdef DEBUG
            printf("ambientlight_value : %d\n", ambientlight_value);
            #endif
            store_file("/tmp/ambientlight_value","AMBIENT_LIGHT",ambientlight_value);

            if ( set_tls2550_device(file) == 0)
            {
                set_tls2550_powerup(file);
                backlight_sensor_value = tls2550_get_light(file);
                store_file("/tmp/backlight_sensor_value","BACKLIGHT_SENSOR",backlight_sensor_value);
            }

            #ifdef DEBUG
            printf("backlight_sensor_value : %d\n", backlight_sensor_value);
            #endif

            set_lm77_device(file);
            carrier_temp = (read_lm77_read_temperature(file) >> 3)/2;
            #ifdef DEBUG
            printf("Carrier temperature : %d\n",carrier_temp);
            #endif
            store_file("/tmp/carrier_temp","INTERNAL_TEMPERATURE",carrier_temp);

            processor_temp = get_processor_temp();
            #ifdef DEBUG
            printf("Processors temperature : %d\n",processor_temp);
            #endif
            store_file("/tmp/processor_temp","CPU_TEMPERATURE",processor_temp);

            usleep (100000);
            k++;
        }
    }
    system("echo 1 > /tmp/ext_ambientlight_fault");
    system("echo 1 > /tmp/ext_temp_fault");
    return 0;
}
