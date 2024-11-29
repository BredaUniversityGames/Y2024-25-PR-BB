cd ..
python scripts/python/package.py -o "package" -e "build/WSL-RelWithDebInfo/Game" "steam_appid.txt" "external/steamworks/lib/Linux/libsteam_api.so" -f "external/fmod/lib/Linux/Debug" -a "assets" "shaders/bin"
pause