/************************************************************************/
/*  (C) COPYRIGHT 2015 by Bombardier Transportation                     */
/*                                                                      */
/*  Bombardier Transportation Sweden Dep. GSC/NETPS                     */
/************************************************************************/
/*                                                                      */
/*  PROJECT:      ED Download                                           */
/*                                                                      */
/*  MODULE:       dledsDefines                                          */
/*                                                                      */
/*  ABSTRACT:     This is the header file for platform specific code    */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/*  REMARKS:                                                            */
/*                                                                      */
/*  DEPENDENCIES: See include list                                      */
/*                                                                      */
/*  ACCEPTED:                                                           */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/*  HISTORY:                                                            */
/*                                                                      */
/*  version  yy-mm-dd  name       depart.    ref   status               */
/*  -------  --------  ---------  -------    ----  ---------            */
/*    1.0    15-06-08  S.Linder   GSC/NETPS   --   created              */
/*                                                                      */
/************************************************************************/
#ifndef DLEDS_DEFINES_H
#define DLEDS_DEFINES_H

#ifdef LINUX
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <sys/statvfs.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
    #include <dirent.h>
    #include <time.h>
    #include <arpa/inet.h>

    #ifndef BOOL
      #define BOOL char
    #endif
    #ifndef CHAR8
      #define CHAR8 char
    #endif
    #ifndef UCHAR8
      #define UCHAR8 unsigned char
    #endif
    #ifndef UINT8
      #define UINT8 unsigned char
    #endif
    #ifndef UINT16
      #define UINT16 unsigned short
    #endif
    #ifndef INT16
      #define INT16 short
    #endif
    #ifndef UINT32
      #define UINT32 unsigned int
    #endif
    #ifndef INT32
      #define INT32  int
    #endif

    #ifndef TRUE
      #define TRUE       (0==0)          /* Boolean True       */
    #endif

    #ifndef FALSE
      #define FALSE      (0!=0)          /* Boolean False      */
    #endif

    #ifndef OK
      #define OK        0
    #endif

    #ifndef ERROR
      #define ERROR   (-1)
    #endif

    #ifndef NULL
      #ifdef __cplusplus   /* to be compatible with C++ */
        #define NULL    0               /* Used for pointers */
      #else
        #define NULL    ((void *)0)     /* Used for pointers */
      #endif
    #endif
#else
    #error "LINUX should be defined at compile stage"
#endif // #ifdef LINUX


#include "dludatarep.h"

/* IPTCom */
#define DLEDS_REQ_QUEUE_NAME        "dledsRequestQueue"
#define DLEDS_RES_QUEUE_NAME        "dledsResultQueue"
#define DLEDS_ECHO_QUEUE_NAME       "dledsEchoQueue"
#define DLEDS_PROGRESS_QUEUE_NAME   "dledsResponseQueue"
#define DLEDS_SCHEDULE_GROUP_ID     1
#define EDIDENT_COMID               271
#define EDIDENT_DESTURI             NULL

#define VERSION_STR_MAX_LEN         32

#endif /* DLEDS_DEFINES_H */
