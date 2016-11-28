/*******************************************************************************
*  COPYRIGHT      : (c) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT        : IPTrain
*
*  MODULE         : vos_socket.h
*
*  ABSTRACT       : Operating system independent socket definitions
*
********************************************************************************
*  HISTORY     :
*	
* $Id: vos_socket.h 12809 2012-12-17 15:08:43Z gweiss $
*
*  CR-3326 (Bernd Loehr, 2012-02-10)
*           Improvement for 3rd party use / Darwin port added.
*
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
*
*******************************************************************************/
#ifndef VOSSOCKET_H
#define VOSSOCKET_H

/*******************************************************************************
* INCLUDES */

/* Windows XP */
#if defined(WIN32)
#include <winsock2.h>	/* Standard BSD socket definitions */
#include <ws2tcpip.h>	/* TCP and UDP specific defines (socket options) */

/* LINUX */
#elif defined(LINUX) || defined(DARWIN)
#include <sys/socket.h>	
#include <netinet/in.h>
#include <netinet/ip.h>

/* VXWORKS */
#elif defined(VXWORKS)
#include <netinet/in.h> /* for htonl, ntohl etc */
#include <inetLib.h>
#include <sockLib.h>
#include <ioLib.h>


#elif defined(__INTEGRITY)
#include <INTEGRITY.h>
#include <sys/types.h>	
#include <netinet/in.h> /* for htonl, ntohl etc */
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>

#endif

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* DEFINES */
/* Windows XP */
#if defined(WIN32)

/* LINUX */
#elif defined(LINUX) || defined(DARWIN)
#define INVALID_SOCKET 	-1	/* ??????????????? */
#define SOCKET_ERROR		-1	/* ??????????????? */
#define SD_BOTH			SHUT_RDWR
#define closesocket		close

/* VXWORKS */
#elif defined(VXWORKS)
#define INVALID_SOCKET	ERROR
#define SOCKET_ERROR		ERROR
#define SD_BOTH			2
#define closesocket		close


/* INTEGRITY */
#elif defined(__INTEGRITY)
#define INVALID_SOCKET	-1
#define SOCKET_ERROR		-1
#define SD_BOTH			2
#define closesocket		close

#endif

/*******************************************************************************
* TYPEDEFS */
/* Windows XP */
#if defined(WIN32)
typedef struct sockaddr_in SOCKADDR_IN;
typedef int IPT_TTL_TYPE;        /* Size of multicast ttl */
/* LINUX */
#elif defined(LINUX) || defined(DARWIN)
typedef int SOCKET;					/* Socket definition    */
typedef struct sockaddr_in SOCKADDR_IN;
typedef int IPT_TTL_TYPE;        /* Size of multicast ttl */

/* VXWORKS */
#elif defined(VXWORKS)
typedef int SOCKET;					/* Socket definition    */
typedef struct sockaddr_in SOCKADDR_IN;
typedef char IPT_TTL_TYPE;       /* Size of multicast ttl */

/* INTEGRITY */
#elif defined(__INTEGRITY)
typedef int SOCKET;					/* Socket definition    */
typedef struct sockaddr_in SOCKADDR_IN;
typedef char IPT_TTL_TYPE;       /* Size of multicast ttl */

#endif

/*******************************************************************************
* GLOBALS */

/*******************************************************************************
* LOCALS */

/*******************************************************************************
* LOCAL FUNCTIONS */

/*******************************************************************************
* GLOBAL FUNCTIONS */
/* Socket */
void IPTVosPrintSocketError(void);


#ifdef __cplusplus
}
#endif

#endif /* VOSSOCKET_H */
