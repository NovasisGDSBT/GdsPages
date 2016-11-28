/*
 * $Id: tdcOsSock.c 11184 2009-05-14 13:28:06Z mritz $
 *
 * DESCRIPTION    Functions for managing the ethernet Interface
 *
 *  AUTHOR         M.Ritz         PPC/EBT
 *
 * REMARKS        The switch WIN32 has to be set
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

#if !defined (WIN32)
   #error "This is the Windows Version of tdcOsSock, i.e. WIN32 has to be specified"
#endif

//-----------------------------------------------------------------------------

#include <windows.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "tdc.h"

/* ---------------------------------------------------------------------------- */


#define O_IPV4_SUPPORT_ONLY


/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL Setsockopt (T_TDC_SOCKET                sockFd,
                              int                         level,
                              int                         optName,
                              const void*                 pOptVal,
                              T_TDC_SOCKLEN_T             optLen);

// ----------------------------------------------------------------------------

static T_TDC_BOOL Setsockopt (T_TDC_SOCKET       sockFd,
                              int                level,
                              int                optName,
                              const void*        pOptVal,
                              T_TDC_SOCKLEN_T    optLen)
{
	return (setsockopt (sockFd, level, optName, pOptVal, (socklen_t) optLen) == 0);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

void tdcInitTcpIpWinsock (const char* pModname)
{
   static T_TDC_BOOL       bInitDone = FALSE;

   if (!bInitDone)
   {
	   WORD           reqVer;
      WSADATA        wsaData;

	   reqVer = MAKEWORD (2, 2);

      if (WSAStartup (reqVer, &wsaData) != 0)
      {
         DEBUG_WARN1 (pModname, "Error calling WSAStartup() - errCode=%d\n", WSAGetLastError  ());
      }
      else
      {
         bInitDone = TRUE;
      }
   }
}

/* ---------------------------------------------------------------------------- */

UINT8 tdcH2N8 (UINT8    var8)
{
   return (var8);
}

/* ---------------------------------------------------------------------------- */

UINT16 tdcH2N16 (UINT16    var16)
{
   return ((UINT16) (htons (var16)));
}

/* ---------------------------------------------------------------------------- */

UINT32 tdcH2N32 (UINT32    var32)
{
   return ((UINT32) (htonl (var32)));
}

/* ---------------------------------------------------------------------------- */

UINT8 tdcN2H8 (UINT8    var8)
{
   return (var8);
}

/* ---------------------------------------------------------------------------- */

UINT16 tdcN2H16 (UINT16    var16)
{
   return ((UINT16) (ntohs (var16)));
}

/* ---------------------------------------------------------------------------- */

UINT32 tdcN2H32 (UINT32    var32)
{
   return ((UINT32) (ntohl (var32)));
}

/* ---------------------------------------------------------------------------- */

int tdcGet_AF_INET (void)
{
   return (AF_INET);
}

/* ---------------------------------------------------------------------------- */

int tdcGet_SOCK_DGRAM (void)
{
   return (SOCK_DGRAM);
}

/* ---------------------------------------------------------------------------- */

int tdcGet_SOCK_STREAM (void)
{
   return (SOCK_STREAM);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

void tdcInitCli_SERV_ADDR (const char*             pModname,
                           T_TDC_SOCK_ADDR_IN*     pServAddr,
                           UINT32*                 pLen,
                           UINT16                  portNo)
{
   if (*pLen < (UINT32) sizeof (struct sockaddr_in))
   {
      DEBUG_ERROR2 (pModname, "Size of buffer for 'struct sockaddr_in' too small (%d <-> %d)",
                    *pLen, sizeof (struct sockaddr_in));
   }

   *pLen = (UINT32) sizeof (struct sockaddr_in);

   /*@ -type */
   ((struct sockaddr_in *) pServAddr)->sin_family = AF_INET;
   ((struct sockaddr_in *) pServAddr)->sin_port   = htons (portNo);
   /*@ =type */
}

/* ---------------------------------------------------------------------------- */

void tdcInitSrv_SERV_ADDR (const char*             pModname,
                           T_TDC_SOCK_ADDR_IN*     pServAddr,
                           UINT32*                 pLen,
                           UINT16                  portNo)
{
   if (*pLen < (UINT32) sizeof (struct sockaddr_in))
   {
      DEBUG_ERROR2 (pModname, "Size of buffer for 'struct sockaddr_in' too small (%d <-> %d)", *pLen, sizeof (struct sockaddr_in));
   }

   *pLen = (UINT32) sizeof (struct sockaddr_in);

   /*@ -type */
   ((struct sockaddr_in *) pServAddr)->sin_family      = AF_INET;
   ((struct sockaddr_in *) pServAddr)->sin_port        = htons (portNo);
   ((struct sockaddr_in *) pServAddr)->sin_addr.s_addr = htonl (INADDR_ANY);
   /*@ =type */
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_SOCKET Socket (int    family,
                     int    type,
                     int    protocol)
{
   SOCKET      socketFd = socket (family, type, protocol);

   return ((socketFd == INVALID_SOCKET) ? (TDC_INVALID_SOCKET) : (socketFd));
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL setSockTimeoutOpt (const char*          pModname,
                              T_TDC_SOCKET         sockFd,
                              T_TDC_TIMEVAL        timeout,
                              int                  optName)
{
   struct timeval    timevalue;

   timevalue.tv_sec  = timeout / 1000;
   timevalue.tv_usec = (timeout % 1000) * 1000;

   pModname = pModname;

   return (Setsockopt (sockFd, SOL_SOCKET, optName, &timevalue, (T_TDC_SOCKLEN_T) sizeof (timevalue)));
}

// ----------------------------------------------------------------------------

T_TDC_BOOL setLingerOpt (const char*            pModname,
                         T_TDC_SOCKET           sockFd,
                         T_TDC_BOOL             lingerOn,
                         UINT32                 seconds)
{
   T_TDC_BOOL     bResult = FALSE;

   if (seconds < MAX_INT32)
   {
      struct linger       lingerOpt;

      lingerOpt.l_linger = (u_short) seconds;
      lingerOpt.l_onoff  = (u_short) lingerOn;

      if (Setsockopt (sockFd, SOL_SOCKET, SO_LINGER, &lingerOpt, (T_TDC_SOCKLEN_T) sizeof (lingerOpt)))
      {
         bResult = TRUE;;
         /*DEBUG_INFO2 (pModname, "Successfully called SetSockOpt(Linger) - On(%d) - time(%d)", (int) lingerOn, (int) seconds);*/
      }
      else
      {
         DEBUG_WARN3 (pModname, "Failed to call SetSockOpt(Linger) - On(%d) - time(%d) - errCode=%d",
                                 (int) lingerOn, (int) seconds, WSAGetLastError ());
      }
   }

   return (bResult);
}

// ----------------------------------------------------------------------------

T_TDC_BOOL setReuseAddrOpt (const char*         pModname,
                            T_TDC_SOCKET        sockFd,
                            T_TDC_BOOL          bReuseAddr)
{
   T_TDC_REUSEADDR     reuseAddr = bReuseAddr ? (1) : (0);

   pModname = pModname;

   if (Setsockopt (sockFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, (T_TDC_SOCKLEN_T) sizeof (reuseAddr)))
   {
      /*DEBUG_INFO1 (pModname, "Successfully called SetSockOpt(ReuseAddr) - reuse(%d)!", bReuseAddr); */
      return (TRUE);
   }

   DEBUG_WARN2 (pModname, "Failed to call SetSockOpt(ReuseAddr) - reuse(%d) - errCode=%d", bReuseAddr, WSAGetLastError());

   return (FALSE);
}

// ----------------------------------------------------------------------------

T_TDC_BOOL setRecvTimeoutOpt (const char*          pModname,
                              T_TDC_SOCKET         sockFd,
                              T_TDC_TIMEVAL        timeout)
{
   if (setSockTimeoutOpt (pModname, sockFd, timeout, SO_RCVTIMEO))
   {
      /*DEBUG_INFO1 (pModname, "Successfully called SetSockOpt(RecvTimeout) - (%d)", (int) timeout); */
      return (TRUE);
   }

   DEBUG_WARN2 (pModname, "Failed to call SetSockOpt(RecvTimeout) - (%d), errCode=%d", (int) timeout, WSAGetLastError());

   return (FALSE);
}

// ----------------------------------------------------------------------------

T_TDC_BOOL setSendTimeoutOpt (const char*       pModname,
                              T_TDC_SOCKET      sockFd,
                              T_TDC_TIMEVAL     timeout)
{
   if (setSockTimeoutOpt (pModname, sockFd, timeout, SO_SNDTIMEO))
   {
      /*DEBUG_INFO1 (pModname, "Successfully called SetSockOpt(SendTimeout) - (%d)", (int) timeout); */
      return (TRUE);
   }

   DEBUG_WARN2 (pModname, "Failed to call SetSockOpt(SendTimeout) - (%d), errCode=%d", (int) timeout, WSAGetLastError());

   return (FALSE);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL Connect (T_TDC_SOCKET             sockFd,
                    const T_TDC_SOCK_ADDR*   pServerAddr,
                    int                      sockAddrSize)
{
   return (connect (sockFd, (const struct sockaddr *) pServerAddr, sockAddrSize) == 0);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

/*@unused@*/ static int Fcntl (int fd, int cmd, int arg)
{
#if defined (O_XXXXX)
	int  n;

	if ((n = fcntl (fd, cmd, arg)) == -1)
	{
		DEBUG_WARN (MOD_MAIN, "Failed to call fcntl()");
	}

	return (n);
#else
   TDC_UNUSED (fd)
   TDC_UNUSED (cmd)
   TDC_UNUSED (arg)

	return (0);
#endif
}

// ----------------------------------------------------------------------------
/*@unused@*/
static int Select (int               noFdSet,
                   fd_set*           readFdSet,
                   fd_set*           writeFdSet,
                   fd_set*           exceptFdSet,
                   struct timeval*   pTimeout)
{
  int    n;

  if ( (n = select(noFdSet, readFdSet, writeFdSet, exceptFdSet, pTimeout)) < 0)
  {
     DEBUG_ERROR1 (MOD_MAIN, "Failed to call select(), errCode=%d", WSAGetLastError());
  }

  return (n);    /* can return 0 on timeout */
}

// ----------------------------------------------------------------------------


T_TDC_BOOL ConnectTimo (T_TDC_SOCKET               sockFd,
                        const T_TDC_SOCK_ADDR*     pServerAddr,
                        int                        sockAddrSize,
                        UINT32                     timeout)
{
   if (timeout > 0)
   {
      DEBUG_WARN (MOD_LIB, "Timeot currently not supported with connect");
   }

   return (Connect (sockFd, pServerAddr, sockAddrSize));



#if defined (O_XXXXX)
   int         flags;
   int         n;

   if (timeout == 0)
   {
      return (Connect (sockFd, pServerAddr, sockAddrSize));
   }

   flags = Fcntl (sockFd, F_GETFL, 0);
   Fcntl (sockFd, F_SETFL, flags | O_NONBLOCK);

   if ((n = connect (sockFd, (const struct sockaddr *) pServerAddr, sockAddrSize) < 0))
   {
      if (    (errno != EINPROGRESS)
           && (errno != EWOULDBLOCK)
         )
      {
         return (FALSE);            /* Socket will be closed by Caller */
      }
   }

   /* Do whatever we want while the connect is taking place. */

   if (n > 0)             /* connect didn't complete immediately, select on writability */
   {
      fd_set            writeSet;
      struct timeval    timeVal;

      FD_ZERO (&writeSet);
      FD_SET  (sockFd, &writeSet);
      timeVal.tv_sec  = timeout;
      timeVal.tv_usec = 0;

      if ( (n = Select (sockFd + 1, NULL, &writeSet, NULL, &timeVal)) == 0)
      {
         errno = ETIMEDOUT;            /* timeout, Socket will be closed by Caller */
         return (FALSE);
      }

      if (!FD_ISSET (sockFd, &writeSet))
      {
         DEBUG_WARN (MOD_MAIN, "Failed to call Select()");
         return (FALSE);
      }
   }
   else
   {
      /* MRi, 2005/06/10                                                           */
      /* sockFd is writable, assume that the connection succeeded.                 */
      /* If it didn't succeed, it will be detected on the first attempt to write() */
      /* and error handling will be handled there.                                 */

      /* socklen_t   len = sizeof (error);                                 */
      /*                                                                   */
      /* if (getsockopt (sockFd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)  */
      /* {                                                                 */
      /*    return (FALSE);      / * Solaris pending error * /             */
      /* }                                                                 */

   }

   Fcntl (sockFd, F_SETFL, flags);  /* restore file status flags */

   return (TRUE);
#endif
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL Bind (T_TDC_SOCKET             sockFd,
                 const T_TDC_SOCK_ADDR*   pMyAddr,
                 int                      addrLen)
{
   return (bind (sockFd, (const struct sockaddr *) pMyAddr, (socklen_t) addrLen) == 0);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL Listen (T_TDC_SOCKET   sockFd,  int   backlog)
{
   return (listen (sockFd, backlog) == 0);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_SOCKET Accept (const char*          pModname,
                     T_TDC_SOCKET         listenFd,
                     T_TDC_SOCK_ADDR*     pSa,
                     T_TDC_SOCKLEN_T*     pSaLen)
{
   SOCKET      connFd = accept (listenFd, (struct sockaddr *) pSa, (int *) pSaLen);

   if (connFd == INVALID_SOCKET)
   {
      DEBUG_WARN2 (pModname, "Error calling accept listenFd(%d), errCode=%d", listenFd, WSAGetLastError());
      return (TDC_INVALID_SOCKET);
   }

   DEBUG_INFO2 (pModname, "Server(%d) successfully accepted Client(%d)", (int) listenFd, (int) connFd);

   return (connFd);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL ShutDown (T_TDC_SOCKET      sockFd,
                     int               how)
{
   return (shutdown (sockFd, how) == 0);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL CloseSocket (T_TDC_SOCKET   sockFd)
{
   return (closesocket (sockFd) == 0);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


T_TDC_BOOL tdcTcpSendN (const char*       pModname,
                        T_TDC_SOCKET      fdWr,
                        void*             pSendBuf,
                        UINT32*           pBytesSent,
                        UINT32            bytesExpected)
{
   *pBytesSent = (UINT32) 0;

   return (tdcStreamWriteN (pModname, (T_TDC_STREAM) fdWr, pSendBuf, pBytesSent, bytesExpected, SOCKET_RECV_SEND_FLAGS));
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL tdcTcpRecvN (const char*       pModname,
                        T_TDC_SOCKET      fdRd,
                        void*             pRecvBuf,
                        UINT32*           pBytesReceived,
                        UINT32            bytesExpected)
{
   * (UINT8 *) pRecvBuf = (UINT8) 0;
   * pBytesReceived     = (UINT32) 0;

   /*@ -mustdefine */
   return (tdcStreamReadN (pModname, (T_TDC_STREAM) fdRd, pRecvBuf, pBytesReceived, bytesExpected, SOCKET_RECV_SEND_FLAGS));
   /*@ =mustdefine */
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL getInetAddr (const char*          pServAddr,
                        T_TDC_SOCK_ADDR_IN*  pSockAddr)
{
   T_TDC_BOOL              bOk = TRUE;
   struct hostent FAR*     pHostEnt;
   const char*             pTdcServIpAddr = getenv (ENV_TDC_SERVICE_IP_ADDR);

   if (pTdcServIpAddr == NULL)
   {
      pTdcServIpAddr = pServAddr;
   }
   else
   {
      char     text[200];

      (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Overwriting loocal host for service-addr with %s", pTdcServIpAddr);
      text[sizeof (text) - 1] = '\0';
      DEBUG_INFO (MOD_MAIN, text);
   }

   if ((pHostEnt = gethostbyname (pTdcServIpAddr)) != NULL)
   {
      UINT32*        pSAddr = (UINT32 *) ((void *) pHostEnt->h_addr_list[0]);

      ((struct sockaddr_in *) pSockAddr)->sin_addr.s_addr = *pSAddr;  
                     /*@ -mustdefine */
      return (TRUE); /*@ =mustdefine */
   }
   else
   {
                                                                                          /*@ -type */
      ((struct sockaddr_in *) pSockAddr)->sin_addr.s_addr = inet_addr (pTdcServIpAddr);
      bOk = ((((struct sockaddr_in *) pSockAddr)->sin_addr.s_addr) != INADDR_NONE);       /*@ =type */
   }
                  /*@ -mustdefine */
   return (bOk);  /*@ =mustdefine */
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL tdcTcpConnect46 (const char*            pModname,
                            T_TDC_CONNECT_PAR*     pConnPar)
{
	#if defined (O_XXXXX)
   if (    (pConnPar          != NULL)
        && (pConnPar->pSockFd != NULL)
      )
   {
      int                  n;
      struct addrinfo      hints;
      struct addrinfo*     pRes;
      struct addrinfo*     pResSave;
      char                 servPortNo[20];

      tdcMemClear (&hints,     (UINT32) sizeof (struct addrinfo));
      tdcSNPrintf (servPortNo, (UINT32) sizeof (servPortNo), "%d", (int) pConnPar->serverAddr.portNo);
      servPortNo[sizeof (servPortNo) - 1] = '\0';

      hints.ai_family   = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;

      // Get two address structures (one for IPv4 and one for IPv6)
      if ((n = getaddrinfo (pConnPar->serverAddr.ipAddr, servPortNo, &hints, &pRes)) != 0)  // IPv4 and IPv6 support
      {
         char     text[120];

         tdcSNPrintf (text, (UINT32) sizeof (text),
                      "Failed to call getaddrinfo in tdcTcpConnect46 - Host(%s), PortNo(%s): (%s)",
                      pConnPar->serverAddr.ipAddr, servPortNo, gai_strerror (n));
         text[sizeof (text) - 1] = '\0';
         DEBUG_WARN  (pModname, text);
         return (FALSE);
      }

      for (pResSave = pRes; pRes != NULL; /*@ -type */ pRes = pRes->ai_next /*@ =type */)
      {                                                                                         /*@ -type */
         *pConnPar->pSockFd = Socket (pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol);   /*@ =type */
         //DEBUG_INFO1 (pModname, "Successfully called Socket (%d)", *pConnPar->pSockFd);

         if (*pConnPar->pSockFd != TDC_INVALID_SOCKET)          // otherwise try next one
         {
            tdcSetSocketOptions (pModname, *pConnPar->pSockFd, pConnPar->pSockOpt);

            if (Connect (*pConnPar->pSockFd, pRes->ai_addr, pRes->ai_addrlen))
            {
               freeaddrinfo (pResSave);
               return (TRUE);
            }
         }

         DEBUG_INFO1 (pModname, "Closing Socket (%d)", *pConnPar->pSockFd);
         tdcCloseSocket (pModname, pConnPar->pSockFd);         // ignore this one
      }

      freeaddrinfo (pResSave);

      DEBUG_WARN (pModname, "Failed to call tcpConnect()!");
   }
   #else

   pModname = pModname;
   pConnPar = pConnPar;

	#endif
   return (FALSE);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL tdcTcpBind46 (const char*         pModname,
                         T_TDC_LISTEN_PAR*   pListenPar)
{
	#if defined (O_XXXXX)
   if (    (pListenPar            != NULL)
        && (pListenPar->pListenFd != NULL)
      )
   {
      int                  n;
      const char*          pHost                    = NULL;
      struct addrinfo      hints, *pResSave, *pRes  = NULL;
      char                 servPortNo[20];
      char                 text[120];

      *pListenPar->pListenFd = TDC_INVALID_SOCKET;

      tdcMemClear (&hints,     (UINT32) sizeof (struct addrinfo));
      tdcSNPrintf (servPortNo, (UINT32) sizeof (servPortNo), "%d", (int) pListenPar->portNo);
      servPortNo[sizeof (servPortNo) - 1] = '\0';

      hints.ai_flags    = AI_PASSIVE;
      hints.ai_family   = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;

      // Get two address structures (one for IPv4 and one for IPv6)
      if ((n = getaddrinfo (pHost, servPortNo, &hints, &pRes)) != 0)   // IPv4 and IPv6 support
      {
         tdcSNPrintf (text,  (UINT32) sizeof (text),
                      "Failed to call getaddrinfo in tdcTcpBind46 - Host(%s), PortNo(%s): (%s)",
                      pHost, servPortNo, gai_strerror (n));
         text[sizeof (text) - 1] = '\0';
         DEBUG_ERROR  (pModname, text);
         return (FALSE);
      }

      for (pResSave = pRes; pRes != NULL; /*@ -type */ pRes = pRes->ai_next /*@ =type */)
      {                                                                                            /*@ -type */
         *pListenPar->pListenFd = Socket (pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol);  /*@ =type */

         if (*pListenPar->pListenFd != TDC_INVALID_SOCKET)          // otherwise try next one
         {
            tdcSetSocketOptions (pModname, *pListenPar->pListenFd, pListenPar->pSockOpt);
                                                                                 /*@ -type */
            if (Bind (*pListenPar->pListenFd, pRes->ai_addr, pRes->ai_addrlen))  /*@ =type */
            {
               //DEBUG_INFO2 (module, "Successfully called Bind(%d) - Port(%d)", *pListenPar->pListenFd, pListenPar->portNo);

               pListenPar->addrLen = pRes->ai_addrlen;      // return size of protocol address
               freeaddrinfo (pResSave);

               return (TRUE);       // success
            }

            CloseSocket (*pListenPar->pListenFd);    // Close and try next one
            *pListenPar->pListenFd = TDC_INVALID_SOCKET;
         }
      }

      // errno from final socket() or bind()
      tdcSNPrintf (text, (UINT32) sizeof (text),
                   "Failed to call tdcTcpBind46() - Host(%s), PortNo(%d)", pHost, (int) pListenPar->portNo);
      text[sizeof (text) - 1] = '\0';
      DEBUG_WARN (pModname, text);

      freeaddrinfo (pResSave);
   }
   #else

   pModname   = pModname;
   pListenPar = pListenPar;

	#endif
   return (FALSE);
}

// ----------------------------------------------------------------------------

T_TDC_BOOL tdcServerUdpSock4 (const char*       pModname,
                              T_TDC_SOCKET*     pServerSockFd,
                              UINT16            portNo)
{
   struct sockaddr_in     serverAddr;

   (void) tdcCloseSocket (pModname, pServerSockFd);

   tdcMemClear (&serverAddr, (UINT32) sizeof (serverAddr));

   serverAddr.sin_family      = AF_INET;
   serverAddr.sin_port        = htons (portNo);
   serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);

   if (tdcUdpSocket4 (pModname, pServerSockFd))
   {
      if (Bind (*pServerSockFd, (struct sockaddr *) &serverAddr, (int) sizeof (serverAddr)))
      {
         DEBUG_INFO1 (pModname, "Successfully called tdcServerUdpSock4 - Bind(%d)", portNo);

         return (TRUE);
      }

      DEBUG_WARN     (pModname, "Failed to call Bind()");
      (void) tdcCloseSocket (pModname, pServerSockFd);
   }

   return (FALSE);
}

// ----------------------------------------------------------------------------

T_TDC_BOOL tdcServerUdpSock46 (const char*         pModname,
                               T_TDC_SOCKET*       pServerSockFd,
                               UINT16              portNo)
{
	#if defined (O_XXXXX)

   int                  n;
   struct addrinfo      hints;
   struct addrinfo*     pRes;
   struct addrinfo*     pResSave;
   char                 serverPortNo[20];
   char                 text [120];

   tdcMemClear (&hints,       (UINT32) sizeof (struct addrinfo));
   tdcSNPrintf (serverPortNo, (UINT32) sizeof (serverPortNo), "%d", (int) portNo);
   serverPortNo[sizeof (serverPortNo) - 1] = '\0';

   hints.ai_flags    = AI_PASSIVE;
   hints.ai_family   = AF_UNSPEC;
   hints.ai_socktype = SOCK_DGRAM;

   if ((n = getaddrinfo (NULL, serverPortNo, &hints, &pRes))  != 0)
   {
      tdcSNPrintf (text, (UINT32) sizeof (text),
                   "Failed to call getaddrinfo in tdcServerUdpSock46 - PortNo(%d): (%s)", portNo, gai_strerror (n));
      text[sizeof (text) - 1] = '\0';
      DEBUG_WARN (pModname, text);
      return (FALSE);
   }

   for (pResSave = pRes; pRes != NULL; pRes = pRes->ai_next)
   {
      T_TDC_SOCKET     serverSockFd = socket (pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol);

      if (serverSockFd >= 0)
      {
         if (bind (serverSockFd, pRes->ai_addr, pRes->ai_addrlen) == 0)
         {
            DEBUG_INFO1 (pModname, "Successfully called bind(%d)", portNo);

            freeaddrinfo (pResSave);
            *pServerSockFd = serverSockFd;

            /* pRes->ai_addrlen, i.e. the size of protocoll address may be needed by caller */
            /* in some cases (but currently not by TDC) */
            return (TRUE);
         }

         tdcCloseSocket (pModname, &serverSockFd);
      }
   }

   /* errno set from final socket() */
   tdcSNPrintf (text, (UINT32) sizeof (text),
                "Failed to call tdcServerUdpSock46 - PortNo(%d)", portNo);
   text[sizeof (text) - 1] = '\0';
   DEBUG_WARN (pModname, text);

   freeaddrinfo (pResSave);
   #else

   pModname      = pModname;
   pServerSockFd = pServerSockFd;
   portNo        = portNo;

	#endif
   return (FALSE);
}

// ----------------------------------------------------------------------------

T_TDC_BOOL tdcClientUdpSock46 (const char*            pModname,
                               T_TDC_SOCKET*          pClientSockFd,
                               T_TDC_SERVER_ADDR*     pRemAddr,
                               void**                 ppServerAddr,
                               T_TDC_SOCKLEN_T*       pServerSockLen)
{
	#if defined (O_XXXXX)

   int                     n;
   struct addrinfo         hints;
   struct addrinfo*        pRes;
   struct addrinfo*        pResSave;
   char                    serverPortNo[20];
   char                    text [160];

   tdcMemClear (&hints,       (UINT32) sizeof (hints));
   tdcSNPrintf (serverPortNo, (UINT32) sizeof (serverPortNo), "%d", (int) pRemAddr->portNo);
   serverPortNo[sizeof (serverPortNo) - 1] = '\0';

   hints.ai_family   = AF_UNSPEC;
   hints.ai_socktype = SOCK_DGRAM;

   if ((n = getaddrinfo (pRemAddr->ipAddr, serverPortNo, &hints, &pRes)) != 0)
   {
      tdcSNPrintf (text, (UINT32) sizeof (text),
                   "Failed to call getaddrinfo in tdcClientUdpSock46 - host(%s), PortNo(%s): %s",
                   pRemAddr->ipAddr, serverPortNo, gai_strerror(n));
      text[sizeof (text) - 1] = '\0';
      DEBUG_WARN (pModname, text);
      return     (FALSE);
   }

   for (pResSave = pRes; pRes != NULL; pRes = pRes->ai_next)
   {
      T_TDC_SOCKET     clientSockFd = socket (pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol);

      if (clientSockFd >= 0)            /* success */
      {
         *pClientSockFd  = clientSockFd;
         *pServerSockLen = pRes->ai_addrlen;
         *ppServerAddr   = tdcAllocMemChk (pModname, pRes->ai_addrlen);

         memcpy (*ppServerAddr, pRes->ai_addr, pRes->ai_addrlen);

         freeaddrinfo (pResSave);

         return(TRUE);
      }
   }

   /* errno set from final socket() */
   rdcSNPrintf (text, (UINT32) sizeof (text),
                "Failed to call getaddrinfo in tdcClientUdpSock46 - host(%s), PortNo(%s)", pRemAddr->ipAddr, serverPortNo);
   text[sizeof (text) - 1] = '\0';
   DEBUG_WARN  (pModname, text);

   freeaddrinfo (pResSave);
   #else

   pModname       = pModname;
   pClientSockFd  = pClientSockFd;
   pRemAddr       = pRemAddr;
   ppServerAddr   = ppServerAddr;
   pServerSockLen = pServerSockLen;

	#endif
   return(FALSE);
}

/* ---------------------------------------------------------------------------- */

/*@ -exportlocal */
void Inet_pton (int family, const char*   pServerName, void*   pServerAddr)
{
	#if defined (O_XXXXX)

		char     text[120];
		int      n = inet_pton (family, pServerName, pServerAddr);

		tdcSNPrintf (text, (UINT32) sizeof (text), "Failed to call inet_pton (%s) - %d", pServerName, n);
      text[sizeof (text) - 1] = '\0';

		if (n < 0)
		{
			DEBUG_ERROR (MOD_MAIN, text);         /* errno set */
		}

		if (n == 0)
		{
			DEBUG_ERROR (MOD_MAIN, text);         /* errno not set */
		}
   #else
      family = family;
	#endif

	/* inet_pton currently not supported (IPV4 only) */

                                                                          /*@ -type */
   ((struct in_addr *) pServerAddr)->s_addr = inet_addr (pServerName);    /*@ =type */

}
/*@ =exportlocal */

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcClientUdpSock4 (const char*             pModname,
                              T_TDC_SOCKET*           pServSockFd,
                              T_TDC_SERVER_ADDR*      pRemAddr,
                              void**                  ppServAddr,
                              T_TDC_SOCKLEN_T*        pServSockLen)
{
   struct sockaddr_in*     pServAddr = tdcAllocMemChk (pModname, (UINT32) sizeof (struct sockaddr_in));

   *ppServAddr = NULL;

   if (pServAddr != NULL)
   {
      if (tdcUdpSocket4 (pModname, pServSockFd))
      {
                                                                        /*@ -type */
         pServAddr->sin_family = AF_INET;
         pServAddr->sin_port   = htons (pRemAddr->portNo);              /*@ =type */

                                                                        /*@ -type */
         Inet_pton (AF_INET, pRemAddr->ipAddr, &pServAddr->sin_addr);   /*@ =type */

         *ppServAddr   = pServAddr;
         *pServSockLen = (T_TDC_SOCKLEN_T) sizeof (struct sockaddr_in);

         DEBUG_INFO (pModname, "Successfully setup UDP-clientSocket");
         return (TRUE);
      }
   }

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcUdpSendN (const char*         pModname,
                        T_TDC_SOCKET        fdWr,
                        void*               pSendBuf,
                        UINT16              bytesExpected,
                        void*               pServerAddr,
                        T_TDC_SOCKLEN_T     serverSockLen)
{
   if (sendto (fdWr, pSendBuf, bytesExpected, SOCKET_RECV_SEND_FLAGS,
               (const struct sockaddr *)  pServerAddr, (socklen_t) serverSockLen)!= bytesExpected)
   {
      DEBUG_INFO (pModname, "Failed to call sendto ()");
      return (FALSE);
   }

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcUdpRecvN (const char*       pModname,
                        T_TDC_SOCKET      fdRd,
                        void*             pRecvBuf,
                        UINT16            bytesExpected)
{
   /*@ -mustdefine */
   if (recvfrom (fdRd, pRecvBuf, bytesExpected, SOCKET_RECV_SEND_FLAGS, NULL, NULL) < 0)
   {
      DEBUG_INFO (pModname, "Failed to call recvfrom ()");
      return (FALSE);
   }

   return (TRUE);
   /*@ =mustdefine */
}

/* ---------------------------------------------------------------------------- */

void initTcpIpWinsock (void)
{
   tdcInitTcpIpWinsock (MOD_LIB);
}

