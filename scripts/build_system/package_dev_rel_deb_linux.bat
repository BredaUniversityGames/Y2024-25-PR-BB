cd ../..
python scripts/python/package.py -o "package/linux-dev-rel-deb" -e "build/WSL-RelWithDebInfo/CustomTech" -a "assets" "shaders/bin" "game" -f "build/WSL-RelWithDebInfo/libsteam_api.so" "build/WSL-RelWithDebInfo/libfmodL.so.14" "build/WSL-RelWithDebInfo/libfmodstudioL.so.14" "steam_appid.txt"
pause
