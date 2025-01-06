cd ../..
python scripts/python/package.py -o "package/windows-dist" -e "build/x64-Release/CustomTech.exe" -a "assets" "shaders/bin" "game" -f "build/x64-Release/steam_api64.dll" "build/x64-Release/fmod.dll" "build/x64-Release/fmodstudio.dll"
cd scripts/build_system
echo Windows distribution build packaged.