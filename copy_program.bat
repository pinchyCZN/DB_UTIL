if not exist "C:\Program Files\DB_UTIL\DB_UTIL.exe" goto exit
upx ".\release\DB_UTIL.exe"
copy ".\release\DB_UTIL.exe" "C:\Program Files\DB_UTIL\DB_UTIL.exe"
:exit