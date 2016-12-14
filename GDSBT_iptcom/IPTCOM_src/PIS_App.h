/*
module: PIS_App.h
purpose:
reference:
*/

#ifndef _PIS_APP_H
#define _PIS_APP_H

/************************* COM-ID ********************************/
//#include "serp_icd.h"
//#include "mcg_eds_icd.h"
#include "infd_icd.h"
int Application(void);

#define TIMEOUT ((unsigned short int)60)  // 60 seconds


#define     NOTAVAIBLE  ((unsigned char)0)
#define     NORMAL      ((unsigned char)1)
#define     ERROR       ((unsigned char)2)
#define     DEGRADED    ((unsigned char)3)
#define     SHUTDOWN    ((unsigned char)4)
#define     TEST        ((unsigned char)5)
#define     PROGRAMMING ((unsigned char)6)

#define READ 1
#define WRITE 0

/* Input */
#define BACKLIGHT_FAULT     "/sys/class/gpio/gpio158/value"
/* Output */
#define SW_FAULT            "/sys/class/gpio/gpio159/value"
#define URL_COM             "/sys/class/gpio/gpio162/value"
#define OVERTEMP            "/sys/class/gpio/gpio163/value"
#define PANEL_LIGHT_FAIL    "/sys/class/gpio/gpio165/value"
#define BACKLIGHT_CMD       "/sys/class/backlight/backlight_lvds0.28/bl_power"

extern unsigned char  curr_mode;
extern IPT_SEM   semflag;
extern unsigned char flag_mode;


#endif
