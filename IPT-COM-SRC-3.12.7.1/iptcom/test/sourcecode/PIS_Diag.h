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

#define     NOERROR            (0x01FF)
#define     IPMODWATCHDOG      ((unsigned int)1)
#define     IPMODCCUTIMEOUT    ((unsigned int)2)
#define     APPMODULEWATCHDOG  ((unsigned int)4)
#define     APPMODULEFAULT     ((unsigned int)8)
#define     LEDBKLFAULT        ((unsigned int)16)
#define     TEMPSENSFAULT      ((unsigned int)32)
#define     AMBLIGHTFAULT      ((unsigned int)64)
#define     TFTTEMPRANGEHIGHT  ((unsigned int)128)
#define     TFTTEMPRANGELOW    ((unsigned int)256)


#define ERRTYPE_0   ((unsigned char)0)
#define ERRTYPE_1   ((unsigned char)1)


#define BCKL_FAULT_PATH "/sys/class/gpio/gpio165/value"
#define TEMPSENS_FAULT_PATH "/tmp/ext_temp_fault"
#define TEMPSENS_VALUE_PATH "/tmp/SensorTemperatureValue"
#define AMBLIGHT_FAULT_PATH "/tmp/ext_ambientlight_fault"

#endif
