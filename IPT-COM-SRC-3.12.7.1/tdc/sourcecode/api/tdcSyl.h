/*                                                                     */
/* $Id: tdcSyl.h 11853 2012-02-10 17:14:13Z bloehr $                 */
/*                                                                     */
/* DESCRIPTION    System level definitions for IP-Train                */
/*                                                                     */
/* AUTHOR         M.Ritz         PPC/EBTS                              */
/*                                                                     */
/* REMARKS                                                             */
/*                                                                     */
/* DEPENDENCIES                                                        */
/*                                                                     */
/*                                                                     */
/* All rights reserved. Reproduction, modification, use or disclosure  */
/* to third parties without express authority is forbidden.            */
/* Copyright Bombardier Transportation GmbH, Germany, 2012.            */

/*                                                                     
 *  CR-3477 (Bernd Loehr, 2012-02-06)
 *            Findings during TÜV-Assessment, here:
 *            Macro definition of MIN/MAX, brackets added
 *
*/

/* ---------------------------------------------------------------------------- */

#if !defined (TDC_SYL_H)
   #define TDC_SYL_H

/* ---------------------------------------------------------------------------- */

#if defined (__cplusplus)
extern "C" {
#endif

/* -----------------------------------------------------------------*/

#include "iptDef.h"        /* IPT definitions */


/* MRi 05/06/01,   Use #pragme pack or similiar for other plattforms ... */
#undef GNU_PACKED
#define GNU_PACKED

#if defined (__GNUC__)
   #if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 6))
      #undef GNU_PACKED
      /* MRi 05/02/07, assert Minimum alignment (packed) for structure elements for GNU Compiler. */
      #define GNU_PACKED   __attribute__ ((__packed__))
   #endif
#endif

/* 2008/07/16, MRi - Definition for DLL export in Windows (set only for building a DLL!) */
#if defined (O_WIN32_DLL) || defined (WIN32)
   #define EXP_DLL_DECL          __declspec(dllexport)
#else
   #define EXP_DLL_DECL          /* empty declaration */
#endif

/* ----------------------------------------------------------------- */
/* --------------------   Standard Definitions   ------------------- */
/* ----------------------------------------------------------------- */

#if !defined (TRUE)
   #define TRUE                     (1 == 1)
#endif

#if !defined (FALSE)
   #define FALSE                    (1 == 0)
#endif

#if !defined (NULL)
   #define NULL                     ((void *) 0)
#endif

/* ----------------------------------------------------------------- */

#if !defined (BIT0)
   #define BIT0                           0x0001
   #define BIT1                           0x0002
   #define BIT2                           0x0004
   #define BIT3                           0x0008
   #define BIT4                           0x0010
   #define BIT5                           0x0020
   #define BIT6                           0x0040
   #define BIT7                           0x0080
   #define BIT8                           0x0100
   #define BIT9                           0x0200
   #define BIT10                          0x0400
   #define BIT11                          0x0800
   #define BIT12                          0x1000
   #define BIT13                          0x2000
   #define BIT14                          0x4000
   #define BIT15                          0x8000
   #define BIT16                          0x00010000
   #define BIT17                          0x00020000
   #define BIT18                          0x00040000
   #define BIT19                          0x00080000
   #define BIT20                          0x00100000
   #define BIT21                          0x00200000
   #define BIT22                          0x00400000
   #define BIT23                          0x00800000
   #define BIT24                          0x01000000
   #define BIT25                          0x02000000
   #define BIT26                          0x04000000
   #define BIT27                          0x08000000
   #define BIT28                          0x10000000
   #define BIT29                          0x20040000
   #define BIT30                          0x40000000
   #define BIT31                          0x80000000
#endif

/* ----------------------------------------------------------------- */

#if !defined (MAX_UINT8)
   #define MAX_UINT8                      ((UINT8) (~0))
#endif

#if !defined (MAX_UINT16)
   #define MAX_UINT16                     ((UINT16) (~0))
#endif

#if !defined (MAX_UINT32)
   #define MAX_UINT32                     ((UINT32) (~0))
#endif

#if !defined (MAX_INT8)
   #define MAX_INT8                       ((INT8) (MAX_UINT8 / 2))
#endif

#if !defined (MAX_INT16)
   #define MAX_INT16                      ((INT16) (MAX_UINT16 / 2))
#endif

#if !defined (MAX_INT32)
   #define MAX_INT32                      ((INT32) (MAX_UINT32 / 2))
#endif

/* ----------------------------------------------------------------- */

#if !defined (MAX)
   #define MAX(x, y)                      (((x) > (y)) ? (x) : (y))
#endif

#if !defined (MIN)
   #define MIN(x, y)                      (((x) < (y)) ? (x) : (y))
#endif

#if !defined (TAB_SIZE)
   #define TAB_SIZE(x)                    ((int) ((int) sizeof (x) / (int) sizeof (x[0])))
#endif

/* ---------------------------------------------------------------------------- */

#define ELEM_OFFSET_IN_STRUC(type, elem)           ((UINT32) (((UINT32) (&(((type *) 8)->elem))) - 8))
#define ARRAY_OFFSET_IN_STRUC(type, array)         ((UINT32) (((UINT32) (((type *) 8)->array)) - 8))

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#define IPT_MAX_UIC_CST_NO                22
#define IPT_UIC_IDENTIFIER_CNT            5
#define IPT_UIC_CAR_PROPERTY_CNT          6
#define IPT_UIC_CONF_POS_CNT              8

/* ---------------------------------------------------------------------------- */

typedef struct
{
   UINT32               protVer;
   UINT8                iptInaugState;
   UINT8                iptTopoCnt;
   UINT8                uicInaugState;
   UINT8                uicTopoCnt;
   UINT8                dynState;
   UINT8                dynCnt;
   UINT8                tbType;
   UINT8                reserved0;
   UINT32               iptInfoSize;
   UINT32               uicInfoSize;
   UINT32               iptDirServerIpAddr;
   UINT32               gatewayIpAddr;
   UINT32               reserved1;
} T_IPT_IPTDIR_PD;

/* ---------------------------------------------------------------------------- */

typedef UINT32                T_IPT_RES_QW_DATA_SET;

typedef struct
{
   UINT32                     resQwCnt;
   T_IPT_RES_QW_DATA_SET      resQw[1];
} T_IPT_RES_QW_LIST;
#define RES_QW_DATA_SET_SIZE(cnt)                     ((UINT32) (cnt * sizeof (T_IPT_RES_QW_DATA_SET)))
#define RES_QW_LIST_RESQW_OFFSET()                    ARRAY_OFFSET_IN_STRUC (T_IPT_RES_QW_LIST, resQw)
#define RES_QW_LIST_SIZE(cnt)                         (RES_QW_LIST_RESQW_OFFSET () + RES_QW_DATA_SET_SIZE (cnt))

typedef struct
{
   UINT16                     lblIdx;
   UINT16                     no;
   T_IPT_RES_QW_LIST          resQwLst;
} T_IPT_DEV_DATA_SET;
#define DEV_DATA_SET_RESQW_LIST_OFFSET()              ELEM_OFFSET_IN_STRUC (T_IPT_DEV_DATA_SET, resQwLst)
#define DEV_DATA_SET_SIZE(cnt)                        (DEV_DATA_SET_RESQW_LIST_OFFSET () + RES_QW_LIST_SIZE (cnt))

typedef struct
{
   UINT32                     devCnt;
   T_IPT_DEV_DATA_SET         dev[1];
} T_IPT_DEV_LIST;
#define DEV_LIST_DEV_DATA_OFFSET()                    ARRAY_OFFSET_IN_STRUC (T_IPT_DEV_LIST, dev)

typedef struct
{
   UINT16                     lblIdx;
   UINT16                     no;
} T_IPT_MC_GRP_DATA_SET;

typedef struct
{
   UINT32                     grpCnt;
   T_IPT_MC_GRP_DATA_SET      grp[1];
} T_IPT_MC_GRP_LIST;
#define MC_GRP_DATA_SET_SIZE(cnt)                     ((UINT32) (cnt * sizeof (T_IPT_MC_GRP_DATA_SET)))
#define MC_GRP_LIST_GRP_OFFSET()                      ARRAY_OFFSET_IN_STRUC (T_IPT_MC_GRP_LIST, grp)
#define MC_GRP_LIST_SIZE(cnt)                         (MC_GRP_LIST_GRP_OFFSET () + MC_GRP_DATA_SET_SIZE (cnt))

typedef struct
{
   T_IPT_LABEL                carLbl;
   UINT8                      cstCarNo;
   UINT8                      TCrrrrrr;
   UINT16                     carTypeLblIdx;
   UINT8                      uicID[IPT_UIC_IDENTIFIER_CNT];
   UINT8                      reserved0;
   UINT8                      reserved1;
   UINT8                      reserved2;
   T_IPT_MC_GRP_LIST          mcLst;
} T_IPT_CAR_DATA_SET;
#define CAR_DATA_SET_MCLIST_OFFSET()                  ELEM_OFFSET_IN_STRUC (T_IPT_CAR_DATA_SET, mcLst)
#define CAR_DATA_SET_SIZE(cnt)                        (CAR_DATA_SET_MCLIST_OFFSET () + MC_GRP_LIST_SIZE (cnt))

#define IPT_GET_CARDATA_CST_ORIENT(TCrrrrrr)          (((TCrrrrrr & BIT6) != 0) ? (UINT8) (1) : (UINT8) 0)
#define IPT_GET_CARDATA_TRN_ORIENT(TCrrrrrr)          (((TCrrrrrr & BIT7) != 0) ? (UINT8) (1) : (UINT8) 0)

#define IPT_SET_CARDATA_TCRRRRRR(t, c)                ((UINT8) (   (((int) t != 0) ? BIT7 : 0)    \
                                                                 + (((int) c != 0) ? BIT6 : 0)    \
                                                               )                                  \
                                                      )

typedef struct
{
   UINT32                     carCnt;
   T_IPT_CAR_DATA_SET         car[1];
} T_IPT_CAR_LIST;
#define CAR_LIST_CAR_OFFSET()                         ARRAY_OFFSET_IN_STRUC (T_IPT_CAR_LIST, car)


typedef struct
{
   T_IPT_LABEL                cstLbl;
   UINT8                      trnCstNo;
   UINT8                      bIsLocal;
   UINT8                      orient;
   UINT8                      reserved;
   T_IPT_MC_GRP_LIST          mcLst;
} T_IPT_CONSIST_DATA_SET;
#define CONSIST_DATA_SET_MCLIST_OFFSET()              ELEM_OFFSET_IN_STRUC (T_IPT_CONSIST_DATA_SET, mcLst)

typedef T_IPT_LABEL           T_IPT_DEV_LABEL_DATA_SET;
typedef struct
{
   UINT32                              lblCnt;
   T_IPT_DEV_LABEL_DATA_SET            lbl[1];
} T_IPT_DEV_LABEL_LIST;
#define DEV_LABEL_DATA_SET_SIZE(cnt)                  ((UINT32) (cnt * sizeof (T_IPT_DEV_LABEL_DATA_SET)))
#define DEV_LABEL_LIST_LABEL_OFFSET()                 ARRAY_OFFSET_IN_STRUC (T_IPT_DEV_LABEL_LIST, lbl)
#define DEV_LABEL_LIST_SIZE(cnt)                      (DEV_LABEL_LIST_LABEL_OFFSET () + DEV_LABEL_DATA_SET_SIZE (cnt))

typedef T_IPT_LABEL           T_IPT_GRP_LABEL_DATA_SET;
typedef struct
{
   UINT32                              lblCnt;
   T_IPT_GRP_LABEL_DATA_SET            lbl[1];
} T_IPT_GRP_LABEL_LIST;
#define GRP_LABEL_DATA_SET_SIZE(cnt)                  ((UINT32) (cnt * sizeof (T_IPT_GRP_LABEL_DATA_SET)))
#define GRP_LABEL_LIST_LABEL_OFFSET()                 ARRAY_OFFSET_IN_STRUC (T_IPT_GRP_LABEL_LIST, lbl)
#define GRP_LABEL_LIST_SIZE(cnt)                      (GRP_LABEL_LIST_LABEL_OFFSET () + GRP_LABEL_DATA_SET_SIZE (cnt))

typedef struct
{
   UINT32                              cstCnt;
   T_IPT_CONSIST_DATA_SET              cst[1];
} T_IPT_CONSIST_LIST;
#define CONSIST_DATA_SET_SIZE(cnt)                    ((UINT32) (cnt * sizeof (T_IPT_CONSIST_DATA_SET)))
#define CONSIST_LIST_CST_OFFSET()                     ARRAY_OFFSET_IN_STRUC (T_IPT_CONSIST_LIST, cst)
#define CONSIST_LIST_SIZE(cnt)                        (CONSIST_LIST_CST_OFFSET () + CONSIST_DATA_SET_SIZE (cnt))

typedef T_IPT_LABEL              T_IPT_CAR_TYPE_DATA_SET;
typedef struct
{
   UINT32                              lblCnt;
   T_IPT_CAR_TYPE_DATA_SET             lbl[1];
} T_IPT_CAR_TYPE_LIST;
#define CAR_TYPE_DATA_SET_SIZE(cnt)                   ((UINT32) (cnt * sizeof (T_IPT_CAR_TYPE_DATA_SET)))
#define CAR_TYPE_LIST_LABEL_OFFSET()                  ARRAY_OFFSET_IN_STRUC (T_IPT_CAR_TYPE_LIST, lbl)
#define CAR_TYPE_LIST_SIZE(cnt)                       (CAR_TYPE_LIST_LABEL_OFFSET () + CAR_TYPE_DATA_SET_SIZE (cnt))

typedef struct
{
   UINT32                              protVer;
   UINT8                               inaugState;
   UINT8                               topoCnt;
   UINT16                              reserved0;
   T_IPT_CAR_TYPE_LIST                 carTypeLst;
} T_IPTDIR_IPT_MD;
#define IPT_MD_CAR_TYPE_LIST_OFFSET()                 ELEM_OFFSET_IN_STRUC (T_IPTDIR_IPT_MD, carTypeLst)
#define IPT_MD_CAR_TYPE_DATA_SET_OFFSET()             (IPT_MD_CAR_TYPE_LIST_OFFSET () + CAR_TYPE_LIST_LABEL_OFFSET ())

/* ---------------------------------------------------------------------------- */

typedef UINT8                             T_UIC_CST_PROP[IPT_MAX_UIC_CST_NO];
typedef UINT8                             T_UIC_IDENT[IPT_UIC_IDENTIFIER_CNT];
typedef UINT8                             T_UIC_CAR_PROP[IPT_UIC_CAR_PROPERTY_CNT];
typedef UINT8                             T_UIC_CONF_POS_CNT[IPT_UIC_CONF_POS_CNT];

typedef struct
{
   UINT8                reserved0;
   UINT8                uicCstNo;
   UINT8                contrCarCnt;
   UINT8                uicCarSeqNo;
   UINT8                operat;
   UINT8                owner;
   UINT8                natAppl;
   UINT8                natVer;
   T_UIC_CST_PROP       uicCstProp;
   UINT16               reserved1;
   T_UIC_IDENT          uicIdent;
   T_UIC_CAR_PROP       uicCarProp;
   UINT8                reserved2;
   UINT16               seatResNo;
   UINT8                TCLRrrrr;
   UINT8                trnSwInCarCnt;
} GNU_PACKED T_IPT_UIC_CAR_DATA_SET;

#define UIC_GET_CARDATA_TRN_ORIENT(TCLRrrrr)             (UINT8) (((TCLRrrrr) >> 7) & BIT0)
#define UIC_GET_CARDATA_CST_ORIENT(TCLRrrrr)             (UINT8) (((TCLRrrrr) >> 6) & BIT0)
#define UIC_GET_CARDATA_LEADING(TCLRrrrr)                (UINT8) (((TCLRrrrr) >> 5) & BIT0)
#define UIC_GET_CARDATA_LEADREQU(TCLRrrrr)               (UINT8) (((TCLRrrrr) >> 4) & BIT0)

#define UIC_SET_CARDATA_TCLRRRRR(t, c, l, r)             ((UINT8) (   (((int) t != 0) ? BIT7 : 0)    \
                                                                    + (((int) c != 0) ? BIT6 : 0)    \
                                                                    + (((int) l != 0) ? BIT5 : 0)    \
                                                                    + (((int) r != 0) ? BIT4 : 0)    \
                                                                  )                                  \
                                                         )


typedef struct
{
   UINT32                     trnCarCnt;
   T_IPT_UIC_CAR_DATA_SET     car[1];
} T_IPT_UIC_CAR_LIST;
#define UIC_CAR_DATA_SET_SIZE(cnt)                       ((UINT32) (cnt * sizeof (T_IPT_UIC_CAR_DATA_SET)))
#define UIC_CAR_LIST_CAR_OFFSET()                        ARRAY_OFFSET_IN_STRUC (T_IPT_UIC_CAR_LIST, car)
#define UIC_CAR_LIST_SIZE(cnt)                           (UIC_CAR_LIST_CAR_OFFSET () + UIC_CAR_DATA_SET_SIZE (cnt))

typedef struct
{
   UINT32                     protVer;
   UINT32                     optPresent;
   UINT8                      inaugFrameVer;
   UINT8                      rdataVer;
   UINT8                      uicInaugState;
   UINT8                      uicTopoCnt;
   UINT16                     reserved0;
   UINT8                      reserved1;
   UINT8                      OACrrrrr;
   T_UIC_CONF_POS_CNT         confirmedPos;
   T_IPT_UIC_CAR_LIST         uicCarDataSet;
} T_IPTDIR_UIC_MD;
#define UIC_MD_CAR_DATA_SET_OFFSET()                     ELEM_OFFSET_IN_STRUC (T_IPTDIR_UIC_MD, uicCarDataSet)
#define UIC_MD_SIZE(cnt)                                 (UIC_MD_CAR_DATA_SET_OFFSET () + UIC_CAR_LIST_SIZE (cnt))


#define UIC_GET_TRNDATA_OPT_CONFPOS(optPresent)             (UINT8) (((optPresent) >> 0) & BIT0) 
#define UIC_GET_TRNDATA_OPT_OPERATOWNER(optPresent)         (UINT8) (((optPresent) >> 1) & BIT0) 
#define UIC_GET_TRNDATA_OPT_NATAPPL(optPresent)             (UINT8) (((optPresent) >> 2) & BIT0) 
#define UIC_GET_TRNDATA_OPT_CSTPROP(optPresent)             (UINT8) (((optPresent) >> 3) & BIT0) 
#define UIC_GET_TRNDATA_OPT_CARPROP(optPresent)             (UINT8) (((optPresent) >> 5) & BIT0) 
#define UIC_GET_TRNDATA_OPT_SEATRES(optPresent)             (UINT8) (((optPresent) >> 6) & BIT0) 
            
#define UIC_SET_TRNDATA_OPT(co, op, na, cs, ca, se)         ((UINT32) (   (((int) co != 0) ? BIT0 : 0)    \
                                                                        + (((int) op != 0) ? BIT1 : 0)    \
                                                                        + (((int) na != 0) ? BIT2 : 0)    \
                                                                        + (((int) cs != 0) ? BIT3 : 0)    \
                                                                        + (((int) ca != 0) ? BIT5 : 0)    \
                                                                        + (((int) se != 0) ? BIT6 : 0)    \
                                                                     )                                    \
                                                            )

#define UIC_GET_TRNDATA_ORIENT(OACrrrrr)                    (UINT8) (((OACrrrrr) >> 7) & BIT0)
#define UIC_GET_TRNDATA_NOTALLCONF(OACrrrrr)                (UINT8) (((OACrrrrr) >> 6) & BIT0)
#define UIC_GET_TRNDATA_CONFCANCELED(OACrrrrr)              (UINT8) (((OACrrrrr) >> 5) & BIT0)

#define UIC_SET_TRNDATA_OACRRRRR(o, a, c)                   ((UINT8) (   (((int) o != 0) ? BIT7 : 0)    \
                                                                       + (((int) a != 0) ? BIT6 : 0)    \
                                                                       + (((int) c != 0) ? BIT5 : 0)    \
                                                                     )                                  \
                                                            )

/* ---------------------------------------------------------------------------- */

#define IPT_IPTDIR_REQ_SEND_IPT				1
#define IPT_IPTDIR_REQ_SEND_UIC				2

typedef struct
{
   UINT32                  protVer;
	UINT16						requType;
	UINT16 						reserved0;
} T_IPT_IPTDIR_REQ_MD;

/* ---------------------------------------------------------------------------- */

#if defined (__cplusplus)
}
#endif

#endif
