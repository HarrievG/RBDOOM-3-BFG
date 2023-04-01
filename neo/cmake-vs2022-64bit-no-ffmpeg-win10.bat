cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 17" -A x64 ^
-DFFMPEG=OFF ^
-DBINKDEC=ON ^
-DWINDOWS10=ON ^
-DUSE_DX12=ON ^
-DDBG_CMD_Debug:PATH="debugCmds.txt" ^
-DDBG_CMD_Release:PATH="releaseCmds.txt" ^
../neo 

