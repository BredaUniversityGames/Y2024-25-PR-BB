@echo off

:: Get the current date (in YYYY-MM-DD format)
for /f "tokens=2-4 delims=/.- " %%a in ('date /t') do (
    set year=%%c
    set month=%%a
    set day=%%b
)

:: Get the current time (in HH-MM-SS format)
for /f "tokens=1-3 delims=:., " %%a in ('echo %time%') do (
    set hour=%%a
    set minute=%%b
    set second=%%c
)

:: Remove leading spaces from hour (if present)
if %hour% lss 10 (set hour=0%hour%)

:: Combine date and time into a single variable
set datestamp=%year%-%month%-%day%_%hour%-%minute%-%second%

:: Set variables for remote and local files
set steamDeckIP=192.168.1.105
set remoteFilePath=/home/deck/devkit-game/bb_game/vma_stats.json
set outDir=../memory-vis
set localFilePath=%outDir%/vma_stats_%datestamp%.json
set outputMemVisPath=%outDir%/vma_stats_%datestamp%.png

if not exist "%outDir%" (
    mkdir "%outDir%"
)

scp deck@%steamDeckIP%:%remoteFilePath% %localFilePath%

python ../external/VulkanMemoryAllocator/tools/GpuMemDumpVis/GpuMemDumpVis.py -o %outputMemVisPath% %localFilePath%

pause