@echo off
where cl >nul 2>&1
if %errorlevel% equ 0 (
    cl /nologo /O2 /W3 /Fe:WindowMan.exe main.c /link /subsystem:console user32.lib shell32.lib
) else (
    where x86_64-w64-mingw32-gcc >nul 2>&1
    if %errorlevel% equ 0 (
        x86_64-w64-mingw32-gcc -O2 -Wall -o WindowMan.exe main.c -luser32 -lshell32 -mconsole
    ) else (
        echo Error: No compiler found. Install MSVC or mingw-w64.
        exit /b 1
    )
)
if %errorlevel% equ 0 (
    echo.
    echo Build successful: WindowMan.exe
) else (
    echo.
    echo Build failed.
)
