# Microsoft Developer Studio Project File - Name="DB_UTIL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 60000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=DB_UTIL - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DB_UTIL.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DB_UTIL.mak" CFG="DB_UTIL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DB_UTIL - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "DB_UTIL - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DB_UTIL - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W2 /GX /O2 /FI"pragma.h" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib Ws2_32.lib RPCRT4.LIB /nologo /subsystem:windows /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy_program.bat
# End Special Build Tool

!ELSEIF  "$(CFG)" == "DB_UTIL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W2 /Gm /GX /ZI /Od /I ".\\" /FI"pragma.h" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 nafxcwd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib Ws2_32.lib RPCRT4.LIB /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "DB_UTIL - Win32 Release"
# Name "DB_UTIL - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "lemon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\lemon\example5.c
# End Source File
# Begin Source File

SOURCE=.\lemon\example5.y

!IF  "$(CFG)" == "DB_UTIL - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\lemon\example5.y
InputName=example5

".\lemon\example5.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo custom build 
	.\lemon\lemon .\lemon\$(InputName).y 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "DB_UTIL - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\lemon\example5.y
InputName=example5

".\lemon\example5.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo custom build on example5.y 
	.\lemon\lemon .\lemon\$(InputName).y 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lemon\lex.yy.c
# End Source File
# Begin Source File

SOURCE=.\lemon\lexer.l

!IF  "$(CFG)" == "DB_UTIL - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\lemon\lexer.l

".\lemon\lex.yy.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	.\lemon\flex -o.\lemon\lex.yy.c .\lemon\lexer.l

# End Custom Build

!ELSEIF  "$(CFG)" == "DB_UTIL - Win32 Debug"

# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\cmd_line.c
# End Source File
# Begin Source File

SOURCE=.\DB_UTIL.c
# End Source File
# Begin Source File

SOURCE=.\debug_print.c
# End Source File
# Begin Source File

SOURCE=.\find_table.c
# End Source File
# Begin Source File

SOURCE=.\ini_file.c
# End Source File
# Begin Source File

SOURCE=.\insert_row.c
# End Source File
# Begin Source File

SOURCE=.\intellisense.c
# End Source File
# Begin Source File

SOURCE=.\mdi_crap.c
# End Source File
# Begin Source File

SOURCE=.\memtest.c
# End Source File
# Begin Source File

SOURCE=.\resize.c
# End Source File
# Begin Source File

SOURCE=.\settings.c
# End Source File
# Begin Source File

SOURCE=.\sql_reserved_words.c
# End Source File
# Begin Source File

SOURCE=.\worker_thread.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\col_info.h
# End Source File
# Begin Source File

SOURCE=.\DB_stuff.h
# End Source File
# Begin Source File

SOURCE=.\listview.h
# End Source File
# Begin Source File

SOURCE=.\pragma.h
# End Source File
# Begin Source File

SOURCE=.\ram_ini_file.h
# End Source File
# Begin Source File

SOURCE=.\search.h
# End Source File
# Begin Source File

SOURCE=.\structs.h
# End Source File
# Begin Source File

SOURCE=.\treeview.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\database.ico
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# End Group
# End Target
# End Project
