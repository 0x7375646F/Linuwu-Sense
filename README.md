# Linuwu-Sense: Acer Gaming Laptop Control

Control your Acer Predator or Nitro gaming laptop's hardware features including RGB keyboard, performance profiles, fan speeds, and battery management.

## 🪟 Windows 11 Version Available!

**NEW**: A complete Windows 11 port is now available! Control your Acer laptop on Windows with a modern GUI application.

👉 **[Go to Windows 11 Version →](Windows11/README.md)**

Features include:
- 🎨 4-Zone RGB Keyboard Control
- ⚡ Performance Profiles (Quiet/Balanced/Performance/Turbo)
- 🌀 CPU & GPU Fan Control
- 🔋 Battery Health Management
- 💻 Modern Windows 11 UI
- 🎮 Full Predator and Nitro Series Support

---

## 🐧 Linux Kernel Module (Original)

# Unofficial Linux Kernel Module for Acer Gaming RGB Keyboard Backlight and Turbo Mode (Acer Predator , Nitro)
The code base is still in its early stages, as I've just started working on developing this kernel module. It's a bit messy at the moment, but I'm hopeful that, with your help, we can collaborate to improve its structure and make it more organized over time.

Inspired by [acer-predator-turbo](https://github.com/JafarAkhondali/acer-predator-turbo-and-rgb-keyboard-linux-module), which has a similar goal, this project was born out of my own challenges. I faced issues detecting the Turbo key and ended up using [acer_wmi](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/acer-wmi.c), but it lacked key features like RGB , custom fan support, battery limiter, and more. As a result, I decided to implement these missing features in my own project.

## 🚀 Installation
To begin, identify your current kernel version:
```bash
uname -r
```

Install the appropriate Linux headers based on your kernel version. This module has been tested with kernel version (6.12,6.13 ([previous code base](https://github.com/0x7375646F/Linuwu-Sense/tree/v6.13)),6.14) zen. 
For Arch Linux:
```bash
sudo pacman -S linux-headers
```
Next, clone the repository and build the module:
```bash
git clone https://github.com/0x7375646F/Linuwu-Sense.git
cd Linuwu-Sense
make install
```
The make command will remove the current acer_wmi module and load the patched version.

To Uninstall:
```bash
make uninstall
```
> **⚠️ Warning!**
> ## Use at your own risk! This driver is independently developed through reverse engineering the official PredatorSense app, without any involvement from Acer. It interacts with low-level WMI methods, which may not be tested across all models.

## 🛠️ Usage
# Example Usage and Configuration

Thermal profiles can be easily switched with a single click! 😎 For battery mode, you can choose between Eco and Balanced, while when plugged into AC, you have the options for Quiet, Balanced, Performance, and Turbo. ⚡💻 Each profile will be different for battery and AC, and the thermal and fan settings will automatically adjust based on your current power source. Customize it to fit your preferences! 🌟

---

For **Predator** laptops, the following path is used: `/sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense`

For **Nitro** laptops, the following path is used: `/sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/nitro_sense`

predator_sense – This directory includes all the features, excluding the custom boot logo functionality.
four_zoned_kb – If your keyboard is four-zoned, this directory provides support for it. Unfortunately, there is no support for per-key RGB keyboards.
Here is how to interact with the Virtual Filesystems (VFS) mounted in this path:

### **0. Thermal Profiles (Nitro users especially who don't have switch key) 🚀**

Some acer nitro laptops don't come up with the thermal profile switch button in this case we manually need to set it:

To probe the current thermal profile:

`cat /sys/firmware/acpi/platform_profile`

To check the supported thermal profile:

`cat /sys/firmware/acpi/platform_profile_choices`

To switch the platform profile:

`echo balanced | sudo tee /sys/firmware/acpi/platform_profile`

Replace the balanced with the supported profile you have.

#### **1. Backlight Timeout ⏰**

This feature turns off the keyboard RGB after 30 seconds of idle mode.

- **0** – Disabled
- **1** – Enabled

To check the current status, use:

`cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/backlight_timeout`

To change the state, use:

`echo 1 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/backlight_timeout`

---

#### **2. Battery Calibration 🔋**

This function calibrates your battery to provide a more accurate percentage reading. It involves charging the battery to 100%, draining it to 0%, and recharging it back to 100%. **Do not unplug the laptop from AC power during calibration.**

- **1** – Start calibration
- **0** – Stop calibration

To check the current status:

`cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/battery_calibration`

To change the state:

`echo 1 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/battery_calibration`

---

#### **3. Battery Limiter ⚡**

Limits battery charging to 80%, preserving battery health for laptops primarily used while plugged into AC power.

- **1** – Enabled
- **0** – Disabled

To check the current status:

`cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/battery_limiter`

To change the state:

`echo 1 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/battery_limiter`

---

#### **4. Boot Animation Sound 🎶**

Enables or disables custom boot animation and sound.

- **1** – Enabled
- **0** – Disabled

To check the current status:

`cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/boot_animation_sound`

To change the state:

`echo 0 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/boot_animation_sound`

---

#### **5. Fan Speed  🌬️**

Controls the CPU and GPU fan speeds.

- **0** – Auto
- **1** – Minimum fan speed (not recommended)
- **100** – Maximum fan speed
- Other values like **50, 55, 70** can be set according to your preference.

Example (set CPU to 50 and GPU to 70):

`echo 50,70 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/fan_speed`

---

#### **6. LCD Override 🖥️**

Reduces LCD latency and minimizes ghosting.

- **1** – Enabled
- **0** – Disabled

To check the current status:

`cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/lcd_override`

To change the state:

`echo 1 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/lcd_override`

---

#### **7. USB Charging ⚡**

Allows the USB charging port to provide power even when the laptop is off.

- **0** – Disabled
- **10** – Provides power until battery reaches 10%
- **20** – Provides power until battery reaches 20%
- **30** – Provides power until battery reaches 30%

To check the current status:

`cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/usb_charging`

To change the state:

`echo 20 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/usb_charging`

---
## 💻 Keyboard Configuration 
### **Directory: `four_zoned_kb`**

The `four_zoned_kb` directory contains two Virtual File Systems (VFS) that control the RGB backlight behavior of the four-zone keyboard:

1. **`four_zone_mode`**
2. **`per_zone_mode`**

#### **1. Per-Zone Mode (`per_zone_mode`) 🎨**

This mode allows you to set a specific RGB color for each of the four keyboard zones individually. Each zone is represented by an RGB value in hexadecimal format (e.g., `4287f5` where `42` is Red, `87` is Green, and `f5` is Blue).

- **Parameters:**
    
    - The `per_zone_mode` file accepts four parameters, one for each zone, separated by commas.
    - The `per_zone_mode` also accepts brightness value.
    - Each parameter represents the RGB value for a specific zone in the format `RRGGBB`.
- **Example:**

To set all four zones to the same color (`4287f5`) and brightness to full:

`echo 4287f5,4287f5,4287f5,4287f5,100 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/four_zoned_kb/per_zone_mode`

To set each zone with unique colors:

`echo 4287f5,ff5733,33ff57,ff33a6,100 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/four_zoned_kb/per_zone_mode`

When reading (`cat`) the `per_zone_mode` file, the current color values for each zone are displayed in the format:

`4287f5,4287f5,4287f5,4287f5,100`

This indicates the current RGB color for each of the four zones.

### **Four-Zone Mode (`four_zone_mode`) ✨**

The `four_zone_mode` controls advanced RGB effects for your keyboard, requiring seven parameters:

- **Parameters:**
    
    - **Mode (0-7):** Lighting effect type (e.g., static, breathing, wave).
    - **Speed (0-9):** Speed of the effect (if applicable).
    - **Brightness (0-100):** Intensity of the lighting effect.
    - **Direction (1-2):** Direction of the effect (1 = right to left, 2 = left to right).
    - **Red (0-255), Green (0-255), Blue (0-255):** RGB color values.
- **Modes:**
    
    - **0:** Static Mode – Fixed color, no animation.
    - **1:** Breathing Mode – Color fades in and out.
    - **2:** Neon Mode – Neon glow effect, fixed color (black), direction ignored.
    - **3:** Wave Mode – Wave-like effect, color transitions across the keyboard.
    - **4:** Shifting Mode – Shifting light effect, full control over speed, direction, and color.
    - **5:** Zoom Mode – Zoom effect, direction ignored.
    - **6:** Meteor Mode – Meteor-like effect, direction ignored.
    - **7:** Twinkling Mode – Twinkling light effect, direction ignored.
- **Example Command:**
    
    Set to **Neon Mode** with speed 1, full brightness, and top-to-bottom direction:
    
    `echo 3,1,100,2,0,0,0 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/four_zoned_kb/four_zone_mode`
    
    **Explanation:**
    
    - `3`: Neon Mode
    - `1`: Speed (1)
    - `100`: Full brightness
    - `2`: Direction (top to bottom)
    - `0`: Red (black for Neon)
    - `0`: Green (black for Neon)
    - `0`: Blue (black for Neon)
 
The thermal and fan profiles will be saved and loaded on each reboot, ensuring that the settings remain persistent across restarts.
## GUI:
- [Div Acer Manager Max By PXDiv](https://github.com/PXDiv/Div-Acer-Manager-Max)
- [GUI LinuwuSense By KumarVivek](https://github.com/kumarvivek1752/Linuwu-Sense-GUI/tree/main)

## 🚧 Roadmap:
- [x] GUI for keyboard rgb controls to make it noob friendly.
- [x] Module Persistence After Reboot.
- [ ] Custom Boot Logo Feature Support.
- [ ] More device support currently only ( PHN16-71 ) is fully supported.

## License
GNU General Public License v3

### 💖 Donations
Donations are completely optional but show your love for open-source development and motivate me to add more features to this project!
USDT (BEP20 - BNB Smart Chain): 0xDA7aa42B9Fc3041F20f4Ec828A70E9bDD54A6822
