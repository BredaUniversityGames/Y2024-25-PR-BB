@echo off

for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "YY=%dt:~2,2%" & set "YYYY=%dt:~0,4%" & set "MM=%dt:~4,2%" & set "DD=%dt:~6,2%"
set "HH=%dt:~8,2%" & set "Min=%dt:~10,2%" & set "Sec=%dt:~12,2%"

set "fullstamp=%YYYY%-%MM%-%DD%_%HH%-%Min%-%Sec%"

:: Set variables for remote and local files
set steamDeckIP=192.168.1.105
set remoteFilePath=/home/deck/devkit-game/bb_game/vma_stats.json
set outDir=../memory-vis
set localFilePath=%outDir%/vma_stats_%fullstamp%.json
set outputMemVisPath=%outDir%/vma_stats_%fullstamp%.png

if not exist "%outDir%" (
    mkdir "%outDir%"
)

scp deck@%steamDeckIP%:%remoteFilePath% %localFilePath%

python ../external/VulkanMemoryAllocator/tools/GpuMemDumpVis/GpuMemDumpVis.py -o %outputMemVisPath% %localFilePath%

pause