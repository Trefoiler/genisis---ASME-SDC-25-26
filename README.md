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

### Motor Controllers

> Comparison summary:
>
> | Controller | Channels | Current | Voltage | Control signal | Pico pins needed |
> |------------|----------|---------|---------|----------------|-----------------|
> | DRV8833    | 2        | 1.5A/ch | 3–10V   | IN1/IN2 PWM    | 4 GPIO          |
> | DRV8871    | 1        | 3.6A pk | 6.5–45V | IN1/IN2 PWM    | 2 GPIO          |
> | Flipsky ESC| 2        | 5A/ch   | 6–14V   | PPM (RC servo) | 1–2 GPIO        |

---

#### DRV8833 — Dual-Channel DC Motor Driver
- **Supply voltage:** 2.7V–10.8V (motor side); logic 3.3V compatible
- **Current:** 1.5A continuous / 2A peak per channel
- **Channels:** 2 independent H-bridges (Motor A: OUT1/OUT2, Motor B: OUT3/OUT4)
- **Overcurrent protection:** EEP/nFAULT pin goes LOW on fault
- **Sleep:** SLEEP pin must be HIGH to operate (active-low; some modules pull this high by default)

**Pin mapping:**

| Module Pin  | Connects to        | Notes                            |
|-------------|--------------------|----------------------------------|
| IN1         | Pico GPIO (PWM)    | Motor A direction/speed          |
| IN2         | Pico GPIO (PWM)    | Motor A direction/speed          |
| IN3         | Pico GPIO (PWM)    | Motor B direction/speed          |
| IN4         | Pico GPIO (PWM)    | Motor B direction/speed          |
| VCC         | Motor power supply | 3V–10V                           |
| GND         | Common GND         |                                  |
| OUT1–OUT4   | Motor terminals    |                                  |
| SLEEP       | Pico GPIO or VCC   | Pull HIGH to enable              |
| EEP/nFAULT  | Pico GPIO (input)  | Optional fault detection         |

**Control logic (per channel):**

| IN_A | IN_B | Motor behavior      |
|------|------|---------------------|
| PWM  | LOW  | Forward at speed    |
| LOW  | PWM  | Reverse at speed    |
| HIGH | HIGH | Brake (active stop) |
| LOW  | LOW  | Coast (freewheel)   |

Speed is set by PWM duty cycle (0–100%) on the active IN pin. Recommended PWM frequency: 1kHz–20kHz.

---

#### DRV8871 — Single-Channel DC Motor Driver ([Adafruit #3190](https://www.adafruit.com/product/3190))
- **Supply voltage:** 6.5V–45V (motor side); logic up to 5.5V (3.3V compatible)
- **Current:** 3.6A peak; default ~2A limit (adjustable via Rlim resistor)
- **Channels:** 1 H-bridge (single motor)
- **Protections:** Current limiting (no inline sense resistor needed), undervoltage lockout, thermal shutdown

**Pin mapping:**

| Pin    | Connects to        | Notes                                  |
|--------|--------------------|----------------------------------------|
| IN1    | Pico GPIO (PWM)    | Direction/speed control                |
| IN2    | Pico GPIO (PWM)    | Direction/speed control                |
| VMotor | Motor power supply | 6.5V–45V (screw terminal)              |
| GND    | Common GND         | (screw terminal)                       |
| OUT    | Motor terminals    | (screw terminal)                       |
| Rlim   | Resistor to GND    | Sets current limit; 30kΩ default (~2A) |

**Control logic:** Same IN1/IN2 PWM scheme as the DRV8833 (see above). Use this driver for higher-voltage or higher-current motors.

---

#### Flipsky Brushed ESC — Dual-Channel ([AliExpress](https://www.aliexpress.com/i/2255800780744923.html))
- **Supply voltage:** 6V–14V (2S–3S LiPo)
- **Current:** 5A per channel (10A total)
- **Channels:** 2 (drives two brushed motors)
- **Motor compatibility:** 130/180 brushed motors
- **Protections:** 130°C thermal shutdown, hardware overcurrent, signal loss protection, undervoltage cutoff below 4V
- **Dimensions:** 24×45mm

**Control signal — PPM (RC servo format):**

Unlike the DRV8833/8871, this ESC is controlled by standard RC PPM pulses, not raw IN/IN PWM. The Pico generates a 50Hz signal with a pulse width between 1ms and 2ms.

| Pulse width | Motor behavior |
|-------------|----------------|
| 1.0ms       | Full reverse   |
| 1.5ms       | Stop (neutral) |
| 2.0ms       | Full forward   |

**Pin mapping:**

| ESC wire   | Connects to       | Notes                              |
|------------|-------------------|------------------------------------|
| PPM1       | Pico GPIO (PWM)   | Controls channel 1 (or both in sync mode) |
| PPM2       | Pico GPIO (PWM)   | Controls channel 2 (differential/independent modes) |
| Power +    | LiPo battery +    | 6V–14V                             |
| Power −    | LiPo battery −    | Also connect to Pico GND           |
| Motor A +/−| Motor terminals   |                                    |
| Motor B +/−| Motor terminals   |                                    |

**Operating modes** (hardware toggle switch on board):

| Mode          | PPM signals | Behavior                                      |
|---------------|-------------|-----------------------------------------------|
| Differential  | 2           | Tank steering — each channel controlled independently |
| Synchronous   | 1           | Both motors mirror the same signal            |
| Independent   | 2           | Fully separate control per channel            |

**Arming requirement:** On power-up, the PPM signal must be at neutral (1.5ms) before the ESC will respond. If signal is lost, return to neutral and re-arm. This is a safety feature — factor it into startup code.

---

### Motors

> _TBD — add motor model, voltage/current ratings, and which controller/channel each motor connects to._

---

### Servos

> _TBD — add servo model, voltage/current ratings, signal frequency (typically 50Hz), and Pico GPIO pin assignments._

---

### Pico 2 W GPIO Assignments

| GPIO | Function         | Notes                                      |
|------|------------------|--------------------------------------------|
| GP0  | (reserved)       | UART0 TX — keep free                       |
| GP1  | (reserved)       | UART0 RX — keep free                       |
| GP2  | ESC 1 — PPM1     | Motor: Front-Left (PWM slice 1A)           |
| GP3  | ESC 1 — PPM2     | Motor: Front-Right (PWM slice 1B)          |
| GP4  | ESC 2 — PPM1     | Motor: Back-Left (PWM slice 2A)            |
| GP5  | ESC 2 — PPM2     | Motor: Back-Right (PWM slice 2B)           |
| GP6  | ESC 3 — PPM1     | Motor: Strafe (PWM slice 3A)        |
| GP7  | ESC 3 — PPM2     | Spare ESC channel (PWM slice 3B)           |
| …    | _TBD_            | Servos, sensors — to be assigned           |

> ESC pairs (PPM1+PPM2) must share the same PWM slice for consistent 50Hz timing. Consecutive even/odd GPIO pairs always share a slice on the RP2350.

> GP14/GP15 are used in the current ESC test code (`test_esc_controller.cpp`) and will be reassigned once the motor abstraction layer is implemented.

