# Microsoft Developer Studio Project File - Name="gopuzzle_win32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=gopuzzle_win32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gopuzzle_win32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gopuzzle_win32.mak" CFG="gopuzzle_win32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gopuzzle_win32 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "gopuzzle_win32 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
# PROP WCE_FormatVersion "6.0"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gopuzzle_win32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "gopuzzle_win32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "gopuzzle_win32 - Win32 Release"
# Name "gopuzzle_win32 - Win32 Debug"
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\event_queue.c
# End Source File
# Begin Source File

SOURCE=..\event_queue.h
# End Source File
# Begin Source File

SOURCE=..\gongshu_gdi.c
# End Source File
# Begin Source File

SOURCE=..\win_main.c
# End Source File
# Begin Source File

SOURCE=..\wrapper.h
# End Source File
# Begin Source File

SOURCE=..\wrapper_graph.c
# End Source File
# Begin Source File

SOURCE=..\wrapper_platform.c
# End Source File
# End Group
# Begin Group "Gopuzzle"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\gongshu.h
# End Source File
# Begin Source File

SOURCE=..\..\gopuzzle.c
# End Source File
# Begin Source File

SOURCE=..\..\gopuzzle.h
# End Source File
# Begin Source File

SOURCE=..\..\modi.c
# End Source File
# Begin Source File

SOURCE=..\..\modi.h
# End Source File
# End Group
# Begin Group "Jasmine89"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\artifacts\jasmine89\jasmine.c
# End Source File
# Begin Source File

SOURCE=..\..\..\artifacts\jasmine89\jasmine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\artifacts\jasmine89\jasmine_utils.c
# End Source File
# End Group
# Begin Group "Salvia89"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\artifacts\salvia89\salvia.c
# End Source File
# Begin Source File

SOURCE=..\..\..\artifacts\salvia89\salvia.h
# End Source File
# End Group
# End Target
# End Project
