/*
module: PIS_Diag.h
purpose:
reference:
*/

#ifndef _PIS_DIAG_H
#define _PIS_DIAG_H

/************************* COM-ID ********************************/
//#include "serp_icd.h"
//#include "mcg_eds_icd.h"
#include "infd_icd.h"

int Diagnostic(void);
void checkErrorPost(void);
#define START       1
#define RUNTIME     2


#define     NOERROR            (0x01FF)
#define     IPMODWATCHDOG      ((unsigned int)0x001)
#define     IPMODCCUTIMEOUT    ((unsigned int)0x002)
#define     APPMODULEWATCHDOG  ((unsigned int)0x004)
#define     APPMODULEFAULT     ((unsigned int)0x008)
#define     LEDBKLFAULT        ((unsigned int)0x010)
#define     TEMPSENSFAULT      ((unsigned int)0x020)
#define     AMBLIGHTFAULT      ((unsigned int)0x040)
#define     TFTTEMPRANGEHIGH   ((unsigned int)0x080)
#define     TFTTEMPRANGELOW    ((unsigned int)0x100)

#define ERRTYPE_0   ((unsigned char)0)
#define ERRTYPE_1   ((unsigned char)1)

#define TEMPSENS_FAULT_PATH "/tmp/ext_temp_fault"
#define TEMPSENS_VALUE_PATH "/tmp/SensorTemperatureValue"
#define AMBLIGHT_FAULT_PATH "/tmp/ext_ambientlight_fault"
#define AMBLIGHT_VALUE_PATH "/tmp/ambientlight_value"

#endif
