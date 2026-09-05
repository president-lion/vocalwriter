<html>
<body>
<pre>
<h1>Build Log</h1>
<h3>
--------------------Configuration: DTalkML - Win32 (WCE x86) Release--------------------
</h3>
<h3>Command Lines</h3>
Creating temporary file "C:\DOCUME~1\build\LOCALS~1\Temp\RSPCA1.tmp" with contents
[
/nologo /W3 /GX- /O2 /I "..\..\dapi\src\include" /I "..\..\dapi\src\nt" /I "..\..\dapi\src\api\\" /I "..\.." /D _WIN32_WCE=300 /D "WIN32_PLATFORM_HPC2000" /D "_i386_" /D UNDER_CE=300 /D "UNICODE" /D "_UNICODE" /D "_X86_" /D "x86" /D "NDEBUG" /D "WIN32" /D "ENGLISH" /D "ENGLISH_US" /D "_USRDLL" /D "BLD_DECTALK_DLL" /Fp".\..\Build\ce\x86\Release\link/DTalkMLCE.pch" /YX /Fo".\..\Build\ce\x86\Release\link/" /Gs8192 /MC /c 
"D:\work\PRODUCT\dtalkml\src\dtalk_ml.c"
]
Creating command line "cl.exe @C:\DOCUME~1\build\LOCALS~1\Temp\RSPCA1.tmp" 
Creating temporary file "C:\DOCUME~1\build\LOCALS~1\Temp\RSPCA2.tmp" with contents
[
commctrl.lib coredll.lib corelibc.lib /nologo /base:"0x00100000" /stack:0x10000,0x1000 /entry:"_DllMainCRTStartup" /dll /incremental:no /pdb:".\..\Build\ce\x86\Release/dectalk.pdb" /nodefaultlib:"OLDNAMES.lib" /nodefaultlib:libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib /def:"..\..\dapi\src\dectalk.def" /out:".\..\build\ce\x86\Release/dectalk.dll" /implib:".\..\Build\ce\x86\Release/dectalk.lib" /subsystem:windowsce,3.00 
\work\PRODUCT\dtalkml\Build\ce\x86\Release\link\dtalk_ml.obj
]
Creating command line "link.exe @C:\DOCUME~1\build\LOCALS~1\Temp\RSPCA2.tmp"
<h3>Output Window</h3>
Compiling...
Command line warning D4002 : ignoring unknown option '/MC'
dtalk_ml.c
D:\work\PRODUCT\dtalkml\src\dtalk_ml.c(531) : warning C4101: 'config_file' : unreferenced local variable
D:\work\PRODUCT\dtalkml\src\dtalk_ml.c(1424) : warning C4133: 'function' : incompatible types - from 'unsigned short *' to 'char *'
Linking...
..\..\dapi\src\dectalk.def : warning LNK4017: DESCRIPTION statement not supported for the target platform; ignored
   Creating library .\..\Build\ce\x86\Release/dectalk.lib and object .\..\Build\ce\x86\Release/dectalk.exp



<h3>Results</h3>
dectalk.dll - 0 error(s), 4 warning(s)
</pre>
</body>
</html>
