#include "main.h"

#define MIN_LUX                 210
#define MAX_LUX                 680
#define BACKLIGHT_STEPS         7
#define DELTA_LUX               (MAX_LUX-MIN_LUX)
#define STEP_LUX                DELTA_LUX/BACKLIGHT_STEPS
#define STEP_LUX_HYST           STEP_LUX/4


//#define DEBUG                   1
int discrete_sample,working_sample;
int current_level=1;
int last_level=1;
int light_value,last_sample=0;

int backlight_max_at_max_light;
int backlight_min_at_min_light;
int max_ambient_light;
int min_ambient_light;
int light_array[8];

void set_bkg(int value)
{
char    cmd[128];

    if ( value <= backlight_min_at_min_light)
        value = backlight_min_at_min_light;
    sprintf(cmd,"echo %d > %s/brightness",value,lvds_ptr);
    /*
    printf("%s\n",cmd);
    */
    system(cmd);
}

int bkg_algo(void)
{
int i;
FILE    *fp;
        system("cat /tmp/backlight_limits | grep BACKLIGHT_MAX_AT_MAXLIGHT | sed 's/BACKLIGHT_MAX_AT_MAXLIGHT=//g' > /tmp/var");
        fp = fopen("/tmp/var","r");
        if ( fp )
        {
            fscanf(fp,"%d",&backlight_max_at_max_light);
            //printf("%d\n",backlight_max_at_max_light);
            fclose(fp);
        }
        else
            return 0;
        system("cat /tmp/backlight_limits | grep BACKLIGHT_MIN_AT_MINLIGHT | sed 's/BACKLIGHT_MIN_AT_MINLIGHT=//g' > /tmp/var");
        fp = fopen("/tmp/var","r");
        if ( fp )
        {
            fscanf(fp,"%d",&backlight_min_at_min_light);
            fclose(fp);
        }
        else
            return 0;

        system("cat /tmp/backlight_limits | grep MAX_AMBIENT_LIGHT | sed 's/MAX_AMBIENT_LIGHT=//g' > /tmp/var");
        fp = fopen("/tmp/var","r");
        if ( fp )
        {
            fscanf(fp,"%d",&max_ambient_light);
            fclose(fp);
        }
        else
            return 0;
        system("cat /tmp/backlight_limits | grep MIN_AMBIENT_LIGHT | sed 's/MIN_AMBIENT_LIGHT=//g' > /tmp/var");
        fp = fopen("/tmp/var","r");
        if ( fp )
        {
            fscanf(fp,"%d",&min_ambient_light);
            fclose(fp);
        }
        else
            return 0;
        /*
        printf("backlight_max_at_max_light = %d\n",backlight_max_at_max_light);
        printf("backlight_min_at_min_light = %d\n",backlight_min_at_min_light);
        printf("max_ambient_light          = %d\n",max_ambient_light);
        printf("min_ambient_light          = %d\n",min_ambient_light);
        printf("light_value                = %d\n",light_value);
        */
        discrete_sample = working_sample = (max_ambient_light - min_ambient_light)/(backlight_max_at_max_light - backlight_min_at_min_light);
        working_sample += min_ambient_light;

        for(i=1;i<(backlight_max_at_max_light-backlight_min_at_min_light);i++)
        {
            /*
            printf("light_value    %d \n",light_value);
            printf("working_sample %d \n",working_sample);
            */
            if ( working_sample > light_value )
            {
                set_bkg(i);
                return light_value;
            }
            working_sample += discrete_sample;
        }
        set_bkg(backlight_max_at_max_light);
        return light_value;
}
