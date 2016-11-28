#//
#// $Id: Makefile 11875 2012-11-12 17:44:18Z hlindbom $
#//
#// DESCRIPTION    TDC top level Makefile
#//
#// AUTHOR         Manfred Ritz          ...
#//
#// All rights reserved. Reproduction, modification, use or disclosure
#// to third parties without express authority is forbidden.
#// Copyright Bombardier Transportation GmbH, Germany, 2004.
#//


 .EXPORT_ALL_VARIABLES:

# make VERBOSE=1 if you want so all output
ifdef VERBOSE
   SILENCE_CMD =
else
   ifeq ($(OS), Windows_NT)
      SILENCE_CMD = > nul
   else
      SILENCE_CMD = > /dev/null
   endif
.SILENT:
endif

RM = rm -f
MD = mkdir -p
CP = cp

ifeq ($(OS), Windows_NT)
   TOPDIR:=$(shell cd;)
else
   TOPDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
   #export TOPDIR
endif

# 2008/07/10, MRi - define used LUA version
USE_LUA_VERSION = 514

#Check if configuration present
ifeq (config/config.mk,$(wildcard config/config.mk))
# load ARCH and CPU specific configuration
include config/config.mk

vpath %.c   sourcecode/shared sourcecode/$(OSDEP_NAME)/osDep test/sourcecode test/sourcecode/Script test/LUA-$(USE_LUA_VERSION)
vpath %.cpp sourcecode/shared test/sourcecode/Script

CROSS_COMPILE:=$(TOOLCHAIN)

# 2008/07/10, MRi - separation of outdirs for static and shared objects
#                 - as well as a delivery directory
ifeq ($(DEBUG), TRUE)
   OUTDIR_STATIC  = output/$(ARCH)-static-dbg
   OUTDIR_SHARED  = output/$(ARCH)-shared-dbg
   DELIVER_STATIC = export-tdc/$(ARCH)-static-dbg
   DELIVER_SHARED = export-tdc/$(ARCH)-shared-dbg
else
   OUTDIR_STATIC  = output/$(ARCH)-static-rel
   OUTDIR_SHARED  = output/$(ARCH)-shared-rel
   DELIVER_STATIC = export-tdc/$(ARCH)-static-rel
   DELIVER_SHARED = export-tdc/$(ARCH)-shared-rel
endif

ifeq ($(NO_CPP_SUPPORT), TRUE)
   TDC_CLI_CPP_OBJS := 
   TDC_PIQ_CPP_OBJS := 
   STD_CPP_LIB      :=
else
   TDC_CLI_CPP_OBJS := tdcApi.o
   TDC_PIQ_CPP_OBJS := tdcApiPiq.o
   STD_CPP_LIB      := -lstdc++
endif

# load global configuration
include config.mk

SUBDIRS	= sourcecode

OBJS_OS         = tdcDbgUtil.o                \
		  tdcOsMem.o                  \
		  tdcOsMisc.o                 \
		  tdcOsResolve.o              \
		  tdcOsSema.o                 \
		  tdcOsSock.o                 \
		  tdcOsString.o               \
		  tdcOsThread.o               \
		  tdcOsTimer.o                \
		      		   
OBJS_DAEMON     = $(OBJS_OS)                  \
		  tdcConfig.o                 \
		  tdcDaemon.o                 \
		  tdcDB.o                     \
		  tdcDBipt.o                  \
		  tdcDBuic.o                  \
		  tdcInit.o                   \
		  tdcIpc.o                    \
		  tdcIptCom.o                 \
		  tdcSock.o                   \
		  tdcTCyclic.o                \
		  tdcTIpc.o                   \
		  tdcTMsgData.o               \
		  tdcTProcData.o              \
		  tdcXML2bin.o                \
		  tdcIptdir_wire.o            \
		  tdcCst_info.o               \
		  tdcCrc32.o                  \
		  tdcPicoxml.o                \

OBJS_CLIENT     = $(OBJS_OS)                  \
		  tdcLib.o                    \
		  tdcIpc.o                    \
		  tdcSock.o                   \
		  $(TDC_CLI_CPP_OBJS)         \

OBJS_PIQ        = tdcPiq.o                    \
		  $(TDC_PIQ_CPP_OBJS)         \

OBJS_TST_DAEMON = tdcMain.o                   \
                  tdcIptComStubs.o            \

OBJS_TST_APPL   = tdcTest.o                   \
                  tdcTestConfig.o             \
                  tdcTestCreateProtoFile.o    \

OBJS_SCRIPT     = luaFrame.o                  \
                  tdcScript.o                 \
		  wrapLua.o                   \
		  wrapLuaPiq.o                \
		  wrapTdc.o                   \
		  
OBJS_LUA        = lapi.o                      \
		  lauxlib.o                   \
		  lbaselib.o                  \
		  lcode.o                     \
		  ldblib.o                    \
		  ldebug.o                    \
		  ldo.o                       \
		  ldump.o                     \
		  lfunc.o                     \
		  lgc.o                       \
		  linit.o                     \
		  liolib.o                    \
		  llex.o                      \
		  lmathlib.o                  \
		  lmem.o                      \
		  loadlib.o                   \
		  lobject.o                   \
		  lopcodes.o                  \
		  loslib.o                    \
		  lparser.o                   \
		  lstate.o                    \
		  lstring.o                   \
		  lstrlib.o                   \
		  ltable.o                    \
		  ltablib.o                   \
		  ltm.o                       \
		  lundump.o                   \
		  lvm.o                       \
		  lzio.o                      \


#########################################################################

OBJS_DAEMON_STATIC     = $(addprefix $(OUTDIR_STATIC)/,$(notdir $(OBJS_DAEMON)))
OBJS_TST_DAEMON_STATIC = $(addprefix $(OUTDIR_STATIC)/,$(notdir $(OBJS_TST_DAEMON)))
OBJS_TST_APPL_STATIC   = $(addprefix $(OUTDIR_STATIC)/,$(notdir $(OBJS_TST_APPL)))
OBJS_CLIENT_STATIC     = $(addprefix $(OUTDIR_STATIC)/,$(notdir $(OBJS_CLIENT)))
OBJS_PIQ_STATIC        = $(addprefix $(OUTDIR_STATIC)/,$(notdir $(OBJS_PIQ)))
OBJS_LUA_STATIC        = $(addprefix $(OUTDIR_STATIC)/,$(notdir $(OBJS_LUA)))
OBJS_SCRIPT_STATIC     = $(addprefix $(OUTDIR_STATIC)/,$(notdir $(OBJS_SCRIPT)))

OBJS_CLIENT_SHARED     = $(addprefix $(OUTDIR_SHARED)/,$(notdir $(OBJS_CLIENT)))
OBJS_PIQ_SHARED        = $(addprefix $(OUTDIR_SHARED)/,$(notdir $(OBJS_PIQ)))

#########################################################################

TEST_APPL        = 
TEST_SCRIPT      = 
TEST_SCRIPT_SH   = 

AR_DAEMON        = $(OUTDIR_STATIC)/libtdcd.a
AR_DAEMON_T      = $(OUTDIR_STATIC)/libtdcsimudevd.a               # Simulate own device 

AR_LUA           = $(OUTDIR_STATIC)/libtdclua.a
AR_CLIENT        = $(OUTDIR_STATIC)/libtdcipc.a
AR_PIQ           = $(OUTDIR_STATIC)/libtdcpiq.a
SHLIB_CLIENT     = 
SHLIB_PIQ        = 

MK_DIRS          = $(OUTDIR_STATIC)   $(DELIVER_STATIC)   $(DELIVER_STATIC)/../api
RM_DIRS          = $(OUTDIR_STATIC)/* $(DELIVER_STATIC)/* $(DELIVER_STATIC)/../api/* 

ifeq ($(TARGET_OS), LINUX)
   ifneq ($(NO_CPP_SUPPORT), TRUE)
      TEST_SCRIPT    = $(OUTDIR_STATIC)/tdcScript
      TEST_SCRIPT_SH = $(OUTDIR_STATIC)/tdcScriptSh
      SHLIB_CLIENT   = $(OUTDIR_SHARED)/libtdcipcsh.so
      SHLIB_PIQ      = $(OUTDIR_SHARED)/libtdcpiqsh.so
   endif

   TEST_DAEMON   = $(OUTDIR_STATIC)/testDaemon
   TEST_APPL     = $(OUTDIR_STATIC)/tdcTest22
   BLD_CMD       = $(CC)
   BLD_DAEMON    = $(CFLAGS) -lpthread -ltdcsimudevd
   BLD_TEST      = $(CFLAGS) -lpthread -ltdcipc
   BLD_SCRIPT    = $(CFLAGS) -lm -lpthread  $(STD_CPP_LIB)
   BLD_SHSCRIPT  = $(CFLAGS) -lm -lpthread -ltdcipcsh -ltdcpiqsh $(STD_CPP_LIB)
   BLD_SHLIB_CLI = -Wl,-soname,libtdcipc.so.3
   BLD_SHLIB_PIQ =
   MK_DIRS      +=  $(OUTDIR_SHARED)    $(DELIVER_SHARED)  
   RM_DIRS      +=  $(OUTDIR_SHARED)/*  $(DELIVER_SHARED)/*
endif

ifeq ($(TARGET_OS), WINDOWS)
   ifneq ($(NO_CPP_SUPPORT), TRUE)
      TEST_SCRIPT         = $(OUTDIR_STATIC)/tdcScript.exe
      TEST_SCRIPT_SH      = $(OUTDIR_STATIC)/tdcScriptSh.exe
      SHLIB_CLIENT        = $(OUTDIR_SHARED)/tdcipc.dll
      SHLIB_CLIENT_IMPORT = $(OUTDIR_SHARED)/tdcipc.lib
      SHLIB_PIQ           = $(OUTDIR_SHARED)/tdcpiq.dll
      SHLIB_PIQ_IMPORT    = $(OUTDIR_SHARED)/tdcpiq.lib
   endif

   TEST_DAEMON   = $(OUTDIR_STATIC)/testDaemon.exe
   TEST_APPL     = $(OUTDIR_STATIC)/tdcTest22.exe
   BLD_CMD       = $(CC)
   BLD_DAEMON    = -ltdcsimudevd $(WIN_LIBS)
   BLD_TEST      = -ltdcipc      $(WIN_LIBS)
   BLD_SCRIPT    = $(WIN_LIBS)
   BLD_SHSCRIPT  = $(SHLIB_PIQ_IMPORT) $(SHLIB_CLIENT_IMPORT) $(WIN_LIBS)
   BLD_SHLIB_CLI = $(WIN_LIBS) sourcecode/win32/WinTdcIpcDll/tdcipc.def    \
                   -Wl,--out-implib,$(OUTDIR_SHARED)/tdcipc.lib
   BLD_SHLIB_PIQ = $(OUTDIR_SHARED)/tdcipc.lib     \
                   -Wl,--out-implib,$(OUTDIR_SHARED)/tdcpiq.lib
   MK_DIRS      +=  $(OUTDIR_SHARED)    $(DELIVER_SHARED)  
   RM_DIRS      +=  $(OUTDIR_SHARED)/*  $(DELIVER_SHARED)/*
endif

ifeq ($(TARGET_OS), VXWORKS)
   TEST_DAEMON   = $(OUTDIR_STATIC)/testDaemon.out
   AR_LUA        = 
   AR_CLIENT     =
   BLD_CMD       = $(LD)
   BLD_DAEMON    = -r -ltdcsimudevd
   BLD_TEST      = -r
   BLD_SCRIPT    = -r
   BLD_SHLIB_CLI =
   BLD_SHLIB_PIQ =
endif

#########################################################################

all:          outdir $(SHLIB_CLIENT_IMPORT) test
libs:
	$(MAKE) outdir
	$(MAKE) all_libs
	
all_libs:         $(SHLIB_CLIENT_IMPORT) libDaemon libClient libPiq libLua
libDaemon:    outdir $(AR_DAEMON_T) $(AR_DAEMON)
libClient:    outdir $(AR_CLIENT)   $(SHLIB_CLIENT)
libPiq:       outdir libClient $(AR_PIQ) $(SHLIB_PIQ)
libLua:	      outdir $(AR_LUA)

test:         outdir $(SHLIB_CLIENT_IMPORT) libs testDaemon testAppl testScript testScriptSh
testDaemon:   libDaemon $(TEST_DAEMON)
testAppl:     libClient $(TEST_APPL)
testScript:   libLua libClient libPiq $(TEST_SCRIPT)
testScriptSh: libLua libClient libPiq $(TEST_SCRIPT_SH)

#########################################################################

$(AR_DAEMON_T):	$(OBJS_DAEMON_STATIC) 
			@$(ECHO) ' ### Linking static archive $(@F)'
			$(RM) -f $@
			$(AR) cq $@ $^

$(AR_DAEMON):		$(OBJS_DAEMON_STATIC) 
			@$(ECHO) ' ### Linking static archive $(@F)'
			$(RM) -f $@
			$(AR) cq $@ $^
			cp -f $(AR_DAEMON) $(DELIVER_STATIC)/

$(AR_CLIENT):		$(OBJS_CLIENT_STATIC)
			@$(ECHO) ' ### Linking static archive $(@F)'
			$(RM) -f $@
			$(AR) cq $@ $^
			cp -f $(AR_CLIENT) $(DELIVER_STATIC)/

$(SHLIB_CLIENT_IMPORT): $(SHLIB_CLIENT)

$(SHLIB_CLIENT):  	$(OBJS_CLIENT_SHARED)
			@$(ECHO) ' ### Linking shared Library $(@F)'
			$(RM) -f $@
			$(BLD_CMD) -shared $^                                     \
			$(LDFLAGS)  $(LDLIBS)                                     \
			$(BLD_SHLIB_CLI)                                          \
			-o $@ $(STD_CPP_LIB)
			cp -f $(SHLIB_CLIENT) $(SHLIB_CLIENT_IMPORT)  $(DELIVER_SHARED)/

$(AR_PIQ):              $(OBJS_PIQ_STATIC)
			@$(ECHO) ' ### Linking static archive $(@F)'
			$(RM) -f $@
			$(AR) cq $@ $^
			cp -f $(AR_PIQ) $(DELIVER_STATIC)/

$(SHLIB_PIQ):  		$(SHLIB_CLIENT_IMPORT) $(OBJS_PIQ_SHARED)
			@$(ECHO) ' ### Linking shared Library $(@F)'
			$(RM) -f $@
			$(BLD_CMD) -shared $(OBJS_PIQ_SHARED)                                     \
			$(LDFLAGS)  $(LDLIBS)                                     \
			$(BLD_SHLIB_PIQ)                                          \
			-o $@ $(STD_CPP_LIB)
			cp -f $(SHLIB_PIQ) $(SHLIB_PIQ_IMPORT)  $(DELIVER_SHARED)/

$(AR_LUA):		$(OBJS_LUA_STATIC)
			@$(ECHO) ' ### Linking static archive $(@F)'
			$(RM) -f $@
			$(AR) cq $@ $^

$(TEST_DAEMON): 	$(OBJS_TST_DAEMON_STATIC)
			@$(ECHO) ' ### Linking - $(@F)'
			$(BLD_CMD) $^ $(LDFLAGS)  $(LDLIBS) -L$(OUTDIR_STATIC)/    \
			$(BLD_DAEMON)                                              \
			-o $@

$(TEST_APPL):		$(OBJS_TST_APPL_STATIC) $(AR_CLIENT)
			@$(ECHO) ' ### Linking - $(@F)'
			$(BLD_CMD) $^ $(LDFLAGS) $(LDLIBS) -L$(OUTDIR_STATIC)/     \
			$(BLD_TEST)                                                \
			-o $@
			cp -f $(TEST_APPL) $(DELIVER_STATIC)/

$(TEST_SCRIPT):		$(OBJS_SCRIPT_STATIC) $(AR_CLIENT) $(AR_PIQ) $(AR_LUA)
			@$(ECHO) ' ### Linking - $(@F)'
			$(BLD_CMD) $^ $(LDFLAGS) $(LDLIBS)                         \
			-L$(OUTDIR_STATIC)/                                        \
			$(BLD_SCRIPT)                                              \
			-o $@

$(TEST_SCRIPT_SH):	$(OBJS_SCRIPT_STATIC) $(AR_LUA)
			@$(ECHO) ' ### Linking - $(@F)'
			$(BLD_CMD) $^ $(LDFLAGS) $(LDLIBS) 			   \
			-L$(OUTDIR_SHARED)/                                        \
			$(BLD_SHSCRIPT)                                            \
			-o $@


outdir:
	@$(MD) $(MK_DIRS)
	cp -f sourcecode/api/*.h $(DELIVER_STATIC)/../api/

clean:
	$(RM) $(RM_DIRS)

endif

#########################################################################

help:
	@echo " " >&2
	@echo "BUILD ENVIRONMENT FOR TDC (IPTDir Client)" >&2
	@echo "Load one of the configurations below with 'make <configuration>' first:" >&2
	@echo "  " >&2
	@echo "  * LINUX_ARM_NOVAsom_config     - Building ARM binary Novasom board" >&2
	@echo "  * LINUX_X86_config          - Native build for Linux Desktop PC" >&2
	@echo "  * LINUX_PPC_UCLIBC_config   - Target VCU-C" >&2
	@echo "  * LINUX_ARMEB_UCLIBC_config - Building ARM binaries with our compiler" >&2
	@echo "  * LINUX_ARM_OTN_config      - Building ARM binaries for OnTime Switch" >&2
	@echo "  * VXWORKS_PPC_config        - VCU-C running PowerPC VxWorks " >&2
	@echo "  * VXWORKS_PPC_CSS3_config   - VCU-C running PowerPC VxWorks " >&2
	@echo "  * WINDOWS_config            - Experimental Windows build" >&2
	@echo " " >&2
	@echo "Then call 'make' or 'make all' to build everything." >&2
	@echo "To build debug binaries, append 'DEBUG=TRUE' to the make command " >&2
	@echo "To see the full output, append 'VERBOSE=1' to the make command " >&2
	@echo " " >&2
	@echo "Other builds:" >&2
	@echo "  * make libDaemon    - build only the daemon-library" >&2
	@echo "  * make libClient    - build only the Client (IPC) - library" >&2
	@echo "  * make libLua       - build only the Lua-library" >&2
	@echo "  * make libs         - build all -libraries" >&2
	@echo "  * make testDaemon   - build the test daemon" >&2
	@echo "  * make testAppl     - build the test application" >&2
	@echo "  * make testScript   - build the test Script Processor" >&2
	@echo "  * make testScriptSh - build the test Script Processor (shared lib version)" >&2
	@echo "  * make test         - build the test applications" >&2
	@echo "  * make clean        - remove all binaries and objects of the current target" >&2
	@echo "  * make unconfig     - remove the configuration file" >&2
	@echo "  * make distclean    - make clean unconfig" >&2
	@echo " " >&2

#########################################################################

unconfig:
	-$(RM) config/config.mk

%_config: config/%_cfg
	cp -f config/$(@:config=cfg) config/config.mk

distclean:	clean unconfig

#########################################################################
