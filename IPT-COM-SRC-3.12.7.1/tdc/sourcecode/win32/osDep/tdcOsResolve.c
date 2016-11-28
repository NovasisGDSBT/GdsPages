/*
 * $Id: tdcOsResolve.c 11719 2010-11-03 14:49:17Z gweiss $
 *
 * DESCRIPTION    Functions for resolving addresses
 *
 *  AUTHOR         M.Ritz         PPC/EBT
 *
 * REMARKS        The switch WIN32 has to be set
 *
 * DEPENDENCIES
 *
 * MODIFICATIONS (log starts 23-Aug-2010)
 *
 * INTERNAL (Gerhard Weiss, 2010-11-03
 *      iphlpapi.h is now a system lib. To reflect this, use <..> for include.
 *
 * CR-684 (Gerhard Weiss, 23-Aug-2010)
 *
 *      For Windows, the own IP address was only recognized if only a single
 *      address per interface was configured. Removed limitation.
 *
 * All rights reserved. Reproduction, modification, use or disclosure
 * to third parties without express authority is forbidden.
 * Copyright Bombardier Transportation GmbH, Germany, 2002-2010.
 */


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#if !defined (WIN32)
   #error "This is the Windows Version of tdcOsResolve, i.e. WIN32 has to be specified"
#endif

//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>	/* TCP and UDP specific defines (socket options) */

#include "tdc.h"
#include "tdcResolve.h"

/* ---------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

UINT32 myPrintAddrs1 (const char*     pModname,
                      UINT32          adrrCnt,
                      UINT32          ipAddr[])                  /* output */
{
#if defined (O_XXXXX)
   struct ifi_info*  ifi;
   struct ifi_info*  ifihead;
   int               family    = AF_INET;
   int               doaliases = 1;

   for (ifihead = ifi = GetIfiInfo (pModname, family, doaliases); ifi != NULL; ifi = ifi->ifi_next)
   {
      struct sockaddr*  sa;
      int               i;

      printf ("%s: <", ifi->ifi_name);
                                                                           /* *INDENT-OFF* */
      if (ifi->ifi_flags & IFF_UP)           printf("UP ");
      if (ifi->ifi_flags & IFF_BROADCAST)    printf("BCAST ");
      if (ifi->ifi_flags & IFF_MULTICAST)    printf("MCAST ");
      if (ifi->ifi_flags & IFF_LOOPBACK)     printf("LOOP ");
      if (ifi->ifi_flags & IFF_POINTOPOINT)  printf("P2P ");
      printf(">\n");

      if ( (i = ifi->ifi_hlen) > 0)
      {
         u_char*           ptr = ifi->ifi_haddr;

         do
         {
            printf ("%s%x", (i == ifi->ifi_hlen) ? "  " : ":", *ptr++);
         } while (--i > 0);

         printf("\n");
      }

      if ((sa = ifi->ifi_addr) != NULL)
      {
         printf("  IP addr: %s\n", Sock_ntop_host (pModname, sa, sizeof (*sa)));
      }

      if ((sa = ifi->ifi_brdaddr) != NULL)
      {
         printf("  broadcast addr: %s\n", Sock_ntop_host (pModname, sa, sizeof (*sa)));
      }

      if ((sa = ifi->ifi_dstaddr) != NULL)
      {
         printf("  destination addr: %s\n", Sock_ntop_host (pModname, sa, sizeof(*sa)));
      }
   }

   freeIfiInfo (ifihead);
#else
   TDC_UNUSED (pModname)
   TDC_UNUSED (adrrCnt)
   ipAddr[0] = 0;
#endif


   return (0);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#include <iphlpapi.h>

UINT32 tdcGetOwnIpAddrs (const char*      pModname,
                         UINT32           maxAddrCnt,
                         UINT32           ipAddrs[])            /* output */
{
   UINT32               addrCnt      = 0;
   PIP_ADAPTER_INFO     pAdapterInfo = (IP_ADAPTER_INFO *) malloc (sizeof (IP_ADAPTER_INFO));
   DWORD                dwRetVal     = 0;
   ULONG                ulOutBufLen  = (ULONG) sizeof (IP_ADAPTER_INFO);

   ipAddrs[0] = 0;

   // Make an initial call to GetAdaptersInfo to get
   // the necessary size into the ulOutBufLen variable
   if (GetAdaptersInfo (pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
   {                                               /*@ -compdestroy@*/
      free (pAdapterInfo);                         /*@ =compdestroy@*/
      pAdapterInfo = (IP_ADAPTER_INFO *) malloc ((size_t) ulOutBufLen);
   }

   if ((dwRetVal = GetAdaptersInfo (pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
   {
      PIP_ADAPTER_INFO        pAdapter;

      for (pAdapter = pAdapterInfo; pAdapter != NULL; pAdapter = pAdapter->Next)
      {
         UINT32      byte0, byte1, byte2, byte3;
              // CR-684 move through the list of possible adresses
         IP_ADDR_STRING * pIpAddrString = &pAdapter->IpAddressList;
         
         do {
                                                                     /*@ -type @*/
             if (sscanf (pIpAddrString->IpAddress.String,           /*@ =type @*/
                         "%d.%d.%d.%d",
                         &byte3, &byte2, &byte1, &byte0)             == 4)
             {
                 DEBUG_INFO4 (pModname, "found local IP-Addr: %3d.%3d.%3d.%3d", byte3, byte2, byte1, byte0);
                 
                 if (addrCnt < maxAddrCnt)
                 {
                     ipAddrs[addrCnt] = (byte3 << 24) + (byte2 <<16) + (byte1 <<8) + byte0;
                     addrCnt++;
                 }
                 else
                 {
                     DEBUG_WARN1 (pModname, "Too many IP-Addresses to evaluate - %d exceeded", maxAddrCnt);
                 }
             }
             else
             {
                 DEBUG_WARN (pModname, "Error calling sscanf in tdcGetOwnIpAddrs");
             }
         } while (NULL != (pIpAddrString = pIpAddrString->Next));
      }
   }
   else
   {
      DEBUG_WARN (pModname, "Call to GetAdaptersInfo failed");
   }

   if (pAdapterInfo != NULL)
   {                          /*@ -compdestroy@*/
      free (pAdapterInfo);    /*@ =compdestroy@*/
   }

   return (addrCnt);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcAssertMCsupport (const char*      pModname,
                               UINT32           ipAddr)
{
   T_TDC_BOOL             bOk     = FALSE;
   T_TDC_SOCKET           serverSockFd;

   if (tdcUdpSocket4 (pModname, &serverSockFd))
   {
      struct sockaddr_in     serverAddr;

      tdcMemClear (&serverAddr, (UINT32) sizeof (serverAddr));

      serverAddr.sin_family      = AF_INET;
      serverAddr.sin_port        = htons (TDC_IPC_SERVER_PORT);
      serverAddr.sin_addr.s_addr = htonl (ipAddr);

      if (Bind (serverSockFd, (struct sockaddr *) &serverAddr, (int) sizeof (serverAddr)))
      {
         UINT32               grpAllACar = MC_ADDR (239, 0, 0, 0);
         struct ip_mreq       ipMReq;
   
         /* Fill in the argument structure to join the multicast group */   
         ipMReq.imr_multiaddr.s_addr = htonl (grpAllACar);
   
         /* Unicast interface addr from which to receive the multicast packets */
         ipMReq.imr_interface.s_addr = htonl (ipAddr);    
         ipMReq.imr_interface.s_addr = htonl (INADDR_ANY);  /* inet_addr (ifAddr);  */  /* use INADDR_ANY - socket already bound to ipAddr */
   
         /* Now check if it is possible to join the Multicastgroup associated with grpAll.aCar */
         if (setsockopt (serverSockFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *) &ipMReq, sizeof (ipMReq)) == 0)
         {
            /* Successfully joined to grpAll.aCar */
            /* We inmmediately leave the group, since it is IPTCom's job to handle the sockets */
   
            DEBUG_INFO (pModname, "Successfully joined to Multicastgroup 'grpAll.aCar'");
   
            if (setsockopt (serverSockFd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char *) &ipMReq, sizeof (ipMReq)) != 0)
            {
               DEBUG_WARN1 (pModname, "Failed to leave Multicastgroup - err = %d", WSAGetLastError ());
            }
            else
            {
               DEBUG_INFO (pModname, "Successfully left      Multicastgroup 'grpAll.aCar'");
            }
   
            bOk = TRUE;
         }
         else
         {
            DEBUG_WARN1 (pModname, "Failed to join Multicastgroup - err = %d - missing multicast route?", WSAGetLastError ());
         }
      }
      else
      {
         DEBUG_WARN1 (pModname, "Failed to bind socket for testing of Multicastsupport - err = %d", WSAGetLastError ());
      }

      (void) tdcCloseSocket (pModname, &serverSockFd);         // socket no longer needed
   }
   else
   {
      DEBUG_WARN1 (pModname, "Failed to open socket for testing of Multicastsupport - err = %d", WSAGetLastError ());
   }

   return (bOk);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcGetHostByAddr (const char*         pModName,
                             UINT32              ipAddr,
                             UINT32              hostNameLen,
                             char*               pHostName)
{
   T_TDC_BOOL           bOk = FALSE;

#if defined (O_XXXXX)

   struct in_addr       inAddr;
   char                 ipAddrStr[50];

   tdcSNPrintf (ipAddrStr, (UINT32) sizeof (ipAddrStr),
                "%d.%d.%d.%d", (ipAddr >> 24) & 0xFF, (ipAddr >> 16) & 0xFF, (ipAddr >> 8) & 0xFF, ipAddr & 0xFF);
   ipAddrStr[sizeof (ipAddrStr) - 1] = '\0';

   if (inet_aton (ipAddrStr, &inAddr) != 0)
   {
      struct hostent*      pHostEnt;

      if ((pHostEnt = gethostbyaddr ((char *) &inAddr, sizeof (inAddr), AF_INET)) != NULL)
      {
         char     text[300];

         (void) tdcStrNCpy (pHostName, pHostEnt->h_name, hostNameLen);
         tdcSNPrintf (text, sizeof (text), "gethostbyaddr (%s) = %s", ipAddrStr, pHostName);
         text[sizeof (text) - 1] = 0;

         DEBUG_INFO (pModName, text);

         bOk = TRUE;
      }
   }
#else
   TDC_UNUSED (pModName)
   TDC_UNUSED (ipAddr)
   TDC_UNUSED (hostNameLen)
   pHostName[0] = '\0';
#endif
   return (bOk);
}
