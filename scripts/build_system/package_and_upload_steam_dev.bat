@echo off
call package_dist_linux.bat
call package_dist_windows.bat
echo Uploading to the development branch.
set /p username="Steam username: "
set /p password="Steam password: "
steam\builder\steamcmd.exe +login %username% %password% +run_app_build ..\config\app_build_dev.vdf
