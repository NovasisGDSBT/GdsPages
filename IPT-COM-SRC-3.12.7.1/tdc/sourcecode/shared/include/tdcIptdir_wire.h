/*
 * $Id: tdcIptdir_wire.h 11632 2010-08-16 15:33:12Z bloehr $
 */

/**
 * @file            tdcIptdir_wire.h
 *
 * @brief           IPTDir wire protocol services
 *
 * @details         Routines for generating IPTDir datasets from internal
 *                  data structures.
 *
 * @note            Project: TCMS IP-Train - Ethernet train backbone handler
 *
 * @author          Thomas Gallenkamp, PPC/EMTC
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2006.
 *
 */

#ifndef TDC_IPTDIR_WIRE_H
#define TDC_IPTDIR_WIRE_H

/*******************************************************************************
 * INCLUDES
 */

#include "tdcIptdir.h"

/****************************************************************************
 * DEFINITIONS
 */

#define PROTO_VERSION  0x02010000   /**< */

/*******************************************************************************
 * TYPEDEFS
 */

/** Dataset for process data */
typedef struct iptdir_processdataset_t
{
    UINT32 ProtocolVersion;

    UINT8  IPT_InaugStatus;
    UINT8  IPT_TopoCount;
    UINT8  UIC_InaugStatus;
    UINT8  UIC_TopoCount;

    UINT8  DynStatus;
    UINT8  DynCount;
    UINT8  TBType;
    UINT8  reserved1;

    UINT32 SizeOfIptInfo;
    UINT32 SizeOfUicInfo;
    UINT32 ServerIpAddress;
    UINT32 GatewayIpAddress;
    UINT32 reserved3;
#ifndef WIN32
} __attribute__((packed)) IPTDIR_PROCESSDATASET_T;
#else
} IPTDIR_PROCESSDATASET_T;
#endif

/*******************************************************************************
 *  GLOBALS
 */

/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

/*
 * Creates PD and MD frames based upon information in the linked lists
 * describing the train.
 */
INT32 iptdir_wire(const IPT_CST_T		*cst_root,
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
                INT32                   *uic_traindataset_actuallen);


#endif /* IPTDIR_WIRE_H */
