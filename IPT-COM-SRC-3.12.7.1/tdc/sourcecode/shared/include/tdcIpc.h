/*                                                                            */
/*  $Id: tdcIpc.h 11665 2010-08-27 12:59:40Z gweiss $                      */
/*                                                                            */
/*  DESCRIPTION    Definitions, for the Interprocess Communication of TDC     */
/*                                                                            */
/*  AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                            */
/*  REMARKS                                                                   */
/*                                                                            */
/*  DEPENDENCIES                                                              */
/*                                                                            */
/*  MODIFICATIONS                           								  */
/*   																		  */
/*  CR-590  (Gerhard Weiss, 2010-08-26)                                       */
/*          Removed a problem if grpAll.aCar.lCst is resolved.                */
/*          Current topocnt wasn't retrieved, so resolving this URI resulted  */
/*          in TDC_WRONG_TOPOCOUNT if the requested topocount isn't 0         */
/*                                                                            */
/*  All rights reserved. Reproduction, modification, use or disclosure        */
/*  to third parties without express authority is forbidden.                  */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002-2010.             */
/*                                                                            */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#if !defined (_TDC_IPC_H)
   #define _TDC_IPC_H

// ----------------------------------------------------------------------------

#ifdef __cplusplus                            // to be compatible with C++
   extern "C" {
#endif

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#define IPT_DBG_LEVEL_STRING_LEN             20

// -----------------------------------------------------------------------------

typedef char            T_IPC_DBG_LEVEL_STR    [ALIGN_4 (IPT_DBG_LEVEL_STRING_LEN)];
typedef char            T_IPC_IPT_LABEL        [ALIGN_4 (IPT_MAX_LABEL_LEN)];
typedef char            T_IPC_IPT_URI          [ALIGN_4 (IPT_MAX_URI_LEN)];
typedef UINT8           T_IPC_UIC_IDENT        [ALIGN_4 (IPT_UIC_IDENTIFIER_CNT)];
typedef UINT8           T_IPC_UIC_CST_PROP     [ALIGN_4 (IPT_MAX_UIC_CST_NO)];
typedef UINT8           T_IPC_UIC_CAR_PROP     [ALIGN_4 (IPT_UIC_CAR_PROPERTY_CNT)];
typedef UINT8           T_IPC_UIC_CONF_POS_CNT [ALIGN_4 (IPT_UIC_CONF_POS_CNT)];


// -----------------------------------------------------------------------------

#if defined (WIN32)
#pragma pack(push, 1)
#endif
    
#define IPC_CALL_GET_VERSION_SIZE                     0

typedef struct
{
   UINT32               version;
} GNU_PACKED T_IPC_REPLY_GET_VERSION;
#define IPC_REPLY_GET_VERSION_SIZE                    ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_VERSION))

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPC_DBG_LEVEL_STR           dbgLevel;
} GNU_PACKED T_IPC_CALL_SET_DEBUG_LEVEL;
#define IPC_CALL_SET_DEBUG_LEVEL_SIZE                 ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_SET_DEBUG_LEVEL))

#define IPC_REPLY_SET_DEBUG_LEVEL_SIZE                0

// -----------------------------------------------------------------------------

#define IPC_CALL_GET_TBTYPE_SIZE                      0

typedef struct
{
   T_IPT_IP_ADDR        gatewayIpAddr;
   UINT8 	            tbType;
   UINT8 	            padding1;
   UINT8 	            padding2;
   UINT8 	            padding3;
} GNU_PACKED T_IPC_REPLY_GET_TBTYPE;
#define IPC_REPLY_GET_TBTYPE_SIZE                     ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_TBTYPE))

// -----------------------------------------------------------------------------

#define IPC_CALL_GET_IPT_STATE_SIZE                   0

typedef struct
{
   UINT8                inaugState;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_REPLY_GET_IPT_STATE;
#define IPC_REPLY_GET_IPT_STATE_SIZE                  ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_IPT_STATE))

// -----------------------------------------------------------------------------

#define IPC_CALL_GET_OWN_IDS_SIZE                     0

typedef struct
{
   T_IPC_IPT_LABEL      devId;
   T_IPC_IPT_LABEL      carId;
   T_IPC_IPT_LABEL      cstId;
} GNU_PACKED T_IPC_REPLY_GET_OWN_IDS;
#define IPC_REPLY_GET_OWN_IDS_SIZE                    ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_OWN_IDS))

// ----------------------------------------------------------------------------

typedef struct
{
   T_IPC_IPT_URI        uri;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_CALL_GET_ADDR_BY_NAME;
#define IPC_CALL_GET_ADDR_BY_NAME_SIZE                ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_GET_ADDR_BY_NAME))

typedef struct
{
   T_IPT_IP_ADDR        ipAddr;
   UINT8                topoCnt;
   UINT8 	            padding1;
   UINT8 	            padding2;
   UINT8 	            padding3;
} GNU_PACKED T_IPC_REPLY_GET_ADDR_BY_NAME;
#define IPC_REPLY_GET_ADDR_BY_NAME_SIZE               ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_ADDR_BY_NAME))

// ----------------------------------------------------------------------------

typedef struct
{
   T_IPC_IPT_URI        uri;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_CALL_GET_ADDR_BY_NAME_EXT;
#define IPC_CALL_GET_ADDR_BY_NAME_EXT_SIZE            ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_GET_ADDR_BY_NAME_EXT))

typedef struct
{
   T_IPT_IP_ADDR        ipAddr;
   INT32                bIsFRG;
   UINT8                topoCnt;
   UINT8 	            padding1;
   UINT8 	            padding2;
   UINT8 	            padding3;
} GNU_PACKED T_IPC_REPLY_GET_ADDR_BY_NAME_EXT;
#define IPC_REPLY_GET_ADDR_BY_NAME_EXT_SIZE           ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_ADDR_BY_NAME_EXT))

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPT_IP_ADDR        ipAddr;
   UINT8                topoCnt;
   UINT8 	            padding1;
   UINT8 	            padding2;
   UINT8 	            padding3;
} GNU_PACKED T_IPC_CALL_GET_URI_HOST_PART;
#define IPC_CALL_GET_URI_HOST_PART_SIZE               ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_GET_URI_HOST_PART))

typedef struct
{
   T_IPC_IPT_URI        uri;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_REPLY_GET_URI_HOST_PART;
#define IPC_REPLY_GET_URI_HOST_PART_SIZE              ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_URI_HOST_PART))

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPC_IPT_LABEL      cstLabel;
   T_IPC_IPT_LABEL      carLabel;                     
   UINT8                topoCnt;
} GNU_PACKED T_IPC_CALL_LABEL_2_CAR_ID;
#define IPC_CALL_LABEL_2_CAR_ID_SIZE                  ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_LABEL_2_CAR_ID))

typedef struct
{
   T_IPC_IPT_LABEL      carId;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_REPLY_LABEL_2_CAR_ID;
#define IPC_REPLY_LABEL_2_CAR_ID_SIZE                 ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_LABEL_2_CAR_ID))

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPT_IP_ADDR        ipAddr;
   UINT8                topoCnt;
   UINT8                padding1;
   UINT8                padding2;
   UINT8                padding3;
} GNU_PACKED T_IPC_CALL_ADDR_2_CAR_ID;
#define IPC_CALL_ADDR_2_CAR_ID_SIZE                   ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_ADDR_2_CAR_ID))

typedef struct
{
   T_IPC_IPT_LABEL      carId;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_REPLY_ADDR_2_CAR_ID;
#define IPC_REPLY_ADDR_2_CAR_ID_SIZE                  ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_ADDR_2_CAR_ID))

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPC_IPT_LABEL      carLabel;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_CALL_LABEL_2_CST_ID;
#define IPC_CALL_LABEL_2_CST_ID_SIZE                  ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_LABEL_2_CST_ID))

typedef struct
{
   T_IPC_IPT_LABEL      cstId;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_REPLY_LABEL_2_CST_ID;
#define IPC_REPLY_LABEL_2_CST_ID_SIZE                 ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_LABEL_2_CST_ID))

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPT_IP_ADDR        ipAddr;
   UINT8                topoCnt;
   UINT8                padding1;
   UINT8                padding2;
   UINT8                padding3;
} GNU_PACKED T_IPC_CALL_ADDR_2_CST_ID;
#define IPC_CALL_ADDR_2_CST_ID_SIZE                   ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_ADDR_2_CST_ID))

typedef struct
{
   T_IPC_IPT_LABEL      cstId;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_REPLY_ADDR_2_CST_ID;
#define IPC_REPLY_ADDR_2_CST_ID_SIZE                  ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_ADDR_2_CST_ID))

// -----------------------------------------------------------------------------

typedef struct
{
   UINT8                trnCstNo;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_CALL_CSTNO_2_CST_ID;
#define IPC_CALL_CSTNO_2_CST_ID_SIZE                  ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_CSTNO_2_CST_ID))

typedef struct
{
   T_IPC_IPT_LABEL      cstId;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_REPLY_CSTNO_2_CST_ID;
#define IPC_REPLY_CSTNO_2_CST_ID_SIZE                 ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_CSTNO_2_CST_ID))

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPC_IPT_LABEL      carLabel;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_CALL_LABEL_2_TRN_CST_NO;
#define IPC_CALL_LABEL_2_TRN_CST_NO_SIZE              ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_LABEL_2_TRN_CST_NO))

typedef struct
{
   UINT8                trnCstNo;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_REPLY_LABEL_2_TRN_CST_NO;
#define IPC_REPLY_LABEL_2_TRN_CST_NO_SIZE             ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_LABEL_2_TRN_CST_NO))
                                                  
// -----------------------------------------------------------------------------

typedef struct
{
   T_IPT_IP_ADDR        ipAddr;
   UINT8                topoCnt;
   UINT8                padding1;
   UINT8                padding2;
   UINT8                padding3;
} GNU_PACKED T_IPC_CALL_ADDR_2_TRN_CST_NO;
#define IPC_CALL_ADDR_2_TRN_CST_NO_SIZE               ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_ADDR_2_TRN_CST_NO))

typedef struct
{
   UINT8                trnCstNo;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_REPLY_ADDR_2_TRN_CST_NO;
#define IPC_REPLY_ADDR_2_TRN_CST_NO_SIZE              ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_ADDR_2_TRN_CST_NO))

// -----------------------------------------------------------------------------

typedef struct
{
   UINT8                topoCnt;
} GNU_PACKED T_IPC_CALL_GET_TRN_CST_CNT;
#define IPC_CALL_GET_TRN_CST_CNT_SIZE                 ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_GET_TRN_CST_CNT))

typedef struct
{
   UINT8                trnCstCnt;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_REPLY_GET_TRN_CST_CNT;
#define IPC_REPLY_GET_TRN_CST_CNT_SIZE                ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_TRN_CST_CNT))

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPC_IPT_LABEL      cstLabel;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_CALL_GET_CST_CAR_CNT;
#define IPC_CALL_GET_CST_CAR_CNT_SIZE                 ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_GET_CST_CAR_CNT))

typedef struct
{
   UINT8                carCnt;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_REPLY_GET_CST_CAR_CNT;
#define IPC_REPLY_GET_CST_CAR_CNT_SIZE                ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_CST_CAR_CNT))

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPC_IPT_LABEL      cstLabel;
   T_IPC_IPT_LABEL      carLabel;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_CALL_GET_CAR_DEV_CNT;
#define IPC_CALL_GET_CAR_DEV_CNT_SIZE                 ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_GET_CAR_DEV_CNT))

typedef struct
{
   UINT16               devCnt;
   UINT8                topoCnt;
   UINT8                padding1;
} GNU_PACKED T_IPC_REPLY_GET_CAR_DEV_CNT;
#define IPC_REPLY_GET_CAR_DEV_CNT_SIZE                (T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_CAR_DEV_CNT)

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPC_IPT_LABEL      cstLabel;
   T_IPC_IPT_LABEL      carLabel;
   UINT16               maxDev;
   UINT8                topoCnt;
   UINT8                padding1;
} GNU_PACKED T_IPC_CALL_GET_CAR_INFO;
#define IPC_CALL_GET_CAR_INFO_SIZE                    ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_GET_CAR_INFO))

typedef struct
{
   T_IPC_IPT_LABEL      devId;
   UINT16               hostId;
} GNU_PACKED T_IPC_DEV_DATA;

#define IPC_MAX_DEV_PER_CAR            100            /* restricted, in order to be able to handle */
                                                      /* IPC-Calls/Replies without dynamic memory  */
                                                      /* 100 should be enough for virtual all systems */
typedef struct
{
   T_IPC_IPT_LABEL      carId;
   T_IPC_IPT_LABEL      carType; 
   T_IPC_UIC_IDENT      uicIdent;
   UINT16               devCnt;   
   UINT16               padding;   
   UINT8                cstCarNo;
   UINT8                trnOrient;
   UINT8                cstOrient;
   UINT8                topoCnt;
   T_IPC_DEV_DATA       devData[IPC_MAX_DEV_PER_CAR];
} GNU_PACKED T_IPC_REPLY_GET_CAR_INFO;                           
#define IPC_DEV_DATA_SIZE(cnt)                        ((UINT32) (cnt * sizeof (T_IPC_DEV_DATA)))
#define IPC_REPLY_GET_CAR_INFO_DEV_OFFSET()           ARRAY_OFFSET_IN_STRUC (T_IPC_REPLY_GET_CAR_INFO, devData)
#define IPC_REPLY_GET_CAR_INFO_SIZE(cnt)              (IPC_REPLY_GET_CAR_INFO_DEV_OFFSET () + IPC_DEV_DATA_SIZE (cnt))

// -----------------------------------------------------------------------------

#define IPC_CALL_GET_UIC_STATE_SIZE                   ((T_TDC_IPC_MSGLEN) 0)

typedef struct
{
   UINT8                inaugState;
   UINT8                topoCnt;
} GNU_PACKED T_IPC_REPLY_GET_UIC_STATE;
#define IPC_REPLY_GET_UIC_STATE_SIZE                  ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_UIC_STATE))

// -----------------------------------------------------------------------------

typedef struct
{
   UINT8                topoCnt;
} GNU_PACKED T_IPC_CALL_GET_UIC_GLOB_DATA;
#define IPC_CALL_GET_UIC_GLOB_DATA_SIZE                ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_GET_UIC_GLOB_DATA))

typedef struct
{
   T_IPC_UIC_CONF_POS_CNT     confPos;
   UINT8                      confPosAvail;     
   UINT8                      operatAvail;      
   UINT8                      natApplAvail;     
   UINT8                      cstPropAvail;     
   UINT8                      carPropAvail;     
   UINT8                      seatResNoAvail;   
   UINT8                      inaugFrameVer;    
   UINT8                      rDataVer;         
   UINT8                      inaugState;       
   UINT8                      topoCnt;          
   UINT8                      orient;           
   UINT8                      notAllConf;       
   UINT8                      confCancelled;    
   UINT8                      trnCarCnt;        
} GNU_PACKED T_IPC_REPLY_GET_UIC_GLOB_DATA;
#define IPC_REPLY_GET_UIC_GLOB_DATA_SIZE              ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_UIC_GLOB_DATA))

// -----------------------------------------------------------------------------

typedef struct
{
   UINT8             carSeqNo;
   UINT8             topoCnt;
} GNU_PACKED T_IPC_CALL_GET_UIC_CAR_DATA;
#define IPC_CALL_GET_UIC_CAR_DATA_SIZE                ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_GET_UIC_CAR_DATA))


typedef struct
{
   T_IPC_UIC_CST_PROP      cstProp;    
   T_IPC_UIC_CAR_PROP      carProp;     
   T_IPC_UIC_IDENT         uicIdent;
   UINT16                  seatResNo;      
   UINT8                   trnCstNo;       
   UINT8                   carSeqNo;
   INT8                    contrCarCnt;    
   UINT8                   operat;         
   UINT8                   owner;          
   UINT8                   natAppl;        
   UINT8                   natVer;         
   UINT8                   trnOrient;      
   UINT8                   cstOrient;      
   UINT8                   isLeading;      
   UINT8                   isLeadRequ;     
   UINT8                   trnSwInCarCnt;  
   UINT8                   topoCnt;
} GNU_PACKED T_IPC_REPLY_GET_UIC_CAR_DATA;
#define IPC_REPLY_GET_UIC_CAR_DATA_SIZE               ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_GET_UIC_CAR_DATA))

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPC_IPT_LABEL         cstLabel;
   T_IPC_IPT_LABEL         carLabel;
   UINT8                   iptTopoCnt;
   UINT8                   uicTopoCnt;
} GNU_PACKED T_IPC_CALL_LABEL_2_UIC_CAR_SEQ_NO;
#define IPC_CALL_LABEL_2_UIC_CAR_SEQ_NO_SIZE          ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_LABEL_2_UIC_CAR_SEQ_NO))

typedef struct
{
   UINT8                   carSeqNo;
   UINT8                   iptTopoCnt;
   UINT8                   uicTopoCnt;
} GNU_PACKED T_IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO;
#define IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO_SIZE         ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO))

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPT_IP_ADDR           ipAddr;
   UINT8                   iptTopoCnt;
   UINT8                   uicTopoCnt;
   UINT8                   padding1;
   UINT8                   padding2;
} GNU_PACKED T_IPC_CALL_ADDR_2_UIC_CAR_SEQ_NO;
#define IPC_CALL_ADDR_2_UIC_CAR_SEQ_NO_SIZE           ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_ADDR_2_UIC_CAR_SEQ_NO))

typedef struct
{
   UINT8                   carSeqNo;
   UINT8                   iptTopoCnt;
   UINT8                   uicTopoCnt;
} GNU_PACKED T_IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO;
#define IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO_SIZE          ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO))

// -----------------------------------------------------------------------------

typedef struct
{
   UINT8                   carSeqNo;
   UINT8                   iptTopoCnt;
   UINT8                   uicTopoCnt;
} GNU_PACKED T_IPC_CALL_UIC_CAR_SEQ_NO_2_IDS;
#define IPC_CALL_UIC_CAR_SEQ_NO_2_IDS_SIZE              ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_CALL_UIC_CAR_SEQ_NO_2_IDS))

typedef struct
{
   T_IPC_IPT_LABEL         cstId;
   T_IPC_IPT_LABEL         carId;
   UINT8                   iptTopoCnt;
   UINT8                   uicTopoCnt;
} GNU_PACKED T_IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS;
#define IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS_SIZE             ((T_TDC_IPC_MSGLEN) sizeof (T_IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS))

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef union
{
   T_IPC_REPLY_GET_VERSION             rGetVersion;

   T_IPC_REPLY_GET_TBTYPE              rGetTbType;
   T_IPC_REPLY_GET_IPT_STATE           rGetIptState;

   T_IPC_REPLY_GET_OWN_IDS             rGetOwnIds;

   T_IPC_CALL_GET_ADDR_BY_NAME         cGetAddrByName;
   T_IPC_REPLY_GET_ADDR_BY_NAME        rGetAddrByName;

   T_IPC_CALL_GET_URI_HOST_PART        cGetUriHostPart;
   T_IPC_REPLY_GET_URI_HOST_PART       rGetUriHostPart;

   T_IPC_CALL_LABEL_2_CAR_ID           cLabel2CarId;
   T_IPC_REPLY_LABEL_2_CAR_ID          rLabel2CarId;

   T_IPC_CALL_ADDR_2_CAR_ID            cAddr2CarId;
   T_IPC_REPLY_ADDR_2_CAR_ID           rAddr2CarId;

   T_IPC_CALL_LABEL_2_CST_ID           cLabel2CstId;
   T_IPC_REPLY_LABEL_2_CST_ID          rLabel2CstId;

   T_IPC_CALL_ADDR_2_CST_ID            cAddr2CstId;
   T_IPC_REPLY_ADDR_2_CST_ID           rAddr2CstId;

   T_IPC_CALL_CSTNO_2_CST_ID           cCstNo2CstId;
   T_IPC_REPLY_CSTNO_2_CST_ID          rCstNo2CstId;

   T_IPC_CALL_LABEL_2_TRN_CST_NO       cLabel2TrnCstNo;
   T_IPC_REPLY_LABEL_2_TRN_CST_NO      rLabel2TrnCstNo;

   T_IPC_CALL_ADDR_2_TRN_CST_NO        cAddr2TrnCstNo;
   T_IPC_REPLY_ADDR_2_TRN_CST_NO       rAddr2TrnCstNo;

   T_IPC_CALL_GET_TRN_CST_CNT          cGetTrnCstCnt;
   T_IPC_REPLY_GET_TRN_CST_CNT         rGetTrnCstCnt;

   T_IPC_CALL_GET_CST_CAR_CNT          cGetCstCarCnt;
   T_IPC_REPLY_GET_CST_CAR_CNT         rGetCstCarCnt;

   T_IPC_CALL_GET_CAR_DEV_CNT          cGetCarDevCnt;
   T_IPC_REPLY_GET_CAR_DEV_CNT         rGetCarDevCnt;

   T_IPC_CALL_GET_CAR_INFO             cGetCarInfo;
   T_IPC_REPLY_GET_CAR_INFO            rGetCarInfo;

   T_IPC_REPLY_GET_UIC_STATE           rGetUicState;

   T_IPC_CALL_GET_UIC_GLOB_DATA        cGetUicGlobData;
   T_IPC_REPLY_GET_UIC_GLOB_DATA       rGetUicGlobData;

   T_IPC_CALL_GET_UIC_CAR_DATA         cGetUicCarData;
   T_IPC_REPLY_GET_UIC_CAR_DATA        rGetUicCarData;

   T_IPC_CALL_LABEL_2_UIC_CAR_SEQ_NO   cLabel2UicCarSeqNo;
   T_IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO  rLabel2UicCarSeqNo;

   T_IPC_CALL_ADDR_2_UIC_CAR_SEQ_NO    cAddr2UicCarSeqNo;
   T_IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO   rAddr2UicCarSeqNo;

   T_IPC_CALL_UIC_CAR_SEQ_NO_2_IDS     cUicCarSeqNo2Ids;
   T_IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS    rUicCarSeqNo2Ids;

   T_IPC_CALL_SET_DEBUG_LEVEL          cSetDebugLevel;

   T_IPC_CALL_GET_ADDR_BY_NAME_EXT     cGetAddrByNameExt;
   T_IPC_REPLY_GET_ADDR_BY_NAME_EXT    rGetAddrByNameExt;

} T_TDC_IPC_DATA;                          

// -----------------------------------------------------------------------------

typedef UINT32                         T_TDC_IPC_MAGICNO;
typedef UINT32                         T_TDC_IPC_MSGTYPE;
typedef UINT32                         T_TDC_IPC_MSGLEN;


typedef struct
{
   T_TDC_IPC_MAGICNO    magicNo; 
   T_TDC_IPC_MSGTYPE    msgType;
   T_TDC_RESULT         tdcResult;           // Used in replies only!
   T_TDC_IPC_MSGLEN     msgLen;              // sizeof actual T_TDC_IPC_DATA
} T_TDC_IPC_HEADER;

// -----------------------------------------------------------------------------

typedef struct
{
   T_TDC_IPC_HEADER     head;
   T_TDC_IPC_DATA       data;
} T_TDC_IPC_MSG;

#if defined (WIN32)
#pragma pack(pop)
#endif
       
// -----------------------------------------------------------------------------

#define TDC_IPC_MAGIC_NO                        0x39785613
#define TDC_IPC_HEAD_SIZE                       (UINT32) (sizeof (T_TDC_IPC_HEADER))
#define TDC_IPC_MAX_MSG_SIZE                    60000

// --------------------   TDC IPC Request Types (Ver 1)   --------------------

#define TDC_IPC_CALL_GET_IPT_STATE              ((UINT32) 0)
#define TDC_IPC_CALL_GET_OWN_IDS                ((UINT32) 1)
#define TDC_IPC_CALL_GET_ADDR_BY_NAME           ((UINT32) 2)
#define TDC_IPC_CALL_GET_URI_HOST_PART          ((UINT32) 3)
#define TDC_IPC_CALL_LABEL_2_CAR_ID             ((UINT32) 4)
#define TDC_IPC_CALL_ADDR_2_CAR_ID              ((UINT32) 5)
#define TDC_IPC_CALL_LABEL_2_CST_ID             ((UINT32) 6)
#define TDC_IPC_CALL_ADDR_2_CST_ID              ((UINT32) 7)
#define TDC_IPC_CALL_LABEL_2_TRN_CST_NO         ((UINT32) 8)
#define TDC_IPC_CALL_ADDR_2_TRN_CST_NO          ((UINT32) 9)
#define TDC_IPC_CALL_GET_TRN_CST_CNT            ((UINT32) 10)
#define TDC_IPC_CALL_GET_CST_CAR_CNT            ((UINT32) 11)
#define TDC_IPC_CALL_GET_CAR_DEV_CNT            ((UINT32) 12)
#define TDC_IPC_CALL_GET_CAR_INFO               ((UINT32) 13)
#define TDC_IPC_CALL_GET_UIC_STATE              ((UINT32) 14)
#define TDC_IPC_CALL_GET_UIC_GLOB_DATA          ((UINT32) 15)
#define TDC_IPC_CALL_GET_UIC_CAR_DATA           ((UINT32) 16)
#define TDC_IPC_CALL_LABEL_2_UIC_CAR_SEQ_NO     ((UINT32) 17)
#define TDC_IPC_CALL_ADDR_2_UIC_CAR_SEQ_NO      ((UINT32) 18)
#define TDC_IPC_CALL_UIC_CAR_SEQ_NO_2_IDS       ((UINT32) 19)
#define TDC_IPC_CALL_SET_DEBUG_LEVEL            ((UINT32) 20)
#define TDC_IPC_CALL_GET_ADDR_BY_NAME_EXT       ((UINT32) 21)
#define TDC_IPC_CALL_CSTNO_2_CST_ID             ((UINT32) 22)
#define TDC_IPC_CALL_GET_VERSION                ((UINT32) 23)
#define TDC_IPC_CALL_GET_TBTYPE                 ((UINT32) 24)

#define TDC_IPC_REPLY_GET_IPT_STATE             ((UINT32) 100000)
#define TDC_IPC_REPLY_GET_OWN_IDS               ((UINT32) 100001)
#define TDC_IPC_REPLY_GET_ADDR_BY_NAME          ((UINT32) 100002)
#define TDC_IPC_REPLY_GET_URI_HOST_PART         ((UINT32) 100003)
#define TDC_IPC_REPLY_LABEL_2_CAR_ID            ((UINT32) 100004)
#define TDC_IPC_REPLY_ADDR_2_CAR_ID             ((UINT32) 100005)
#define TDC_IPC_REPLY_LABEL_2_CST_ID            ((UINT32) 100006)
#define TDC_IPC_REPLY_ADDR_2_CST_ID             ((UINT32) 100007)
#define TDC_IPC_REPLY_LABEL_2_TRN_CST_NO        ((UINT32) 100008)
#define TDC_IPC_REPLY_ADDR_2_TRN_CST_NO         ((UINT32) 100009)
#define TDC_IPC_REPLY_GET_TRN_CST_CNT           ((UINT32) 100010)
#define TDC_IPC_REPLY_GET_CST_CAR_CNT           ((UINT32) 100011)
#define TDC_IPC_REPLY_GET_CAR_DEV_CNT           ((UINT32) 100012)
#define TDC_IPC_REPLY_GET_CAR_INFO              ((UINT32) 100013)
#define TDC_IPC_REPLY_GET_UIC_STATE             ((UINT32) 100014)
#define TDC_IPC_REPLY_GET_UIC_GLOB_DATA         ((UINT32) 100015)
#define TDC_IPC_REPLY_GET_UIC_CAR_DATA          ((UINT32) 100016)
#define TDC_IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO    ((UINT32) 100017)
#define TDC_IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO     ((UINT32) 100018)
#define TDC_IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS      ((UINT32) 100019)
#define TDC_IPC_REPLY_SET_DEBUG_LEVEL           ((UINT32) 100020)
#define TDC_IPC_REPLY_GET_ADDR_BY_NAME_EXT      ((UINT32) 100021)
#define TDC_IPC_REPLY_CSTNO_2_CST_ID            ((UINT32) 100022)
#define TDC_IPC_REPLY_GET_VERSION               ((UINT32) 100023)
#define TDC_IPC_REPLY_GET_TBTYPE                ((UINT32) 100024)

#define TDC_IPC_REPLY_UNKNOWN_REQU              ((UINT32) 200000)

//---------------------------------------------------------------------------


extern T_TDC_BOOL tdcSendIpcMsg    (const char*       pModname,
                                    T_TDC_SOCKET      socketFd,
                                    T_TDC_IPC_MSG*    pMsg);


extern T_TDC_BOOL tdcReceiveIpcMsg (            const char*       pModname,
                                                T_TDC_SOCKET      socketFd,
                                     /*@out@*/  T_TDC_IPC_MSG*    pMsg,
                                                UINT32            maxLen);

extern void       tdcTIpcBody      (T_TDC_SOCKET      connFd);

extern /*@null@*/ void*  serviceThread    (/*@in@*/ void*             pArgv);

//---------------------------------------------------------------------------

#ifdef __cplusplus                            // to be compatible with C++
   }
#endif



#endif



