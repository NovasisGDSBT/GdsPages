/*
 * $Id: tdcOsSock.c 11853 2012-02-10 17:14:13Z bloehr $
 *
 * DESCRIPTION    Functions for managing the ethernet Interface
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
   #error "This is the Linux Version of tdcOsSock, i.e. DARWIN has to be specified"
#endif

//-----------------------------------------------------------------------------

#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
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

UINT8 tdcH2N8 (UINT8    var8)
{
   return (var8);
}

/* ---------------------------------------------------------------------------- */

UINT16 tdcH2N16 (UINT16    var16)
{
   return ((UINT16) htons (var16));
}

/* ---------------------------------------------------------------------------- */

UINT32 tdcH2N32 (UINT32    var32)
{
   return ((UINT32) htonl (var32));
}

/* ---------------------------------------------------------------------------- */

UINT8 tdcN2H8 (UINT8    var8)
{
   return (var8);
}

/* ---------------------------------------------------------------------------- */

UINT16 tdcN2H16 (UINT16    var16)
{
   return ((UINT16) ntohs (var16));
}

/* ---------------------------------------------------------------------------- */

UINT32 tdcN2H32 (UINT32    var32)
{
   return ((UINT32) ntohl (var32));
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

   *pLen  = (UINT32) sizeof (struct sockaddr_in);
                                                                           /*@ -type */
   ((struct sockaddr_in *) pServAddr)->sin_family = AF_INET;
   ((struct sockaddr_in *) pServAddr)->sin_port   = htons (portNo);     /*@ =type */
}

/* ---------------------------------------------------------------------------- */

void tdcInitSrv_SERV_ADDR (const char*             pModname,
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
   ((struct sockaddr_in *) pServAddr)->sin_family      = AF_INET;
   ((struct sockaddr_in *) pServAddr)->sin_port        = htons (portNo);
   ((struct sockaddr_in *) pServAddr)->sin_addr.s_addr = INADDR_ANY;          /*@ =type */
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_SOCKET Socket (int    family,
                     int    type,
                     int    protocol)
{
   T_TDC_SOCKET      socketFd = socket (family, type, protocol);

   return ((socketFd < 0) ? (TDC_INVALID_SOCKET) : (socketFd));
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
      struct linger       linger;

      linger.l_linger = seconds;
      linger.l_onoff  = lingerOn;

      if (Setsockopt (sockFd, SOL_SOCKET, SO_LINGER, &linger, (T_TDC_SOCKLEN_T) sizeof (linger)))
      {
         bResult = TRUE;;
         /*DEBUG_INFO2 (pModname, "Successfully called SetSockOpt(Linger) - On(%d) - time(%d)", (int) lingerOn, (int) seconds);*/
      }
      else
      {
         DEBUG_WARN2 (pModname, "Failed to call SetSockOpt(Linger) - On(%d) - time(%d) !",
                                 (int) lingerOn, (int) seconds);
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

   if (Setsockopt (sockFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, (T_TDC_SOCKLEN_T) sizeof (reuseAddr)))
   {
      /*DEBUG_INFO1 (pModname, "Successfully called SetSockOpt(ReuseAddr) - reuse(%d)!", bReuseAddr); */
      return (TRUE);
   }

   DEBUG_WARN1 (pModname, "Failed to call SetSockOpt(ReuseAddr) - reuse(%d)!", bReuseAddr);

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

   DEBUG_WARN1 (pModname, "Failed to call SetSockOpt(RecvTimeout) - (%d) !", (int) timeout);

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

   DEBUG_WARN1 (pModname, "Failed to call SetSockOpt(SendTimeout) - (%d) !", (int) timeout);

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

static int Fcntl (int fd, int cmd, int arg)
{
  int  n;

  if ((n = fcntl (fd, cmd, arg)) == -1)
  {
     DEBUG_WARN (MOD_MAIN, "Failed to call fcntl()");
  }

  return (n);
}

// ----------------------------------------------------------------------------

static int Select (            int                 noFdSet,
                   /*@null@*/  fd_set*             readFdSet,
                   /*@null@*/  fd_set*             writeFdSet,
                   /*@null@*/  fd_set*             exceptFdSet,
                               struct timeval*     pTimeout)
{
  int    n;

  if ( (n = select(noFdSet, readFdSet, writeFdSet, exceptFdSet, pTimeout)) < 0)
  {
     DEBUG_ERROR (MOD_MAIN, "Failed to call select()");
  }

  return (n);    /* can return 0 on timeout */
}

// ----------------------------------------------------------------------------


T_TDC_BOOL ConnectTimo (T_TDC_SOCKET               sockFd,
                        const T_TDC_SOCK_ADDR*     pServerAddr,
                        int                        sockAddrSize,
                        UINT32                     timeout)
{
   int         flags;
   int         n;

   if (timeout == 0)
   {
      return (Connect (sockFd, pServerAddr, sockAddrSize));
   }

   flags = Fcntl (sockFd, F_GETFL, 0);
   (void) Fcntl (sockFd, F_SETFL, flags | O_NONBLOCK);

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

   (void) Fcntl (sockFd, F_SETFL, flags);  /* restore file status flags */

   return (TRUE);
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
   T_TDC_SOCKET      connFd = -1;

   for (;;)
   {
      socklen_t   saLen = 0;
      socklen_t*  pLen = &saLen;

      if (    (pSa    == NULL)
           || (pSaLen == NULL)
         )
      {
         pLen = NULL;
      }

      if ( (connFd = accept (listenFd, (struct sockaddr *) pSa, pLen)) < 0)
      {
         switch (errno)
         {
            case EINTR:          {break;}
            case ECONNABORTED:   {break;}
#if defined (EPROTO)
            case EPROTO:         {break;}
#endif
            default:
            {
               DEBUG_ERROR2 (pModname, "Error calling accept listenFd(%d): (%d)", listenFd, errno); break;
            }
         }
      }
      else
      {
         if (pLen != NULL)
         {                                      /*@ -nullderef */
            *pSaLen = (T_TDC_SOCKLEN_T) saLen;  /*@ =nullderef */
         }
         break;         // success
      }
   }

   DEBUG_INFO2 (pModname, "Server(%d) successfully accepted Client(%d)", (int) listenFd, (int) connFd);

   return (connFd);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL ShutDown (T_TDC_SOCKET      socket,
                     int               how)
{
   return (shutdown (socket, how) == 0);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL CloseSocket (T_TDC_SOCKET   sockFd)
{
   return (close (sockFd) == 0);
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
   return (tdcStreamWriteN (pModname, (T_TDC_STREAM) fdWr, pSendBuf, pBytesSent, bytesExpected));
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
   return (tdcStreamReadN (pModname, (T_TDC_STREAM) fdRd, pRecvBuf, pBytesReceived, bytesExpected));
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL getInetAddr (const char*          pServAddr,
                        T_TDC_SOCK_ADDR_IN*  pSockAddr)
{
   in_addr_t            sAddr;
   struct hostent*      pHostEnt;

   if ((sAddr = inet_addr ((char*) pServAddr)) != INADDR_NONE)
   {                                                                 /*@ -type */
      ((struct sockaddr_in *) pSockAddr)->sin_addr.s_addr = sAddr;   /*@ =type */ /*@ -mustdefine */
      return (TRUE);                                                              /*@ =mustdefine */
   }

   if ((pHostEnt = gethostbyname (pServAddr)) != NULL)
   {                                                                                                     /*@ -type */
      ((struct sockaddr_in *) pSockAddr)->sin_addr.s_addr = *((in_addr_t *)(pHostEnt->h_addr_list[0]));  /*@ =type -mustdefine */
      return (TRUE);                                                                                     /*@ =mustdefine */
   }
                     /*@ -mustdefine */
   return (FALSE);   /*@ =mustdefine */
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL tdcTcpConnect46 (const char*            pModname,
                            T_TDC_CONNECT_PAR*     pConnPar)
{
   if (    (pConnPar          != NULL)
        && (pConnPar->pSockFd != NULL)
      )
   {
      int                  n;
      struct addrinfo      hints;
      struct addrinfo*     pRes;
      struct addrinfo*     pResSave;
      char                 servPortNo[20];

      tdcMemClear (&hints, (UINT32) sizeof (struct addrinfo));
      (void) tdcSNPrintf (servPortNo, (unsigned int) sizeof (servPortNo), "%d", (int) pConnPar->serverAddr.portNo);
      servPortNo[sizeof (servPortNo) - 1] = '\0';

      hints.ai_family   = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;

      // Get two address structures (one for IPv4 and one for IPv6)
      if ((n = getaddrinfo (pConnPar->serverAddr.ipAddr, servPortNo, &hints, &pRes)) != 0)  // IPv4 and IPv6 support
      {
         char     text[120];

         (void) tdcSNPrintf (text, (unsigned int) sizeof (text),
                      "Failed to call getaddrinfo in tdcTcpConnect46 - Host(%s), PortNo(%s): (%s)",
                      pConnPar->serverAddr.ipAddr, servPortNo, gai_strerror (n));
         text[sizeof (text) - 1] = '\0';

         DEBUG_WARN  (pModname, text);
         return (FALSE);
      }

      for (pResSave = pRes; pRes != NULL; /*@ -type*/ pRes = pRes->ai_next /*@ =type*/)
      {                                                                                         /*@ -type */
         *pConnPar->pSockFd = Socket (pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol);   /*@ =type */
         //DEBUG_INFO1 (pModname, "Successfully called Socket (%d)", *pConnPar->pSockFd);

         if (*pConnPar->pSockFd != TDC_INVALID_SOCKET)          // otherwise try next one
         {
            (void) tdcSetSocketOptions (pModname, *pConnPar->pSockFd, pConnPar->pSockOpt);
                                                                                 /*@ -type */
            if (Connect (*pConnPar->pSockFd, pRes->ai_addr, pRes->ai_addrlen))   /*@ =type */
            {
               freeaddrinfo (pResSave);
               return (TRUE);
            }
         }

         DEBUG_INFO1 (pModname, "Closing Socket (%d)", *pConnPar->pSockFd);
         (void) tdcCloseSocket (pModname, pConnPar->pSockFd);         // ignore this one
      }

      freeaddrinfo (pResSave);

      DEBUG_WARN (pModname, "Failed to call tcpConnect()!");
   }

   return (FALSE);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL tdcTcpBind46 (const char*         pModname,
                         T_TDC_LISTEN_PAR*   pListenPar)
{
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
      (void) tdcSNPrintf (servPortNo, (unsigned int) sizeof (servPortNo), "%d", (int) pListenPar->portNo);
      servPortNo[sizeof (servPortNo) - 1] = '\0';

      hints.ai_flags    = AI_PASSIVE;
      hints.ai_family   = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;

      // Get two address structures (one for IPv4 and one for IPv6)
      /*@ -nullpass */
      if ((n = getaddrinfo (pHost, servPortNo, &hints, &pRes)) != 0) /*@ =nullpass */  // IPv4 and IPv6 support
      {
         (void) tdcSNPrintf (text, (unsigned int) sizeof (text),
                      "Failed to call getaddrinfo in tdcTcpBind46 - Host(%s), PortNo(%s): (%s)",  /*@ -nullpass */
                             pHost, servPortNo, gai_strerror (n));                                /*@ =nullpass */
         text[sizeof (text) - 1] = '\0';
         DEBUG_ERROR  (pModname, text);
         return (FALSE);
      }

      for (pResSave = pRes; pRes != NULL; /*@ -type */ pRes = pRes->ai_next /*@ =type */)
      {                                                                                            /*@ -type */
         *pListenPar->pListenFd = Socket (pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol);  /*@ =type */

         if (*pListenPar->pListenFd != TDC_INVALID_SOCKET)          // otherwise try next one
         {
            (void) tdcSetSocketOptions (pModname, *pListenPar->pListenFd, pListenPar->pSockOpt);
                                                                                 /*@ -type */
            if (Bind (*pListenPar->pListenFd, pRes->ai_addr, pRes->ai_addrlen))  /*@ =type */
            {
               //DEBUG_INFO2 (module, "Successfully called Bind(%d) - Port(%d)", *pListenPar->pListenFd, pListenPar->portNo);
                                                        /*@ -type */
               pListenPar->addrLen = pRes->ai_addrlen;  /*@ =type */    // return size of protocol address
               freeaddrinfo (pResSave);

               return (TRUE);       // success
            }

            (void) CloseSocket (*pListenPar->pListenFd);    // Close and try next one
            *pListenPar->pListenFd = TDC_INVALID_SOCKET;
         }
      }

      // errno from final socket() or bind()
      (void) tdcSNPrintf (text, (unsigned int) sizeof (text),                                                                      /*@ -nullpass */
                   "Failed to call tdcTcpBind46() - Host(%s), PortNo(%d)", pHost, (int) pListenPar->portNo); /*@ =nullpass */
      text[sizeof (text) - 1] = '\0';
      DEBUG_WARN (pModname, text);

      freeaddrinfo (pResSave);
   }

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
   int                  n;
   struct addrinfo      hints;
   struct addrinfo*     pRes;
   struct addrinfo*     pResSave;
   char                 serverPortNo[20];
   char                 text [120];

   tdcMemClear (&hints, (UINT32) sizeof (struct addrinfo));
   (void) tdcSNPrintf (serverPortNo, (unsigned int) sizeof (serverPortNo), "%d", (int) portNo);
   serverPortNo[sizeof (serverPortNo) - 1] = '\0';

   hints.ai_flags    = AI_PASSIVE;
   hints.ai_family   = AF_UNSPEC;
   hints.ai_socktype = SOCK_DGRAM;

   if ((n = getaddrinfo (NULL, serverPortNo, &hints, &pRes))  != 0)
   {
      (void) tdcSNPrintf (text, (unsigned int) sizeof (text),
                   "Failed to call getaddrinfo in tdcServerUdpSock46 - PortNo(%d): (%s)", portNo, gai_strerror (n));
      text[sizeof (text) - 1] = '\0';
      DEBUG_WARN (pModname, text);
      return (FALSE);
   }
                                                              /*@ -type */
   for (pResSave = pRes; pRes != NULL; pRes = pRes->ai_next)  /*@ =type */
   {                                                                                                  /*@ -type */
      T_TDC_SOCKET     serverSockFd = socket (pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol); /*@ =type */

      if (serverSockFd >= 0)
      {                                                                  /*@ -type */
         if (bind (serverSockFd, pRes->ai_addr, pRes->ai_addrlen) == 0)  /*@ =type */
         {
            DEBUG_INFO1 (pModname, "Successfully called bind(%d)", portNo);

            freeaddrinfo (pResSave);
            *pServerSockFd = serverSockFd;

            /* pRes->ai_addrlen, i.e. the size of protocoll address may be needed by caller */
            /* in some cases (but currently not by TDC) */
            return (TRUE);
         }

         (void) tdcCloseSocket (pModname, &serverSockFd);
      }
   }

   /* errno set from final socket() */
   (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Failed to call tdcServerUdpSock46 - PortNo(%d)", (int) portNo);
   text[sizeof (text) - 1] = '\0';
   DEBUG_WARN (pModname, text);

   freeaddrinfo (pResSave);

   return (FALSE);
}

// ----------------------------------------------------------------------------

T_TDC_BOOL tdcClientUdpSock46 (const char*            pModname,
                               T_TDC_SOCKET*          pClientSockFd,
                               T_TDC_SERVER_ADDR*     pRemAddr,
                               void**                 ppServerAddr,
                               T_TDC_SOCKLEN_T*       pServerSockLen)
{
   int                     n;
   struct addrinfo         hints;
   struct addrinfo*        pRes;
   struct addrinfo*        pResSave;
   char                    serverPortNo[20];
   char                    text [160];

   tdcMemClear (&hints, (UINT32) sizeof (hints));
   (void) tdcSNPrintf (serverPortNo, (unsigned int) sizeof (serverPortNo), "%d", (int) pRemAddr->portNo);
   serverPortNo[sizeof (serverPortNo) - 1] = '\0';

   hints.ai_family   = AF_UNSPEC;
   hints.ai_socktype = SOCK_DGRAM;

   if ((n = getaddrinfo (pRemAddr->ipAddr, serverPortNo, &hints, &pRes)) != 0)
   {
      (void) tdcSNPrintf (text, (unsigned int) sizeof (text),
                   "Failed to call getaddrinfo in tdcClientUdpSock46 - host(%s), PortNo(%s): %s",
                   pRemAddr->ipAddr, serverPortNo, gai_strerror(n));
      text[sizeof (text) - 1] = '\0';
      DEBUG_WARN (pModname, text);
      return     (FALSE);
   }
                                                               /*@ -type */
   for (pResSave = pRes; pRes != NULL; pRes = pRes->ai_next)   /*@ =type */
   {                                                                                                  /*@ -type */
      T_TDC_SOCKET     clientSockFd = socket (pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol); /*@ =type */

      if (clientSockFd >= 0)            /* success */
      {
         *pClientSockFd  = clientSockFd;        /*@ -type */
         *pServerSockLen = pRes->ai_addrlen;    /*@ =type */            /*@ -type */
         *ppServerAddr   = tdcAllocMemChk (pModname, pRes->ai_addrlen); /*@ =type */
                                                                        /*@ -type -nullpass */
         memcpy (*ppServerAddr, pRes->ai_addr, pRes->ai_addrlen);       /*@ =type =nullpass */

         freeaddrinfo (pResSave);
                           /*@ -nullstate */
         return(TRUE);     /*@ =nullstate */
      }
   }

   /* errno set from final socket() */
   (void) tdcSNPrintf (text, (unsigned int) sizeof (text),
                "Failed to call getaddrinfo in tdcClientUdpSock46 - host(%s), PortNo(%s)", pRemAddr->ipAddr, serverPortNo);
   text[sizeof (text) - 1] = '\0';
   DEBUG_WARN  (pModname, text);

   freeaddrinfo (pResSave);

   return(FALSE);
}

/* ---------------------------------------------------------------------------- */
/*@ -exportlocal */
void Inet_pton (int family, const char*   pServerName, void*   pServerAddr)
{
   char     text[120];
   int      n = inet_pton (family, pServerName, pServerAddr);

   (void) tdcSNPrintf (text, (unsigned int) sizeof (text),
                       "Failed to call inet_pton (%s) - %d", pServerName, n);
   text[sizeof (text) - 1] = '\0';

   if (n < 0)
   {
      DEBUG_ERROR (MOD_MAIN, text);         /* errno set */
   }

   if (n == 0)
   {
      DEBUG_ERROR (MOD_MAIN, text);         /* errno not set */
   }
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

   /*tdcMemClear (pServAddr, sizeof (struct sockaddr_in));   already donme by alloc */
                                                         /*@ -type */
   pServAddr->sin_family = AF_INET;
   pServAddr->sin_port   = htons (pRemAddr->portNo);  /*@ =type */

   if (tdcUdpSocket4 (pModname, pServSockFd))
   {                                                                 /*@ -type */
      Inet_pton (AF_INET, pRemAddr->ipAddr, &pServAddr->sin_addr);   /*@ =type */

      *ppServAddr   = pServAddr;
      *pServSockLen = (T_TDC_SOCKLEN_T) sizeof (struct sockaddr_in);

      DEBUG_INFO (pModname, "Successfully setup UDP-clientSocket");
      return (TRUE);
   }
                     /*@ -mustdefine */
   return (FALSE);   /*@ =mustdefine */
}

/* ---------------------------------------------------------------------------- */

#define SOCKET_RECV_SEND_FLAGS            0     /* NO_FLAGS_SET */

T_TDC_BOOL tdcUdpSendN (const char*         pModname,
                        T_TDC_SOCKET        fdWr,
                        void*               pSendBuf,
                        UINT16              bytesExpected,
                        void*               pServerAddr,
                        T_TDC_SOCKLEN_T     serverSockLen)
{
   if (sendto (fdWr, pSendBuf, (size_t) bytesExpected, SOCKET_RECV_SEND_FLAGS,
               (const struct sockaddr *)  pServerAddr, (socklen_t) serverSockLen)!= (ssize_t) bytesExpected)
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
   if (recvfrom (fdRd, pRecvBuf, (size_t) bytesExpected, SOCKET_RECV_SEND_FLAGS, NULL, NULL) < 0)
   {
      DEBUG_INFO (pModname, "Failed to call recvfrom ()");
      return (FALSE);
   }

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

void tdcInitTcpIpWinsock (const char* pModname)
{
   pModname = pModname;

   /* Nothing to do in Linux */
}


