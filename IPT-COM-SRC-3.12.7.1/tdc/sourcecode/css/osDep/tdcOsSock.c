/*
 *  $Id: tdcOsSock.c 11861 2012-04-19 11:30:50Z gweiss $
 *
 *  DESCRIPTION    Front-End for managing the ethernet Interface
 *
 *  AUTHOR         Manfred Ritz
 *
 *  REMARKS        The switch VXWORKS has to be set
 *
 *  DEPENDENCIES
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

/* ---------------------------------------------------------------------------- */

#if !defined (VXWORKS)
   #error "This is the CSS Version of TDC_OS_SOCK, i.e. VXWORKS has to be specified"
#endif

#include <vxWorks.h>
#include <stdio.h>
#include <sockLib.h>
#include <inetLib.h>
#include <hostLib.h>
#include <unistd.h>
#include <sys/socket.h>

#define TDC_BASETYPES_H    /* Use original VxWorks Header File definitions */
#include "tdc.h"

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL Setsockopt (T_TDC_SOCKET                sockFd,
                              int                         level,
                              int                         optName,
                              const void*                 pOptVal,
                              T_TDC_SOCKLEN_T             optLen);

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL Setsockopt (T_TDC_SOCKET        sockFd,
                              int                 level,
                              int                 optName,
                              const void*         pOptVal,
                              T_TDC_SOCKLEN_T     optLen)
{
   if (    (optName == SO_SNDTIMEO)
        || (optName == SO_RCVTIMEO)
      )
   {
      /* MRi, 2005/05/19, It seems that Send/Recv Timeout are not supported by vxWorks Sockets */
      return (TRUE);
   }

   return (setsockopt (sockFd, level, optName, (char *) pOptVal, optLen) == OK);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

UINT8   tdcH2N8  (UINT8   var8)
{
   return (var8);
}

/* ---------------------------------------------------------------------------- */

UINT16  tdcH2N16 (UINT16   var16)
{
   return ((UINT16) htons (var16));
}

/* ---------------------------------------------------------------------------- */

UINT32  tdcH2N32 (UINT32   var32)
{
   return ((UINT32) htonl (var32));
}

/* ---------------------------------------------------------------------------- */

UINT8   tdcN2H8  (UINT8   var8)
{
   return (var8);
}

/* ---------------------------------------------------------------------------- */

UINT16  tdcN2H16 (UINT16   var16)
{
   return ((UINT16) ntohs (var16));
}

/* ---------------------------------------------------------------------------- */

UINT32  tdcN2H32 (UINT32   var32)
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

   *pLen = (UINT32) sizeof (struct sockaddr_in);

   /*@ -type */
   ((struct sockaddr_in *) pServAddr)->sin_len    = (u_char) sizeof (struct sockaddr_in); /* vxWorks V5.4++ */
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
      DEBUG_ERROR2 (pModname, "Size of buffer for 'struct sockaddr_in' too small (%d <-> %d)",
                    *pLen, sizeof (struct sockaddr_in));
   }

   *pLen = (UINT32) sizeof (struct sockaddr_in);

   /*@ -type */
   ((struct sockaddr_in *) pServAddr)->sin_len         = (u_char) sizeof (struct sockaddr_in); /* vxWorks V5.4++ */
   ((struct sockaddr_in *) pServAddr)->sin_family      = AF_INET;
   ((struct sockaddr_in *) pServAddr)->sin_port        = htons (portNo);
   ((struct sockaddr_in *) pServAddr)->sin_addr.s_addr = htonl (INADDR_ANY);
   /*@ =type */
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL getInetAddr (const char*             pServerAddr,
                        T_TDC_SOCK_ADDR_IN*     pSockAddr)
{
   T_TDC_BOOL     bOK = TRUE;
   u_long         s_addr;

   if (    ((s_addr = inet_addr     ((char*) pServerAddr)) == (unsigned) ERROR)
        && ((s_addr = hostGetByName ((char*) pServerAddr)) == (unsigned) ERROR)
      )
   {    
      bOK = FALSE;

   }
                                             /*@ -type */
   pSockAddr->sin_addr.s_addr = s_addr;      /*@ =type */
   
                     /*@ -mustdefine */
   return (bOK);     /*@ =mustdefine */
 }

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

 T_TDC_SOCKET Socket (int    family,
                      int    type,
                      int    protocol)
{
   T_TDC_SOCKET      socketFd = socket (family, type, protocol);
   return ((socketFd == ERROR) ? (TDC_INVALID_SOCKET) : (socketFd));
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL setSockTimeoutOpt (const char*          pModname,
                              T_TDC_SOCKET         sockFd,
                              T_TDC_TIMEVAL        timeout,
                              int                  optName)
{
   struct timeval    timevalue;

   TDC_UNUSED (pModname)

   timevalue.tv_sec  = timeout / 1000;
   timevalue.tv_usec = (timeout % 1000) * 1000;

   return (Setsockopt (sockFd, SOL_SOCKET, optName, &timevalue, (T_TDC_SOCKLEN_T) sizeof (timevalue)));
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL setLingerOpt (const char*            pModname,
                         T_TDC_SOCKET           sockFd,
                         T_TDC_BOOL             lingerOn,
                         UINT32                 seconds)
{
   T_TDC_BOOL     bResult = FALSE;

   TDC_UNUSED (pModname)

   if (seconds < MAX_INT32)
   {
      struct linger       lingerOpt;

      lingerOpt.l_linger = seconds;
      lingerOpt.l_onoff  = lingerOn;

      if (Setsockopt (sockFd, SOL_SOCKET, SO_LINGER, &lingerOpt, (T_TDC_SOCKLEN_T) sizeof (lingerOpt)))
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
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL setReuseAddrOpt (const char*         pModname,
                            T_TDC_SOCKET        sockFd,
                            T_TDC_BOOL          bReuseAddr)
{
   T_TDC_REUSEADDR     reuseAddr = bReuseAddr ? (1) : (0);

   TDC_UNUSED (pModname)

   if (Setsockopt (sockFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, (T_TDC_SOCKLEN_T) sizeof (reuseAddr)))
   {
      /*DEBUG_INFO1 (pModname, "Successfully called SetSockOpt(ReuseAddr) - reuse(%d)!", bReuseAddr); */
      return (TRUE);
   }

   DEBUG_WARN1 (pModname, "Failed to call SetSockOpt(ReuseAddr) - reuse(%d)!", bReuseAddr);

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL setRecvTimeoutOpt (const char*          pModname,
                              T_TDC_SOCKET         sockFd,
                              T_TDC_TIMEVAL        timeout)
{
   TDC_UNUSED (pModname)

   if (setSockTimeoutOpt (pModname, sockFd, timeout, SO_RCVTIMEO))
   {
      /*DEBUG_INFO1 (pModname, "Successfully called SetSockOpt(RecvTimeout) - (%d)", (int) timeout); */
      return (TRUE);
   }

   DEBUG_WARN1 (pModname, "Failed to call SetSockOpt(RecvTimeout) - (%d) !", (int) timeout);

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL setSendTimeoutOpt (const char*       pModname,
                              T_TDC_SOCKET      sockFd,
                              T_TDC_TIMEVAL     timeout)
{
   TDC_UNUSED (pModname)

   if (setSockTimeoutOpt (pModname, sockFd, timeout, SO_SNDTIMEO))
   {
      /*DEBUG_INFO1 (pModname, "Successfully called SetSockOpt(SendTimeout) - (%d)", (int) timeout); */
      return (TRUE);
   }

   DEBUG_WARN1 (pModname, "Failed to call SetSockOpt(SendTimeout) - (%d) !", (int) timeout);

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL ConnectTimo (T_TDC_SOCKET               sockFd,
                        const T_TDC_SOCK_ADDR*     pServerAddr,
                        int                        sockAddrSize,
                        UINT32                     timeout)
{
   struct timeval       timeVal;

   if (timeout == 0)
   {
      return (Connect (sockFd, pServerAddr, sockAddrSize));
   }

   timeVal.tv_sec  = timeout;
   timeVal.tv_usec = 0;

   return (connectWithTimeout (sockFd, (T_TDC_SOCK_ADDR*) pServerAddr, sockAddrSize, &timeVal) == OK);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL Connect (T_TDC_SOCKET              sockFd,
                    const T_TDC_SOCK_ADDR*    pServerAddr,
                    int                       sockAddrSize)
{
   return (connect (sockFd, (T_TDC_SOCK_ADDR *) pServerAddr, sockAddrSize) == OK);
}
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL Bind (T_TDC_SOCKET             sockFd,
                 const T_TDC_SOCK_ADDR*   pMyAddr,
                 int                      addrLen)
{
   return (bind (sockFd, (T_TDC_SOCK_ADDR*) pMyAddr, addrLen) == OK);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL Listen (T_TDC_SOCKET   sockFd,  int   backlog)
{
   return (listen (sockFd, backlog) == OK);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
T_TDC_SOCKET Accept (const char*        pModname,
                     T_TDC_SOCKET       listenFd,
                     T_TDC_SOCK_ADDR*   pSa,
                     T_TDC_SOCKLEN_T*   pSaLen)
{
   T_TDC_SOCKET     connFd = accept (listenFd, pSa, pSaLen);

   if (connFd != ERROR)
   {
      DEBUG_INFO2 (pModname, "Server(%d) successfully accepted Client(%d)", (int) listenFd, (int) connFd);
      return (connFd);
   }

   DEBUG_ERROR1 (pModname, "Error calling accept listenFd(%d)!", listenFd);

   return (TDC_INVALID_SOCKET);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL ShutDown (T_TDC_SOCKET   sockFd,
                     int            how)
{
   return (shutdown (sockFd, how) == OK);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL CloseSocket (T_TDC_SOCKET   sockFd)
{
   return (close (sockFd) == OK);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/*lint -save -esym(429, pRecvBuf) pRecvBuf is not custotory */
T_TDC_BOOL tdcTcpRecvN (const char*       pModname,
                        T_TDC_SOCKET      fdRd,
                        void*             pRecvBuf,
                        UINT32*           pBytesReceived,
                        UINT32            bytesExpected)
{
   *pBytesReceived = 0;

   if (bytesExpected > 0)
   {
      for (; (*pBytesReceived) < bytesExpected; )
      {
         int               requSize = bytesExpected - (*pBytesReceived);
         int               recvSize;

         recvSize = (int) read (fdRd, pRecvBuf, (size_t) requSize); /*lint !e64 pRecvBuf type ok */

         if (recvSize <= 0)
         {
            const char  *pText = (recvSize == 0) ? ("Sender gracefully shutdown!")
                                                 : ("Sender shutdown!");
            DEBUG_INFO (pModname, pText);
            *pBytesReceived = recvSize;

            return (FALSE);
         }
         else
         {
            if (recvSize > requSize)
            {
               DEBUG_ERROR (pModname, "Too many Bytes read !");
               *pBytesReceived = (UINT32) 0xFFFFFFFF;

               return (FALSE);
            }
            else
            {
               *pBytesReceived += recvSize;
               pRecvBuf         = & (((UINT8 *) pRecvBuf)[recvSize]);
            }
         }
      }
   }

   return ((*pBytesReceived) == bytesExpected);
}
/*lint -restore*/
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcTcpSendN (const char*       pModname,
                        T_TDC_SOCKET      fdWr,
                        void*             pSendBuf,
                        UINT32*           pBytesSent,
                        UINT32            bytesExpected)
{
   *pBytesSent = 0;

   if (bytesExpected > 0)
   {
      for (; (*pBytesSent) < bytesExpected;)
      {
         int      requSize = bytesExpected - (*pBytesSent);
         int      sendSize;

         sendSize = (int) write (fdWr, (char *) pSendBuf, (size_t) requSize);

         if (sendSize <= 0)
         {
            const char  *pText = (sendSize == 0) ? ("Receiver gracefully shutdown!")
                                                 : ("Receiver shutdown!");
            DEBUG_INFO (pModname, pText);
            *pBytesSent = sendSize;

            return (FALSE);
         }
         else
         {
            if (sendSize > requSize)
            {
               DEBUG_ERROR (pModname, "Too many Bytes written !");
               *pBytesSent = -1;

               return (FALSE);
            }
            else
            {
               *pBytesSent += sendSize;
               pSendBuf     = & (((UINT8 *) pSendBuf)[sendSize]);
            }
         }
      }
   }

   return ((*pBytesSent) == bytesExpected);
}

/* ---------------------------------------------------------------------------- */

#define SOCKET_RECV_SEND_FLAGS            0     /* NO_FLAGS_SET */

T_TDC_BOOL tdcUdpRecvN (const char*       pModname,
                        T_TDC_SOCKET      fdRd,
                        void*             pRecvBuf,
                        UINT16            bytesExpected)
{
   /* We are currently not interrested, in the client address */

   if (recvfrom (fdRd, pRecvBuf, bytesExpected, SOCKET_RECV_SEND_FLAGS, NULL, NULL) == ERROR)/*lint !e64 pRecvBuf type ok */
   {
      DEBUG_INFO (pModname, "Failed to call os_ip_recvfrom ()");
      return     (FALSE);
   }

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcUdpSendN (const char*          pModname,
                        T_TDC_SOCKET         fdWr,
                        void*                pSendBuf,
                        UINT16               bytesExpected,
                        void*                pServerAddr,
                        T_TDC_SOCKLEN_T      serverSockLen)
{
   if (sendto (fdWr,
               pSendBuf, /*lint !e64 pSendBuf type ok */
               bytesExpected,
               SOCKET_RECV_SEND_FLAGS,
               (T_TDC_SOCK_ADDR *) pServerAddr,
               serverSockLen)                          == ERROR)
   {
      DEBUG_INFO (pModname, "Failed to call os_ip_sendto ()");

      return (FALSE);
   }

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcServerUdpSock4 (const char*  pModname, T_TDC_SOCKET*    pServerSockFd, UINT16   portNo)
{
   if (tdcUdpSocket4 (pModname, pServerSockFd))
   {
      T_TDC_SOCK_ADDR_IN        serverAddr;

      tdcMemClear (&serverAddr, (UINT32) sizeof (T_TDC_SOCK_ADDR_IN));

      serverAddr.sin_len         = (u_char) sizeof (T_TDC_SOCK_ADDR_IN);   /* vxWorks V5.4++ */
      serverAddr.sin_family      = AF_INET;
      serverAddr.sin_port        = htons (portNo);
      serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);

      if (Bind (*pServerSockFd, (T_TDC_SOCK_ADDR *) &serverAddr, (int) sizeof (serverAddr)))
      {
         DEBUG_INFO1 (pModname, "Successfully called Bind(%d)", (int) portNo);
         return (TRUE);
      }

      DEBUG_WARN1    (pModname, "Failed to call Bind(%d)", (int) portNo);
      (void) tdcCloseSocket (pModname, pServerSockFd);

      return (FALSE);
   }

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcClientUdpSock4 (const char*             pModname,
                              T_TDC_SOCKET*           pClientSockFd,
                              T_TDC_SERVER_ADDR*      pRemAddr,
                              void**                  ppServerAddr,
                              T_TDC_SOCKLEN_T*        pServerSockLen)
{
   T_TDC_SOCK_ADDR_IN*    pServerAddr;

   *ppServerAddr = NULL;

   if (!tdcUdpSocket4 (pModname, pClientSockFd))
   {                    
      return (FALSE);   
   }

   pServerAddr = (T_TDC_SOCK_ADDR_IN*)tdcAllocMemChk (pModname, (UINT32) sizeof (T_TDC_SOCK_ADDR_IN));

   if (pServerAddr != NULL)
   {
      tdcMemClear (pServerAddr, (UINT32) sizeof (T_TDC_SOCK_ADDR_IN));

      /*@ -type */
      pServerAddr->sin_len    = (u_char) sizeof (T_TDC_SOCK_ADDR_IN);   /* vxWorks V5.4++ */
      pServerAddr->sin_family = AF_INET;
      pServerAddr->sin_port   = htons (pRemAddr->portNo);

      if (    ((pServerAddr->sin_addr.s_addr = inet_addr     (pRemAddr->ipAddr)) == (unsigned long) ERROR)
           && ((pServerAddr->sin_addr.s_addr = hostGetByName (pRemAddr->ipAddr)) == (unsigned long) ERROR)
         )
      {
         DEBUG_INFO1 (pModname, "Failed to set up client UDP socket, closing it (%d)", *pClientSockFd);
         (void) tdcCloseSocket (pModname, pClientSockFd);
         tdcFreeMem (pServerAddr);
                           
         return (FALSE);   
      }
      /*@ =type */

      *ppServerAddr   = pServerAddr;
      *pServerSockLen = (T_TDC_SOCKLEN_T) sizeof (T_TDC_SOCK_ADDR_IN);

      return (TRUE);
   }
                     
   return (FALSE);   
}

/* ---------------------------------------------------------------------------- */

void tdcInitTcpIpWinsock (const char* pModname)
{
   /* Nothing to do in CSS */

   TDC_UNUSED (pModname)
}
