/*
module: PIS_IptCom.h
purpose:
reference:
*/

#ifndef _PIS_IPTCOM_H
#define _PIS_IPTCOM_H

/************************* COM-ID ********************************/
//#include "serp_icd.h"
//#include "mcg_eds_icd.h"
#include "infd_icd.h"

#define PDOUT_URISTRING "grpIFD.aCar.lCst"

#define NUM_OF_MSG            100   /* Number of messages in queue */
#define REMOVE_QUEUE          0     /* Don't destroy the queue if it is in use */
#define REMOVE_QUEUE_ALL_USE  1     /* <> 0 = Remove all use of the queue before it is destroyed */



int IptComStart(void);
void pd_ReportProcess(BYTE *byte_pd_OutData);
void pd_CCUProcess(CCCUC_INFDIS  pd_InData);
void md_Op(CINFDISCtrlOp md_InDataOp,int comId , int srcIpAddr);
void md_Maint(CINFDISCtrlMaint md_InDataMaint,int comId , int srcIpAddr );
void IptComClean(void);

#endif
