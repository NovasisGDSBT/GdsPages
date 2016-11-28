
/*
 * $Id: tdcOsResolve.c 11308 2009-08-31 12:51:00Z mritz $
 *
 * DESCRIPTION    Functions for resolving addresses
 *
 *  AUTHOR         M.Ritz         PPC/EBT
 *
 * REMARKS        The switch VXWORKS has to be set
 *
 * DEPENDENCIES
 *
 *
 * All rights reserved. Reproduction, modification, use or disclosure
 * to third parties without express authority is forbidden.
 * Copyright Bombardier Transportation GmbH, Germany, 2002.
 */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#if !defined (VXWORKS)
   #error "This is the Windows Version of tdcOsResolve, i.e. VXWORKS has to be specified"
#endif

//-----------------------------------------------------------------------------

#include <vxWorks.h>
#include <endLib.h>
#include <inetLib.h>
#include <sockLib.h>

#define TDC_BASETYPES_H    /* Use original VxWorks Header File definitions */
#include "tdc.h"
#include "tdcResolve.h"

// -----------------------------------------------------------------------------

#define MAX_INTERFACES                 20

// Externals from CSS3 Header file <iflib.h> - should work for CSS2 as well
extern STATUS  ifIndexToIfName (unsigned short     ifIndex, 
                                char*              pIfName);
extern STATUS 	ifAddrGet       (char*              pIfName, 
                                char*              pIfAddr);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

static UINT32 ipAddrText2No (const char*     pModname,
                             const char*     pAddrText)
{
   int         byte3;
   int         byte2;
   int         byte1;
   int         byte0;

   if (sscanf (pAddrText, "%d.%d.%d.%d", &byte3, &byte2, &byte1, &byte0) == 4)
   {
      return (    (byte3 * 256 * 256 * 256)
                + (byte2 * 256 * 256)
                + (byte1 * 256)
                + (byte0)
             );
   }
   else
   {
      char  text[100];

      (void) tdcSNPrintf (text, (UINT32) sizeof (text), 
                          "Unable to retrieve IP-Addr from ifAddrGet() - %s", pAddrText);
      text[sizeof (text) - 1] = '\0';

      DEBUG_WARN (pModname, text);
   }

   return (0);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

T_TDC_BOOL tdcGetHostByAddr (const char*         pModName,
                             UINT32              ipAddr,
                             UINT32              hostNameLen,
                             char*               pHostName)
{
   T_TDC_BOOL           bOk = FALSE;

   TDC_UNUSED (pModName)
   TDC_UNUSED (ipAddr)
   TDC_UNUSED (hostNameLen)

   pHostName[0] = '\0';

   return (bOk);
}

// -----------------------------------------------------------------------------

UINT32 tdcGetOwnIpAddrs (const char*      pModname,
                         UINT32           maxAddrCnt,
                         UINT32           ipAddrs[])            /* output */
{
   int         ifIdx;
   UINT32      addrCnt = 0;

   ipAddrs[0] = (UINT32) 0;

   for (ifIdx = 1; (ifIdx <= MAX_INTERFACES)  &&  (addrCnt < maxAddrCnt); ifIdx++)
   {
      char        ifName [END_NAME_MAX + 1];

      if (ifIndexToIfName (ifIdx, ifName) == OK)
      {
         char       ipAddr [INET_ADDR_LEN];

         if (ifAddrGet (ifName, ipAddr) == OK)
         {
            ipAddrs[addrCnt] = ipAddrText2No (pModname, ipAddr);

            DEBUG_INFO4 (pModname, "found local IP-Addr: %3d.%3d.%3d.%3d",
                                   (ipAddrs[addrCnt] >> 24) & 0xFF,
                                   (ipAddrs[addrCnt] >> 16) & 0xFF,
                                   (ipAddrs[addrCnt] >> 8)  & 0xFF,
                                    ipAddrs[addrCnt]        & 0xFF);

            addrCnt++;
         }
      }
   }

   if (addrCnt == 0)
   {
      DEBUG_WARN (pModname, "Failed to call ifAddrGet()");
   }

   return (addrCnt);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcAssertMCsupport (const char*      pModname,
                               UINT32           ipAddr)
{
   T_TDC_BOOL           bOk     = FALSE;
   T_TDC_SOCKET         serverSockFd;

   if (tdcUdpSocket4 (pModname, &serverSockFd))
   {
      T_TDC_SOCK_ADDR_IN        serverAddr;

      tdcMemClear (&serverAddr, (UINT32) sizeof (T_TDC_SOCK_ADDR_IN));

      serverAddr.sin_len         = (u_char) sizeof (T_TDC_SOCK_ADDR_IN);   /* vxWorks V5.4++ */
      serverAddr.sin_family      = AF_INET;
      serverAddr.sin_port        = htons (TDC_IPC_SERVER_PORT);
      serverAddr.sin_addr.s_addr = htonl (ipAddr);

      if (Bind (serverSockFd, (T_TDC_SOCK_ADDR *) &serverAddr, (int) sizeof (serverAddr)))
      {
         UINT32               grpAllACar = MC_ADDR (239, 0, 0, 0);
         struct ip_mreq       ipMReq;

         /* Fill in the argument structure to join the multicast group */   
         ipMReq.imr_multiaddr.s_addr = htonl (grpAllACar);

         /* Unicast interface addr from which to receive the multicast packets */
         ipMReq.imr_interface.s_addr = htonl (ipAddr);    
         ipMReq.imr_interface.s_addr = htonl (INADDR_ANY);  /* inet_addr (ifAddr);  */  /* use INADDR_ANY - socket already bound to ipAddr */

         /* Now check if it is possible to join the Multicastgroup associated with grpAll.aCar */
         if (setsockopt (serverSockFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &ipMReq, sizeof (ipMReq)) == OK)
         {
            /* Successfully joined to grpAll.aCar */
            /* We inmmediately leave the group, since it is IPTCom's job to handle the sockets */

            DEBUG_INFO (pModname, "Successfully joined to Multicastgroup 'grpAll.aCar'");


            if (setsockopt (serverSockFd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &ipMReq, sizeof (ipMReq)) != OK)
            {
               DEBUG_WARN1 (pModname, "Failed to leave Multicastgroup (err = %d)", errno);

            }
            else
            {
               DEBUG_INFO (pModname, "Successfully left      Multicastgroup 'grpAll.aCar'");
            }

            bOk = TRUE;
         }
         else
         {
            DEBUG_WARN1 (pModname, "Failed to join Multicastgroup (err = %d) - missing multicast route?", errno);
         }
      }
      else
      {
         DEBUG_WARN (pModname, "Failed to bind socket for testing of Multicastsupport");
      }

      (void) tdcCloseSocket (pModname, &serverSockFd);         // socket no longer needed
   }
   else
   {
      DEBUG_WARN (pModname, "Failed to open socket for testing of Multicastsupport");
   }

   return (bOk);
}




