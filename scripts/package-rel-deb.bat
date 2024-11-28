cd ..
python scripts/python/package.py -o "package" -e "build/WSL-RelWithDebInfo/Game" "steam_appid.txt" "external/steamworks/lib/Linux/libsteam_api.so" -a "assets" "shaders/bin"
pause