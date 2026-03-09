@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"

set "BACKEND_DIR=%ROOT_DIR%\backend"
set "FRONTEND_DIR=%ROOT_DIR%\frontend"
set "BACKEND_BUILD_DIR=%BACKEND_DIR%\build"
if "%SMTP_ENV_FILE%"=="" set "SMTP_ENV_FILE=%BACKEND_DIR%\.env.smtp.bat"

if "%FRONTEND_PORT%"=="" set "FRONTEND_PORT=5173"

if not exist "%BACKEND_DIR%" (
  echo [start_windows] Missing directory: %BACKEND_DIR%
  exit /b 1
)
if not exist "%FRONTEND_DIR%" (
  echo [start_windows] Missing directory: %FRONTEND_DIR%
  exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
  echo [start_windows] Missing command: cmake
  exit /b 1
)
where npm >nul 2>nul
if errorlevel 1 (
  echo [start_windows] Missing command: npm
  exit /b 1
)

if exist "%SMTP_ENV_FILE%" (
  echo [start_windows] Loading SMTP env from %SMTP_ENV_FILE%
  call "%SMTP_ENV_FILE%"
)

if not "%GM_SMTP_HOST%"=="" (
  where curl >nul 2>nul
  if errorlevel 1 (
    echo [start_windows] Missing command: curl ^(required when GM_SMTP_HOST is set^)
    exit /b 1
  )
)

echo [start_windows] Configuring backend
cmake -S "%BACKEND_DIR%" -B "%BACKEND_BUILD_DIR%"
if errorlevel 1 exit /b 1

echo [start_windows] Building backend
cmake --build "%BACKEND_BUILD_DIR%" --config Release
if errorlevel 1 (
  cmake --build "%BACKEND_BUILD_DIR%"
  if errorlevel 1 exit /b 1
)

if /I "%FORCE_NPM_INSTALL%"=="1" goto install_frontend
if exist "%FRONTEND_DIR%\node_modules" goto skip_install

:install_frontend
if exist "%FRONTEND_DIR%\package-lock.json" (
  echo [start_windows] Installing frontend dependencies with npm ci
  npm --prefix "%FRONTEND_DIR%" ci
) else (
  echo [start_windows] Installing frontend dependencies with npm install
  npm --prefix "%FRONTEND_DIR%" install
)
if errorlevel 1 exit /b 1

goto choose_backend

:skip_install
echo [start_windows] Skip npm install ^(node_modules exists^). Set FORCE_NPM_INSTALL=1 to force reinstall.

:choose_backend
set "BACKEND_CMD="
if exist "%BACKEND_BUILD_DIR%\graduate_match_backend.exe" set "BACKEND_CMD=graduate_match_backend.exe"
if not defined BACKEND_CMD if exist "%BACKEND_BUILD_DIR%\Release\graduate_match_backend.exe" set "BACKEND_CMD=Release\graduate_match_backend.exe"
if not defined BACKEND_CMD if exist "%BACKEND_BUILD_DIR%\Debug\graduate_match_backend.exe" set "BACKEND_CMD=Debug\graduate_match_backend.exe"

if not defined BACKEND_CMD (
  echo [start_windows] Backend binary not found under %BACKEND_BUILD_DIR%
  exit /b 1
)

echo [start_windows] Launching backend in a new terminal
start "graduate_match_backend" cmd /k "cd /d \"%BACKEND_BUILD_DIR%\" && %BACKEND_CMD%"

echo [start_windows] Launching frontend in a new terminal
start "graduate_match_frontend" cmd /k "cd /d \"%FRONTEND_DIR%\" && npm run dev -- --host 0.0.0.0 --port %FRONTEND_PORT%"

echo [start_windows] Started.
echo [start_windows] Backend health: http://127.0.0.1:5555/api/health
echo [start_windows] Frontend:      http://127.0.0.1:%FRONTEND_PORT%

exit /b 0
