# ASME SDC 2025/26 - Project Genesis

## Using the Pico

### Compile the code (option A):
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

### Compile the code (option B):
- Open the start menu
- Launch "Developer PowerShell for VS"
- Run `cd "C:\Users\OwenS\OneDrive\Documents\Programs\NCSU ASME SDC\ASME_SDC_2526_C++\genisis"`
- Run any of:
    - `.\build.ps1` to do a normal build
    - `.\build.ps1 -Clean` to do a clean build, which is needed when:
        - changes have been made to `CMakeLists.txt`
        - source files have been renamed/moved
        - project structure/folders have changed
        - you want to be sure everything rebuilds from scratch
        - NOT when you just edited normal `.c`, `.h`, or `.cpp` files
    - `.\build.ps1 -OpenUf2` to do a normal build and then locate the build file location
    - `.\build.ps1 -Clean -OpenUf2` to do a clean build then locate the build file location

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

## Electronics

### Motor Drivers

#### [DRV8833](https://www.aliexpress.us/item/3256808500729294.html?src=google&src=google&albch=shopping&acnt=708-803-3821&isdl=y&slnk=&plac=&mtctp=&albbt=Google_7_shopping&aff_platform=google&aff_short_key=UneMJZVf&albagn=888888&ds_e_adid=&ds_e_matchtype=&ds_e_device=c&ds_e_network=x&ds_e_product_group_id=&ds_e_product_id=en3256808500729294&ds_e_product_merchant_id=5564243987&ds_e_product_country=US&ds_e_product_language=en&ds_e_product_channel=online&ds_e_product_store_id=&ds_url_v=2&albcp=19558607238&albag=&isSmbAutoCall=false&needSmbHouyi=false&gatewayAdapt=glo2usa#nav-specification) — Dual-Channel DC Motor Driver
- **Supply voltage:** 2.7V–10.8V (motor side); logic 3.3V compatible
- **Current:** 1.5A continuous / 2A peak per channel
- **Channels:** 2 independent H-bridges (Motor A: OUT1/OUT2, Motor B: OUT3/OUT4)
- **Overcurrent protection:** EEP/nFAULT pin goes LOW on fault
- **Sleep:** SLEEP pin must be HIGH to operate (active-low; some modules pull this high by default)

**Pin mapping:**

| Module Pin | Connects to       | Notes                        |
|------------|-------------------|------------------------------|
| IN1        | Pico GPIO (PWM)   | Motor A direction/speed      |
| IN2        | Pico GPIO (PWM)   | Motor A direction/speed      |
| IN3        | Pico GPIO (PWM)   | Motor B direction/speed      |
| IN4        | Pico GPIO (PWM)   | Motor B direction/speed      |
| GND        | Common GND        |                              |
| VCC        | Motor power supply| 3V–10V                       |
| OUT1–OUT4  | Motor terminals   |                              |
| SLEEP      | Pico GPIO or VCC  | Pull HIGH to enable          |
| EEP/nFAULT | Pico GPIO (input) | Optional fault detection     |

**Control logic (per channel):**

| IN_A | IN_B | Motor behavior     |
|------|------|--------------------|
| PWM  | LOW  | Forward at speed   |
| LOW  | PWM  | Reverse at speed   |
| HIGH | HIGH | Brake (active stop)|
| LOW  | LOW  | Coast (freewheel)  |

Speed is set by PWM duty cycle (0–100%) on the active IN pin. Recommended PWM frequency: 1kHz–20kHz.

---

#### [DRV8871](https://www.adafruit.com/product/3190?srsltid=AfmBOoqrHaKgkMo3pqW3c-sySkwM7nNNwSVYoT-E6YyM2p9z19s4wzS5) — Single-Channel DC Motor Driver (Adafruit #3190)
- **Supply voltage:** 6.5V–45V (motor side); logic up to 5.5V (3.3V compatible)
- **Current:** 3.6A peak; default ~2A limit (adjustable via Rlim resistor)
- **Channels:** 1 H-bridge (single motor)
- **Overcurrent protection:** Yes (current limiting via Rlim — no inline sense resistor needed)
- **Additional protections:** Undervoltage lockout, thermal shutdown

**Pin mapping:**

| Pin     | Connects to        | Notes                           |
|---------|--------------------|---------------------------------|
| IN1     | Pico GPIO (PWM)    | Direction/speed control         |
| IN2     | Pico GPIO (PWM)    | Direction/speed control         |
| VMotor  | Motor power supply | 6.5V–45V (screw terminal)       |
| GND     | Common GND         | (screw terminal)                |
| OUT     | Motor terminals    | (screw terminal)                |
| Rlim    | Resistor to GND    | Sets current limit; 30kΩ default (~2A) |

**Control logic:**

| IN1  | IN2  | Motor behavior     |
|------|------|--------------------|
| PWM  | LOW  | Forward at speed   |
| LOW  | PWM  | Reverse at speed   |
| HIGH | HIGH | Brake (active stop)|
| LOW  | LOW  | Coast (freewheel)  |

Same PWM control scheme as the DRV8833. Use this driver for higher-voltage or higher-current motors.

---

### Motors

> _TBD — add motor model, voltage/current ratings, and which DRV8833 channel each motor connects to._

---

### Servos

> _TBD — add servo model, voltage/current ratings, signal frequency (typically 50Hz), and Pico GPIO pin assignments._

---

### Pico 2 W GPIO Assignments

> _TBD — add a full table of which GPIO pins are assigned to what (motors, servos, sensors, etc.) once hardware is finalized._

