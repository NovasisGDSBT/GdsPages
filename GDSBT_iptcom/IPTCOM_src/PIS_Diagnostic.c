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
        FTempORHigh=1;
        sprintf(cmd,"echo 0 > %s",OVERTEMP);
        system(cmd);
    }
    else
    {
        ErrorDescription &= (~TFTTEMPRANGEHIGH);
        FTempORHigh=0;
        sprintf(cmd,"echo 1 > %s",OVERTEMP);
        system(cmd);
    }

    if ( SensTempValue < UnderTempLimit )
    {
        ErrorDescription |= TFTTEMPRANGELOW;
        FTempORLow=1;
    }
    else
    {
         ErrorDescription &= (~TFTTEMPRANGELOW);
         FTempORLow=0;
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

    static unsigned int  LOG_IPMODWATCHDOG=0,LOG_APPMODULEWATCHDOG=0,LOG_LEDBKLFAULT=0,LOG_APPMODULEFAULT=0,LOG_IPMODCCUTIMEOUT=0;
    static unsigned int LOG_TFTTEMPRANGEHIGH=0,LOG_TFTTEMPRANGELOW=0,LOG_TEMPSENSFAULT=0,LOG_AMBLIGHTFAULT=0;

    static unsigned  char prev_curr_mode=0;


    if((ErrorDescription & IPMODWATCHDOG)!=0)
    {
        if(LOG_IPMODWATCHDOG ==0)
        {
            LOG_IPMODWATCHDOG=1;
            LOG_SYS("ERROR","DIAG_TASK","WATCHDOG");
        }
    }
    else
       LOG_IPMODWATCHDOG=0;

    if((ErrorDescription & APPMODULEWATCHDOG)!=0)
    {
        if(LOG_APPMODULEWATCHDOG ==0)
        {
            LOG_APPMODULEWATCHDOG=1;
           LOG_SYS("ERROR","DIAG_TASK","APPMODULEWATCHDOG");
        }

    }
    else
      LOG_APPMODULEWATCHDOG=0;

    if((ErrorDescription & LEDBKLFAULT)!=0)
    {

        if(LOG_LEDBKLFAULT==0)
        {
            LOG_LEDBKLFAULT=1;
            LOG_SYS("ERROR","DIAG_TASK","LEDBKLFAULT");
        }
    }
    else
        LOG_LEDBKLFAULT=0;


    if((ErrorDescription & APPMODULEFAULT)!=0)
    {

        if(LOG_APPMODULEFAULT==0)
        {
            LOG_APPMODULEFAULT=1;
            LOG_SYS("ERROR","DIAG_TASK","APPMODULEFAULT");
        }
    }
    else
        LOG_APPMODULEFAULT=0;

    if((ErrorDescription & IPMODCCUTIMEOUT)!=0)
    {

    if(LOG_IPMODCCUTIMEOUT==0)
        {
            LOG_IPMODCCUTIMEOUT=1;
            LOG_SYS("ERROR","DIAG_TASK","CCUTIMEOUT");
        }
    }
    else
        LOG_IPMODCCUTIMEOUT=0;


    if((ErrorDescription & TFTTEMPRANGEHIGH)!=0)
    {

        if(LOG_TFTTEMPRANGEHIGH==0)
        {
            LOG_TFTTEMPRANGEHIGH=1;
            LOG_SYS("ERROR","DIAG_TASK","TEMPRANGEHIGH");
        }
    }
    else
        LOG_TFTTEMPRANGEHIGH=0;


    if((ErrorDescription & TFTTEMPRANGELOW)!=0)
    {

     if(LOG_TFTTEMPRANGELOW==0)
        {
            LOG_TFTTEMPRANGELOW=1;
            LOG_SYS("ERROR","DIAG_TASK","TEMPRANGELOW");
        }
    }
    else
        LOG_TFTTEMPRANGELOW=0;

    if((ErrorDescription & TEMPSENSFAULT)!=0)
    {

      if(LOG_TEMPSENSFAULT==0)
        {
            LOG_TEMPSENSFAULT=1;
            LOG_SYS("ERROR","DIAG_TASK","TEMPSENSFAULT");
        }
    }
    else
        LOG_TEMPSENSFAULT=0;


    if((ErrorDescription & AMBLIGHTFAULT)!=0 )
    {

        if(LOG_AMBLIGHTFAULT==0)
        {
            LOG_AMBLIGHTFAULT=1;
            LOG_SYS("ERROR","DIAG_TASK","AMBLIGHTFAULT");
        }
    }
    else
        LOG_AMBLIGHTFAULT=0;





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




