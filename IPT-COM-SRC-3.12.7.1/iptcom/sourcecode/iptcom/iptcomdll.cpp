/*******************************************************************************
*  COPYRIGHT   : (C) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT     :  IPTrain
*
*  MODULE      :  iptcomdll.cpp
*
*  ABSTRACT    :  Stubs for virtual CM methods to be used when creating iptcom.dll
*
********************************************************************************
*  HISTORY     :
*	
* $Id: iptcomdll.cpp 11620 2010-08-16 13:06:37Z bloehr $
*
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
*
*
*******************************************************************************/

/*******************************************************************************
*  INCLUDES */
#include "ipt.h"
#include "iptcom_cpp.h"

/*******************************************************************************
*  NAME     :  Name
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
Name::Name(const char* name)
{
};

/*******************************************************************************
*  NAME     :  TCMS_CO
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
TCMS_CO::TCMS_CO(const char* name)
{
};

/*******************************************************************************
*  NAME     :  CO
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
CO::CO(const char* name) : TCMS_CO(name)
{
};

/*******************************************************************************
*  NAME     :  doRegister
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void CO::doRegister(CM *pCM)
{
};

/*******************************************************************************
*  NAME     :  ACO
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
ACO::ACO(const char* name) : CO(name)
{
};

/*******************************************************************************
*  NAME     :  initiate
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void ACO::initiate()
{
};

/*******************************************************************************
*  NAME     :  schedule
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void ACO::schedule()
{
};

/*******************************************************************************
*  NAME     :  scheduleExit
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void ACO::scheduleExit()
{
};

/*******************************************************************************
*  NAME     :  scheduleEntry
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void ACO::scheduleEntry()
{
};

/*******************************************************************************
*  NAME     :  setScheduling
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void ACO::setScheduling(CM_INT32 priority, E_ACO_ACTIVITY activityStatus, CM_UINT32 cyclePeriod, CM_INT32 schedListId, CM_INT32 positionInList, SynchronousACO* pfirstSACOInList)
{
};

/*******************************************************************************
*  NAME     :  runInMain
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void ACO::runInMain()
{
};

/*******************************************************************************
*  NAME     :  terminateProcess
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void ACO::terminateProcess(CM_INT32 remainingTime)
{
};

/*******************************************************************************
*  NAME     :  SynchronousACO
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
SynchronousACO::SynchronousACO(char const * name) : ACO(name)
{
};

/*******************************************************************************
*  NAME     :  setScheduling
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void SynchronousACO::setScheduling(CM_INT32 priority, E_ACO_ACTIVITY activityStatus, CM_UINT32 cyclePeriod, CM_INT32 schedListId, CM_INT32 positionInList, SynchronousACO* pfirstSACOInList)
{
};

/*******************************************************************************
*  NAME     :  schedule
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void SynchronousACO::schedule()
{
};

/*******************************************************************************
*  NAME     :  initiate
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void SynchronousACO::initiate()
{
};

/*******************************************************************************
*  NAME     :  doRegister
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void SynchronousACO::doRegister(CM *pCM)
{
};

/*******************************************************************************
*  NAME     :  cycleEntry
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void SynchronousACO::cycleEntry()
{
};

/*******************************************************************************
*  NAME     :  cycleExit
*  ABSTRACT :  Stub
*  RETURNS  :  -
*/
void SynchronousACO::cycleExit()
{
};
