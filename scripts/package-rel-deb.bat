cd ..
python scripts/python/package.py -o "package" -e "build/WSL-RelWithDebInfo/Game" "steam_appid.txt" "build/WSL-RelWithDebInfo/libsteam_api.so" "build/WSL-RelWithDebInfo/libfmodL.so.14" "build/WSL-RelWithDebInfo/libfmodstudioL.so.14" -a "assets" "shaders/bin"
pause
