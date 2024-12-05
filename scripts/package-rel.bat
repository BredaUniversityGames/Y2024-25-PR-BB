cd ..
python scripts/python/package.py -o "package" -e "build/WSL-Release/Game" "steam_appid.txt" "build/WSL-Release/libsteam_api.so" "build/WSL-Release/libfmod.so.14" "build/WSL-Release/libfmodstudio.so.14" -a "assets" "shaders/bin" 
pause