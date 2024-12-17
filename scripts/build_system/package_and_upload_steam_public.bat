@echo off
call package_dist_linux.bat
call package_dist_windows.bat
echo Uploading to the public branch. Setting the build live is not done automatically for public builds, that must be done via the admin panel!
set /p username="Steam username: "
set /p password="Steam password: "
steam\builder\steamcmd.exe +login %username% %password% +run_app_build ..\config\app_build_public.vdf