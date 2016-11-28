#//
#// $Id: config.mk 11875 2012-11-12 17:44:18Z hlindbom $
#//
#// DESCRIPTION    Tdc top level config.mk
#//
#// AUTHOR         Bernhard Remenyi          ...
#//
#// All rights reserved. Reproduction, modification, use or disclosure
#// to third parties without express authority is forbidden.
#// Copyright Bombardier Transportation GmbH, Germany, 2004.
#//

#
# Include the make variables (CC, etc...)
#

ECHO    = echo

ifeq ($(TARGET_OS), VXWORKS)
   AS	= $(TCPATH)as$(TOOLCHAIN)
   LD	= $(TCPATH)ld$(TOOLCHAIN)
   CC	= $(TCPATH)cc$(TOOLCHAIN)
   CPP	= $(CC) -E
   AR	= $(TCPATH)ar$(TOOLCHAIN)
   NM	= $(TCPATH)nm$(TOOLCHAIN)
   STRIP   = $(TCPATH)strip$(TOOLCHAIN)
   OBJCOPY = $(TCPATH)objcopy$(TOOLCHAIN)
   OBJDUMP = $(TCPATH)objdump$(TOOLCHAIN)
   RANLIB  = $(TCPATH)RANLIB$(TOOLCHAIN)
else
   #WINDOWS, LINUX and all other
   AS	= $(TOOLCHAIN)as
   LD	= $(TOOLCHAIN)ld
   CC	= $(TOOLCHAIN)gcc
   CPP	= $(CC) -E
   AR	= $(TOOLCHAIN)ar
   NM	= $(TOOLCHAIN)nm
   STRIP	= $(TOOLCHAIN)strip
   OBJCOPY = $(TOOLCHAIN)objcopy
   OBJDUMP = $(TOOLCHAIN)objdump
   RANLIB	= $(TOOLCHAIN)RANLIB
endif

ifeq ($(DEBUG), TRUE)
   DBGFLAGS= -g
else
   #RELFLAGS= $(PLATFORM_RELFLAGS)
   #OPTFLAGS= #-Os #-fomit-frame-pointer
endif

CPPFLAGS := $(DBGFLAGS) $(OPTFLAGS) $(RELFLAGS)	\
   -Isourcecode/api	                           	\
   -Isourcecode/shared/include	                	\
	-Isourcecode/$(OSDEP_NAME)/osDep/include	\
 	-Itest/LUA-$(USE_LUA_VERSION)		\
   -I../iptcom/sourcecode/api				\
   -D$(TARGET_OS)				\
   -DO_HAVE_CONVERSION				\
	$(PLATFORM_CPPFLAGS)			\

ifeq ($(TARGET_OS), LINUX)
   CC_SH_OPT := -fPIC
else
   ifeq ($(TARGET_OS), WINDOWS)
      CC_SH_OPT := -DO_WIN32_DLL
   else
      CC_SH_OPT :=
   endif
endif


CFLAGS := $(CPPFLAGS) -Wall -W

AFLAGS_DEBUG := -Wa,-gstabs
AFLAGS := $(AFLAGS_DEBUG) -D__ASSEMBLY__ $(CPPFLAGS)

LDFLAGS :=  $(PLATFORM_LDFLAGS)


#########################################################################

export	HPATH CROSS_COMPILE \
	AS LD CC CPP AR NM STRIP OBJCOPY OBJDUMP \
	MAKE
export	TEXT_BASE PLATFORM_CPPFLAGS PLATFORM_RELFLAGS CPPFLAGS CFLAGS AFLAGS

#########################################################################

%.s:	%.S
	$(CPP) $(AFLAGS) -o $@ $(CURDIR)/$<
%.o:	%.S
	$(CC) $(AFLAGS) -c -o $@ $(CURDIR)/$<
$(OUTDIR_STATIC)/%.o: 	%.c
	@$(ECHO) ' --- Compiling $(@F)'
	$(CC) $(CFLAGS) -c $< -o $@
$(OUTDIR_STATIC)/%.o: 	%.cpp
	@$(ECHO) ' --- Compiling $(@F)'
	$(CC) $(CPPFLAGS) -c $< -o $@

# 2008/07/10, MRi - TDC shared libries - Position independent code (linux)
#                                        DLL export definitions (windows)
$(OUTDIR_SHARED)/%.o: 	%.c
	@$(ECHO) ' --- Compiling $(@F)'
	$(CC) $(CFLAGS) $(CC_SH_OPT) -c $< -o $@

$(OUTDIR_SHARED)/%.o: 	%.cpp
	@$(ECHO) ' --- Compiling $(@F)'
	$(CC) $(CPPFLAGS) $(CC_SH_OPT) -c $< -o $@

#########################################################################
