/*
 *  $Id: tdcCst_info.c 21400 2013-05-28 13:18:14Z gweiss $
 *
 *  DESCRIPTION    Parsing XML file and constructing internal database.
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


#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tdc.h"
#include "iptDef.h"
#include "tdcIptdir.h"
#include "tdcIptdir_wire.h"
#include "tdcCst_info.h"

#include "tdcPicoxml.h"

/*******************************************************************************
 * DEFINITIONS
 */

#define PROGNAME  "cst_info"    /**< */

#ifdef WIN32
/* typedef unsigned __int64 u64; */
    #define PRINTF_FORMAT_UINT64  "%I64u"       /**< */
#else
/* RMY  typedef unsigned long long u64; */
    #define PRINTF_FORMAT_UINT64  "%llu"        /**< */
#endif

#define BASE_NUMBER_10      10      /**< The number in decimal format */

#define LOCOS_PER_LINE  7           /**< */

/*******************************************************************************
 *  TYPEDEFS
 */

/*******************************************************************************
 *  GLOBALS
 */

/*******************************************************************************
 * LOCALS
 */

/*******************************************************************************
 *  LOCAL FUNCTION DECLARATIONS
 */

/**
 *  @name Static functions 
 *  @{*/
static UINT32 dq2bin(char *dq);
static void strtolower(char *p);
/**@}*/

/*******************************************************************************
 * LOCAL FUNCTIONS
 */

/******************************************************************************/
/** Convert dotted quad to binary.
 *
 *  Convert dotted quad to bin. If no dots are in "dotted quad" assume
 *  binary decimal value.
 *
 *  @param[in,out]      dq            Pointer to dotted quad representation.
 *  @retval             binary value
 */
static UINT32 dq2bin(char *dq)
{ 
    UINT32 rv;
    
    rv = 0;

    /*
     * Convert dotted quad to bin. If no dots are in "dotted quad" assume
     * binary decimal value
     */
    if (strchr(dq, '.') == NULL)
    {
        return (UINT32) strtol(dq, NULL, BASE_NUMBER_10);
    }
    else
    {
        while (*dq)
        {
            rv <<= 8;
            rv  |= (UINT32) strtol(dq, NULL, BASE_NUMBER_10);
            while (*dq && (*dq != '.') )
            {
                dq++;
            }
            if (*dq == '.')
            {
                dq++;
            }
        }
        return rv;
    }
}

/******************************************************************************/
/** Strtolower.
 *
 *  @param[in,out]      p           Pointer to string.
 *  @retval             none        
 */
static void strtolower(char *p)
{
    while (*p)
    {
        *p = (char)tolower(*p);
        p++;
    }
}

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */
 

/******************************************************************************/
/** Patch device IPs to reflect topo count and position in train.
 *
 *  @param[in,out]      cst         Pointer to consist struct.
 *  @param[in]          topo        Topology count.
 *  @retval             none
 */
void cst_info_cst_set_topo(IPT_CST_T    *cst,
                           UINT32       topo)
{
    INT32       cst_no;
    IPT_CAR_T   *car;
    IPT_DEV_T   *dev;
    IPT_MC_T    *mc;
    IPT_SRC_T   *src;

    for (cst_no = 1; cst; cst = cst->next, cst_no++)
    {
        for (car = cst->cars; car; car = car->next)
        {
            for (dev = car->devs; dev; dev = dev->next)
            {
                if (cst->isLocal)
                {
                    dev->ipAddr &= 0xff000fff;
                }
                else
                {
                    dev->ipAddr = (dev->ipAddr  & 0xff000fff)
                                  | ((topo << 18)   & 0x00fc0000)
                                  | ((cst_no << 12) & 0x0003f000);
                } /* fi isLocal */
            } /* rof dev */
        } /* rof car */
        for (mc = cst->mCastsTrn; mc; mc = mc->next)
        {
            if (cst->isLocal)
            {
                for (src = mc->srcs; src; src = src->next)
                {
                    src->ipAddr &= 0xff000fff;
                } /* rof src */
            }
            else
            {
                for (src = mc->srcs; src; src = src->next)
                {
                    src->ipAddr = (src->ipAddr & 0xff000fff)
                                  | ((topo << 18)   & 0x00fc0000)
                                  | ((cst_no << 12) & 0x0003f000);
                } /* rof src */
            } /* fi isLocal */
        } /* rof mc */
    } /* rof cst */
} /* corp */

/******************************************************************************/
/** cst_info_uic_inauguration ???
 *
 *  @param[in,out]      cst         Pointer to consist struct.
 *  @param[in]          is_reverse  ???
 *  @retval             none
 */
void cst_info_uic_inauguration(IPT_CST_T    *cst,
                               INT32        is_reverse)
{
    INT32 car_num;
    INT32 i;
    IPT_CST_T   *cst_p;
    IPT_CAR_T   *car;


    if (!cst)
    {
        return;
    }

    /*
     * Determine number of cars
     */
    for (cst_p = cst, car_num = 0; cst_p; cst_p = cst_p->next)
    {
        for (car = cst_p->cars; car; car = car->next)
        {
            car_num++;
        }
    }
    //printf("+++%d\n", car_num);
    /*
     * Number cars according to UIC
     */
    for (cst_p = cst, i = 0; cst_p; cst_p = cst_p->next)
    {
        for (car = cst_p->cars; car; car = car->next)
        {
            car->info.uicCarSeqNum = is_reverse ? (car_num - i) : (i + 1);
            i++;
        }
    }
    cst->info.isInverse = is_reverse;
    cst->info.notAllConfirmed    = 0;
    cst->info.confirmedCancelled = 0;
}


/******************************************************************************/
/** Free the device list queue (and all elements).
 *
 *  @param[in,out]      p           Pointer to device struct.
 *  @retval             none
 */
void cst_info_free_dev(IPT_DEV_T *p)
{
    for (; p;)
    {
        IPT_DEV_T *q;

        q = p;
        p = p->next;
        tdcFreeMem(q->name);
        tdcFreeMem(q);
    }
}

/******************************************************************************/
/** Free the mc list queue (and all elements).
 *
 *  @param[in,out]      p           Pointer to mc struct.
 *  @retval             none
 */
void cst_info_free_mc(IPT_MC_T *p)
{
    for (; p;)
    {
        IPT_MC_T *q;

        q = p;
        p = p->next;
        tdcFreeMem(q->name);
        tdcFreeMem(q);
    }
}

/******************************************************************************/
/** Free the carriage list queue (and all elements).
 *
 *  @param[in,out]      p           Pointer to carriage struct.
 *  @retval             none
 */
void cst_info_free_car(IPT_CAR_T *p)
{
    for (; p;)
    {
        IPT_CAR_T *q;
        q = p;
        p = p->next;
        cst_info_free_dev(q->devs);
        cst_info_free_mc(q->mCasts);
        tdcFreeMem(q->id);
        tdcFreeMem(q->type);
        tdcFreeMem(q);
    }
}

/******************************************************************************/
/** Free the consist list queue (and all elements except one).
 *
 *  @param[in,out]      p           Pointer to consist struct.
 *  @param[in]          keep_this   Pointer to consist struct to be kept.
 *  @retval             none
 */
void cst_info_free_cst(IPT_CST_T    *p,
                       IPT_CST_T    *keep_this)
{
    for (; p;)
    {
        IPT_CST_T *q;
        q = p;
        p = p->next;
        if (q != keep_this)
        {
            cst_info_free_car(q->cars);
            cst_info_free_mc(q->mCastsCst);
            cst_info_free_mc(q->mCastsTrn);
            tdcFreeMem(q->id);
            tdcFreeMem(q);
        }
        else
        {
            q->next = NULL; /* at least reset next pointer from "keeper" */
        }
    }
}

/******************************************************************************/
/** Get no of consist elements.
 *
 *  @param[in,out]      p       Pointer to consist struct.
 *  @retval             no. of entries
 */
INT32 cst_info_get_cst_no_last(const IPT_CST_T *p)
{
    INT32 i;
    for (i = 0; p; p = p->next, i++)
    {
        ;
    }
    return i;
}

/******************************************************************************/
/** Get no of local consist elements.
 *
 *  @param[in,out]      p       Pointer to consist struct.
 *  @retval             no. of entries
 */
INT32 cst_info_get_cst_no_local(const IPT_CST_T *p)
{
    INT32 i;
    for (i = 0; p && !p->isLocal; p = p->next, i++)
    {
        ;
    }
    return (p && p->isLocal ? i + 1 : 0);
}

/******************************************************************************/
/** Add entry to vlan queue.
 *
 *  @param[in,out]      root_pp     Pointer to pointer to vlan info queue.
 *  @param[in]          id          ???
 *  @param[in]          netAddr     ???
 *  @retval             none
 */
void cst_info_vlan_add(IPT_VLAN_T   **root_pp,
                       UINT32     	id,
                       UINT32     	netAddr)
{
    IPT_VLAN_T *p;

    /* printf("VLAN add %d %s\n",id,bin2dq(netAddr)); */

    for (p = *root_pp; p && (p->id != id); p = p->next)
    {
        ;
    }
    if (!p)
    {
        p = (IPT_VLAN_T *) tdcAllocMem(sizeof(IPT_VLAN_T));
        if (p != NULL)
        {
           p->id      = id;
           p->netAddr = netAddr;
           p->next    = *root_pp;
        }
        *root_pp   = p;
    }
}

/******************************************************************************/
/** Find and return vlan entry.
 *
 *  @param[in]      root        Pointer to vlan info queue.
 *  @param[in]      id          ???
 *  @retval         netAddr
 */
UINT32 cst_info_vlan_find(IPT_VLAN_T  *root,
                            UINT32    id)
{
    IPT_VLAN_T *p;

    for (p = root; p && (p->id != id); p = p->next)
    {
        ;
    }
    /* printf("VLAN find %d %s\n",id,bin2dq(p ? p->netAddr : 0xffffffff)); */
    return p ? p->netAddr : 0xffffffff;
}


/******************************************************************************/
/** parse_multicast_devs ???
 *
 *  @param[in,out]      mc_root_pp      Pointer to vlan info queue.
 *  @param[in,out]      num_mc_p        Pointer to ???.
 *  @param[in]          offset          ???
 *  @param[in]          vlan_root       Pointer to ???.
 *  @retval             none
 */
void cst_info_parse_multicast_devs(IPT_MC_T     **mc_root_pp,
                                   INT32        *num_mc_p,
                                   UINT32     	offset,
                                   IPT_VLAN_T   *vlan_root)
{
    char        tag[100];
    char        data[100];
    UINT32    my_mcIp;
    UINT32    my_netOffset;
    char        my_mcLbl[100];
    IPT_MC_T    *mc_p;
    IPT_SRC_T   *srcs;
    
    my_mcIp      = 0xffffffff;
    my_netOffset = cst_info_vlan_find(vlan_root, 1);
    *my_mcLbl    = 0;
    srcs         = NULL;

    while (picoxml_seek_any(tag, 100) == 0)
    {
        picoxml_data(data, 100);
        if (tdcStrCmp(tag, "grpNo") == 0)
        {
            my_mcIp = dq2bin(data) | offset;
            srcs    = NULL;
        }
        else if (tdcStrCmp(tag, "vlanMemberId") == 0)
        {
            picoxml_data(data, 100);
            my_netOffset = cst_info_vlan_find(vlan_root, 
                                        (UINT32)strtol(data, NULL,
                                                         BASE_NUMBER_10));
        }
        else if (tdcStrCmp(tag, "srcNo") == 0)
        {
            IPT_SRC_T *new_src_p;

            new_src_p = (IPT_SRC_T *)tdcAllocMem(sizeof(IPT_SRC_T));

            /*
               only works if vlan description comes before multicast
               description in XML file.
               new_src_p->ipAddr = dq2bin(data) | my_netOffset;
             */
           if (new_src_p != NULL)
           {
               new_src_p->ipAddr = dq2bin(data) | 0x0a000000;
               switch (data[tdcStrLen(data) - 1])
               {
                   case 'b':
                   case 'B':
                       new_src_p->redundant_grp = IPTDIR_MC_ON_BYPASS;
                       break;
                   case 'l':
                   case 'L':
                       new_src_p->redundant_grp = IPTDIR_MC_ON_LOWIP;
                       break;
                   case 'h':
                   case 'H':
                       new_src_p->redundant_grp = IPTDIR_MC_ON_HIGHIP;
                       break;
                   default: 
                       new_src_p->redundant_grp = IPTDIR_MC_ON_MASTER;
                       break;
               }
               new_src_p->next = srcs;
               srcs = new_src_p;
           }
        }
        else if (tdcStrCmp(tag, "grpLbl") == 0)
        {
            tdcStrCpy(my_mcLbl, data); 
            strtolower(my_mcLbl);
            if ( (my_mcIp != 0xffffffff) && *my_mcLbl)
            {
                /*
                 * Insert device at start of linked list.
                 * (List is not sorted by any means)
                 */
                mc_p = (IPT_MC_T *)tdcAllocMem(sizeof(IPT_MC_T));
                if (mc_p != NULL)
                {
                   mc_p->ipAddr = my_mcIp;
                   mc_p->srcs   = srcs;
                   mc_p->name   = (char *)tdcAllocMem(tdcStrLen(my_mcLbl) + 1);
                   if (mc_p->name)
                   {
                      tdcStrCpy(mc_p->name, my_mcLbl);
                   }
                   mc_p->next  = *mc_root_pp;
                   if (num_mc_p)
                   {
                       (*num_mc_p)++;
                   }
                }
               *mc_root_pp = mc_p;
            }
            /*
             * Each time we see "grpLbl" invalidate ipaddr ("my_mcIp")
             */
            my_mcIp = 0xffffffff;
        }
    }
}

/******************************************************************************/
/** Convert ASCII hex representation to binary.
 *
 *  @param[in]      in          Pointer to hex input string.
 *  @param[out]     out         Pointer to output string.
 *  @param[in]      len         String length.
 *  @retval         none
 */
void cst_info_hexBinary2bin(char		*in,
                            UINT8		*out,
                            INT32		len)
{
    int i;
    int ok;

    ok = ((int) tdcStrLen(in) == len * 2);
    for (i = 0; ok && (i < len); i++)
    {

        int tmp, rv;
        /*
         * Anoyingly MS WIN32 does not know about the ANSI hh modifier,
         * so wee need to work around
         */
        tmp    = 0;
        rv     = tdcSScanf(in + i * 2, "%02x", &tmp);
        out[i] = (unsigned char)tmp;
        ok     = ok && (rv == 1);
    }
    if (!ok)
    {
        tdcMemSet(out, 0, len);
    }
}

/******************************************************************************/
/** Read and decode XML file into local cst-queue.
 *
 *  Read file pointed to by fp and return pointer to malloced and filled
 *  IPT_CST_T structure.
 *
 *  @param[in]      filename        Pointer to pathname.
 *  @retval         pointer to consist structure queue
 */
IPT_CST_T *cst_info_read_xml_file(const char *filename)
{
    T_FILE              *fp;
    char                tag[100];
    char                data[100];
    INT32               num_car;
    INT32               num_dev;
    INT32               num_trn_mc_dev;
    INT32               num_cst_mc_dev;
    INT32               num_car_mc_dev;
    UINT32              my_devNo;
    char                my_devLbl[100];
    char                my_switch_hostLbl[100];
    UINT32              my_switch_devNo;
    UINT32              my_switch_netMask;
    INT32               my_carNo        = 0;
    char                my_carId[100]   = {0};
    char                my_carType[100] = {0};
    INT32               my_carIsInverse = 0;
    char                my_cstId[100];
    UINT32              my_vlanId;
    UINT32              my_netMask;
    IPT_CAR_UIC_INFO_T  my_carUicInfo;
    IPT_CST_UIC_INFO_T  my_cstUicInfo;
    IPT_DEV_T           *dev_p;
    IPT_DEV_T           *dev_root;
    IPT_CAR_T           *car_p;
    IPT_CAR_T           *car_root;
    IPT_MC_T            *trn_mc_dev_root;
    IPT_MC_T            *cst_mc_dev_root;
    IPT_MC_T            *car_mc_dev_root;
    IPT_VLAN_T          *vlan_root;
    IPT_CST_T           *my_cst;

    /* Open file */
    fp = tdcFopen(filename, "r");
    if (fp == NULL)
    {
        fprintf(stderr, PROGNAME "-F-Cannot lookup %s\n", filename);
        return NULL;
    }

    picoxml_init(fp);


    *my_cstId       = 0;
    num_car         = 0;
    car_root        = NULL; /* Start with an empty car list */
    num_trn_mc_dev  = 0;
    trn_mc_dev_root = NULL;
    num_cst_mc_dev  = 0;
    cst_mc_dev_root = NULL;
    vlan_root       = NULL;
    tdcMemSet(&my_cstUicInfo, 0, sizeof(my_cstUicInfo));
    if (picoxml_seek("cstSta") == 0)
    {
        picoxml_enter();

        while (picoxml_seek_any(tag, 100) == 0)
        {

            /*
             * cstSta/_uicCstData__
             */
            if (tdcStrCmp(tag, "uicCstData") == 0)
            {
                picoxml_enter();
                while (picoxml_seek_any(tag, 100) == 0)
                {
                    picoxml_data(data, 100);
                    /*
                     * Not all UIC consist level items are currently used.
                     */
                    if (tdcStrCmp(tag, "uicCstLbl") == 0)
                    {
                        tdcStrCpy(my_cstId, data);
                        strtolower(my_cstId);
                    }
                    else if (tdcStrCmp(tag, "operator") == 0)
                    {
                        my_cstUicInfo.operator =
                            (int)strtol(data, NULL, BASE_NUMBER_10);
                    }
                    else if (tdcStrCmp(tag, "owner") == 0)
                    {
                        my_cstUicInfo.owner =
                            (int)strtol(data, NULL, BASE_NUMBER_10);
                    }
                    else if (tdcStrCmp(tag, "nationalAppl") == 0)
                    {
                        my_cstUicInfo.nationalAppl = 
                            (int)strtol(data, NULL, BASE_NUMBER_10);
                    }
                    else if (tdcStrCmp(tag, "nationalVers") == 0)
                    {
                        my_cstUicInfo.nationalVers =
                            (int)strtol(data, NULL, BASE_NUMBER_10);
                    }
                    else if (tdcStrCmp(tag, "properties") == 0)
                    {
                        cst_info_hexBinary2bin(data,
                                               my_cstUicInfo.
                                               uicCstProperties,
                                               22);
                    }
                }
                picoxml_leave();
            }
            /*
             * cstSta/_trnMultiCast_
             */
            else if (tdcStrCmp(tag, "trnMultiCast") == 0)
            {
                UINT32 subnet_addr = 0xef03f000;
                picoxml_enter();
                while (picoxml_seek_any(tag, 100) == 0)
                {

                    /*
                     * ignore netAddr setting. No sense to deviate from default
                     * 239.3.240.0
                     *
                       if (tdcStrCmp(tag,"netAddr")==0)
                       {
                           picoxml_data(data,100);
                           subnet_addr=dq2bin(data);
                       }
                       else
                     */

                    if (tdcStrCmp(tag, "groups") == 0)
                    {
                        picoxml_enter();
                        cst_info_parse_multicast_devs(&trn_mc_dev_root,
                                                      &num_trn_mc_dev,
                                                      subnet_addr,
                                                      vlan_root);
                        picoxml_leave();
                    }
                }
                picoxml_leave();
            }
            /*
             * cstSta/_vlan_
             */
            else if (tdcStrCmp(tag, "vlan") == 0)
            {
                picoxml_enter();
                my_vlanId = 0;
                while (picoxml_seek_any(tag, 100) == 0)
                {
                    if (tdcStrCmp(tag, "id") == 0)
                    {
                        picoxml_data(data, 100);
                        my_vlanId = 
                            (UINT32)strtol(data, NULL, BASE_NUMBER_10);
                    }
                    else if (tdcStrCmp(tag, "multiCast") == 0)
                    {
                        picoxml_enter();
                        while (picoxml_seek_any(tag, 100) == 0)
                        {
                            if (tdcStrCmp(tag, "groups") == 0)
                            {
                                picoxml_enter();
                                cst_info_parse_multicast_devs(&cst_mc_dev_root,
                                                              &num_cst_mc_dev,
                                                              0xef000000,
                                                              vlan_root);
                                picoxml_leave();
                            }
                        }
                        picoxml_leave();
                    }
                    else if (tdcStrCmp(tag, "uniCast") == 0)
                    {
                        picoxml_enter();
                        while (picoxml_seek_any(tag, 100) == 0)
                        {
                            if (tdcStrCmp(tag, "netAddr") == 0)
                            {
                                UINT32 tmp_ip;

                                picoxml_data(data, 100);
                                tmp_ip = dq2bin(data);
                                if (my_vlanId > 0)
                                {
                                    cst_info_vlan_add(&vlan_root,
                                                      my_vlanId,
                                                      tmp_ip);
                                }
                            }
                            else if (tdcStrCmp(tag, "subNetMsk") == 0)
                            {}
                        }
                        picoxml_leave();
                    }
                }
                picoxml_leave();
            }
            /*
             * cstSta/_carSta_
             */
            else if (tdcStrCmp(tag, "carSta") == 0)
            {
                picoxml_enter();

                num_dev         = 0;
                dev_root        = NULL; /* Mext car: Start with an empty
                                           devicelist */
                num_car_mc_dev  = 0;
                car_mc_dev_root = NULL;
                tdcMemSet(&my_carUicInfo, 0, sizeof(my_carUicInfo));

                while (picoxml_seek_any(tag, 100) == 0)
                {
                    /*
                     * cstSta/carSta/ _switch_
                     */
                    if (tdcStrCmp(tag, "switch") == 0)
                    {
                        picoxml_enter();

                        *my_switch_hostLbl = 0;
                        my_switch_devNo    = 0x00000000;
                        my_switch_netMask  = 0xffffffff;

                        while (picoxml_seek_any(tag, 100) == 0)
                        {
                            /*
                             * cstSta/carSta/switch/ _hostLbl_
                             *                       _devNo_
                             *                       _vlanMemberId_
                             */
                            if (tdcStrCmp(tag, "hostLbl") == 0)
                            {
                                picoxml_data(data, 100);
                                tdcStrCpy(my_switch_hostLbl, data);
                                strtolower(my_switch_hostLbl);

                            }
                            else if (tdcStrCmp(tag, "devNo") == 0)
                            {
                                picoxml_data(data, 100);
                                my_switch_devNo = dq2bin(data);
                            }
                            else if (tdcStrCmp(tag, "vlanMemberId") == 0)
                            {
                                picoxml_data(data, 100);
                                my_switch_netMask =
                                    cst_info_vlan_find(
                                        vlan_root,
                                        (UINT32)strtol(data,
                                                         NULL,
                                                         BASE_NUMBER_10));
                            }
                            /*
                             * cstSta/carSta/switch/ _deviceGroup_
                             */
                            else if (tdcStrCmp(tag, "deviceGroup") == 0)
                            {
                                picoxml_enter();
                                while (picoxml_seek("device") == 0)
                                {
                                    picoxml_enter();
                                    *my_devLbl = 0;
                                    my_devNo   = 0x00000000;
                                    my_netMask = 0xffffffff;
                                    while (picoxml_seek_any(tag, 100) == 0)
                                    {
                                        /* cstSta/carSta/switch/deviceGroup/device/ _devNo__
                                         *                                          _devLbl_
                                         */
                                        if (tdcStrCmp(tag, "devNo") == 0)
                                        {
                                            picoxml_data(data, 100);
                                            my_devNo = dq2bin(data);
                                        }
                                        else if (tdcStrCmp(tag, "devLbl") == 0)
                                        {
                                            picoxml_data(data, 100);
                                            tdcStrCpy(my_devLbl, data);
                                            strtolower(my_devLbl);
                                        }
                                        else if (tdcStrCmp(tag, "vlanMemberId")
                                                 == 0)
                                        {
                                            picoxml_data(data, 100);
                                            my_netMask =
                                                cst_info_vlan_find(
                                                    vlan_root,
                                                    (UINT32)strtol(
                                                               data,
                                                               NULL,
                                                               BASE_NUMBER_10));
                                        }
                                    }
                                    picoxml_leave();
                                    /*
                                     * This is were a new device record is
                                     * to be created
                                     */
#if 0
                                    printf("Device: /%-10s/%d.%d.%d.%d\n",
                                           my_devLbl,
                                           my_devIp >> 24,
                                           (my_devIp >> 16) & 0xff,
                                           (my_devIp >> 8 ) & 0xff,
                                           (my_devIp      ) & 0xff);
#endif
                                    /*
                                     * Insert device at start of linked list.
                                     * (List is not sorted by any means)
                                     */
                                    if ((my_netMask | my_devNo) < 0xe0000000)
                                    { /* allow Unicast IPs only */
                                        dev_p = (IPT_DEV_T *)
                                                tdcAllocMem(sizeof(IPT_DEV_T));
                                        if (dev_p)
                                        {
                                           dev_p->ipAddr = my_devNo |
                                                           my_netMask;
                                           dev_p->name =
                                               (char *)
                                               tdcAllocMem(tdcStrLen(my_devLbl) + 1);
                                           if (dev_p->name)
                                           {
                                              tdcStrCpy(dev_p->name, my_devLbl);
                                           }
                                           dev_p->next = dev_root;
                                        }
                                        dev_root    = dev_p;
                                        num_dev++;
                                    }
                                } /* elihw device */
                                picoxml_leave();
                            } /* fiesle deviceGroup, vlanMemberId */
                        } /* elihw seek_any switch  */
                        picoxml_leave();
                        /*
                         * Insert switch itself as an ordinary IPTDir device
                         */
                        if ((my_switch_netMask | my_switch_devNo) < 0xe0000000)
                        { /* allow Unicast IPs only */
                            dev_p = (IPT_DEV_T *)tdcAllocMem(sizeof(IPT_DEV_T));
                            if (dev_p)
                            {
                               dev_p->ipAddr = my_switch_devNo | my_switch_netMask;
                               dev_p->name =
                                     (char *)tdcAllocMem(tdcStrLen(my_switch_hostLbl) + 1);
                               if (dev_p->name)
                               {
                                  tdcStrCpy(dev_p->name, my_switch_hostLbl);
                               }
                               dev_p->next = dev_root;
                               num_dev++;
                           }
                           dev_root    = dev_p;
                        }
                    } /* fi switch */
                      /*
                       * cstSta/carSta/_uicCarData_
                       */
                    else if (tdcStrCmp(tag, "uicCarData") == 0)
                    {
                        picoxml_enter();
                        while (picoxml_seek_any(tag, 100) == 0)
                        {
                            picoxml_data(data, 100);
                            if (tdcStrCmp(tag, "uicIdentifier") == 0)
                            {
                                UINT64 ll;
                                
                                ll = 0;
                                tdcSScanf(data,
                                       PRINTF_FORMAT_UINT64,
                                       &ll);
                                if (tdcN2H16(1) == 1) /* Big endian */
                                {
                                    tdcMemCpy(my_carUicInfo.uicIdentifier,
                                           ((UINT8 *)&ll) + 3,
                                           5);
                                }
                                else        /* Little Endian */
                                {
                                    int i;
                                    for (i = 0; i < 5; i++)
                                    {
                                        my_carUicInfo.uicIdentifier[4 - i] =
                                                            ((UINT8 *)&ll)[i];
                                    }
                                }
                                /* printf(PRINTF_FORMAT_UINT64 "%02x\n",
                                          ll,
                                          my_uicNo[4]); */
                            }
                            else if (tdcStrCmp(tag, "properties") == 0)
                            {
                                cst_info_hexBinary2bin(data,
                                                       my_carUicInfo.
                                                       uicCarProperties,
                                                       6);
                            }
                        }
                        picoxml_leave();
                    }
                    /*
                     * cstSta/carSta/_iptCarData_
                     */
                    else if (tdcStrCmp(tag, "iptCarData") == 0)
                    {
                        picoxml_enter();
                        while (picoxml_seek_any(tag, 100) == 0)
                        {
                            /*
                             * cstSta/carSta/iptCarData/_cstCarNo_
                             *                          _uicCarLbl_
                             *                          _hasCstOrientation_
                             *                          _carType__
                             */
                            picoxml_data(data, 100);
                            if (tdcStrCmp(tag, "cstCarNo") == 0)
                            {
                                my_carNo =
                                    (int)strtol(data, NULL, BASE_NUMBER_10);
                            }
                            else if (tdcStrCmp(tag, "carId") == 0)
                            {
                                tdcStrCpy(my_carId, data);
                                strtolower(my_carId);
                            }
                            else if (tdcStrCmp(tag, "hasCstOrientation") == 0)
                            {
                                my_carIsInverse = ((tdcStrCmp(data, "1") == 0) ||
                                                   (tdcStrCmp(data, "true") == 0))
                                                  ? 0 : 1;
                            }
                            else if (tdcStrCmp(tag, "carType") == 0)
                            {
                                tdcStrCpy(my_carType, data);
                            }
                        }
                        picoxml_leave();
                    }
                    /*
                     * cstSta/carSta/_iptCarData_
                     */
                    else if (tdcStrCmp(tag, "groups") == 0)
                    {
                        picoxml_enter();
                        cst_info_parse_multicast_devs(&car_mc_dev_root,
                                                      &num_car_mc_dev,
                                                      0xef000000,
                                                      vlan_root);
                        picoxml_leave();
                    }
                } /* elihw switch_any */
                picoxml_leave();
                /*
                 *  This is were a new Car record is to be created. Note, that
                 *  device records belonging to this car have been readily
                 *  created, since all data inside the <carSta> region
                 *  have beend read.
                 *  This allows for arbitrary sequence of subrecords,
                 *  e.g. <iptCarData> could be before, in between or
                 *  after <switch> regions.
                 */
#if 0
                printf("Car:    /%d/%-16s/%d\n",
                       my_carNo,
                       my_carId,
                       my_carIsInverse);
#endif
                /*
                 * Insert car in linked list. Arrange sequence so that
                 * low carNo leads high carNo
                 */
                car_p = (IPT_CAR_T *)tdcAllocMem(sizeof(IPT_CAR_T));
                if (car_p)
                {
                   car_p->devs          = dev_root;
                   car_p->mCasts        = car_mc_dev_root;
                   car_p->carNo         = my_carNo;
                   car_p->devNum        = num_dev;
                   car_p->mCastsNum     = num_car_mc_dev;
                   car_p->isInverse     = my_carIsInverse;
                   car_p->isLeading     = 0;
                   car_p->reqLeading    = 0;
                   car_p->isInverse2uic = 0;
                   car_p->id            = (char *)tdcAllocMem(tdcStrLen(my_carId) + 1);
                   tdcStrCpy(car_p->id, my_carId);
                   car_p->type = (char *)tdcAllocMem(tdcStrLen(my_carType) + 1);
                   if (car_p->type)
                   {
                      tdcStrCpy(car_p->type, my_carType);
                   }
                   tdcMemCpy(&(car_p->info), &my_carUicInfo, sizeof(my_carUicInfo));
                   {
                       IPT_CAR_T **pp;
                       for (pp = &car_root;
                            *pp && ((*pp)->carNo < my_carNo);
                            pp = &((*pp)->next))
                       {
                           ;
                       }
                       car_p->next = *pp;
                       *pp         = car_p;
                   }
                   num_car++;
                }

            } /* fi cstSta/carSta */

        } /* elihw cstSta seek_any()  */
        picoxml_leave();
    } /* fi cstSta */

    my_cst = (IPT_CST_T *)tdcAllocMem(sizeof(IPT_CST_T));

    if (my_cst)
    {
       my_cst->subnet       = 0x0a000000;
       my_cst->isLocal      = 1;
       my_cst->cars         = car_root;
       my_cst->mCastsCst    = cst_mc_dev_root;
       my_cst->mCastsTrn    = trn_mc_dev_root;
       my_cst->carNum       = num_car;
       my_cst->mCastsCstNum = num_cst_mc_dev;
       my_cst->mCastsTrnNum = num_trn_mc_dev;
       my_cst->isInverse    = 0;
       my_cst->cstNo        = 1;
       my_cst->id = (char*)tdcAllocMem(tdcStrLen(my_cstId) + 1);
       if (my_cst->id)
       {
          tdcStrCpy(my_cst->id, my_cstId);
       }
       tdcMemCpy(&(my_cst->info), &my_cstUicInfo, sizeof(my_cstUicInfo));
       my_cst->next = NULL;
    }
   
    tdcFclose(fp);

    return my_cst;
}

