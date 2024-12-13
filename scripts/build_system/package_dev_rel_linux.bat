cd ../..
python scripts/python/package.py -o "package/linux-dev-rel" -e "build/WSL-Release/CustomTech" -a "assets" "shaders/bin" "game" -f "build/WSL-Release/libsteam_api.so" "build/WSL-Release/libfmod.so.14" "build/WSL-Release/libfmodstudio.so.14" "steam_appid.txt"
pause