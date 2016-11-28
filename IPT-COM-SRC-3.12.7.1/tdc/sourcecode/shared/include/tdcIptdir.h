/*
 * $Id: tdcIptdir.h 11619 2010-07-22 10:04:00Z bloehr $
 */

/**
 * @file            tdc_iptdir.h
 *
 * @brief           IPT-Dir server internal data structures
 *
 * @note            Project: TCMS IP-Train - Ethernet train backbone handler
 *
 * @author          Thomas Gallenkamp, PPC/EMTC
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 * to third parties without express authority is forbidden,
 * Copyright Bombardier Transportation GmbH, Germany, 2006.
 *
 */

#ifndef TDC_IPTDIR_H
#define TDC_IPTDIR_H

/*******************************************************************************
 * INCLUDES
 */

#include "iptDef.h"

/*******************************************************************************
 * DEFINITIONS
 */

#define IPTDIR_MC_ON_MASTER  1  /**< */
#define IPTDIR_MC_ON_BYPASS  2  /**< */
#define IPTDIR_MC_ON_LOWIP   3  /**< */
#define IPTDIR_MC_ON_HIGHIP  4  /**< */

/*******************************************************************************
 * TYPEDEFS
 */

/**
 *   All variables in ipt_ structs use host endianess. Exception:
 *   UIC byte arrays (for UIC properties and UIC identifier) use
 *   UIC byte order.
 */

/** UIC train data consist
 *  UIC train data is to be kept current in the info-struct of the first
 *  consist ("root") of the linked list of consists.
 *
 */
typedef struct
{
    /* Note:
       UIC train data is to be kept current in the info-struct of the first
       consist ("root") of the linked list of consists.
     */
    /* UINT8 numControlledCars;   / **< replace by cst->carNum * / */
    int operator;                   /**< Read from cstSta XML */
    int     owner;                  /**<  dto. */
    int     nationalAppl;           /**<  dto. */
    int     nationalVers;           /**<  dto. */
    UINT8 uicCstProperties[22];   /**<  dto. */
    int     isInverse;          /**< relative to IP-Train reference direction */
    int     notAllConfirmed;        /**< see 3EGH 0000035-3060 J, ch. 3.2.1 */
    int     confirmedCancelled;     /**< dto. */
} IPT_CST_UIC_INFO_T;

/** UIC car data
 *
 */
typedef struct
{
    int     uicCarSeqNum;           /**< Set during inauguration. */
    UINT8 uicIdentifier[5];       /**< Read from cstSta XML */
    UINT8 uicCarProperties[6];    /**< Read from cstSta XML */
    int     seatResNo;              /**< in host endian */
/*    UINT8 ctrl;                   / **< May be redundant (?) * / */
    int     numTrnSwInCar;          /**< Do we will ever have more than 1 TS
                                         per car ? (not counting standby TS */
} IPT_CAR_UIC_INFO_T;

/** Device address element
 *
 */
typedef struct ipt_dev_t
{
    char             *name;     /**< */
    unsigned         ipAddr;    /**< */
    struct ipt_dev_t *next;     /**< */
} IPT_DEV_T;

/** Src address element
 *
 */
typedef struct ipt_src_t
{
    unsigned            ipAddr;         /**< */
    int                 redundant_grp;  /**< */
    struct ipt_src_t    *next;          /**< */
} IPT_SRC_T;

/** mc address element
 *
 */
typedef struct ipt_mc_t
{
    char            *name;      /**< */
    unsigned        ipAddr;     /**< */
    IPT_SRC_T       *srcs;      /**< */
    struct ipt_mc_t  *next;     /**< */
} IPT_MC_T;

/** Car element
 *
 */
typedef struct ipt_car_t
{
    IPT_DEV_T           *devs;         /**< */
    IPT_MC_T            *mCasts;       /**< */
    int                 carNo;         /**< */
    int                 devNum;        /**< */
    int                 mCastsNum;     /**< */
    int                 isInverse;     /**< */
    int                 isInverse2uic; /**< */
    int                 isLeading;     /**< 0: not, 1:Dir1, 2:Dir2 */
    int                 reqLeading;    /**< 0: not, 1:Dir1, 2:Dir2 */
    char                *id;           /**< */
    char                *type;         /**< */
    IPT_CAR_UIC_INFO_T  info;          /**< */
    struct ipt_car_t    *next;         /**< */
    struct ipt_car_t    *prev;         /**< */
} IPT_CAR_T;

/** Consist info element
 *
 */
typedef struct ipt_cst_t
{
    IPT_CAR_T           *cars;          /**< */
    IPT_MC_T            *mCastsCst;     /**< */
    IPT_MC_T            *mCastsTrn;     /**< */
    int                 cstNo;          /**< */
    int                 carNum;         /**< */
    int                 mCastsCstNum;   /**< */
    int                 mCastsTrnNum;   /**< */
    int                 isInverse;      /**< */
    char                *id;            /**< */
    unsigned            subnet;         /**< */
    int                 isLocal;        /**< */
    IPT_CST_UIC_INFO_T  info;           /**< */
    IPT_CAR_T           *cars_tail;     /**< */
    struct ipt_cst_t    *next;          /**< */
    struct ipt_cst_t    *prev;          /**< */
} IPT_CST_T;

/** vlan address element
 *
 */
typedef struct ipt_vlan_t
{
    unsigned          id;       /**< */
    unsigned          netAddr;  /**< */
    struct ipt_vlan_t *next;    /**< */
} IPT_VLAN_T;

/*******************************************************************************
 *  GLOBALS
 */

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

#endif /* IPTDIR_H */
