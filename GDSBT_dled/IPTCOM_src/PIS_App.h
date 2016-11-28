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

#define TIMEOUT ((unsigned short int)10)  // 10 seconds


#define     NOTAVAIBLE  ((unsigned char)0)
#define     NORMAL      ((unsigned char)1)
#define     ERROR       ((unsigned char)2)
#define     DEGRADED    ((unsigned char)3)
#define     SHUTDOWN    ((unsigned char)4)
#define     TEST        ((unsigned char)5)
#define     PROGRAMMING ((unsigned char)6)

#define READ 1
#define WRITE 0

extern unsigned char  curr_mode;
extern IPT_SEM   semflag;
extern unsigned char flag_mode;


#endif
