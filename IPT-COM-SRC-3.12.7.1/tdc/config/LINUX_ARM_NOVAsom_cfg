ARCH      = linux-arm-NOVAsom
CPU       = FREESCALE i.MX6
TARGET_OS = LINUX
OSDEP_NAME  = linux
FILESYSTEM_ROOT = FileSystem-11
FILESYSTEM_NAME = Chrome-3.10
OLDTOOLCHAIN = /Devel/NOVAsom6_SDK/Xcompiler/ArmCompiler/usr/bin/arm-linux-
TOOLCHAIN = /Devel/NOVAsom6_SDK/$(FILESYSTEM_ROOT)/$(FILESYSTEM_NAME)/output/host/usr/bin/arm-linux-
PLATFORM_CPPFLAGS = -DLINUX -O -march=armv7-a -mthumb-interwork -mfloat-abi=hard -mfpu=neon -mtune=cortex-a9 -fno-common -fno-builtin -Wall \
		    -Dlinux -D__linux__ -Dunix -DEMBED -DREDFOX -I../libs/otn/include \
		    -I../libs/otn/include/h -I../libs/otn/include/h/msApi -DTS_HAS_FIXED_IP
PLATFORM_LDFLAGS   = -nostartfiles -nodefaultlibs -nostdlib -L../libs/otn/lib/
OLDLDLIBS    =  -lm -lc /Devel/NOVAsom6_SDK/Xcompiler/ArmCompiler/usr/lib/gcc/arm-buildroot-linux-gnueabihf/4.8.3/libgcc.a /Devel/NOVAsom6_SDK/Xcompiler/ArmCompiler/arm-buildroot-linux-gnueabihf/sysroot/usr/lib/crt1.o
LDLIBS    =  -lm -lc /Devel/NOVAsom6_SDK/$(FILESYSTEM_ROOT)/$(FILESYSTEM_NAME)/output/host/usr/arm-buildroot-linux-gnueabihf/sysroot/lib/libgcc_s.so /Devel/NOVAsom6_SDK/$(FILESYSTEM_ROOT)/$(FILESYSTEM_NAME)/output/host/usr/arm-buildroot-linux-gnueabihf/sysroot/usr/lib/crt1.o /Devel/NOVAsom6_SDK/$(FILESYSTEM_ROOT)/$(FILESYSTEM_NAME)/output/host/usr/arm-buildroot-linux-gnueabihf/sysroot/usr/lib/liblua.so
 
