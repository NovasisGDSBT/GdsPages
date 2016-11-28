/******************************************************************************
 * COPYRIGHT : (c) 2006 Bombardier Transportation
 ******************************************************************************
 * PRODUCT   : DVS
 *
 * ABSTRACT  : Download and Versioning Service.
 *             Basic data types.
 *
 ******************************************************************************
 * 2006-11-02 Kue   Created.
 *  
 *****************************************************************************/



/******************************************************************************
 * INCLUDES
 */



#ifndef DVS_TYPES_H
#define DVS_TYPES_H


/******************************************************************************
 * INCLUDES
 */


/******************************************************************************
 * DEFINES
 */




#ifdef WIN32
  /* If we are on Win32 platform, we force the include
     of the windows base data types header file in order 
     to ensure, that the UINT32 data type is actually defined.
     That's necessary because C source modules might include
     the mcpdatarep.h header file without including any
     windows related system header files.              */
  #include <basetsd.h>

  #ifndef UINT8  
    #define UINT8       unsigned char       /* 8-bit                     0 ... 255 */
  #endif

#elif defined O_CSS
  #include "css.h"
#elif defined O_INTEGRITY
  #include <INTEGRITY.h>
  #include <typedefs.h>

  #ifndef UINT8
    #define UINT8       unsigned char
  #endif
  #ifndef UINT16
    #define UINT16       unsigned short
  #endif
  #ifndef UINT32
    #define UINT32       unsigned long
  #endif
  #ifndef INT16
    #define INT16       signed short
  #endif



#else

  #ifndef INT32  
    #ifdef O_TGT_16
      #define   INT32       signed long     /* 32-bit  -2147483648 ... +2147483647 */
    #else
      #define   INT32       signed int      /* 32-bit  -2147483648 ... +2147483647 */
    #endif
  #endif

  #ifndef UINT32
    #ifdef O_TGT_16
      #define   UINT32      unsigned long   /* 32-bit             0 ... 4294967295 */
    #else
      #define   UINT32      unsigned int    /* 32-bit             0 ... 4294967295 */
    #endif
  #endif

  #ifndef UINT8  
    #define UINT8       unsigned char       /* 8-bit                     0 ... 255 */
  #endif

  #ifndef UINT16 
    #define UINT16      unsigned short      /* 16-bit                  0 ... 65535 */
  #endif
  
  #ifndef INT16 
    #define INT16       signed short        /* 16-bit                  -32768 ... 32767 */
  #endif


#endif







#ifndef TRUE
  #define TRUE       (0==0)          /* Boolean True       */
#endif

#ifndef FALSE
  #define FALSE      (0!=0)          /* Boolean False      */
#endif


#ifndef NULL
  #ifdef __cplusplus   /* to be compatible with C++ */
    #define NULL    0               /* Used for pointers */
  #else
    #define NULL    ((void *)0)     /* Used for pointers */
  #endif
#endif


#ifndef OK
  #define OK        0
#endif

#ifndef ERROR
  #define ERROR   (-1)
#endif


#define UNUSED_ARGUMENT(x)  (void)x



#endif

/* end of file */
