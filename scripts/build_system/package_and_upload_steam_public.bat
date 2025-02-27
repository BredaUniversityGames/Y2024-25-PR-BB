@echo off
call package_linux_dist.bat
call package_windows_dist.bat
echo Uploading to the public branch. Setting the build live is not done automatically for public builds, that must be done via the admin panel!
set /p username="Steam username: "
set /p password="Steam password: "
steam\builder\steamcmd.exe +login %username% %password% +run_app_build ..\config\app_build_public.vdf