/*                                                                            */
/*  $Id: tdcIpc.c 11653 2010-08-23 08:17:51Z bloehr $                      */
/*                                                                            */
/*  DESCRIPTION    Interprocess Communication of TDC						  */
/*                                                                            */
/*  AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                            */
/*  REMARKS                                                                   */
/*                                                                            */
/*  DEPENDENCIES   Either the switch LINUX or WIN32 has to be set	          */
/*                                                                            */
/*  MODIFICATIONS (log starts 2010-08-11)									  */
/*   																		  */
/*                                                                            */
/*  All rights reserved. Reproduction, modification, use or disclosure        */
/*  to third parties without express authority is forbidden.                  */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002-2010.             */
/*                                                                            */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#include "tdc.h"
//#include "tdcInit.h"
#include "tdcIpc.h"

// ----------------------------------------------------------------------------

#define stringH2N(x)          // strings do not need to be converted
#define stringN2H(x)          // strings do not need to be converted

#define uriH2N(x)             stringH2N(x)
#define uriN2H(x)             stringN2H(x)
#define labelH2N(x)           stringH2N(x)
#define labelN2H(x)           stringN2H(x)


// ----------------------------------------------------------------------------

static void msgH2N                     (T_TDC_IPC_MSG*     pMsg);
static void msgN2H                     (T_TDC_IPC_MSG*     pMsg);

static void cgetVersionH2N             (T_TDC_IPC_MSG*     pMsg);
static void cgetIptStateH2N            (T_TDC_IPC_MSG*     pMsg);
static void cgetOwnIdsH2N              (T_TDC_IPC_MSG*     pMsg);
static void cgetAddrByNameH2N          (T_TDC_IPC_MSG*     pMsg);
static void cgetUriHostPartH2N         (T_TDC_IPC_MSG*     pMsg);
static void cgetLabel2CarIdH2N         (T_TDC_IPC_MSG*     pMsg);
static void cgetAddr2CarIdH2N          (T_TDC_IPC_MSG*     pMsg);
static void cgetLabel2CstIdH2N         (T_TDC_IPC_MSG*     pMsg);
static void cgetAddr2CstIdH2N          (T_TDC_IPC_MSG*     pMsg);
static void cgetCstNo2CstIdH2N         (T_TDC_IPC_MSG*     pMsg);
static void cgetLabel2TrnCstNoH2N      (T_TDC_IPC_MSG*     pMsg);
static void cgetAddr2TrnCstNoH2N       (T_TDC_IPC_MSG*     pMsg);
static void cgetTrnCstCntH2N           (T_TDC_IPC_MSG*     pMsg);
static void cgetCstCarCntH2N           (T_TDC_IPC_MSG*     pMsg);
static void cgetCarDevCntH2N           (T_TDC_IPC_MSG*     pMsg);
static void cgetCarInfoH2N             (T_TDC_IPC_MSG*     pMsg);
static void cgetUicStateH2N            (T_TDC_IPC_MSG*     pMsg);
static void cgetUicGlobDataH2N         (T_TDC_IPC_MSG*     pMsg);
static void cgetUicCarDataH2N          (T_TDC_IPC_MSG*     pMsg);
static void cgetLabel2UicCarSeqNoH2N   (T_TDC_IPC_MSG*     pMsg);
static void cgetAddr2UicCarSeqNoH2N    (T_TDC_IPC_MSG*     pMsg);
static void cgetUicCarSeqNo2IdsH2N     (T_TDC_IPC_MSG*     pMsg);
static void csetDebugLevelH2N          (T_TDC_IPC_MSG*     pMsg);
static void cgetAddrByNameExtH2N       (T_TDC_IPC_MSG*     pMsg);
static void cgetTrnBackboneTypeH2N     (T_TDC_IPC_MSG*     pMsg);

static void rgetVersionH2N             (T_TDC_IPC_MSG*     pMsg);
static void rgetIptStateH2N            (T_TDC_IPC_MSG*     pMsg);
static void rgetOwnIdsH2N              (T_TDC_IPC_MSG*     pMsg);
static void rgetAddrByNameH2N          (T_TDC_IPC_MSG*     pMsg);
static void rgetUriHostPartH2N         (T_TDC_IPC_MSG*     pMsg);
static void rgetLabel2CarIdH2N         (T_TDC_IPC_MSG*     pMsg);
static void rgetAddr2CarIdH2N          (T_TDC_IPC_MSG*     pMsg);
static void rgetLabel2CstIdH2N         (T_TDC_IPC_MSG*     pMsg);
static void rgetAddr2CstIdH2N          (T_TDC_IPC_MSG*     pMsg);
static void rgetCstNo2CstIdH2N         (T_TDC_IPC_MSG*     pMsg);
static void rgetLabel2TrnCstNoH2N      (T_TDC_IPC_MSG*     pMsg);
static void rgetAddr2TrnCstNoH2N       (T_TDC_IPC_MSG*     pMsg);
static void rgetTrnCstCntH2N           (T_TDC_IPC_MSG*     pMsg);
static void rgetCstCarCntH2N           (T_TDC_IPC_MSG*     pMsg);
static void rgetCarDevCntH2N           (T_TDC_IPC_MSG*     pMsg);
static void rgetCarInfoH2N             (T_TDC_IPC_MSG*     pMsg);
static void rgetUicStateH2N            (T_TDC_IPC_MSG*     pMsg);
static void rgetUicGlobDataH2N         (T_TDC_IPC_MSG*     pMsg);
static void rgetUicCarDataH2N          (T_TDC_IPC_MSG*     pMsg);
static void rgetLabel2UicCarSeqNoH2N   (T_TDC_IPC_MSG*     pMsg);
static void rgetAddr2UicCarSeqNoH2N    (T_TDC_IPC_MSG*     pMsg);
static void rgetUicCarSeqNo2IdsH2N     (T_TDC_IPC_MSG*     pMsg);
static void rsetDebugLevelH2N          (T_TDC_IPC_MSG*     pMsg);
static void rgetAddrByNameExtH2N       (T_TDC_IPC_MSG*     pMsg);
static void rgetTrnBackboneTypeH2N     (T_TDC_IPC_MSG*     pMsg);

static void cgetVersionN2H             (T_TDC_IPC_MSG*     pMsg);
static void cgetIptStateN2H            (T_TDC_IPC_MSG*     pMsg);
static void cgetOwnIdsN2H              (T_TDC_IPC_MSG*     pMsg);
static void cgetAddrByNameN2H          (T_TDC_IPC_MSG*     pMsg);
static void cgetUriHostPartN2H         (T_TDC_IPC_MSG*     pMsg);
static void cgetLabel2CarIdN2H         (T_TDC_IPC_MSG*     pMsg);
static void cgetAddr2CarIdN2H          (T_TDC_IPC_MSG*     pMsg);
static void cgetLabel2CstIdN2H         (T_TDC_IPC_MSG*     pMsg);
static void cgetAddr2CstIdN2H          (T_TDC_IPC_MSG*     pMsg);
static void cgetCstNo2CstIdN2H         (T_TDC_IPC_MSG*     pMsg);
static void cgetLabel2TrnCstNoN2H      (T_TDC_IPC_MSG*     pMsg);
static void cgetAddr2TrnCstNoN2H       (T_TDC_IPC_MSG*     pMsg);
static void cgetTrnCstCntN2H           (T_TDC_IPC_MSG*     pMsg);
static void cgetCstCarCntN2H           (T_TDC_IPC_MSG*     pMsg);
static void cgetCarDevCntN2H           (T_TDC_IPC_MSG*     pMsg);
static void cgetCarInfoN2H             (T_TDC_IPC_MSG*     pMsg);
static void cgetUicStateN2H            (T_TDC_IPC_MSG*     pMsg);
static void cgetUicGlobDataN2H         (T_TDC_IPC_MSG*     pMsg);
static void cgetUicCarDataN2H          (T_TDC_IPC_MSG*     pMsg);
static void cgetLabel2UicCarSeqNoN2H   (T_TDC_IPC_MSG*     pMsg);
static void cgetAddr2UicCarSeqNoN2H    (T_TDC_IPC_MSG*     pMsg);
static void cgetUicCarSeqNo2IdsN2H     (T_TDC_IPC_MSG*     pMsg);
static void csetDebugLevelN2H          (T_TDC_IPC_MSG*     pMsg);
static void cgetAddrByNameExtN2H       (T_TDC_IPC_MSG*     pMsg);
static void cgetTrnBackboneTypeN2H     (T_TDC_IPC_MSG*     pMsg);

static void rgetVersionN2H             (T_TDC_IPC_MSG*     pMsg);
static void rgetIptStateN2H            (T_TDC_IPC_MSG*     pMsg);
static void rgetOwnIdsN2H              (T_TDC_IPC_MSG*     pMsg);
static void rgetAddrByNameN2H          (T_TDC_IPC_MSG*     pMsg);
static void rgetUriHostPartN2H         (T_TDC_IPC_MSG*     pMsg);
static void rgetLabel2CarIdN2H         (T_TDC_IPC_MSG*     pMsg);
static void rgetAddr2CarIdN2H          (T_TDC_IPC_MSG*     pMsg);
static void rgetLabel2CstIdN2H         (T_TDC_IPC_MSG*     pMsg);
static void rgetAddr2CstIdN2H          (T_TDC_IPC_MSG*     pMsg);
static void rgetCstNo2CstIdN2H         (T_TDC_IPC_MSG*     pMsg);
static void rgetLabel2TrnCstNoN2H      (T_TDC_IPC_MSG*     pMsg);
static void rgetAddr2TrnCstNoN2H       (T_TDC_IPC_MSG*     pMsg);
static void rgetTrnCstCntN2H           (T_TDC_IPC_MSG*     pMsg);
static void rgetCstCarCntN2H           (T_TDC_IPC_MSG*     pMsg);
static void rgetCarDevCntN2H           (T_TDC_IPC_MSG*     pMsg);
static void rgetCarInfoN2H             (T_TDC_IPC_MSG*     pMsg);
static void rgetUicStateN2H            (T_TDC_IPC_MSG*     pMsg);
static void rgetUicGlobDataN2H         (T_TDC_IPC_MSG*     pMsg);
static void rgetUicCarDataN2H          (T_TDC_IPC_MSG*     pMsg);
static void rgetLabel2UicCarSeqNoN2H   (T_TDC_IPC_MSG*     pMsg);
static void rgetAddr2UicCarSeqNoN2H    (T_TDC_IPC_MSG*     pMsg);
static void rgetUicCarSeqNo2IdsN2H     (T_TDC_IPC_MSG*     pMsg);
static void rsetDebugLevelN2H          (T_TDC_IPC_MSG*     pMsg);
static void rgetAddrByNameExtN2H       (T_TDC_IPC_MSG*     pMsg);
static void rgetTrnBackboneTypeN2H     (T_TDC_IPC_MSG*     pMsg);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void msgH2N (T_TDC_IPC_MSG*     pMsg)
{
   switch (pMsg->head.msgType)
   {
      case TDC_IPC_CALL_GET_IPT_STATE:             {cgetIptStateH2N          (pMsg);  break;}
      case TDC_IPC_CALL_GET_OWN_IDS:               {cgetOwnIdsH2N            (pMsg);  break;}
      case TDC_IPC_CALL_GET_ADDR_BY_NAME:          {cgetAddrByNameH2N        (pMsg);  break;}
      case TDC_IPC_CALL_GET_URI_HOST_PART:         {cgetUriHostPartH2N       (pMsg);  break;}
      case TDC_IPC_CALL_LABEL_2_CAR_ID:            {cgetLabel2CarIdH2N       (pMsg);  break;}
      case TDC_IPC_CALL_ADDR_2_CAR_ID:             {cgetAddr2CarIdH2N        (pMsg);  break;}
      case TDC_IPC_CALL_LABEL_2_CST_ID:            {cgetLabel2CstIdH2N       (pMsg);  break;}
      case TDC_IPC_CALL_ADDR_2_CST_ID:             {cgetAddr2CstIdH2N        (pMsg);  break;}
      case TDC_IPC_CALL_LABEL_2_TRN_CST_NO:        {cgetLabel2TrnCstNoH2N    (pMsg);  break;}
      case TDC_IPC_CALL_ADDR_2_TRN_CST_NO:         {cgetAddr2TrnCstNoH2N     (pMsg);  break;}
      case TDC_IPC_CALL_GET_TRN_CST_CNT:           {cgetTrnCstCntH2N         (pMsg);  break;}
      case TDC_IPC_CALL_GET_CST_CAR_CNT:           {cgetCstCarCntH2N         (pMsg);  break;}
      case TDC_IPC_CALL_GET_CAR_DEV_CNT:           {cgetCarDevCntH2N         (pMsg);  break;}
      case TDC_IPC_CALL_GET_CAR_INFO:              {cgetCarInfoH2N           (pMsg);  break;}
      case TDC_IPC_CALL_GET_UIC_STATE:             {cgetUicStateH2N          (pMsg);  break;}
      case TDC_IPC_CALL_GET_UIC_GLOB_DATA:         {cgetUicGlobDataH2N       (pMsg);  break;}
      case TDC_IPC_CALL_GET_UIC_CAR_DATA:          {cgetUicCarDataH2N        (pMsg);  break;}
      case TDC_IPC_CALL_LABEL_2_UIC_CAR_SEQ_NO:    {cgetLabel2UicCarSeqNoH2N (pMsg);  break;}
      case TDC_IPC_CALL_ADDR_2_UIC_CAR_SEQ_NO:     {cgetAddr2UicCarSeqNoH2N  (pMsg);  break;}
      case TDC_IPC_CALL_UIC_CAR_SEQ_NO_2_IDS:      {cgetUicCarSeqNo2IdsH2N   (pMsg);  break;}
      case TDC_IPC_CALL_SET_DEBUG_LEVEL:           {csetDebugLevelH2N        (pMsg);  break;}
      case TDC_IPC_CALL_GET_ADDR_BY_NAME_EXT:      {cgetAddrByNameExtH2N     (pMsg);  break;}
      case TDC_IPC_CALL_CSTNO_2_CST_ID:            {cgetCstNo2CstIdH2N       (pMsg);  break;}
      case TDC_IPC_CALL_GET_VERSION:               {cgetVersionH2N           (pMsg);  break;}
      case TDC_IPC_CALL_GET_TBTYPE:                {cgetTrnBackboneTypeH2N   (pMsg);  break;}

      case TDC_IPC_REPLY_GET_IPT_STATE:            {rgetIptStateH2N          (pMsg);  break;}
      case TDC_IPC_REPLY_GET_OWN_IDS:              {rgetOwnIdsH2N            (pMsg);  break;}
      case TDC_IPC_REPLY_GET_ADDR_BY_NAME:         {rgetAddrByNameH2N        (pMsg);  break;}
      case TDC_IPC_REPLY_GET_URI_HOST_PART:        {rgetUriHostPartH2N       (pMsg);  break;}
      case TDC_IPC_REPLY_LABEL_2_CAR_ID:           {rgetLabel2CarIdH2N       (pMsg);  break;}
      case TDC_IPC_REPLY_ADDR_2_CAR_ID:            {rgetAddr2CarIdH2N        (pMsg);  break;}
      case TDC_IPC_REPLY_LABEL_2_CST_ID:           {rgetLabel2CstIdH2N       (pMsg);  break;}
      case TDC_IPC_REPLY_ADDR_2_CST_ID:            {rgetAddr2CstIdH2N        (pMsg);  break;}
      case TDC_IPC_REPLY_LABEL_2_TRN_CST_NO:       {rgetLabel2TrnCstNoH2N    (pMsg);  break;}
      case TDC_IPC_REPLY_ADDR_2_TRN_CST_NO:        {rgetAddr2TrnCstNoH2N     (pMsg);  break;}
      case TDC_IPC_REPLY_GET_TRN_CST_CNT:          {rgetTrnCstCntH2N         (pMsg);  break;}
      case TDC_IPC_REPLY_GET_CST_CAR_CNT:          {rgetCstCarCntH2N         (pMsg);  break;}
      case TDC_IPC_REPLY_GET_CAR_DEV_CNT:          {rgetCarDevCntH2N         (pMsg);  break;}
      case TDC_IPC_REPLY_GET_CAR_INFO:             {rgetCarInfoH2N           (pMsg);  break;}
      case TDC_IPC_REPLY_GET_UIC_STATE:            {rgetUicStateH2N          (pMsg);  break;}
      case TDC_IPC_REPLY_GET_UIC_GLOB_DATA:        {rgetUicGlobDataH2N       (pMsg);  break;}
      case TDC_IPC_REPLY_GET_UIC_CAR_DATA:         {rgetUicCarDataH2N        (pMsg);  break;}
      case TDC_IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO:   {rgetLabel2UicCarSeqNoH2N (pMsg);  break;}
      case TDC_IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO:    {rgetAddr2UicCarSeqNoH2N  (pMsg);  break;}
      case TDC_IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS:     {rgetUicCarSeqNo2IdsH2N   (pMsg);  break;}
      case TDC_IPC_REPLY_SET_DEBUG_LEVEL:          {rsetDebugLevelH2N        (pMsg);  break;}
      case TDC_IPC_REPLY_GET_ADDR_BY_NAME_EXT:     {rgetAddrByNameExtH2N     (pMsg);  break;}
      case TDC_IPC_REPLY_CSTNO_2_CST_ID:           {rgetCstNo2CstIdH2N       (pMsg);  break;}
      case TDC_IPC_REPLY_GET_VERSION:              {rgetVersionH2N           (pMsg);  break;}
      case TDC_IPC_REPLY_GET_TBTYPE:               {rgetTrnBackboneTypeH2N   (pMsg);  break;}
   
      default :
      { 
         break;
      }
   }

   pMsg->head.magicNo   = tdcH2N32 (TDC_IPC_MAGIC_NO);
   pMsg->head.msgType   = tdcH2N32 (pMsg->head.msgType);
   pMsg->head.tdcResult = (T_TDC_RESULT)  tdcH2N32 ((UINT32) pMsg->head.tdcResult);
   pMsg->head.msgLen    = tdcH2N32 (pMsg->head.msgLen);
}

//-----------------------------------------------------------------------------

static void msgN2H (T_TDC_IPC_MSG*     pMsg)
{
   switch (pMsg->head.msgType)
   {
      case TDC_IPC_CALL_GET_IPT_STATE:             {cgetIptStateN2H            (pMsg); break;}
      case TDC_IPC_CALL_GET_OWN_IDS:               {cgetOwnIdsN2H              (pMsg); break;}
      case TDC_IPC_CALL_GET_ADDR_BY_NAME:          {cgetAddrByNameN2H          (pMsg); break;}
      case TDC_IPC_CALL_GET_URI_HOST_PART:         {cgetUriHostPartN2H         (pMsg); break;}
      case TDC_IPC_CALL_LABEL_2_CAR_ID:            {cgetLabel2CarIdN2H         (pMsg); break;}
      case TDC_IPC_CALL_ADDR_2_CAR_ID:             {cgetAddr2CarIdN2H          (pMsg); break;}
      case TDC_IPC_CALL_LABEL_2_CST_ID:            {cgetLabel2CstIdN2H         (pMsg); break;}
      case TDC_IPC_CALL_ADDR_2_CST_ID:             {cgetAddr2CstIdN2H          (pMsg); break;}
      case TDC_IPC_CALL_LABEL_2_TRN_CST_NO:        {cgetLabel2TrnCstNoN2H      (pMsg); break;}
      case TDC_IPC_CALL_ADDR_2_TRN_CST_NO:         {cgetAddr2TrnCstNoN2H       (pMsg); break;}
      case TDC_IPC_CALL_GET_TRN_CST_CNT:           {cgetTrnCstCntN2H           (pMsg); break;}
      case TDC_IPC_CALL_GET_CST_CAR_CNT:           {cgetCstCarCntN2H           (pMsg); break;}
      case TDC_IPC_CALL_GET_CAR_DEV_CNT:           {cgetCarDevCntN2H           (pMsg); break;}
      case TDC_IPC_CALL_GET_CAR_INFO:              {cgetCarInfoN2H             (pMsg); break;}
      case TDC_IPC_CALL_GET_UIC_STATE:             {cgetUicStateN2H            (pMsg); break;}
      case TDC_IPC_CALL_GET_UIC_GLOB_DATA:         {cgetUicGlobDataN2H         (pMsg); break;}
      case TDC_IPC_CALL_GET_UIC_CAR_DATA:          {cgetUicCarDataN2H          (pMsg); break;}
      case TDC_IPC_CALL_LABEL_2_UIC_CAR_SEQ_NO:    {cgetLabel2UicCarSeqNoN2H   (pMsg); break;}
      case TDC_IPC_CALL_ADDR_2_UIC_CAR_SEQ_NO:     {cgetAddr2UicCarSeqNoN2H    (pMsg); break;}
      case TDC_IPC_CALL_UIC_CAR_SEQ_NO_2_IDS:      {cgetUicCarSeqNo2IdsN2H     (pMsg); break;}
      case TDC_IPC_CALL_SET_DEBUG_LEVEL:           {csetDebugLevelN2H          (pMsg); break;}
      case TDC_IPC_CALL_GET_ADDR_BY_NAME_EXT:      {cgetAddrByNameExtN2H       (pMsg); break;}
      case TDC_IPC_CALL_CSTNO_2_CST_ID:            {cgetCstNo2CstIdN2H         (pMsg); break;}
      case TDC_IPC_CALL_GET_VERSION:               {cgetVersionN2H             (pMsg); break;}
      case TDC_IPC_CALL_GET_TBTYPE:                {cgetTrnBackboneTypeN2H     (pMsg); break;}

      case TDC_IPC_REPLY_GET_IPT_STATE:            {rgetIptStateN2H            (pMsg); break;}
      case TDC_IPC_REPLY_GET_OWN_IDS:              {rgetOwnIdsN2H              (pMsg); break;}
      case TDC_IPC_REPLY_GET_ADDR_BY_NAME:         {rgetAddrByNameN2H          (pMsg); break;}
      case TDC_IPC_REPLY_GET_URI_HOST_PART:        {rgetUriHostPartN2H         (pMsg); break;}
      case TDC_IPC_REPLY_LABEL_2_CAR_ID:           {rgetLabel2CarIdN2H         (pMsg); break;}
      case TDC_IPC_REPLY_ADDR_2_CAR_ID:            {rgetAddr2CarIdN2H          (pMsg); break;}
      case TDC_IPC_REPLY_LABEL_2_CST_ID:           {rgetLabel2CstIdN2H         (pMsg); break;}
      case TDC_IPC_REPLY_ADDR_2_CST_ID:            {rgetAddr2CstIdN2H          (pMsg); break;}
      case TDC_IPC_REPLY_LABEL_2_TRN_CST_NO:       {rgetLabel2TrnCstNoN2H      (pMsg); break;}
      case TDC_IPC_REPLY_ADDR_2_TRN_CST_NO:        {rgetAddr2TrnCstNoN2H       (pMsg); break;}
      case TDC_IPC_REPLY_GET_TRN_CST_CNT:          {rgetTrnCstCntN2H           (pMsg); break;}
      case TDC_IPC_REPLY_GET_CST_CAR_CNT:          {rgetCstCarCntN2H           (pMsg); break;}
      case TDC_IPC_REPLY_GET_CAR_DEV_CNT:          {rgetCarDevCntN2H           (pMsg); break;}
      case TDC_IPC_REPLY_GET_CAR_INFO:             {rgetCarInfoN2H             (pMsg); break;}
      case TDC_IPC_REPLY_GET_UIC_STATE:            {rgetUicStateN2H            (pMsg); break;}
      case TDC_IPC_REPLY_GET_UIC_GLOB_DATA:        {rgetUicGlobDataN2H         (pMsg); break;}
      case TDC_IPC_REPLY_GET_UIC_CAR_DATA:         {rgetUicCarDataN2H          (pMsg); break;}
      case TDC_IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO:   {rgetLabel2UicCarSeqNoN2H   (pMsg); break;}
      case TDC_IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO:    {rgetAddr2UicCarSeqNoN2H    (pMsg); break;}
      case TDC_IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS:     {rgetUicCarSeqNo2IdsN2H     (pMsg); break;}
      case TDC_IPC_REPLY_SET_DEBUG_LEVEL:          {rsetDebugLevelN2H          (pMsg); break;}
      case TDC_IPC_REPLY_GET_ADDR_BY_NAME_EXT:     {rgetAddrByNameExtN2H       (pMsg); break;}
      case TDC_IPC_REPLY_CSTNO_2_CST_ID:           {rgetCstNo2CstIdN2H         (pMsg); break;}
      case TDC_IPC_REPLY_GET_VERSION:              {rgetVersionN2H             (pMsg); break;}
      case TDC_IPC_REPLY_GET_TBTYPE:               {rgetTrnBackboneTypeN2H     (pMsg); break;}

      default :                
      {
         break;
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void cgetVersionH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg = pMsg;
}

//-----------------------------------------------------------------------------

static void cgetIptStateH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg = pMsg;
}

//-----------------------------------------------------------------------------

static void cgetOwnIdsH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg = pMsg;
}

//-----------------------------------------------------------------------------

static void cgetAddrByNameH2N (T_TDC_IPC_MSG*     pMsg)
{
   uriH2N (pMsg->data.cGetAddrByName.uri);
   pMsg->data.cGetAddrByName.topoCnt = tdcH2N8 (pMsg->data.cGetAddrByName.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetAddrByNameExtH2N (T_TDC_IPC_MSG*     pMsg)
{
   uriH2N (pMsg->data.cGetAddrByNameExt.uri);
   pMsg->data.cGetAddrByNameExt.topoCnt = tdcH2N8 (pMsg->data.cGetAddrByNameExt.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetTrnBackboneTypeH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg = pMsg;
}

//-----------------------------------------------------------------------------

static void cgetUriHostPartH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cGetUriHostPart.ipAddr  = tdcH2N32 (pMsg->data.cGetUriHostPart.ipAddr);
   pMsg->data.cGetUriHostPart.topoCnt = tdcH2N8  (pMsg->data.cGetUriHostPart.topoCnt);
}                             

//-----------------------------------------------------------------------------

static void cgetLabel2CarIdH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cLabel2CarId.carLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cLabel2CarId.cstLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cLabel2CarId.topoCnt                         = tdcH2N8 (pMsg->data.cLabel2CarId.topoCnt);
   labelH2N (pMsg->data.cLabel2CarId.carLabel);
   labelH2N (pMsg->data.cLabel2CarId.cstLabel);
}

//-----------------------------------------------------------------------------

static void cgetAddr2CarIdH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cAddr2CarId.ipAddr  = tdcH2N32 (pMsg->data.cAddr2CarId.ipAddr);
   pMsg->data.cAddr2CarId.topoCnt = tdcH2N8  (pMsg->data.cAddr2CarId.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetLabel2CstIdH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cLabel2CstId.carLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cLabel2CstId.topoCnt                         = tdcH2N8 (pMsg->data.cLabel2CstId.topoCnt);
   labelH2N (pMsg->data.cLabel2CstId.carLabel);
}

//-----------------------------------------------------------------------------

static void cgetAddr2CstIdH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cAddr2CstId.ipAddr  = tdcH2N32 (pMsg->data.cAddr2CstId.ipAddr);
   pMsg->data.cAddr2CstId.topoCnt = tdcH2N8  (pMsg->data.cAddr2CstId.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetCstNo2CstIdH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cCstNo2CstId.trnCstNo = tdcH2N8 (pMsg->data.cCstNo2CstId.trnCstNo);
   pMsg->data.cCstNo2CstId.topoCnt  = tdcH2N8 (pMsg->data.cCstNo2CstId.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetLabel2TrnCstNoH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cLabel2TrnCstNo.carLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cLabel2TrnCstNo.topoCnt                         = tdcH2N8 (pMsg->data.cLabel2TrnCstNo.topoCnt);
   labelH2N (pMsg->data.cLabel2TrnCstNo.carLabel);
}

//-----------------------------------------------------------------------------

static void cgetAddr2TrnCstNoH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cAddr2TrnCstNo.ipAddr  = tdcH2N32 (pMsg->data.cAddr2TrnCstNo.ipAddr);
   pMsg->data.cAddr2TrnCstNo.topoCnt = tdcH2N8  (pMsg->data.cAddr2TrnCstNo.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetTrnCstCntH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cGetTrnCstCnt.topoCnt = tdcH2N8 (pMsg->data.cGetTrnCstCnt.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetCstCarCntH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cGetCstCarCnt.cstLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cGetCstCarCnt.topoCnt                         = tdcH2N8 (pMsg->data.cGetCstCarCnt.topoCnt);
   labelH2N (pMsg->data.cGetCstCarCnt.cstLabel);
}

//-----------------------------------------------------------------------------

static void cgetCarDevCntH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cGetCarDevCnt.carLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cGetCarDevCnt.cstLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cGetCarDevCnt.topoCnt                         = tdcH2N8 (pMsg->data.cGetCarDevCnt.topoCnt);
   labelH2N (pMsg->data.cGetCarDevCnt.carLabel);
   labelH2N (pMsg->data.cGetCarDevCnt.cstLabel);
}

//-----------------------------------------------------------------------------

static void cgetCarInfoH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cGetCarInfo.topoCnt                         = tdcH2N8 (pMsg->data.cGetCarInfo.topoCnt);
   pMsg->data.cGetCarInfo.maxDev                          = tdcH2N16 (pMsg->data.cGetCarInfo.maxDev);
   pMsg->data.cGetCarInfo.carLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cGetCarInfo.cstLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   labelH2N (pMsg->data.cGetCarInfo.carLabel);
   labelH2N (pMsg->data.cGetCarInfo.cstLabel);
}

//-----------------------------------------------------------------------------

static void cgetUicStateH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg = pMsg;
}

//-----------------------------------------------------------------------------

static void cgetUicGlobDataH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cGetUicGlobData.topoCnt = tdcH2N8 (pMsg->data.cGetUicGlobData.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetUicCarDataH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cGetUicCarData.carSeqNo = tdcH2N8 (pMsg->data.cGetUicCarData.carSeqNo);
   pMsg->data.cGetUicCarData.topoCnt  = tdcH2N8 (pMsg->data.cGetUicCarData.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetLabel2UicCarSeqNoH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cLabel2UicCarSeqNo.carLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cLabel2UicCarSeqNo.cstLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cLabel2UicCarSeqNo.iptTopoCnt                      = tdcH2N8 (pMsg->data.cLabel2UicCarSeqNo.iptTopoCnt);
   pMsg->data.cLabel2UicCarSeqNo.uicTopoCnt                      = tdcH2N8 (pMsg->data.cLabel2UicCarSeqNo.uicTopoCnt);

   labelH2N (pMsg->data.cLabel2UicCarSeqNo.carLabel);
   labelH2N (pMsg->data.cLabel2UicCarSeqNo.cstLabel);
}

//-----------------------------------------------------------------------------

static void cgetAddr2UicCarSeqNoH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cAddr2UicCarSeqNo.ipAddr     = tdcH2N32 (pMsg->data.cAddr2UicCarSeqNo.ipAddr);
   pMsg->data.cAddr2UicCarSeqNo.iptTopoCnt = tdcH2N8 (pMsg->data.cAddr2UicCarSeqNo.iptTopoCnt);
   pMsg->data.cAddr2UicCarSeqNo.uicTopoCnt = tdcH2N8 (pMsg->data.cAddr2UicCarSeqNo.uicTopoCnt);
}

//-----------------------------------------------------------------------------

static void cgetUicCarSeqNo2IdsH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cUicCarSeqNo2Ids.carSeqNo   = tdcH2N8 (pMsg->data.cUicCarSeqNo2Ids.carSeqNo);
   pMsg->data.cUicCarSeqNo2Ids.iptTopoCnt = tdcH2N8 (pMsg->data.cUicCarSeqNo2Ids.iptTopoCnt);
   pMsg->data.cUicCarSeqNo2Ids.uicTopoCnt = tdcH2N8 (pMsg->data.cUicCarSeqNo2Ids.uicTopoCnt);
}

//-----------------------------------------------------------------------------

static void csetDebugLevelH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cSetDebugLevel.dbgLevel[IPT_DBG_LEVEL_STRING_LEN - 1] = '\0';
   stringH2N (pMsg->data.cSetDebugLevel.dbgLevel);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void rgetVersionH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetVersion.version = tdcH2N32 (pMsg->data.rGetVersion.version);
}

//-----------------------------------------------------------------------------

static void rgetIptStateH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetIptState.inaugState = tdcH2N8 (pMsg->data.rGetIptState.inaugState);
   pMsg->data.rGetIptState.topoCnt    = tdcH2N8 (pMsg->data.rGetIptState.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetOwnIdsH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetOwnIds.carId[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rGetOwnIds.cstId[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rGetOwnIds.devId[IPT_MAX_LABEL_LEN - 1] = '\0';

   labelH2N (pMsg->data.rGetOwnIds.carId);
   labelH2N (pMsg->data.rGetOwnIds.cstId);
   labelH2N (pMsg->data.rGetOwnIds.devId);
}

//-----------------------------------------------------------------------------

static void rgetAddrByNameH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetAddrByName.ipAddr  = tdcH2N32 (pMsg->data.rGetAddrByName.ipAddr);
   pMsg->data.rGetAddrByName.topoCnt = tdcH2N8  (pMsg->data.rGetAddrByName.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetAddrByNameExtH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetAddrByNameExt.ipAddr  = tdcH2N32 (pMsg->data.rGetAddrByNameExt.ipAddr);
   pMsg->data.rGetAddrByNameExt.bIsFRG  = (INT32) tdcH2N32 ((UINT32) pMsg->data.rGetAddrByNameExt.bIsFRG);
   pMsg->data.rGetAddrByNameExt.topoCnt = tdcH2N8  (pMsg->data.rGetAddrByNameExt.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetTrnBackboneTypeH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetTbType.tbType        = tdcH2N8  (pMsg->data.rGetTbType.tbType);
   pMsg->data.rGetTbType.gatewayIpAddr = tdcH2N32 (pMsg->data.rGetTbType.gatewayIpAddr);
}

//-----------------------------------------------------------------------------

static void rgetUriHostPartH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetUriHostPart.uri[IPT_MAX_URI_LEN - 1] = '\0';
   uriH2N (pMsg->data.rGetUriHostPart.uri);
   pMsg->data.rGetUriHostPart.topoCnt = tdcH2N8 (pMsg->data.rGetUriHostPart.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetLabel2CarIdH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rLabel2CarId.topoCnt                      = tdcH2N8 (pMsg->data.rLabel2CarId.topoCnt);
   pMsg->data.rLabel2CarId.carId[IPT_MAX_LABEL_LEN - 1] = '\0';
   labelH2N (pMsg->data.rLabel2CarId.carId);
}

//-----------------------------------------------------------------------------

static void rgetAddr2CarIdH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rAddr2CarId.topoCnt                      = tdcH2N8 (pMsg->data.rAddr2CarId.topoCnt);
   pMsg->data.rAddr2CarId.carId[IPT_MAX_LABEL_LEN - 1] = '\0';
   labelH2N (pMsg->data.rAddr2CarId.carId);
}

//-----------------------------------------------------------------------------

static void rgetLabel2CstIdH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rLabel2CstId.topoCnt                      = tdcH2N8 (pMsg->data.rLabel2CstId.topoCnt);
   pMsg->data.rLabel2CstId.cstId[IPT_MAX_LABEL_LEN - 1] = '\0';
   labelH2N (pMsg->data.rLabel2CstId.cstId);
}

//-----------------------------------------------------------------------------

static void rgetAddr2CstIdH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rAddr2CstId.topoCnt                      = tdcH2N8 (pMsg->data.rAddr2CstId.topoCnt);
   pMsg->data.rAddr2CstId.cstId[IPT_MAX_LABEL_LEN - 1] = '\0';
   labelH2N (pMsg->data.rAddr2CstId.cstId);
}

//-----------------------------------------------------------------------------

static void rgetCstNo2CstIdH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rCstNo2CstId.topoCnt                      = tdcH2N8 (pMsg->data.rAddr2CstId.topoCnt);
   pMsg->data.rCstNo2CstId.cstId[IPT_MAX_LABEL_LEN - 1] = '\0';
   labelH2N (pMsg->data.rCstNo2CstId.cstId);
}

//-----------------------------------------------------------------------------

static void rgetLabel2TrnCstNoH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rLabel2TrnCstNo.trnCstNo = tdcH2N8 (pMsg->data.rLabel2TrnCstNo.trnCstNo);
   pMsg->data.rLabel2TrnCstNo.topoCnt  = tdcH2N8 (pMsg->data.rLabel2TrnCstNo.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetAddr2TrnCstNoH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rAddr2TrnCstNo.trnCstNo = tdcH2N8 (pMsg->data.rAddr2TrnCstNo.trnCstNo);
   pMsg->data.rAddr2TrnCstNo.topoCnt  = tdcH2N8 (pMsg->data.rAddr2TrnCstNo.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetTrnCstCntH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetTrnCstCnt.trnCstCnt = tdcH2N8 (pMsg->data.rGetTrnCstCnt.trnCstCnt);
   pMsg->data.rGetTrnCstCnt.topoCnt   = tdcH2N8 (pMsg->data.rGetTrnCstCnt.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetCstCarCntH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetCstCarCnt.carCnt  = tdcH2N8 (pMsg->data.rGetCstCarCnt.carCnt);
   pMsg->data.rGetCstCarCnt.topoCnt = tdcH2N8 (pMsg->data.rGetCstCarCnt.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetCarDevCntH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetCarDevCnt.devCnt  = tdcH2N16 (pMsg->data.rGetCarDevCnt.devCnt);
   pMsg->data.rGetCarDevCnt.topoCnt = tdcH2N8  (pMsg->data.rGetCarDevCnt.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetCarInfoH2N (T_TDC_IPC_MSG*     pMsg)
{
   int         i;
   UINT16      devCnt = (pMsg->data.rGetCarInfo.devCnt > IPC_MAX_DEV_PER_CAR) 
                             ? (UINT16) IPC_MAX_DEV_PER_CAR 
                             : pMsg->data.rGetCarInfo.devCnt;

   pMsg->data.rGetCarInfo.carId[IPT_MAX_LABEL_LEN - 1]   = '\0';
   pMsg->data.rGetCarInfo.carType[IPT_MAX_LABEL_LEN - 1] = '\0';
   labelH2N (pMsg->data.rGetCarInfo.carId);
   labelH2N (pMsg->data.rGetCarInfo.carType);

   for (i = 0; i < IPT_UIC_IDENTIFIER_CNT; i++)
   {
      pMsg->data.rGetCarInfo.uicIdent[i] = tdcH2N8 (pMsg->data.rGetCarInfo.uicIdent[i]);
   }

   pMsg->data.rGetCarInfo.cstCarNo  = tdcH2N8  (pMsg->data.rGetCarInfo.cstCarNo);
   pMsg->data.rGetCarInfo.trnOrient = tdcH2N8  (pMsg->data.rGetCarInfo.trnOrient);
   pMsg->data.rGetCarInfo.cstOrient = tdcH2N8  (pMsg->data.rGetCarInfo.cstOrient);
   pMsg->data.rGetCarInfo.devCnt    = tdcH2N16 (pMsg->data.rGetCarInfo.devCnt);
   pMsg->data.rGetCarInfo.topoCnt   = tdcH2N8  (pMsg->data.rGetCarInfo.topoCnt);

   for (i = 0; i < devCnt; i++)
   {
      pMsg->data.rGetCarInfo.devData[i].hostId                       = tdcH2N16 (pMsg->data.rGetCarInfo.devData[i].hostId);
      pMsg->data.rGetCarInfo.devData[i].devId[IPT_MAX_LABEL_LEN - 1] = '\0';
      labelH2N (pMsg->data.rGetCarInfo.devData[i].devId);
   }
}

//-----------------------------------------------------------------------------

static void rgetUicStateH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetUicState.inaugState = tdcH2N8 (pMsg->data.rGetUicState.inaugState);
   pMsg->data.rGetUicState.topoCnt    = tdcH2N8 (pMsg->data.rGetUicState.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetUicGlobDataH2N  (T_TDC_IPC_MSG*     pMsg)
{
   UINT32      i;

   for (i = 0; i < IPT_UIC_CONF_POS_CNT; i++)
   {
      pMsg->data.rGetUicGlobData.confPos[i] = tdcH2N8 (pMsg->data.rGetUicGlobData.confPos[i]);
   }

   pMsg->data.rGetUicGlobData.confPosAvail   = tdcH2N8 (pMsg->data.rGetUicGlobData.confPosAvail);
   pMsg->data.rGetUicGlobData.operatAvail    = tdcH2N8 (pMsg->data.rGetUicGlobData.operatAvail);
   pMsg->data.rGetUicGlobData.natApplAvail   = tdcH2N8 (pMsg->data.rGetUicGlobData.natApplAvail);
   pMsg->data.rGetUicGlobData.cstPropAvail   = tdcH2N8 (pMsg->data.rGetUicGlobData.cstPropAvail);
   pMsg->data.rGetUicGlobData.carPropAvail   = tdcH2N8 (pMsg->data.rGetUicGlobData.carPropAvail);
   pMsg->data.rGetUicGlobData.seatResNoAvail = tdcH2N8 (pMsg->data.rGetUicGlobData.seatResNoAvail);
   pMsg->data.rGetUicGlobData.inaugFrameVer  = tdcH2N8 (pMsg->data.rGetUicGlobData.inaugFrameVer);
   pMsg->data.rGetUicGlobData.rDataVer       = tdcH2N8 (pMsg->data.rGetUicGlobData.rDataVer);
   pMsg->data.rGetUicGlobData.inaugState     = tdcH2N8 (pMsg->data.rGetUicGlobData.inaugState);
   pMsg->data.rGetUicGlobData.topoCnt        = tdcH2N8 (pMsg->data.rGetUicGlobData.topoCnt);
   pMsg->data.rGetUicGlobData.orient         = tdcH2N8 (pMsg->data.rGetUicGlobData.orient);
   pMsg->data.rGetUicGlobData.notAllConf     = tdcH2N8 (pMsg->data.rGetUicGlobData.notAllConf);
   pMsg->data.rGetUicGlobData.confCancelled  = tdcH2N8 (pMsg->data.rGetUicGlobData.confCancelled);
   pMsg->data.rGetUicGlobData.trnCarCnt      = tdcH2N8 (pMsg->data.rGetUicGlobData.trnCarCnt);
   pMsg->data.rGetUicGlobData.topoCnt        = tdcH2N8 (pMsg->data.rGetUicGlobData.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetUicCarDataH2N (T_TDC_IPC_MSG*     pMsg)
{
   UINT32      i;

   for (i = 0; i < IPT_MAX_UIC_CST_NO; i++)
   {
      pMsg->data.rGetUicCarData.cstProp[i] = tdcH2N8 (pMsg->data.rGetUicCarData.cstProp[i]);
   }

   for (i = 0; i < IPT_UIC_CAR_PROPERTY_CNT; i++)
   {
      pMsg->data.rGetUicCarData.carProp[i] = tdcH2N8 (pMsg->data.rGetUicCarData.carProp[i]);
   }

   for (i = 0; i < IPT_UIC_IDENTIFIER_CNT; i++)
   {
      pMsg->data.rGetUicCarData.uicIdent[i] = tdcH2N8 (pMsg->data.rGetUicCarData.uicIdent[i]);
   }

   pMsg->data.rGetUicCarData.trnCstNo      = tdcH2N8  (pMsg->data.rGetUicCarData.trnCstNo);       
   pMsg->data.rGetUicCarData.seatResNo     = tdcH2N16 (pMsg->data.rGetUicCarData.seatResNo);      
   pMsg->data.rGetUicCarData.carSeqNo      = tdcH2N8  (pMsg->data.rGetUicCarData.carSeqNo);       
   pMsg->data.rGetUicCarData.contrCarCnt   = (INT8) tdcH2N8  ((UINT8) pMsg->data.rGetUicCarData.contrCarCnt);    
   pMsg->data.rGetUicCarData.operat        = tdcH2N8  (pMsg->data.rGetUicCarData.operat);         
   pMsg->data.rGetUicCarData.owner         = tdcH2N8  (pMsg->data.rGetUicCarData.owner);          
   pMsg->data.rGetUicCarData.natAppl       = tdcH2N8  (pMsg->data.rGetUicCarData.natAppl);        
   pMsg->data.rGetUicCarData.natVer        = tdcH2N8  (pMsg->data.rGetUicCarData.natVer);         
   pMsg->data.rGetUicCarData.trnOrient     = tdcH2N8  (pMsg->data.rGetUicCarData.trnOrient);      
   pMsg->data.rGetUicCarData.cstOrient     = tdcH2N8  (pMsg->data.rGetUicCarData.cstOrient);      
   pMsg->data.rGetUicCarData.isLeading     = tdcH2N8  (pMsg->data.rGetUicCarData.isLeading);      
   pMsg->data.rGetUicCarData.isLeadRequ    = tdcH2N8  (pMsg->data.rGetUicCarData.isLeadRequ);     
   pMsg->data.rGetUicCarData.trnSwInCarCnt = tdcH2N8  (pMsg->data.rGetUicCarData.trnSwInCarCnt);  
   pMsg->data.rGetUicCarData.topoCnt       = tdcH2N8  (pMsg->data.rGetUicCarData.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetLabel2UicCarSeqNoH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rLabel2UicCarSeqNo.carSeqNo   = tdcH2N8 (pMsg->data.rLabel2UicCarSeqNo.carSeqNo);
   pMsg->data.rLabel2UicCarSeqNo.iptTopoCnt = tdcH2N8 (pMsg->data.rLabel2UicCarSeqNo.iptTopoCnt);
   pMsg->data.rLabel2UicCarSeqNo.uicTopoCnt = tdcH2N8 (pMsg->data.rLabel2UicCarSeqNo.uicTopoCnt);
}

//-----------------------------------------------------------------------------

static void rgetAddr2UicCarSeqNoH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rAddr2UicCarSeqNo.carSeqNo   = tdcH2N8 (pMsg->data.rAddr2UicCarSeqNo.carSeqNo);
   pMsg->data.rAddr2UicCarSeqNo.iptTopoCnt = tdcH2N8 (pMsg->data.rAddr2UicCarSeqNo.iptTopoCnt);
   pMsg->data.rAddr2UicCarSeqNo.uicTopoCnt = tdcH2N8 (pMsg->data.rAddr2UicCarSeqNo.uicTopoCnt);
}

//-----------------------------------------------------------------------------

static void rgetUicCarSeqNo2IdsH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rUicCarSeqNo2Ids.iptTopoCnt                      = tdcH2N8 (pMsg->data.rUicCarSeqNo2Ids.iptTopoCnt);
   pMsg->data.rUicCarSeqNo2Ids.uicTopoCnt                      = tdcH2N8 (pMsg->data.rUicCarSeqNo2Ids.uicTopoCnt);
   pMsg->data.rUicCarSeqNo2Ids.carId[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rUicCarSeqNo2Ids.cstId[IPT_MAX_LABEL_LEN - 1] = '\0';
   labelH2N (pMsg->data.rUicCarSeqNo2Ids.carId);
   labelH2N (pMsg->data.rUicCarSeqNo2Ids.cstId);
}

//-----------------------------------------------------------------------------

static void rsetDebugLevelH2N (T_TDC_IPC_MSG*     pMsg)
{
   pMsg = pMsg;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void cgetVersionN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg = pMsg;
}

//-----------------------------------------------------------------------------

static void cgetIptStateN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg = pMsg;
}

//-----------------------------------------------------------------------------

static void cgetOwnIdsN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg = pMsg;
}

//-----------------------------------------------------------------------------

static void cgetAddrByNameN2H (T_TDC_IPC_MSG*     pMsg)
{
   uriN2H (pMsg->data.cGetAddrByName.uri);
   pMsg->data.cGetAddrByName.uri[IPT_MAX_URI_LEN - 1] = '\0';
   pMsg->data.cGetAddrByName.topoCnt                  = tdcN2H8 (pMsg->data.cGetAddrByName.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetAddrByNameExtN2H (T_TDC_IPC_MSG*     pMsg)
{
   uriN2H (pMsg->data.cGetAddrByNameExt.uri);
   pMsg->data.cGetAddrByNameExt.uri[IPT_MAX_URI_LEN - 1] = '\0';
   pMsg->data.cGetAddrByNameExt.topoCnt                  = tdcN2H8 (pMsg->data.cGetAddrByNameExt.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetTrnBackboneTypeN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg = pMsg;
}

//-----------------------------------------------------------------------------

static void cgetUriHostPartN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cGetUriHostPart.ipAddr  = tdcN2H32 (pMsg->data.cGetUriHostPart.ipAddr);
   pMsg->data.cGetUriHostPart.topoCnt = tdcN2H8 (pMsg->data.cGetUriHostPart.topoCnt);
}                             

//-----------------------------------------------------------------------------

static void cgetLabel2CarIdN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.cLabel2CarId.carLabel);
   labelN2H (pMsg->data.cLabel2CarId.cstLabel);
   pMsg->data.cLabel2CarId.carLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cLabel2CarId.cstLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cLabel2CarId.topoCnt = tdcN2H8 (pMsg->data.cLabel2CarId.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetAddr2CarIdN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cAddr2CarId.ipAddr  = tdcN2H32 (pMsg->data.cAddr2CarId.ipAddr);
   pMsg->data.cAddr2CarId.topoCnt = tdcN2H8  (pMsg->data.cAddr2CarId.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetLabel2CstIdN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.cLabel2CstId.carLabel);
   pMsg->data.cLabel2CstId.topoCnt = tdcN2H8 (pMsg->data.cLabel2CstId.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetAddr2CstIdN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cAddr2CstId.ipAddr  = tdcN2H32 (pMsg->data.cAddr2CstId.ipAddr);
   pMsg->data.cAddr2CstId.topoCnt = tdcN2H8  (pMsg->data.cAddr2CstId.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetCstNo2CstIdN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cCstNo2CstId.trnCstNo = tdcN2H8 (pMsg->data.cCstNo2CstId.trnCstNo);
   pMsg->data.cCstNo2CstId.topoCnt  = tdcN2H8 (pMsg->data.cCstNo2CstId.topoCnt);

}

//-----------------------------------------------------------------------------

static void cgetLabel2TrnCstNoN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.cLabel2TrnCstNo.carLabel);
   pMsg->data.cLabel2TrnCstNo.carLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cLabel2TrnCstNo.topoCnt                         = tdcN2H8 (pMsg->data.cLabel2TrnCstNo.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetAddr2TrnCstNoN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cAddr2TrnCstNo.ipAddr  = tdcN2H32 (pMsg->data.cAddr2TrnCstNo.ipAddr);
   pMsg->data.cAddr2TrnCstNo.topoCnt = tdcN2H8  (pMsg->data.cAddr2TrnCstNo.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetTrnCstCntN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cGetTrnCstCnt.topoCnt = tdcN2H8 (pMsg->data.cGetTrnCstCnt.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetCstCarCntN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.cGetCstCarCnt.cstLabel);
   pMsg->data.cGetCstCarCnt.cstLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cGetCstCarCnt.topoCnt                         = tdcN2H8 (pMsg->data.cGetCstCarCnt.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetCarDevCntN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.cGetCarDevCnt.carLabel);
   labelN2H (pMsg->data.cGetCarDevCnt.cstLabel);
   pMsg->data.cGetCarDevCnt.carLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cGetCarDevCnt.cstLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cGetCarDevCnt.topoCnt                         = tdcN2H8 (pMsg->data.cGetCarDevCnt.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetCarInfoN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.cGetCarInfo.carLabel);
   labelN2H (pMsg->data.cGetCarInfo.cstLabel);
   pMsg->data.cGetCarInfo.carLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cGetCarInfo.cstLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cGetCarInfo.topoCnt                         = tdcN2H8  (pMsg->data.cGetCarInfo.topoCnt);
   pMsg->data.cGetCarInfo.maxDev                          = tdcN2H16 (pMsg->data.cGetCarInfo.maxDev);
}

//-----------------------------------------------------------------------------

static void cgetUicStateN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg = pMsg;
}

//-----------------------------------------------------------------------------

static void cgetUicGlobDataN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cGetUicGlobData.topoCnt = tdcN2H8 (pMsg->data.cGetUicGlobData.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetUicCarDataN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cGetUicCarData.carSeqNo = tdcN2H8 (pMsg->data.cGetUicCarData.carSeqNo);
   pMsg->data.cGetUicCarData.topoCnt  = tdcN2H8 (pMsg->data.cGetUicCarData.topoCnt);
}

//-----------------------------------------------------------------------------

static void cgetLabel2UicCarSeqNoN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.cLabel2UicCarSeqNo.carLabel);
   labelN2H (pMsg->data.cLabel2UicCarSeqNo.cstLabel);
   pMsg->data.cLabel2UicCarSeqNo.carLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cLabel2UicCarSeqNo.cstLabel[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.cLabel2UicCarSeqNo.iptTopoCnt                      = tdcN2H8 (pMsg->data.cLabel2UicCarSeqNo.iptTopoCnt);
   pMsg->data.cLabel2UicCarSeqNo.uicTopoCnt                      = tdcN2H8 (pMsg->data.cLabel2UicCarSeqNo.uicTopoCnt);
}

//-----------------------------------------------------------------------------

static void cgetAddr2UicCarSeqNoN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cAddr2UicCarSeqNo.ipAddr     = tdcN2H32 (pMsg->data.cAddr2UicCarSeqNo.ipAddr);
   pMsg->data.cAddr2UicCarSeqNo.iptTopoCnt = tdcN2H8  (pMsg->data.cAddr2UicCarSeqNo.iptTopoCnt);
   pMsg->data.cAddr2UicCarSeqNo.uicTopoCnt = tdcN2H8  (pMsg->data.cAddr2UicCarSeqNo.uicTopoCnt);
}

//-----------------------------------------------------------------------------

static void cgetUicCarSeqNo2IdsN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cUicCarSeqNo2Ids.carSeqNo   = tdcN2H8 (pMsg->data.cUicCarSeqNo2Ids.carSeqNo);
   pMsg->data.cUicCarSeqNo2Ids.iptTopoCnt = tdcN2H8 (pMsg->data.cUicCarSeqNo2Ids.iptTopoCnt);
   pMsg->data.cUicCarSeqNo2Ids.uicTopoCnt = tdcN2H8 (pMsg->data.cUicCarSeqNo2Ids.uicTopoCnt);
}

//-----------------------------------------------------------------------------

static void csetDebugLevelN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.cSetDebugLevel.dbgLevel[IPT_DBG_LEVEL_STRING_LEN - 1] = '\0';
   stringN2H (pMsg->data.cSetDebugLevel.dbgLevel);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void rgetVersionN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetVersion.version = tdcN2H32 (pMsg->data.rGetVersion.version);
}

//-----------------------------------------------------------------------------

static void rgetIptStateN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetIptState.inaugState = tdcN2H8 (pMsg->data.rGetIptState.inaugState);
   pMsg->data.rGetIptState.topoCnt    = tdcN2H8 (pMsg->data.rGetIptState.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetOwnIdsN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.rGetOwnIds.carId);
   labelN2H (pMsg->data.rGetOwnIds.cstId);
   labelN2H (pMsg->data.rGetOwnIds.devId);

   pMsg->data.rGetOwnIds.carId[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rGetOwnIds.cstId[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rGetOwnIds.devId[IPT_MAX_LABEL_LEN - 1] = '\0';
}

//-----------------------------------------------------------------------------

static void rgetAddrByNameN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetAddrByName.ipAddr  = tdcN2H32 (pMsg->data.rGetAddrByName.ipAddr);
   pMsg->data.rGetAddrByName.topoCnt = tdcN2H8  (pMsg->data.rGetAddrByName.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetAddrByNameExtN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetAddrByNameExt.ipAddr  =         tdcN2H32 (pMsg->data.rGetAddrByNameExt.ipAddr);
   pMsg->data.rGetAddrByNameExt.bIsFRG  = (INT32) tdcN2H32 ((UINT32) pMsg->data.rGetAddrByNameExt.bIsFRG);
   pMsg->data.rGetAddrByNameExt.topoCnt =         tdcN2H8  (pMsg->data.rGetAddrByNameExt.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetTrnBackboneTypeN2H  (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetTbType.tbType        = tdcN2H8  (pMsg->data.rGetTbType.tbType);
   pMsg->data.rGetTbType.gatewayIpAddr = tdcN2H32 (pMsg->data.rGetTbType.gatewayIpAddr);
}

//-----------------------------------------------------------------------------

static void rgetUriHostPartN2H (T_TDC_IPC_MSG*     pMsg)
{
   uriN2H (pMsg->data.rGetUriHostPart.uri);
   pMsg->data.rGetUriHostPart.uri[IPT_MAX_URI_LEN - 1] = '\0';
   pMsg->data.rGetUriHostPart.topoCnt                  = tdcN2H8 (pMsg->data.rGetUriHostPart.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetLabel2CarIdN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.rLabel2CarId.carId);
   pMsg->data.rLabel2CarId.carId[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rLabel2CarId.topoCnt                      = tdcN2H8 (pMsg->data.rLabel2CarId.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetAddr2CarIdN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.rAddr2CarId.carId);
   pMsg->data.rAddr2CarId.carId[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rAddr2CarId.topoCnt                      = tdcN2H8 (pMsg->data.rAddr2CarId.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetLabel2CstIdN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.rLabel2CstId.cstId);
   pMsg->data.rLabel2CstId.cstId[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rLabel2CstId.topoCnt                      = tdcN2H8 (pMsg->data.rLabel2CstId.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetAddr2CstIdN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.rAddr2CstId.cstId);
   pMsg->data.rAddr2CstId.cstId[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rAddr2CstId.topoCnt                      = tdcN2H8 (pMsg->data.rAddr2CstId.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetCstNo2CstIdN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.rCstNo2CstId.cstId);
   pMsg->data.rCstNo2CstId.cstId[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rCstNo2CstId.topoCnt                      = tdcN2H8 (pMsg->data.rCstNo2CstId.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetLabel2TrnCstNoN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rLabel2TrnCstNo.trnCstNo = tdcN2H8 (pMsg->data.rLabel2TrnCstNo.trnCstNo);
   pMsg->data.rLabel2TrnCstNo.topoCnt  = tdcN2H8 (pMsg->data.rLabel2TrnCstNo.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetAddr2TrnCstNoN2H       (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rAddr2TrnCstNo.trnCstNo = tdcN2H8 (pMsg->data.rAddr2TrnCstNo.trnCstNo);
   pMsg->data.rAddr2TrnCstNo.topoCnt  = tdcN2H8 (pMsg->data.rAddr2TrnCstNo.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetTrnCstCntN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetTrnCstCnt.trnCstCnt = tdcN2H8 (pMsg->data.rGetTrnCstCnt.trnCstCnt);
   pMsg->data.rGetTrnCstCnt.topoCnt   = tdcN2H8 (pMsg->data.rGetTrnCstCnt.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetCstCarCntN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetCstCarCnt.carCnt  = tdcN2H8 (pMsg->data.rGetCstCarCnt.carCnt);
   pMsg->data.rGetCstCarCnt.topoCnt = tdcN2H8 (pMsg->data.rGetCstCarCnt.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetCarDevCntN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetCarDevCnt.devCnt  = tdcN2H16 (pMsg->data.rGetCarDevCnt.devCnt);
   pMsg->data.rGetCarDevCnt.topoCnt = tdcN2H8  (pMsg->data.rGetCarDevCnt.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetCarInfoN2H (T_TDC_IPC_MSG*     pMsg)
{
   int      i;
   UINT16   devCnt = tdcN2H16 (pMsg->data.rGetCarInfo.devCnt);

   if (devCnt > IPC_MAX_DEV_PER_CAR)
   {
      devCnt = IPC_MAX_DEV_PER_CAR;
   }

   labelN2H (pMsg->data.rGetCarInfo.carId);
   labelN2H (pMsg->data.rGetCarInfo.carType);
   pMsg->data.rGetCarInfo.carId  [IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rGetCarInfo.carType[IPT_MAX_LABEL_LEN - 1] = '\0';

   for (i = 0; i < IPT_UIC_IDENTIFIER_CNT; i++)
   {
      pMsg->data.rGetCarInfo.uicIdent[i] = tdcN2H8 (pMsg->data.rGetCarInfo.uicIdent[i]);
   }

   pMsg->data.rGetCarInfo.cstCarNo  = tdcN2H8  (pMsg->data.rGetCarInfo.cstCarNo);
   pMsg->data.rGetCarInfo.trnOrient = tdcN2H8  (pMsg->data.rGetCarInfo.trnOrient);
   pMsg->data.rGetCarInfo.cstOrient = tdcN2H8  (pMsg->data.rGetCarInfo.cstOrient);
   pMsg->data.rGetCarInfo.devCnt    = devCnt;
   pMsg->data.rGetCarInfo.topoCnt   = tdcN2H8  (pMsg->data.rGetCarInfo.topoCnt);

   for (i = 0; i < devCnt; i++)
   {
      labelN2H (pMsg->data.rGetCarInfo.devData[i].devId);
      pMsg->data.rGetCarInfo.devData[i].devId[IPT_MAX_LABEL_LEN - 1] = '\0';
      pMsg->data.rGetCarInfo.devData[i].hostId                       = tdcN2H16 (pMsg->data.rGetCarInfo.devData[i].hostId);
   }
}

//-----------------------------------------------------------------------------

static void rgetUicStateN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rGetUicState.inaugState = tdcN2H8 (pMsg->data.rGetUicState.inaugState);
   pMsg->data.rGetUicState.topoCnt    = tdcN2H8 (pMsg->data.rGetUicState.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetUicGlobDataN2H (T_TDC_IPC_MSG*     pMsg)
{
   UINT32      i;

   for (i = 0; i < IPT_UIC_CONF_POS_CNT; i++)
   {
      pMsg->data.rGetUicGlobData.confPos[i] = tdcN2H8 (pMsg->data.rGetUicGlobData.confPos[i]);
   }

   pMsg->data.rGetUicGlobData.confPosAvail   = tdcN2H8 (pMsg->data.rGetUicGlobData.confPosAvail);
   pMsg->data.rGetUicGlobData.operatAvail    = tdcN2H8 (pMsg->data.rGetUicGlobData.operatAvail);
   pMsg->data.rGetUicGlobData.natApplAvail   = tdcN2H8 (pMsg->data.rGetUicGlobData.natApplAvail);
   pMsg->data.rGetUicGlobData.cstPropAvail   = tdcN2H8 (pMsg->data.rGetUicGlobData.cstPropAvail);
   pMsg->data.rGetUicGlobData.carPropAvail   = tdcN2H8 (pMsg->data.rGetUicGlobData.carPropAvail);
   pMsg->data.rGetUicGlobData.seatResNoAvail = tdcN2H8 (pMsg->data.rGetUicGlobData.seatResNoAvail);
   pMsg->data.rGetUicGlobData.inaugFrameVer  = tdcN2H8 (pMsg->data.rGetUicGlobData.inaugFrameVer);
   pMsg->data.rGetUicGlobData.rDataVer       = tdcN2H8 (pMsg->data.rGetUicGlobData.rDataVer);
   pMsg->data.rGetUicGlobData.inaugState     = tdcN2H8 (pMsg->data.rGetUicGlobData.inaugState);
   pMsg->data.rGetUicGlobData.topoCnt        = tdcN2H8 (pMsg->data.rGetUicGlobData.topoCnt);
   pMsg->data.rGetUicGlobData.orient         = tdcN2H8 (pMsg->data.rGetUicGlobData.orient);
   pMsg->data.rGetUicGlobData.notAllConf     = tdcN2H8 (pMsg->data.rGetUicGlobData.notAllConf);
   pMsg->data.rGetUicGlobData.confCancelled  = tdcN2H8 (pMsg->data.rGetUicGlobData.confCancelled);
   pMsg->data.rGetUicGlobData.trnCarCnt      = tdcN2H8 (pMsg->data.rGetUicGlobData.trnCarCnt);
   pMsg->data.rGetUicGlobData.topoCnt        = tdcN2H8 (pMsg->data.rGetUicGlobData.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetUicCarDataN2H (T_TDC_IPC_MSG*     pMsg)
{
   UINT32      i;

   for (i = 0; i < IPT_MAX_UIC_CST_NO; i++)
   {
      pMsg->data.rGetUicCarData.cstProp[i] = tdcN2H8 (pMsg->data.rGetUicCarData.cstProp[i]);
   }
   
   for (i = 0; i < IPT_UIC_CAR_PROPERTY_CNT; i++)
   {
      pMsg->data.rGetUicCarData.carProp[i] = tdcN2H8 (pMsg->data.rGetUicCarData.carProp[i]);
   }
   
   for (i = 0; i < IPT_UIC_IDENTIFIER_CNT; i++)
   {
      pMsg->data.rGetUicCarData.uicIdent[i] = tdcN2H8 (pMsg->data.rGetUicCarData.uicIdent[i]);
   }

   pMsg->data.rGetUicCarData.trnCstNo      =        tdcN2H8  (pMsg->data.rGetUicCarData.trnCstNo);       
   pMsg->data.rGetUicCarData.seatResNo     =        tdcN2H16 (pMsg->data.rGetUicCarData.seatResNo);      
   pMsg->data.rGetUicCarData.carSeqNo      =        tdcN2H8  (pMsg->data.rGetUicCarData.carSeqNo);       
   pMsg->data.rGetUicCarData.contrCarCnt   = (INT8) tdcN2H8  ((UINT8) pMsg->data.rGetUicCarData.contrCarCnt);    
   pMsg->data.rGetUicCarData.operat        =        tdcN2H8  (pMsg->data.rGetUicCarData.operat);         
   pMsg->data.rGetUicCarData.owner         =        tdcN2H8  (pMsg->data.rGetUicCarData.owner);          
   pMsg->data.rGetUicCarData.natAppl       =        tdcN2H8  (pMsg->data.rGetUicCarData.natAppl);        
   pMsg->data.rGetUicCarData.natVer        =        tdcN2H8  (pMsg->data.rGetUicCarData.natVer);         
   pMsg->data.rGetUicCarData.trnOrient     =        tdcN2H8  (pMsg->data.rGetUicCarData.trnOrient);      
   pMsg->data.rGetUicCarData.cstOrient     =        tdcN2H8  (pMsg->data.rGetUicCarData.cstOrient);      
   pMsg->data.rGetUicCarData.isLeading     =        tdcN2H8  (pMsg->data.rGetUicCarData.isLeading);      
   pMsg->data.rGetUicCarData.isLeadRequ    =        tdcN2H8  (pMsg->data.rGetUicCarData.isLeadRequ);     
   pMsg->data.rGetUicCarData.trnSwInCarCnt =        tdcN2H8  (pMsg->data.rGetUicCarData.trnSwInCarCnt);  
   pMsg->data.rGetUicCarData.topoCnt       =        tdcN2H8  (pMsg->data.rGetUicCarData.topoCnt);
}

//-----------------------------------------------------------------------------

static void rgetLabel2UicCarSeqNoN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rLabel2UicCarSeqNo.carSeqNo   = tdcN2H8 (pMsg->data.rLabel2UicCarSeqNo.carSeqNo);
   pMsg->data.rLabel2UicCarSeqNo.iptTopoCnt = tdcN2H8 (pMsg->data.rLabel2UicCarSeqNo.iptTopoCnt);
   pMsg->data.rLabel2UicCarSeqNo.uicTopoCnt = tdcN2H8 (pMsg->data.rLabel2UicCarSeqNo.uicTopoCnt);
}

//-----------------------------------------------------------------------------

static void rgetAddr2UicCarSeqNoN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg->data.rAddr2UicCarSeqNo.carSeqNo   = tdcN2H8 (pMsg->data.rAddr2UicCarSeqNo.carSeqNo);
   pMsg->data.rAddr2UicCarSeqNo.iptTopoCnt = tdcN2H8 (pMsg->data.rAddr2UicCarSeqNo.iptTopoCnt);
   pMsg->data.rAddr2UicCarSeqNo.uicTopoCnt = tdcN2H8 (pMsg->data.rAddr2UicCarSeqNo.uicTopoCnt);
}

//-----------------------------------------------------------------------------

static void rgetUicCarSeqNo2IdsN2H (T_TDC_IPC_MSG*     pMsg)
{
   labelN2H (pMsg->data.rUicCarSeqNo2Ids.carId);
   labelN2H (pMsg->data.rUicCarSeqNo2Ids.cstId);

   pMsg->data.rUicCarSeqNo2Ids.carId[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rUicCarSeqNo2Ids.cstId[IPT_MAX_LABEL_LEN - 1] = '\0';
   pMsg->data.rUicCarSeqNo2Ids.iptTopoCnt                   = tdcN2H8 (pMsg->data.rUicCarSeqNo2Ids.iptTopoCnt);
   pMsg->data.rUicCarSeqNo2Ids.uicTopoCnt                   = tdcN2H8 (pMsg->data.rUicCarSeqNo2Ids.uicTopoCnt);
}

//-----------------------------------------------------------------------------

static void rsetDebugLevelN2H (T_TDC_IPC_MSG*     pMsg)
{
   pMsg = pMsg;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 2006/08/14, MRi - Taking care of Datarepresentation in order to allow
//                   not only to be used for Inter-Process Communication
//                   but also for Inter Device Communication (e.g. Debugging)

T_TDC_BOOL tdcSendIpcMsg (const char*        pModname,
                          T_TDC_SOCKET       socketFd,
                          T_TDC_IPC_MSG*     pHostMsg)
{
   UINT32         msgLen = TDC_IPC_HEAD_SIZE + pHostMsg->head.msgLen;

   if (msgLen <= TDC_IPC_MAX_MSG_SIZE)
   {
      UINT32           sendNo;

      msgH2N (pHostMsg);

      if (tdcTcpSendN (pModname, socketFd, pHostMsg, &sendNo, msgLen))
      {
         return (TRUE);
      }

      DEBUG_WARN2 (pModname, "Could not write all data to stream (%d/%d)", sendNo, msgLen);
   }
   else
   {
      DEBUG_WARN1 (pModname, "tdcSendIpcMsg - message too large (%d)", msgLen);
   }

   return (FALSE);
}

// ----------------------------------------------------------------------------

T_TDC_BOOL tdcReceiveIpcMsg (const char*        pModname,
                             T_TDC_SOCKET       socketFd,
                             T_TDC_IPC_MSG*     pHostMsg,
                             UINT32             maxLen)
{
   T_TDC_BOOL       bOK     = FALSE;
   UINT32           recvNo;

   if (maxLen >= TDC_IPC_HEAD_SIZE)
   {
      // Read and Check Header Information firstly

      if (!tdcTcpRecvN (pModname, socketFd, pHostMsg, &recvNo, TDC_IPC_HEAD_SIZE))
      {
         DEBUG_WARN2 (pModname, "Error reading ipc-msg-head from socket(%d), noBytes(%d)", socketFd, recvNo);
      }
      else
      {
         pHostMsg->head.magicNo   =                tdcN2H32 (pHostMsg->head.magicNo);
         pHostMsg->head.msgType   =                tdcN2H32 (pHostMsg->head.msgType);
         pHostMsg->head.tdcResult = (T_TDC_RESULT) tdcN2H32 ((UINT32) pHostMsg->head.tdcResult);
         pHostMsg->head.msgLen    =                tdcN2H32 (pHostMsg->head.msgLen);

         if (pHostMsg->head.magicNo != TDC_IPC_MAGIC_NO)
         {
            DEBUG_WARN1 (pModname, "Received strange ipc-msg - MAGIC_NO (x%x)", pHostMsg->head.magicNo);
         }
         else
         {
            if (pHostMsg->head.msgLen > maxLen - TDC_IPC_HEAD_SIZE)
            {
               DEBUG_WARN1 (pModname, "Received too large ipc-msg - msgLen (%d)", pHostMsg->head.msgLen);
            }
            else
            {
               bOK = tdcTcpRecvN (pModname, socketFd, &pHostMsg->data, &recvNo, (UINT16) pHostMsg->head.msgLen);

               if (bOK)
               {
                  msgN2H (pHostMsg);
               }
               else
               {
                  DEBUG_WARN2 (pModname, "Error reading from socket (%d / %d)", pHostMsg->head.msgLen, recvNo);
               }
            }
         }
      }
   }

   return (bOK);
}





