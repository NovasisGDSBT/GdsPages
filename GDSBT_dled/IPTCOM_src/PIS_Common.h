/*
module: PIS_IptCom.h
purpose:
reference:
*/

#ifndef _PIS_COM_H
#define _PIS_COM_H

#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "iptcom.h"        /* Common type definitions for IPT  */
#include "vos.h"           /* OS independent system calls */

#include "vos_priv.h"
#include "mdcom_priv.h"
#include "iptcom_priv.h"


#include <stdarg.h>

/* Linux */

/* Thread attributes
 * SCHED_OTHER (for regular non-realtime scheduling)
 * SCHED_RR (realtime round-robin policy)
 * SCHED_FIFO (realtime FIFO policy)
 */

#define POLICY SCHED_RR
#define APPL100_PRIO   9
#define APPL200_PRIO   8
#define APPL300_PRIO   7
#define APPL400_PRIO   6
#define APPL500_PRIO   5
#define APPL600_PRIO   4
#define APPL700_PRIO   3
#define APPL1000_PRIO   1
#define APPL2000_PRIO   2


#define STACK  10000                        /* PD send task stack size */


//#define TIME_MEASURE
#endif
