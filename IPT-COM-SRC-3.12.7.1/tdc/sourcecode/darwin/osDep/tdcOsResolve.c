/*
 * $Id: tdcOsResolve.c 29149 2013-09-18 12:42:31Z bloehr $
 *
 * DESCRIPTION    Functions for resolving addresses
 *
 *  AUTHOR         M.Ritz         PPC/EBT
 *
 * REMARKS        The switch DARWIN has to be set
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

#if !defined (DARWIN)
   #error "This is the Linux Version of tdcOsResolve, i.e. DARWIN has to be specified"
#endif

//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>

#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

#include "tdc.h"
#include "tdcResolve.h"

/* ---------------------------------------------------------------------------- */

#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <net/if.h>
#ifdef	HAVE_SOCKADDR_DL_STRUCT
   #include	<net/if_dl.h>
#endif

/* ---------------------------------------------------------------------------- */

#define	IFI_NAME	            16			      /* same as IFNAMSIZ in <net/if.h> */
#define	IFI_HADDR	         8			      /* allow for 64-bit EUI-64 in future */

struct ifi_info
{
   char              ifi_name[IFI_NAME];     /* interface name, null terminated */
   u_char            ifi_haddr[IFI_HADDR];   /* hardware address */
   u_short           ifi_hlen;               /* #bytes in hardware address: 0, 6, 8 */
   short             ifi_flags;              /* IFF_xxx constants from <net/if.h> */
   short             ifi_myflags;            /* our own IFI_xxx flags */
   struct sockaddr*  ifi_addr;               /* primary address */
   struct sockaddr*  ifi_brdaddr;            /* broadcast address */
   struct sockaddr*  ifi_dstaddr;            /* destination address */
   struct ifi_info*  ifi_next;               /* next of these structures */
};

#define	IFI_ALIAS	         1			      /* ifi_addr is an alias */

/* ---------------------------------------------------------------------------- */

static int               Ioctl          (const char*              pModname,
                                         int                      fd,
                                         int                      request,
                                         void*                    arg);
static /*@null@*/ char*  sock_ntop_host (const char*              pModname,
                                         const struct sockaddr*   sa,
                                         socklen_t                salen);
static /*@null@*/ char*  Sock_ntop_host (const char*              pModname,
                                         const struct sockaddr*   sa,
                                         socklen_t                salen);

static /*@null@*/ struct ifi_info*  getIfiInfo     (const char*               pModname,
                                         int                       family,
                                         int                       doAliases);
static /*@null@*/ struct ifi_info*  GetIfiInfo     (const char*               pModname,
                                         int                       family,
                                         int                       doAliases);
static void              freeIfiInfo    (/*@null@*/ struct ifi_info *);
static UINT32            ipAddrText2No  (const char*               pAddrText);

/* ---------------------------------------------------------------------------- */

static int Ioctl (const char*    pModname,
                  int            fd,
                  int            request,
                  void*          arg)
{
	int		n;

	if ( (n = ioctl (fd, request, arg)) == -1)
   {
		DEBUG_ERROR (pModname, "ioctl error");
   }

	return (n);	/* streamio of I_LIST returns value */
}

/* ---------------------------------------------------------------------------- */

static char* sock_ntop_host (const char*              pModname,
                             const struct sockaddr*   sa,
                             socklen_t                salen)
{
   static char          str[128];     /* Unix domain is largest */

   pModname = pModname;
                                      /*@ -type */
   switch (sa->sa_family)             /*@ =type */
   {
      case AF_INET:
      {
         struct sockaddr_in   *sin = (struct sockaddr_in *) sa;
                                                                               /*@ -type */
         if (inet_ntop (AF_INET, &sin->sin_addr, str, sizeof (str)) == NULL)   /*@ =type */
         {
            return(NULL);
         }
                          /*@ -statictrans */
         return (str);    /*@ =statictrans */
      }

#ifdef	IPV6
      case AF_INET6:
      {
         struct sockaddr_in6  *sin6 = (struct sockaddr_in6 *) sa;

         if (inet_ntop (AF_INET6, &sin6->sin6_addr, str, sizeof (str)) == NULL)
         {
            return(NULL);
         }

         return (str);
      }
#endif

#ifdef	AF_UNIX1
      case AF_UNIX:
      {
         struct sockaddr_un   *unp = (struct sockaddr_un *) sa;

         /* OK to have no pathname bound to the socket: happens on
            every connect() unless client calls bind() first. */
         if (unp->sun_path[0] == 0)
         {
            strcpy (str, "(no pathname bound)");
         }
         else
         {
            snprintf (str, sizeof(str), "%s", unp->sun_path);
         }

         return (str);
      }
#endif

#ifdef	HAVE_SOCKADDR_DL_STRUCT
      case AF_LINK:
      {
         struct sockaddr_dl   *sdl = (struct sockaddr_dl *) sa;

         if (sdl->sdl_nlen > 0)
         {
            snprintf (str, sizeof(str), "%*s", sdl->sdl_nlen, &sdl->sdl_data[0]);
         }
         else
         {
            snprintf (str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
         }

         return (str);
      }
#endif
      default:
      {
         (void) tdcSNPrintf (str, (unsigned int) sizeof(str),                                    /*@ -type */
                      "sock_ntop_host: unknown AF_xxx: %d, len %d", sa->sa_family, (int) salen); /*@ =type */
                              /*@ -statictrans */
         return      (str);   /*@ =statictrans */
      }
   }
                     /*@ -unreachable */
   return(NULL);     /*@ =unreachable */
}

/* ---------------------------------------------------------------------------- */

static char* Sock_ntop_host (const char*              pModname,
                             const struct sockaddr*   sa,
                             socklen_t                salen)
{
   char  *ptr;

   if ( (ptr = sock_ntop_host (pModname, sa, salen)) == NULL)
   {
      DEBUG_ERROR (pModname, "sock_ntop_host error"); /* inet_ntop() sets errno */
   }

   return (ptr);
}

/* ---------------------------------------------------------------------------- */

static struct ifi_info* getIfiInfo (const char*  pModname,  int family, int doaliases)
{
   int                     sockFd  = Socket (AF_INET, SOCK_DGRAM, 0);
   struct ifi_info*        ifihead = NULL;

   if (sockFd != TDC_INVALID_SOCKET)
   {
      struct ifi_info*        ifi;
      struct ifi_info**       ifipnext;
      int                     flags;
      int                     myflags;
      char*                   ptr;
      char*                   buf;
      char                    lastname[IFNAMSIZ];
      char*                   cptr;
      struct ifconf           ifc;
      struct ifreq*           ifr;
      struct ifreq            ifrcopy;
      struct sockaddr_in*     sinptr;
      int                     lastlen = 0;
      int                     len     = (int) (100 * sizeof (struct ifreq));   /* initial buffer size guess */

      for ( ; ; )
      {
         buf = tdcAllocMem (len);                     /*@ -usedef */
         ifc.ifc_len = len;
         ifc.ifc_buf = buf;
         if (ioctl (sockFd, SIOCGIFCONF, &ifc) < 0)   /*@ =usedef */
         {
            if (errno != EINVAL || lastlen != 0)
            {
               DEBUG_ERROR (pModname, "ioctl error");
            }
         }
         else
         {
            if (ifc.ifc_len == lastlen)
            {
               break;      /* success, len has not changed */
            }

            lastlen = ifc.ifc_len;
         }

         len += 10 * sizeof(struct ifreq);   /* increment */
         tdcFreeMem (buf);
      }

      ifipnext    = &ifihead;
      lastname[0] = '\0';
   /* end getIfiInfo1 */
                                                      /*@ -usedef */
      for (ptr = buf; ptr < buf + ifc.ifc_len;)       /*@ =usedef */
      {
         ifr = (struct ifreq *) ptr;

   #ifdef	HAVE_SOCKADDR_SA_LEN
         len = max (sizeof(struct sockaddr), ifr->ifr_addr.sa_len);
   #else
                                             /*@ -type */
         switch (ifr->ifr_addr.sa_family)    /*@ =type */
         {
   #ifdef	IPV6
            case AF_INET6:
               len = sizeof(struct sockaddr_in6);
               break;
   #endif
            case AF_INET:
            default:
               len = (int) sizeof(struct sockaddr);
               break;
         }
   #endif	/* HAVE_SOCKADDR_SA_LEN */
                                                /*@ -type */
         ptr += sizeof(ifr->ifr_name) + len;    /*@ =type */   /* for next one in buffer */
                                                /*@ -type */
         if (ifr->ifr_addr.sa_family != family) /*@ =type */
         {
            continue;   /* ignore if not desired address family */
         }

         myflags = 0;                                          /*@ -type */
         if ( (cptr = strchr (ifr->ifr_name, ':')) != NULL)    /*@ =type */
         {
            *cptr = '\0';     /* replace colon will null */
         }
                                                               /*@ -type */
         if (strncmp(lastname, ifr->ifr_name, IFNAMSIZ) == 0)  /*@ =type */
         {
            if (doaliases == 0)
            {
               continue;   /* already processed this interface */
            }

            myflags = IFI_ALIAS;
         }
                                                      /*@ -type */
         memcpy (lastname, ifr->ifr_name, IFNAMSIZ);  /*@ =type */
                           /*@ -nullderef */
         ifrcopy = *ifr;   /*@ =nullderef */
         (void) Ioctl (pModname, sockFd, SIOCGIFFLAGS, &ifrcopy);
         flags = ifrcopy.ifr_flags;

         if ((flags & IFF_UP) == 0)
         {
            continue;   /* ignore if interface not up */
         }

         ifi              = tdcAllocMem ((UINT32) (1 * (sizeof(struct ifi_info) + 3)));
         //ifi              = Calloc (1, sizeof(struct ifi_info));
         *ifipnext        = ifi;             /*@ -nullderef */          /* prev points to this new one */
         ifipnext         = &ifi->ifi_next;                             /* pointer to next one goes here */

         ifi->ifi_flags   = (short) flags;                              /* IFF_xxx values */
         ifi->ifi_myflags = (short) myflags;             /*@ -type */   /* IFI_xxx values */
         memcpy(ifi->ifi_name, ifr->ifr_name, IFI_NAME); /*@ =type */
         ifi->ifi_name[IFI_NAME-1] = '\0';  /*@ =nullderef */

   /* end getIfiInfo2 */
                                             /*@ -type */
         switch (ifr->ifr_addr.sa_family)    /*@ =type */
         {
            case AF_INET:
            {                                                                             /*@ -type -nullderef */
               sinptr = (struct sockaddr_in *) &ifr->ifr_addr;
               if (ifi->ifi_addr == NULL)
               {
                  ifi->ifi_addr = tdcAllocMem (1 * (sizeof(struct sockaddr_in) + 3));
                  // ifi->ifi_addr = Calloc(1, sizeof(struct sockaddr_in));
                                                                                         /*@ -nullpass */
                  memcpy(ifi->ifi_addr, sinptr, sizeof(struct sockaddr_in));             /*@ =type =nullderef =nullpass */

   #ifdef	SIOCGIFBRDADDR
                  if (flags & IFF_BROADCAST)
                  {
                     Ioctl (pModname, sockFd, SIOCGIFBRDADDR, &ifrcopy);
                     sinptr = (struct sockaddr_in *) &ifrcopy.ifr_broadaddr;
                     ifi->ifi_brdaddr = tdcAllocMem (1 * (sizeof(struct sockaddr_in) + 3));
                     // ifi->ifi_brdaddr = Calloc(1, sizeof(struct sockaddr_in));
                     memcpy(ifi->ifi_brdaddr, sinptr, sizeof(struct sockaddr_in));
                  }
   #endif

   #ifdef	SIOCGIFDSTADDR
                  if (flags & IFF_POINTOPOINT)
                  {
                     Ioctl (pModname, sockFd, SIOCGIFDSTADDR, &ifrcopy);
                     sinptr = (struct sockaddr_in *) &ifrcopy.ifr_dstaddr;
                     ifi->ifi_dstaddr = tdcAllocMem (1 * (sizeof(struct sockaddr_in) + 3));
                     // ifi->ifi_dstaddr = Calloc(1, sizeof(struct sockaddr_in));
                     memcpy(ifi->ifi_dstaddr, sinptr, sizeof(struct sockaddr_in));
                  }
   #endif
               }
               break;
            }
            default:
            {
               break;
            }
         }
      }

      (void) CloseSocket (sockFd);

      tdcFreeMem (buf);
   }

   return (ifihead);  /* pointer to first structure in linked list */
}
/* end getIfiInfo3 */

/* ---------------------------------------------------------------------------- */

static void freeIfiInfo(struct ifi_info *ifihead)
{
   struct ifi_info   *ifi, *ifinext;

   for (ifi = ifihead; ifi != NULL; ifi = ifinext)
   {
      if (ifi->ifi_addr != NULL)
      {
         tdcFreeMem (ifi->ifi_addr);
      }

      if (ifi->ifi_brdaddr != NULL)
      {
         tdcFreeMem (ifi->ifi_brdaddr);
      }

      if (ifi->ifi_dstaddr != NULL)
      {
         tdcFreeMem (ifi->ifi_dstaddr);
      }

      ifinext = ifi->ifi_next;      /* can't fetch ifi_next after free() */
      tdcFreeMem (ifi);              /* the ifi_info{} itself */
   }
}
/* end freeIfiInfo */

/* ---------------------------------------------------------------------------- */

static struct ifi_info * GetIfiInfo (const char* pModname, int family, int doaliases)
{
   struct ifi_info   *ifi;

   if ((ifi = getIfiInfo (pModname, family, doaliases)) == NULL)
   {
      DEBUG_ERROR (pModname, "getIfiInfo error");
   }

   return(ifi);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

UINT32 myPrintAddrs1 (const char*     pModname,
                      UINT32          adrrCnt,
                      UINT32          ipAddr[])                  /* output */
{
   struct ifi_info*  ifi;
   struct ifi_info*  ifihead;
   int               family    = AF_INET;
   int               doaliases = 1;

   TDC_UNUSED (adrrCnt)
   TDC_UNUSED (ipAddr)

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
            printf ("%s%x", (i == ifi->ifi_hlen) ? "  " : ":", (int) (*ptr++));
         } while (--i > 0);

         printf("\n");
      }

      if ((sa = ifi->ifi_addr) != NULL)
      {                                                                                      /*@ -nullpass */
         printf("  IP addr: %s\n", Sock_ntop_host (pModname, sa, (socklen_t) sizeof(*sa)));  /*@ =nullpass */
      }

      if ((sa = ifi->ifi_brdaddr) != NULL)
      {                                                                                            /*@ -nullpass */
         printf("  broadcast addr: %s\n", Sock_ntop_host (pModname, sa, (socklen_t) sizeof(*sa))); /*@ =nullpass */
      }

      if ((sa = ifi->ifi_dstaddr) != NULL)
      {                                                                                               /*@ -nullpass */
         printf("  destination addr: %s\n", Sock_ntop_host (pModname, sa, (socklen_t) sizeof(*sa)));  /*@ =nullpass */
      }
   }

   freeIfiInfo (ifihead);

   return (0);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

static UINT32 ipAddrText2No (const char*  pAddrText)
{
   UINT32      byte3;
   UINT32      byte2;
   UINT32      byte1;
   UINT32      byte0;

   if (sscanf (pAddrText, "%d.%d.%d.%d", &byte3, &byte2, &byte1, &byte0) == 4)
   {
      return (    (byte3 << 24)
                + (byte2 << 16)
                + (byte1 << 8)
                + (byte0)
             );
   }

   return (0);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

UINT32 tdcGetOwnIpAddrs (const char*      pModname,
                         UINT32           maxAddrCnt,
                         UINT32           ipAddrs[])            /* output */
{
    UINT32               addrCnt     = 0;

    int  success;
    struct ifaddrs * addrs;
    struct ifaddrs * cursor;
    const struct sockaddr_dl * dlAddr;
    const unsigned char* base;
    char macAddress[32];
    int i;
    int	count = 0;
        
    success = getifaddrs(&addrs) == 0;
    if (success)
    {
        cursor = addrs;
        while (cursor != 0 && count < maxAddrCnt)
        {
            if (cursor->ifa_addr && (cursor->ifa_addr->sa_family == AF_INET) &&
            	strncmp(cursor->ifa_name, "lo0", 3) != 0)
            {
            	ipAddrs[count] = ipAddrText2No (Sock_ntop_host (pModname, cursor->ifa_addr, (socklen_t) sizeof(*cursor->ifa_addr))); /*@ =nullpass */
                DEBUG_INFO4 (pModname, "found local IP-Addr: %u.%u.%u.%u\n",
                                 (ipAddrs[count] >> 24) & 0xFF,
                                 (ipAddrs[count] >> 16) & 0xFF,
                                 (ipAddrs[count] >> 8)  & 0xFF,
                                  ipAddrs[count]        & 0xFF);
                count++;
            }
            cursor = cursor->ifa_next;
        }
        
        freeifaddrs(addrs);
    }    
    return count;
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcAssertMCsupport (const char*      pModname,
                               UINT32           ipAddr)
{
   T_TDC_BOOL           bOk     = FALSE;
   T_TDC_SOCKET         serverSockFd;

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
         if (setsockopt (serverSockFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ipMReq, sizeof (ipMReq)) == 0)
         {
            /* Successfully joined to grpAll.aCar */
            /* We inmmediately leave the group, since it is IPTCom's job to handle the sockets */

            DEBUG_INFO (pModname, "Successfully joined to Multicastgroup 'grpAll.aCar'");

            if (setsockopt (serverSockFd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &ipMReq, sizeof (ipMReq)) != 0)
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
         DEBUG_WARN1 (pModname, "Failed to bind socket for testing of Multicastsupport - err = %d", errno);
      }

      (void) tdcCloseSocket (pModname, &serverSockFd);         // socket no longer needed
   }
   else
   {
      DEBUG_WARN1 (pModname, "Failed to open socket for testing of Multicastsupport - err = %d", errno);
   }

   return (bOk);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcGetHostByAddr (const char*         pModname,
                             UINT32              ipAddr,
                             UINT32              hostNameLen,
                             char*               pHostName)
{
   T_TDC_BOOL           bOk = FALSE;
   struct in_addr       inAddr;
   char                 ipAddrStr[50];

   (void) tdcSNPrintf (ipAddrStr, (unsigned int) sizeof (ipAddrStr),
                "%d.%d.%d.%d", (ipAddr >> 24) & 0xFF, (ipAddr >> 16) & 0xFF, (ipAddr >> 8) & 0xFF, ipAddr & 0xFF);

   ipAddrStr[sizeof (ipAddrStr) - 1] = '\0';
   pHostName[0]                      = '\0';

   if (inet_aton (ipAddrStr, &inAddr) != 0)
   {
      struct hostent*      pHostEnt;

      if ((pHostEnt = gethostbyaddr ((char *) &inAddr, (socklen_t) sizeof (inAddr), AF_INET)) != NULL)
      {
         char     text[300];
                                                                                             /*@ -type */
         (void) tdcStrNCpy  (pHostName, pHostEnt->h_name, hostNameLen);                      /*@ =type */
         (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "gethostbyaddr (%s) = %s", ipAddrStr, pHostName);
         text[sizeof (text) - 1] = '\0';

         DEBUG_INFO (pModname, text);

         bOk = TRUE;
      }
   }

   return (bOk);
}
