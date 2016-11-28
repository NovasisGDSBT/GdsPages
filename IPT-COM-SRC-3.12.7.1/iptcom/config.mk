#//
#// $Id: config.mk 11878 2012-11-12 17:51:45Z hlindbom $
#//
#// DESCRIPTION    Top level config.mk
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
STRIP	= $(TCPATH)strip$(TOOLCHAIN)
OBJCOPY = $(TCPATH)objcopy$(TOOLCHAIN)
OBJDUMP = $(TCPATH)objdump$(TOOLCHAIN)
RANLIB	= $(TCPATH)RANLIB$(TOOLCHAIN)
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
#OPTFLAGS= -save-temps
OPTFLAGS= -O2
endif

#VER = 2
#REV = 1
#UPD = 0
#EVO = 3
# 
#CPPFLAGS := $(DBGFLAGS) $(OPTFLAGS) $(RELFLAGS) \
#	-Isrc/api \
#	-Isrc/prv \
#	-I../tdc/sourcecode/api \
#	-D$(TARGET_OS) \
#	-DIPT_VERSION=$(VER) \
#	-DIPT_RELEASE=$(REV) \
#	-DIPT_UPDATE=$(UPD)	\
#	-DIPT_EVOLUTION=$(EVO) \
#	$(PLATFORM_CPPFLAGS)
CPPFLAGS := $(DBGFLAGS) $(OPTFLAGS) $(RELFLAGS) \
	-Isourcecode/api \
	-Isourcecode/prv \
	-I../tdc/sourcecode/api \
	-D$(TARGET_OS) \
	-Wall -W\
	$(PLATFORM_CPPFLAGS)

#CFLAGS := $(CPPFLAGS) -Wall
CFLAGS := $(CPPFLAGS)

AFLAGS_DEBUG := -Wa,-gstabs
AFLAGS := $(AFLAGS_DEBUG) -D__ASSEMBLY__ $(CPPFLAGS)

LDFLAGS :=  $(PLATFORM_LDFLAGS)

TESTFLAGS := 
ifeq ($(ARCH), linux-arm-hmi400)
TESTFLAGS := -DHMI400CCS
endif
ifeq ($(ARCH), linux-arm-hmi400wrl)
TESTFLAGS := -DHMI400WRL
endif
ifeq ($(ARCH), linux-arm-hmi500)
TESTFLAGS := -DHMI500CCS
endif
ifeq ($(ARCH), linux-arm-hmi500wrl)
TESTFLAGS := -DHMI500WRL
endif
ifeq ($(ARCH), vxworks-ppc-css2)
TESTFLAGS := -DCCUOCSS2
endif
ifeq ($(ARCH), vxworks-ppc-css3)
TESTFLAGS := -DCCUOCSS3
endif
ifeq ($(ARCH), linux-ppc)
TESTFLAGS := -DCCUC
endif

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

#### Code for Linux single process, Windows and VxWorks
$(OUTDIR)/%.o: 	%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTDIR)/%.o: 	%.cpp
	@$(ECHO) ' --- CPP Compile $(@F)'
	$(CC) $(CPPFLAGS) -c $< -o $@

test/$(OUTDIR)/%.o: 	%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) $(TESTFLAGS) -c $< -o $@

test/$(OUTDIR)/%.o: 	../example/%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) -c $< -o $@

test/$(OUTDIR)/%.o: 	%.cpp
	@$(ECHO) ' --- CPP Compile $(@F)'
	$(CC) $(CPPFLAGS) -c $< -o $@

#### Code for Linux multiprocess
$(OUTDIR2)/%.o: 	%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) -DLINUX_MULTIPROC -c $< -o $@

$(OUTDIR2)/%.o: 	%.cpp
	@$(ECHO) ' --- CPP Compile $(@F)'
	$(CC) $(CPPFLAGS) -DLINUX_MULTIPROC -c $< -o $@

test/$(OUTDIR2)/%.o: 	%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) -DLINUX_MULTIPROC $(TESTFLAGS) -c $< -o $@

test/$(OUTDIR2)/%.o: 	%.cpp
	@$(ECHO) ' --- CPP Compile $(@F)'
	$(CC) $(CPPFLAGS) -DLINUX_MULTIPROC -c $< -o $@

# Position independent code for IPTCom shared libray
$(OUTDIR2)/%_pic.o: 	%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) -fPIC -DLINUX_MULTIPROC -DLINUX_CLIENT -c $< -o $@

$(OUTDIR2)/%_pic.o: 	%.cpp
	@$(ECHO) ' --- CPP Compile $(@F)'
	$(CC) $(CPPFLAGS) -fPIC -DLINUX_MULTIPROC -DLINUX_CLIENT -c $< -o $@

# Test code for secondary process 
$(OUTDIR2)/%_client.o: 	%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) -DLINUX_MULTIPROC -DLINUX_CLIENT -c $< -o $@

$(OUTDIR2)/%_client.o: 	%.cpp
	@$(ECHO) ' --- CPP Compile $(@F)'
	$(CC) $(CPPFLAGS) -DLINUX_MULTIPROC -DLINUX_CLIENT -c $< -o $@

test/$(OUTDIR2)/%_client.o: 	%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) -DLINUX_MULTIPROC -DLINUX_CLIENT $(TESTFLAGS) -c $< -o $@

test/$(OUTDIR2)/%_client.o: 	../example/%.c
	@$(ECHO) ' --- Compile $(@F)'
	$(CC) $(CFLAGS) -DLINUX_MULTIPROC -DLINUX_CLIENT -c $< -o $@

test/$(OUTDIR2)/%_client.o: 	%.cpp
	@$(ECHO) ' --- CPP Compile $(@F)'
	$(CC) $(CPPFLAGS) -DLINUX_MULTIPROC -DLINUX_CLIENT -c $< -o $@

#########################################################################
