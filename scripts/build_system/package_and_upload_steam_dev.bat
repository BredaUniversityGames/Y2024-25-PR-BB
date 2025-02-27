@echo off
call package_linux_dist.bat
call package_windows_dist.bat
echo Uploading to the development branch.
set /p username="Steam username: "
set /p password="Steam password: "
steam\builder\steamcmd.exe +login %username% %password% +run_app_build ..\config\app_build_dev.vdf
