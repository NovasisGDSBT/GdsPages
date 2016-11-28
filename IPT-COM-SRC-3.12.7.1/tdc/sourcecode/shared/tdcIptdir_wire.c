/*
 *  $Id: tdcIptdir_wire.c 11861 2012-04-19 11:30:50Z gweiss $
 *
 *  DESCRIPTION    IPTDir wire protocol services
 *                 Routines for generating IPTDir datasets from internal
 *
 *  AUTHOR         Thomas Gallenkamp
 *
 *  REMARKS        Adapted for TDC usage (CR-382) by B. Loehr
 *
 *  DEPENDENCIES   Either the switch VXWORKS, INTEGRITY, LINUX or WIN32 has to be set
 *
 *  MODIFICATIONS (log starts 2010-08-11)
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *          Adding corrections for TÜV-Assessment
 *
 *  All rights reserved. Reproduction, modification, use or disclosure
 *  to third parties without express authority is forbidden.
 *  Copyright Bombardier Transportation GmbH, Germany, 2002-2012.
 *
 */


/*******************************************************************************
 * INCLUDES
 */

#include "tdc.h"

#include "iptcom.h"
#include "tdcIptdir.h"
#include "tdcIptdir_wire.h"


/*******************************************************************************
 * DEFINITIONS
 */

#define MAX_LBL_NUM  5000   /**< */

#define IPT_DS_MAX  100000  /**< */
#define UIC_DS_MAX  100000  /**< */
#define TW_MC_MAX   100000  /**< */

#define MAX_INFO2STRUCT_LABELS  1024    /**< */

/*******************************************************************************
 * TYPEDEFS
 */

typedef struct
{
    char *phony_cache_name;
    int  num;
    char *lbl[MAX_LBL_NUM];
} LBLCACHE_T;

typedef struct
{
    char value[MAX_INFO2STRUCT_LABELS][16];
    int  num;
} LBL_INFO_T;

/*******************************************************************************
 *  GLOBALS
 */

/*******************************************************************************
 *  LOCALS
 */

static LBLCACHE_T  lblcache_type = {"carType", 0, {0}};
static LBLCACHE_T  lblcache_group = {"groupLbl", 0, {0}};

static LBLCACHE_T  lblcache_dev = {"devLbl", 0, {0}};

/*******************************************************************************
 *  LOCAL FUNCTION DECLARATIONS
 */

/**
 *  @name Static functions 
 *  @{*/
static void lblcache_add(LBLCACHE_T *cache,
                         char       *lbl);
static int lblcache_index(LBLCACHE_T    *cache,
                          char          *lbl);

/**@}*/

/*******************************************************************************
 *  LOCAL FUNCTIONS
 */

/******************************************************************************/
/** Add to cache???
 *  @param[in,out]      cache       Pointer to cache.
 *  @param[in]          lbl         Pointer to label.
 *  @retval             none
 */
static void lblcache_add(LBLCACHE_T *cache,
                         char       *lbl)
{
    int i;
    for (i = 0;
         (i < cache->num) && (tdcStrCmp(lbl, cache->lbl[i]) != 0);
         i++)
    {
        ;
    }
    if (i == cache->num)
    {
        cache->lbl[i] = lbl;
        cache->num++;
    }
}

/******************************************************************************/
/** Cache index???
 *  @param[in]      cache       Pointer to cache.
 *  @param[in]      lbl         Pointer to label.
 *  @retval         > 0:        number of ...
 *  @retval         -1:         error???
 */
static INT32 lblcache_index(LBLCACHE_T    *cache,
                            char          *lbl)
{
    int i;
    for (i = 0;
         (i < cache->num) && (tdcStrCmp(lbl, cache->lbl[i]) != 0);
         i++)
    {
        ;
    }
    if (i == cache->num)
    {
        i = -1;
    }
    return i;
}



/******************************************************************************/
/** Creates an IPT_CST_T structure for a single
 * consist from linear binary payload pointed to by cst_info.
 *
 * The binary payload consists of a concatenation of:
 *      IPT    MD  - (same as IPTDir IPT MD)
 *      UIC    MD  - (same as IPTDir UIC MD)
 *      trainwide multicast  (proprietary format)
 *
 *  @param[in]      cst_root                    Pointer to consist root.
 *  @param[in]      ipt_inaug_status            ???
 *  @param[in]      ipt_topo_count              ???
 *  @param[in]      uic_inaug_status            ???
 *  @param[in]      uic_topo_count              ???
 *  @param[in]      dyn_count                   ???
 *  @param[in]      uic_inaugFrameVersion       ???
 *  @param[in]      uic_rDataVers               ???
 *  @param[in]      serverIpAddress             ???
 *  @param[in,out]  iptdir_processdataset       ???
 *  @param[in]      ipt_traindataset            ???
 *  @param[in]      ipt_traindataset_maxlen     ???
 *  @param[in,out]  ipt_traindataset_actuallen  ???
 *  @param[in]      uic_traindataset            ???
 *  @param[in]      uic_traindataset_maxlen     ???
 *  @param[in,out]  uic_traindataset_actuallen  ???
 *  @retval         0:                          no error
 *  @retval         1:                          parameter error
 */
INT32 iptdir_wire(const IPT_CST_T			*cst_root,
                UINT8	           		ipt_inaug_status,
                UINT32		     		ipt_topo_count,
                UINT8           		uic_inaug_status,
                UINT32  	       	 	uic_topo_count,
                UINT8       	   	 	dyn_count,
                UINT8           		uic_inaugFrameVersion,
                UINT8           		uic_rDataVers,
                UINT32           		serverIpAddress,
                IPTDIR_PROCESSDATASET_T *iptdir_processdataset,
                UINT8           		*ipt_traindataset,
                INT32                   ipt_traindataset_maxlen,
                INT32                   *ipt_traindataset_actuallen,
                UINT8           		*uic_traindataset,
                INT32                   uic_traindataset_maxlen,
                INT32                   *uic_traindataset_actuallen)
{
    const IPT_CST_T *cst;
    const IPT_CAR_T *car;
    const IPT_DEV_T *dev;
    const IPT_MC_T  *mc;
    const IPT_CST_T *cst_tail;

    UINT8		    *p;
    INT32           i;
    INT32           cst_num;
    INT32           car_num;
    INT32           cst_no;
    INT32           car_no;
    INT32           trnIsInverse;


    if (!cst_root)
    {
        return 1;
    }
    /*
     * Fixme: Do an elaborate length check
     */
    if ( (ipt_traindataset_maxlen < 32768)
         || (uic_traindataset_maxlen < 32768) )
    {
        return -1;
    }

    lblcache_dev.num  = 0;
    lblcache_type.num = 0;
    cst_num = 0;
    car_num = 0;
    /*
     * First traversal through train in order to
     * gather CarTypes and DeviceLabels. Remenber number of
     * consists in train.
     */

    for (cst = cst_root; cst; cst = cst->next)
    {
        /*
         * Train wide MCassts
         */
        for (mc = cst->mCastsTrn; mc; mc = mc->next)
        {
            lblcache_add(&lblcache_group, mc->name);
        }
        /*
         * Consist wide MCasts
         */
        for (mc = cst->mCastsCst; mc; mc = mc->next)
        {
            lblcache_add(&lblcache_group, mc->name);
        }
        /*
         *  Iterate over Cars
         */
        for (car = cst->cars; car; car = car->next)
        {
            lblcache_add(&lblcache_type, car->type);
            /*
             *  Car wide MCasts
             */
            for (mc = car->mCasts; mc; mc = mc->next)
            {
                lblcache_add(&lblcache_group, mc->name);
            }
            /*
             * Unicast devices
             */
            for (dev = car->devs; dev; dev = dev->next)
            {
                lblcache_add(&lblcache_dev, dev->name);
            }
        }
        cst_num++;
    }


    /* printf("%d \n",cst_num); */

    /* lblcache_print(&lblcache_type); */
    /* lblcache_print(&lblcache_dev); */

    /*
     *   IPT_TrainDataset
     *   ================
     */

    /*
     * IPT_TrainDataSet (3.1.2), p points to next position in dataset
     */
    p = ipt_traindataset;
    /* IPT_TrainDataSet.ProtocolVersion */
    *((UINT32 *)p) = tdcH2N32(PROTO_VERSION);/*lint !e826 type cast ok*/
    p += 4;

    *(p++) = ipt_inaug_status;  /* IPT_TrainDataSet.IPT_InaugStatus */
    *(p++) = ipt_topo_count;
    ;                         /* IPT_TrainDataSet.IPT_TopoCount   */
    *(p++) = 0x00;              /* IPT_TrainDataSet.reserved        */
    *(p++) = 0x00;              /* IPT_TrainDataSet.reserved        */

    /* IPT_TrainDataSet.NumCarTypesInTrain */
    *((UINT32 *)p) = tdcH2N32(lblcache_type.num);/*lint !e826 type cast ok*/
    p += 4;
    /* IPT_TrainDataSet.CarTypeDataSet[] */
    for (i = 0; i < lblcache_type.num; i++)
    {
        tdcMemSet(p, 0, 16);
        tdcStrNCpy((char *) p, lblcache_type.lbl[i], 15);
        p += 16;
    }
    /* IPT_TrainDataSet.NumDeviceLabelsInTrain */
    *((UINT32 *)p) = tdcH2N32(lblcache_dev.num);/*lint !e826 type cast ok*/
    p += 4;
    /* IPT_TrainDataSet.DeviceLabelDataSet[] */
    for (i = 0; i < lblcache_dev.num; i++)
    {
        tdcMemSet(p, 0, 16);
        tdcStrNCpy((char *) p, lblcache_dev.lbl[i], 15);
        p += 16;
    }

    /* IPT_TrainDataSet.NumGroupLabelsInTrain, NEW in 2.0.0.0 */
    *((UINT32 *)p) = tdcH2N32(lblcache_group.num);/*lint !e826 type cast ok*/
    p += 4;
    /* IPT_TrainDataSet.groupLabelDataSet[] */
    for (i = 0; i < lblcache_group.num; i++)
    {
        tdcMemSet(p, 0, 16);
        tdcStrNCpy((char *)p, lblcache_group.lbl[i], 15);
        p += 16;
    }

    /* IPT_TrainDataSet.NumMcTmGroups, NEW in 2.0.0.0 !!! */
    *((UINT32 *)p) = tdcH2N32(cst_root->mCastsTrnNum);/*lint !e826 type cast ok*/
    p += 4;
    /* IPT_TrainDataSet.McGroupDataSet */
    for (mc = cst_root->mCastsTrn; mc; mc = mc->next)
    {
        *((UINT16 *)p) = 
                        tdcH2N16((UINT16) lblcache_index(&lblcache_group,
                                                              mc->name));/*lint !e826 type cast ok*/
        p += 2;
        *((UINT16 *)p) =
                        tdcH2N16((UINT16) (mc->ipAddr & 0xfff));/*lint !e826 type cast ok*/
        p += 2;
    }

    /* IPT_TrainDataSet.ResQWInTrain, NEW in 2.0.0.0 !!! */
    *((UINT32 *)p) = tdcH2N32(0);/*lint !e826 type cast ok*/
    p += 4;

    /* IPT_TrainDataSet.NumConsistInTrain */
    *((UINT32 *)p) = tdcH2N32(cst_num);/*lint !e826 type cast ok*/
    p += 4;
    for (cst_no = 1, cst = cst_root; cst; cst = cst->next, cst_no++)
    {
        /*
         *    IPT_ConsistDataSet (3.1.3);
         */

        /*   .ConsistLbl  */
        tdcMemSet(p, 0, 16);
        tdcStrNCpy((char *)p, cst->id, 15);
        p += 16;

        *(p++) = cst_no; /* IPT_ConsistDataSet[].TrnCstNo */
        *(p++) = cst->isLocal ? 1 : 0;     /*   .isLocal     */
        *(p++) = cst->isInverse ? 0 : 1;  /*   .Orientation */
        *(p++) = 0;                     /*    <reserved>  */



        *((UINT32 *)p) = tdcH2N32(cst->mCastsCstNum); /*lint !e826 type cast ok*//* .NumMcCstGroups,
                                                        NEW in 2.0.0.0 ! */
        p += 4;

        /*  .NumMcGroupDataSet [] (3.1.3) */
        for (mc = cst->mCastsCst; mc; mc = mc->next)
        {
            *((UINT16 *)p) = 
                        tdcH2N16((UINT16) lblcache_index(&lblcache_group,
                                                              mc->name));/*lint !e826 type cast ok*/
            p += 2;
            *((UINT16 *)p) =
                        tdcH2N16((UINT16) (mc->ipAddr & 0xfff));/*lint !e826 type cast ok*/
            p += 2;
        }

        *((UINT32 *)p) = tdcH2N32(0); /*lint !e826 type cast ok*//*  .NumResQWInConsist, NEW in 2.0.0.0 !*/
        p += 4;

        *((UINT32 *)p) = tdcH2N32(cst->carNum); /*  .NumControlledCars *//*lint !e826 type cast ok*/
        p += 4;
        for (car = cst->cars; car; car = car->next)
        {
            /*
             *     IPT_CarDataSet (3.1.4)
             */
            /*   .CarLbl  */
            tdcMemSet(p, 0, 16);
            tdcStrNCpy((char *)p, car->id, 15);
            p     += 16;
            *(p++) = car->carNo;                        /* .CstCarNo */
            *(p++) = (car->isInverse ? 0x00 : 0x40)     /* .TCrrrrrr */
                     | ((cst->isInverse ? 0x00 : 0x80)  /* see 3 EGH
                                                           0000035-3060 J,
                                                           ch. 3.1.5 */
                        ^ (car->isInverse ? 0x80 : 0x00));
            /* .CarTypeLblIdx */
            *((UINT16 *)p) =
                        tdcH2N16((UINT16) lblcache_index(&lblcache_type,
                                                              car->type));/*lint !e826 type cast ok*/
            p += 2;
            /* .UIC_Identifier, NEW in 2.0.0.0 */
            tdcMemCpy(p, car->info.uicIdentifier, 5);  /*lint !e826 type cast ok*/   /* UIC_Identifier */
            p += 5;
            tdcMemSet(p, 0, 3);                           /* reserved */
            p += 3;

            *((UINT32 *)p) = tdcH2N32(car->mCastsNum);/*lint !e826 type cast ok*/ /* .NumMcCarGroups,
                                                         NEW in 2.0.0.0 ! */
            p += 4;

            /*  .NumMcGroupDataSet [] (3.1.3) */
            for (mc = car->mCasts; mc; mc = mc->next)
            {
                *((UINT16 *)p) = 
                        tdcH2N16((UINT16) lblcache_index(&lblcache_group,
                                                              mc->name));/*lint !e826 type cast ok*/
                p += 2;
                *((UINT16 *)p) =
                        tdcH2N16((UINT16) (mc->ipAddr & 0xfff));/*lint !e826 type cast ok*/
                p += 2;
            }

            *((UINT32 *)p) = tdcH2N32(0); /*lint !e826 type cast ok*//* .NumResQWInConsist,
                                            NEW in 2.0.0.0 ! */
            p += 4;


            *((UINT32 *)p) = tdcH2N32(car->devNum); /*lint !e826 type cast ok*/ /* .NumDevices */
            p += 4;
            car_num++;
            for (dev = car->devs; dev; dev = dev->next)
            {
                /*
                 *     IPT_DeviceDataSet (3.1.5)
                 */
                /* .DeviceLblIdex */
                *((UINT16 *)p) =
                        tdcH2N16((UINT16) lblcache_index(&lblcache_dev,
                                                              dev->name));/*lint !e826 type cast ok*/
                p += 2;
                /* .DeviceNo */
                *((UINT16 *)p) =
                        tdcH2N16((UINT16)(dev->ipAddr & 0xfff));/*lint !e826 type cast ok*/
                p += 2;
                *((UINT32 *)p) = tdcH2N32(0); /*lint !e826 type cast ok*//* .NumResQWInDev,
                                                NEW in 2.0.0.0 ! */
                p += 4;
            } /* rof dev */
        } /* rof car */
    } /* rof cst */

    *ipt_traindataset_actuallen = p - ipt_traindataset;


    /*
     *   UIC_TrainDataSet
     *   ================
     */

    /*
     * Generate doubly linked lists with previous pointers for car and cst
     * for easy UIC car sequencing in forward and backward direction
     */
    {
        IPT_CST_T *prev_cst;
        IPT_CAR_T *prev_car;
        IPT_CST_T *p_cst;
        IPT_CAR_T *p_car;

        for (p_cst = (IPT_CST_T *)cst_root, prev_cst = NULL;
             p_cst;
             prev_cst = p_cst, p_cst = p_cst->next)
        {
            p_cst->prev = prev_cst;
            for (p_car = (IPT_CAR_T *)p_cst->cars, prev_car = NULL;
                 p_car;
                 prev_car = p_car, p_car = p_car->next)
            {
                p_car->prev = prev_car;
            }
            p_cst->cars_tail = prev_car;
        }
        cst_tail = (const IPT_CST_T *)prev_cst;
    }

    /*
     * Determine wheter train is inverse or not.
     * (see 3EGM019001-0021, 2.3.2).
     *  If there is exactly one leading car:
     *   (cases a+b): Invert direction if there are more cars in front
     *                of leading car then behind.
     *  If there are zero leading cars, but exactly one traction
     *  unit at the end
     *   (case c):    Invert train if traction unit is at the end
     *
     * That requires to do an additional traversal of the train in UIC
     * sequence (assuming a non inverted train). Only except one
     * leading car per train.
     */
    {
        int leading_cars_num = 0;
        int leading_car      = 0;
        trnIsInverse = 0;


        /* IPT_TrainDataSet.UIC_CarData[] */
        for (cst = cst_root, car_no = 1; cst; cst = cst->next)
        {
            for (car = cst->isInverse ? cst->cars_tail : cst->cars;
                 car;
                 car = cst->isInverse ? car->prev : car->next,
                 car_no++)
            {
                if (car->isLeading)
                {
                    leading_car = car_no;
                    leading_cars_num++;
                }
            }
        }
        if ( (leading_cars_num == 1)
             && (leading_car > ((car_num + 1) / 2)) )
        {
            trnIsInverse = 1;
        }
        /*  Check both end Train-Sets for traction property (UIC-Property
         *  38 (24/0), 39(24/1)). Inverse train if traction property is at
         *  tail end only.
         */
        else if ( (leading_cars_num == 0)
                  && (cst_root &&
                      ((cst_root->info.uicCstProperties[2] & 0x03) == 0) )
                  && (cst_tail &&
                      ((cst_tail->info.uicCstProperties[2] & 0x03) != 0) )
                )
        {

            trnIsInverse = 1;
        }
    }


    /*
     * UIC_TrainDataSet (3.2.1), p points to next position in dataset
     */
    p = uic_traindataset;

    /* UIC_TrainDataSet.ProtocolVersion */
    *((UINT32 *)p) = tdcH2N32(PROTO_VERSION);/*lint !e826 type cast ok*/
    p += 4;



    *((UINT32 *)p) = tdcH2N32(0x0000006f); /*lint !e826 type cast ok*//* UIC_TrainDataSet.OptionsPresent */

    p += 4;

    *(p++) = uic_inaugFrameVersion;         /* InaugFrameVersion */
    *(p++) = uic_rDataVers;                 /* RDataVers */
    *(p++) = uic_inaug_status;              /* InaugStatus */
    *(p++) = uic_topo_count;                /* TopoCount */

    *(p++) = 0;                             /*  <reserved> */
    *(p++) = 0;                             /*  <reserved> */
    *(p++) = 0;                             /*  <reserved> */
    *(p++) =                                /* OACrrrr */
             (trnIsInverse                        ? 0x00 : 0x80)
             | (cst_root->info.notAllConfirmed    ? 0x40 : 0x00)
             | (cst_root->info.confirmedCancelled ? 0x20 : 0x00);
    tdcMemSet(p, 0, 8);
    p += 8;                                   /* ConfirmedPos */

    /*   UIC_TrainDataSet.NumCarsInTrain */
    *((UINT32 *)p) = tdcH2N32(car_num);
    p += 4;
    /* IPT_TrainDataSet.UIC_CarData[] */
    for (cst = trnIsInverse ? cst_tail : cst_root,
         cst_no = 1, car_no = 1;
         cst;
         cst = trnIsInverse ? cst->prev : cst->next,
         cst_no++)
    {
        for (car = ((cst->isInverse ? 1 : 0) ^ (trnIsInverse ? 1 : 0)) ? 
                   cst->cars_tail : cst->cars;
             car;
             car = ((cst->isInverse ? 1 : 0) ^ (trnIsInverse ? 1 : 0)) ?
                   car->prev : car->next,
             car_no++)
        {
            *(p++) = 0;            /* reserved */
            *(p++) = cst_no;       /* UIC_ConsistNo */
            *(p++) = cst->carNum;  /* NumControlledCars */
            *(p++) = car_no;       /* UIC_CarSeqNum */

            *(p++) = cst->info.operator;     /* Operator */
            *(p++) = cst->info.owner;        /* Owner */
            *(p++) = cst->info.nationalAppl; /* NationalAppl */
            *(p++) = cst->info.nationalVers; /* NationalVers */

            tdcMemCpy(p, cst->info.uicCstProperties, 22); /* UIC_CstProperties */
            p     += 22;
            *(p++) = 0;                                /* reserved */
            *(p++) = 0;                                /* reserved */

            tdcMemCpy(p, car->info.uicIdentifier, 5);     /* UIC_Identifier */
            p += 5;
            tdcMemCpy(p, car->info.uicCarProperties, 6);  /* UIC_CarProperties */
            p     += 6;
            *(p++) = 0;                                /* reserved */

            /* SeatResNo */
            *((UINT16 *)p) = tdcH2N16((UINT16)car->info.seatResNo);/*lint !e826 type cast ok*/
            p     += 2;
            *(p++) = (( (car->isInverse ? 1 : 0)
                        ^ (cst->isInverse ? 1 : 0)
                        ^ (trnIsInverse ? 1 : 0) )
                      ? 0x00 : 0x80)                    /* TCLRxxx */
                     | (car->isInverse     ? 0x00 : 0x40)
                     | (car->isLeading     ? 0x20 : 0x00)
                     | (car->reqLeading    ? 0x10 : 0x00);
            *(p++) = (car == cst->cars) ? 1 : 0;             /* NumTmSwInCar */
        }
    }
    *uic_traindataset_actuallen = p - uic_traindataset; /* Return actual
                                                           length */

    /*
     *   IPTDirProcessDataset
     *   ====================
     */
    if (iptdir_processdataset)
    {
        iptdir_processdataset->ProtocolVersion = tdcH2N32(PROTO_VERSION);
        iptdir_processdataset->IPT_InaugStatus = ipt_inaug_status;
        iptdir_processdataset->IPT_TopoCount   = ipt_topo_count;
        iptdir_processdataset->UIC_InaugStatus = uic_inaug_status;
        iptdir_processdataset->UIC_TopoCount   = uic_topo_count;
        iptdir_processdataset->DynStatus       = 0;
        iptdir_processdataset->DynCount        = dyn_count;
#ifdef TCN_GW_C
        iptdir_processdataset->TBType = 1;            /* WTB (TCN-GW-C) */
#else
        iptdir_processdataset->TBType = 0;            /* ETB (TS) */
#endif
        iptdir_processdataset->reserved1        = 0;
        iptdir_processdataset->SizeOfIptInfo    = tdcH2N32(
                                                   *ipt_traindataset_actuallen);
        iptdir_processdataset->SizeOfUicInfo    = tdcH2N32(
                                                   *uic_traindataset_actuallen);
        iptdir_processdataset->ServerIpAddress  = tdcH2N32(serverIpAddress);
        iptdir_processdataset->GatewayIpAddress = tdcH2N32(serverIpAddress);
        iptdir_processdataset->reserved3 = tdcH2N32(0);
    }
    return 0;
}
