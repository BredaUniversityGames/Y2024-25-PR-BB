# Y2024-25-PR-Bubonic Brotherhood

NAME PENDING - Custom Tech Project (24/25)

## How to compile and build project

Clone repository to desired folder

Update Git Submodules with `git submodule update --init --recursive` or by running the script `update_modules.bat`

Build using IDE of choice or with `cmake --preset=PRESET_NAME` and `cmake --build --preset=PRESET_NAME`

## How to run the project

In order to run the application, the working directory must contain the `assets/` and `shaders/` folders. After building in MSVC, you can simply run the `setup_launch.bat` file to automatically set the run configuration for the project, otherwise it must be done manually.