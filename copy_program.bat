upx ".\release\DB_UTIL.exe"
if not exist "C:\Program Files\DB_UTIL\DB_UTIL.exe" goto exit
copy ".\release\DB_UTIL.exe" "C:\Program Files\DB_UTIL\DB_UTIL.exe"
:exit

pause
