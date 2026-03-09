@echo off
REM Copy to backend\.env.smtp.bat and replace values.
REM Windows format for start_windows.bat

set GM_SMTP_HOST=smtp.qq.com
set GM_SMTP_PORT=465
set GM_SMTP_USERNAME=2097697894@qq.com
set GM_SMTP_PASSWORD=REPLACE_WITH_QQ_SMTP_AUTH_CODE
set GM_SMTP_FROM=2097697894@qq.com
set GM_SMTP_AUTH=LOGIN

set GM_SMTP_USE_SSL=true
set GM_SMTP_STARTTLS=false
set GM_SMTP_SKIP_TLS_VERIFY=false
set GM_SMTP_TIMEOUT_MS=10000

set GM_PASSWORD_RESET_PAGE_URL=http://127.0.0.1:5173/password
set GM_PASSWORD_RESET_DEBUG_RETURN_TOKEN=false
