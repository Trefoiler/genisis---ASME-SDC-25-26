# ASME SDC 2025/26 - Project Genesis

## Using the Pico

### Compile the code:
- Open the start menu
- Launch "Developer PowerShell for VS"
- Run each of these commands:
```powershell
cl

$env:PICO_SDK_PATH = "$HOME\.pico-sdk\sdk\2.2.0"
$env:PICO_TOOLCHAIN_PATH = "$HOME\.pico-sdk\toolchain\14_2_Rel1"

cd "C:\Users\OwenS\OneDrive\Documents\Programs\NCSU ASME SDC\ASME_SDC_2526_C++\genisis"
Remove-Item -Recurse -Force build
mkdir build
cd build

cmake -G Ninja -DCMAKE_MAKE_PROGRAM="C:\Users\OwenS\.pico-sdk\ninja\v1.12.1\ninja.exe" ..
cmake --build . -j
```
- Find the build file at `C:\Users\OwenS\OneDrive\Documents\Programs\NCSU ASME SDC\ASME_SDC_2526_C++\genisis\build\genisis.uf2`

### Flash the code:
- Plug in the Pico 2 W in BOOTSEL mode
- Find the Pico's drive in file explorer
- Copy the build file (from the above compiling directions) to the Pico's drive
- The Pico's drive should automatically disappear

### Monitor the serial monitor while the code runs:
- Plug in the Pico 2 W not in BOOTSEL mode
- In VSCode, open the command palette with the shortcut `ctrl+shift+p`
- Run the command `Serial Monitor: Focus on Serial Monitor View`
- Choose the port of the Pico
- Click `Stop Monitoring`
