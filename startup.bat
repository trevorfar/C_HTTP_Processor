@echo off
REM compile the server.c file
gcc -o server server.c -lws2_32

IF %ERRORLEVEL% NEQ 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

REM run the compiled program
echo Running the server...
server.exe

REM Keep open 
pause
