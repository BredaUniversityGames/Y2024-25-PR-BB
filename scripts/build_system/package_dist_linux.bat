cd ../..
python scripts/python/package.py -o "package/linux-dist" -e "build/WSL-Release/CustomTech" -a "assets" "shaders/bin" "game" -f "build/WSL-Release/libsteam_api.so" "build/WSL-Release/libfmod.so.14" "build/WSL-Release/libfmodstudio.so.14"
cd scripts/build_system
echo Linux distribution build packaged.