cd ..
python scripts/python/package.py -o "package" -e "build/WSL-Release/Game" "steam_appid.txt" "external/steamworks/lib/Linux/libsteam_api.so" -f "external/fmod/lib/Linux/Release" -a "assets" "shaders/bin"
pause