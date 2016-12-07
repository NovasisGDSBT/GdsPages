#include "PIS_Common.h"
#include "PIS_Diag.h"
#include "PIS_App.h"


unsigned int ErrorDescription=0;

unsigned char FBcklightFault=0;
unsigned char FTempSensor=0;
unsigned char FTempORHigh=0;
unsigned char FTempORLow=0;
unsigned char FAmbLightSensor=0;


static void Diagnostic_Core(void);
static int ErrorType(unsigned char ErrorType);
static int get_value(const char *path);
static void check_InnerErrors(void);

extern int test_in_progress ;


int Diagnostic(void)
{
    int res=IPT_OK;

 /* Register IPTCom PD-process in the C scheduler */
   IPTVosRegisterCyclicThread(Diagnostic_Core,"Diagnostic_Core",
                              500,
                              POLICY,
                              APPL1000_PRIO,
                              STACK);

    return res;

}

 /*******************************************************************************
NAME:       Diagnostic_Core
ABSTRACT:   application with 200 ms cycle time

RETURNS:    -
*/
static void Diagnostic_Core(void)
{
    check_InnerErrors();
    if(ErrorDescription == 0)
    {
        if (test_in_progress == 1)
        {
            curr_mode=TEST;
        }
        else
        {
            curr_mode=NORMAL;
        }
    }
    else
    {
        if(((ErrorDescription & IPMODWATCHDOG)!=0) || ((ErrorDescription & APPMODULEWATCHDOG)!=0) || ((ErrorDescription & APPMODULEFAULT)!=0) || ((ErrorDescription & LEDBKLFAULT)!=0)  )
        {
            curr_mode=ERROR;
            ErrorType(ERRTYPE_1);
        //    MON_PRINTF("----------ERROR TYPE 1---------\n");
        }
        else
        {
            if(((ErrorDescription & IPMODCCUTIMEOUT)!=0) || ((ErrorDescription & TFTTEMPRANGEHIGHT)!=0) || ((ErrorDescription & TFTTEMPRANGELOW)!=0))
            {
                 curr_mode=ERROR;
                 ErrorType(ERRTYPE_0);
               //   MON_PRINTF("----------ERROR TYPE 0---------\n");

            }
            else if(((ErrorDescription & TEMPSENSFAULT)!=0) || ((ErrorDescription & AMBLIGHTFAULT)!=0 ) )
            {
                curr_mode=DEGRADED;
             //    MON_PRINTF("----------DEGRADADED---------\n");

            }
            else
            {
             //    MON_PRINTF("----------IMPOSSIBILE---------\n");
            }
        }
    }
}

 /*******************************************************************************
NAME:       static int ErrorType
ABSTRACT:

RETURNS:    -
*/
static int ErrorType(unsigned char ErrorType)
{
    int retval=TRUE;

    if(ErrorType==ERRTYPE_0)
    {


    }
    else if(ErrorType==ERRTYPE_1)
    {

    }
    else{

    }

return retval;

}


static void check_InnerErrors()
{
    int OverTempLimit;
    int UnderTempLimit;
    int SensTemp_value=0;
    int AmbLightSensor;


    FBcklightFault=(unsigned char )get_value(BCKL_FAULT_PATH);


    if(FBcklightFault==TRUE)
    {
         ErrorDescription=ErrorDescription|LEDBKLFAULT;
         system("echo 0 > /sys/class/gpio/gpio165/value");

    //     MON_PRINTF("FBcklightFault: ErrorDescription=%x\n",ErrorDescription);
    }
    else
    {
        //Error T1 Permanent
         system("echo 1 > /sys/class/gpio/gpio165/value");
    }

    FTempSensor=(unsigned char )get_value(TEMPSENS_FAULT_PATH);
    if(FTempSensor==TRUE)
    {
        ErrorDescription=ErrorDescription|TEMPSENSFAULT;
     //    MON_PRINTF("TEMPSENSFAULT: ErrorDescription=%x\n",ErrorDescription);
    }
    else
    {
        ErrorDescription &= (~TEMPSENSFAULT);
    }
    system("cat /tmp/temperature_limits | sed -e '2d' | sed 's/INTERNAL_OVERTEMPERATURE=//g'  > /tmp/tempLimitsUP");
    OverTempLimit=get_value("/tmp/tempLimitsUP");
    system("cat /tmp/temperature_limits | sed -e '1d' | sed 's/INTERNAL_UNDERTEMPERATURE=//g' > /tmp/tempLimitsDOWN");
    UnderTempLimit=get_value("/tmp/tempLimitsDOWN");

    system("cat /tmp/carrier_temp | sed 's/INTERNAL_TEMPERATURE=//g' > /tmp/SensorTemperatureValue");
    SensTemp_value=get_value(TEMPSENS_VALUE_PATH);

    if (SensTemp_value>OverTempLimit)
    {
        FTempORHigh=1;
        ErrorDescription=ErrorDescription|TFTTEMPRANGEHIGHT;
        system("echo 0 > /sys/class/gpio/gpio163/value");
        system("echo 1 > /sys/class/backlight/backlight_lvds0.28/bl_power");
    }
    else
    {
        FTempORHigh=0;
        ErrorDescription &= (~TFTTEMPRANGEHIGHT);
        system("echo 1 > /sys/class/gpio/gpio163/value");
        system("echo 0 > /sys/class/backlight/backlight_lvds0.28/bl_power");
    }

    if (SensTemp_value<UnderTempLimit)
    {
        FTempORLow=1;
        ErrorDescription=ErrorDescription|TFTTEMPRANGELOW;
        system("echo 1 > /sys/class/backlight/backlight_lvds0.28/bl_power");
    }
    else
    {
        FTempORLow=0;
        ErrorDescription &= (~TFTTEMPRANGELOW);
        system("echo 0 > /sys/class/backlight/backlight_lvds0.28/bl_power");
    }

    system("cat /tmp/ambientlight_value | sed 's/AMBIENT_LIGHT=//g' > /tmp/AmbientLightValue");

    AmbLightSensor=get_value("/tmp/AmbientLightValue");
    ErrorDescription &= (~ AMBLIGHTFAULT);
    if(AmbLightSensor < 50)
    {
        ErrorDescription |= AMBLIGHTFAULT;
        FAmbLightSensor=1;
    }
    else
        FAmbLightSensor=0;
}

static int get_value(const char *path)
{
FILE *fp;
char    p[32];
int retval=0;

    fp=fopen(path,"r");

    if ( fp !=NULL)
    {
        fscanf(fp,"%s",p);
        fclose(fp);
        retval=(int)atoi(p);
    }

    return retval;
}
