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
static void setLog_Diagnostic(void);

extern int test_in_progress ;
extern unsigned char FApiMod;
extern unsigned char FWatchdogApiMod;

extern  int  backlight_state;


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
            //ErrorType(ERRTYPE_1);
        }
        else
        {
            if(((ErrorDescription & IPMODCCUTIMEOUT)!=0) || ((ErrorDescription & TFTTEMPRANGEHIGH)!=0) || ((ErrorDescription & TFTTEMPRANGELOW)!=0))
            {
                curr_mode=ERROR;
                //ErrorType(ERRTYPE_0);
            }
            else if(((ErrorDescription & TEMPSENSFAULT)!=0) || ((ErrorDescription & AMBLIGHTFAULT)!=0 ) )
            {
                if (test_in_progress == 1)
                {
                    curr_mode=TEST;
                }
                else
                {
                    curr_mode=DEGRADED;
                }
            }
            else
            {
            }
        }
    }


    setLog_Diagnostic();
}

 /*******************************************************************************
NAME:       static int ErrorType
ABSTRACT:

RETURNS:    -
*/
/*
static int ErrorType(unsigned char ErrorType)
{
int retval=TRUE;

    if(ErrorType==ERRTYPE_0)
    {

    }
    else if(ErrorType==ERRTYPE_1)
    {

    }
    else
    {

    }
    return retval;
}
*/

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

static void check_InnerErrors()
{
int OverTempLimit;
int UnderTempLimit;
int SensTempValue=0;
int AmbLightSensor;
char    cmd[64];

    system("cat /tmp/temperature_limits | grep INTERNAL_OVERTEMPERATURE | sed 's/INTERNAL_OVERTEMPERATURE=//g'  > /tmp/tempLimitsUP");
    OverTempLimit=get_value("/tmp/tempLimitsUP");
    system("cat /tmp/temperature_limits | grep INTERNAL_UNDERTEMPERATURE | sed 's/INTERNAL_UNDERTEMPERATURE=//g' > /tmp/tempLimitsDOWN");
    UnderTempLimit=get_value("/tmp/tempLimitsDOWN");
    system("cat /tmp/carrier_temp | sed 's/INTERNAL_TEMPERATURE=//g' > /tmp/SensorTemperatureValue");
    SensTempValue=get_value("/tmp/SensorTemperatureValue");


    if (SensTempValue > OverTempLimit)
    {
        ErrorDescription |= TFTTEMPRANGEHIGH;
        sprintf(cmd,"echo 0 > %s",OVERTEMP);
        system(cmd);
    }
    else
    {
        ErrorDescription &= (~TFTTEMPRANGEHIGH);
        sprintf(cmd,"echo 1 > %s",OVERTEMP);
        system(cmd);
    }

    if ( SensTempValue < UnderTempLimit )
    {
        ErrorDescription |= TFTTEMPRANGELOW;
    }
    else
    {
         ErrorDescription &= (~TFTTEMPRANGELOW);
    }

    if (((ErrorDescription & TFTTEMPRANGELOW)!=0) || ((ErrorDescription & TFTTEMPRANGEHIGH)!=0) )
    {
        sprintf(cmd,"echo 1 > %s",BACKLIGHT_CMD);
        system(cmd);
    }
    else
    {
        if ( backlight_state == 1)  /* Power on the backlight only if the backlight was on */
        {
            sprintf(cmd,"echo 0 > %s",BACKLIGHT_CMD);
            system(cmd);
        }
    }

    FBcklightFault=(unsigned char )get_value(BACKLIGHT_FAULT);
    if(FBcklightFault==TRUE)
    {
        ErrorDescription |= LEDBKLFAULT;
        //system("echo 0 > /sys/class/gpio/gpio165/value");
        sprintf(cmd,"echo 0 > %s",PANEL_LIGHT_FAIL);
        system(cmd);
    }
    //NO ELSE --> ERRORTYPE_1



    FTempSensor=(unsigned char )get_value(TEMPSENS_FAULT_PATH);
    if(FTempSensor==TRUE)
    {
        ErrorDescription |= TEMPSENSFAULT;
    }
    else
    {
         ErrorDescription &= (~TEMPSENSFAULT);
    }

    system("cat /tmp/ambientlight_value | sed 's/AMBIENT_LIGHT=//g' > /tmp/AmbientLightValue");

    AmbLightSensor=get_value("/tmp/AmbientLightValue");
    if(AmbLightSensor < 50)
    {
        ErrorDescription |= AMBLIGHTFAULT;
        FAmbLightSensor=1;
    }
    else
    {
         ErrorDescription &= (~AMBLIGHTFAULT);
         FAmbLightSensor=0;
    }


    if(FApiMod==1)
    {
        ErrorDescription |= APPMODULEFAULT;
    }
    // ERR TYPE 1



    if(FWatchdogApiMod==1)
    {
        ErrorDescription |= APPMODULEWATCHDOG;
    }
    // ERR TYPE 1


}


static void setLog_Diagnostic()
{

    static unsigned int  prev_ErrorDescription=0;

    static unsigned  char prev_curr_mode=0;

    if(prev_ErrorDescription != ErrorDescription)
    {
        if((ErrorDescription & IPMODWATCHDOG)!=0)
        {
            LOG_SYS("ERROR","DIAG_TASK","WATCHDOG");
        }

        if((ErrorDescription & APPMODULEWATCHDOG)!=0)
        {
            LOG_SYS("ERROR","DIAG_TASK","APPMODULEWATCHDOG");
        }

        if((ErrorDescription & LEDBKLFAULT)!=0)
        {
            LOG_SYS("ERROR","DIAG_TASK","LEDBKLFAULT");
        }


        if((ErrorDescription & APPMODULEFAULT)!=0)
        {
            LOG_SYS("ERROR","DIAG_TASK","APPMODULEFAULT");
        }

        if((ErrorDescription & IPMODCCUTIMEOUT)!=0)
        {
            LOG_SYS("ERROR","DIAG_TASK","CCUTIMEOUT");
        }
        if((ErrorDescription & TFTTEMPRANGEHIGH)!=0)
        {
            LOG_SYS("ERROR","DIAG_TASK","TEMPRANGEHIGH");
        }

        if((ErrorDescription & TFTTEMPRANGELOW)!=0)
        {
            LOG_SYS("ERROR","DIAG_TASK","TEMPRANGELOW");
        }

        if((ErrorDescription & TEMPSENSFAULT)!=0)
        {
             LOG_SYS("ERROR","DIAG_TASK","TEMPSENSFAULT");
        }

        if((ErrorDescription & AMBLIGHTFAULT)!=0 )
        {
             LOG_SYS("ERROR","DIAG_TASK","AMBLIGHTFAULT");
        }


        prev_ErrorDescription=ErrorDescription;
    }



   if(prev_curr_mode != curr_mode)
    {
        if(curr_mode == NORMAL )
        {
            LOG_SYS("ERROR","DIAG_TASK","NORMAL MODE");
        }
        if(curr_mode == ERROR )
        {
            LOG_SYS("INFO","DIAG_TASK","ERROR MODE");
        }
        if(curr_mode == DEGRADED )
        {
            LOG_SYS("INFO","DIAG_TASK","DEGRADED MODE");
        }
        if(curr_mode == TEST )
        {
            LOG_SYS("INFO","DIAG_TASK","TEST MODE");
        }
        if(curr_mode == PROGRAMMING )
        {
            LOG_SYS("INFO","DIAG_TASK","PROGRAMMING MODE");
        }


        prev_curr_mode=curr_mode;
    }




}




