# Microsoft Developer Studio Project File - Name="iptcom_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=iptcom_dll - Win32 Debug Simulation
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "iptcom_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "iptcom_dll.mak" CFG="iptcom_dll - Win32 Debug Simulation"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "iptcom_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "iptcom_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "iptcom_dll - Win32 Debug Simulation" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "iptcom_dll - Win32 Release Simulation" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "iptcom_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iptcom_dll___Win32_Release"
# PROP BASE Intermediate_Dir "iptcom_dll___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\export-iptcom\windows-x86-dll-rel"
# PROP Intermediate_Dir "..\output\windows-x86-dll-rel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IPTCOM_DLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\src\api" /I "..\src\prv" /I "..\..\tdc\src\api" /I "..\..\cm\src\api" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IPTCOM_DLL_EXPORTS" /D "DLL_EXPORT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x41d /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 libcmtd.lib ..\..\tdc\export-tdc\windows-x86-static-rel\tdcd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib ..\..\libs\win32\PThread\pthreadVC2.lib ..\..\libs\win32\iphlpapi\iphlpapi.lib Winmm.lib /nologo /dll /machine:I386 /nodefaultlib /out:"..\export-iptcom\windows-x86-dll-rel/iptcom.dll" /IGNORE:4089
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "iptcom_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "iptcom_dll___Win32_Debug"
# PROP BASE Intermediate_Dir "iptcom_dll___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\export-iptcom\windows-x86-dll-dbg"
# PROP Intermediate_Dir "..\output\windows-x86-dll-dbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IPTCOM_DLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\src\api" /I "..\src\prv" /I "..\..\tdc\src\api" /I "..\..\cm\src\api" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IPTCOM_DLL_EXPORTS" /D IPTCOM_MEMORY_POOL=4000000 /D "DLL_EXPORT" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x41d /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libcmtd.lib ..\..\tdc\export-tdc\windows-x86-static-dbg\tdcd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib ..\..\libs\win32\PThread\pthreadVC2.lib ..\..\libs\win32\iphlpapi\iphlpapi.lib Winmm.lib /nologo /dll /debug /machine:I386 /nodefaultlib /out:"..\export-iptcom\windows-x86-dll-dbg/iptcom.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "iptcom_dll - Win32 Debug Simulation"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug Simulation"
# PROP BASE Intermediate_Dir "Debug Simulation"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\export-iptcom\windows-x86-sim-dll-dbg"
# PROP Intermediate_Dir "..\output\windows-x86-sim-dll-dbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\src\api" /I "..\src\prv" /I "..\..\tdc\src\api" /I "..\..\cm\src\api" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IPTCOM_DLL_EXPORTS" /D IPTCOM_MEMORY_POOL=4000000 /D "DLL_EXPORT" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\src\api" /I "..\src\prv" /I "..\..\tdc\src\api" /I "..\..\cm\src\api" /I "..\..\libs\win32\winpcap" /I "..\..\libs\win32\IPHlpApi" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IPTCOM_DLL_EXPORTS" /D IPTCOM_MEMORY_POOL=4000000 /D "DLL_EXPORT" /D "TARGET_SIMU" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x41d /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libcmtd.lib ..\..\tdc\export-tdc\windows-x86-static-dbg\tdcd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib ..\..\libs\win32\PThread\pthreadVC2.lib ..\..\libs\win32\iphlpapi\iphlpapi.lib Winmm.lib /nologo /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept
# ADD LINK32 libcmtd.lib ..\..\tdc\export-tdc\windows-x86-static-dbg\tdcd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib ..\..\libs\win32\PThread\pthreadVC2.lib ..\..\libs\win32\iphlpapi\iphlpapi.lib Winmm.lib ..\..\libs\win32\winpcap\wpcap.lib /nologo /dll /debug /machine:I386 /nodefaultlib /out:"..\export-iptcom\windows-x86-sim-dll-dbg/iptcom.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "iptcom_dll - Win32 Release Simulation"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release Simulation"
# PROP BASE Intermediate_Dir "Release Simulation"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\export-iptcom\windows-x86-sim-dll-rel"
# PROP Intermediate_Dir "..\output\windows-x86-sim-dll-rel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\src\api" /I "..\src\prv" /I "..\..\tdc\src\api" /I "..\..\cm\src\api" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IPTCOM_DLL_EXPORTS" /D "DLL_EXPORT" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\src\api" /I "..\src\prv" /I "..\..\tdc\src\api" /I "..\..\cm\src\api" /I "..\..\libs\win32\winpcap" /I "..\..\libs\win32\IPHlpApi" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IPTCOM_DLL_EXPORTS" /D "DLL_EXPORT" /D "TARGET_SIMU" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x41d /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libcmtd.lib ..\..\tdc\export-tdc\windows-x86-static-rel\tdcd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib ..\..\libs\win32\PThread\pthreadVC2.lib ..\..\libs\win32\iphlpapi\iphlpapi.lib Winmm.lib /nologo /dll /machine:I386 /nodefaultlib
# ADD LINK32 libcmtd.lib ..\..\tdc\export-tdc\windows-x86-static-rel\tdcd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib ..\..\libs\win32\PThread\pthreadVC2.lib ..\..\libs\win32\iphlpapi\iphlpapi.lib Winmm.lib ..\..\libs\win32\winpcap\wpcap.lib /nologo /dll /machine:I386 /nodefaultlib /out:"..\export-iptcom\windows-x86-sim-dll-rel/iptcom.dll"

!ENDIF 

# Begin Target

# Name "iptcom_dll - Win32 Release"
# Name "iptcom_dll - Win32 Debug"
# Name "iptcom_dll - Win32 Debug Simulation"
# Name "iptcom_dll - Win32 Release Simulation"
# Begin Group "mdcom"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\mdcom\mdcom.c
# End Source File
# Begin Source File

SOURCE=..\src\api\mdcom.h
# End Source File
# Begin Source File

SOURCE=..\src\mdcom\mdcom_cls.cpp
# End Source File
# Begin Source File

SOURCE=..\src\prv\mdcom_priv.h
# End Source File
# Begin Source File

SOURCE=..\src\mdcom\mdses.c
# End Source File
# Begin Source File

SOURCE=..\src\prv\mdses.h
# End Source File
# Begin Source File

SOURCE=..\src\mdcom\mdtrp.c
# End Source File
# Begin Source File

SOURCE=..\src\prv\mdtrp.h
# End Source File
# End Group
# Begin Group "pdcom"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\pdcom\pdcom.c
# End Source File
# Begin Source File

SOURCE=..\src\api\pdcom.h
# End Source File
# Begin Source File

SOURCE=..\src\pdcom\pdcom_cls.cpp
# End Source File
# Begin Source File

SOURCE=..\src\prv\pdcom_priv.h
# End Source File
# Begin Source File

SOURCE=..\src\pdcom\pdcom_receive.c
# End Source File
# Begin Source File

SOURCE=..\src\pdcom\pdcom_send.c
# End Source File
# Begin Source File

SOURCE=..\src\pdcom\pdcom_util.c
# End Source File
# End Group
# Begin Group "iptcom"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\api\ipt.h
# End Source File
# Begin Source File

SOURCE=..\src\iptcom\iptcom.c
# End Source File
# Begin Source File

SOURCE=..\src\api\iptcom.h
# End Source File
# Begin Source File

SOURCE=..\src\iptcom\iptcom_cls.cpp
# End Source File
# Begin Source File

SOURCE=..\src\iptcom\iptcom_config.c
# End Source File
# Begin Source File

SOURCE=..\src\api\iptcom_cpp.h
# End Source File
# Begin Source File

SOURCE=..\src\iptcom\iptcom_parse.c
# End Source File
# Begin Source File

SOURCE=..\src\prv\iptcom_priv.h
# End Source File
# Begin Source File

SOURCE=..\src\iptcom\iptcom_snmp.c
# End Source File
# Begin Source File

SOURCE=..\src\iptcom\iptcom_statistic.c
# End Source File
# Begin Source File

SOURCE=..\src\iptcom\iptcom_table.c
# End Source File
# Begin Source File

SOURCE=..\src\iptcom\iptcom_util.c
# End Source File
# Begin Source File

SOURCE=..\src\iptcom\iptcom_xml.c
# End Source File
# Begin Source File

SOURCE=..\src\prv\iptcom_xml.h
# End Source File
# End Group
# Begin Group "netdriver"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\netdriver\netdriver.c
# End Source File
# Begin Source File

SOURCE=..\src\prv\netdriver.h
# End Source File
# Begin Source File

SOURCE=..\src\netdriver\sock_simu.c
# End Source File
# Begin Source File

SOURCE=..\src\netdriver\win_basics.h
# End Source File
# End Group
# Begin Group "vos"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\vos\vos.c
# End Source File
# Begin Source File

SOURCE=..\src\api\vos.h
# End Source File
# Begin Source File

SOURCE=..\src\vos\vos_dbgutil.c
# End Source File
# Begin Source File

SOURCE=..\src\vos\vos_mem.c
# End Source File
# Begin Source File

SOURCE=..\src\prv\vos_socket.h
# End Source File
# End Group
# Begin Group "tdcsim"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\tdcsim\tdcsim.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\iptcom\iptcom.def
# End Source File
# End Target
# End Project
