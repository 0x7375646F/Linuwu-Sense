# Linuwu Sense

**Unofficial Linux kernel module for Acer Predator / Nitro gaming laptops** — RGB keyboard backlight, turbo mode, fan control, battery limiter, and more, reverse-engineered from the official PredatorSense app.

The code base is still evolving. It's a bit messy in places, but contributions to structure and organize it further are welcome.

Inspired by [acer-predator-turbo](https://github.com/JafarAkhondali/acer-predator-turbo-and-rgb-keyboard-linux-module), which has a similar goal, this project was born out of my own challenges. I faced issues detecting the Turbo key and ended up using [acer_wmi](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/acer-wmi.c), but it lacked key features like RGB, custom fan support, battery limiter, and more. As a result, I decided to implement these missing features in my own project.

> **⚠️ Use at your own risk.** This driver is independently developed through reverse engineering the official PredatorSense app, without any involvement from Acer. It interacts with low-level WMI methods, which may not be tested across all models.

## Contents

- [Installation](#-installation)
- [Usage](#️-usage)
  - [Thermal Profiles](#0-thermal-profiles-nitro-users-especially-who-dont-have-switch-key-)
  - [Backlight Timeout](#1-backlight-timeout-)
  - [Battery Calibration](#2-battery-calibration-)
  - [Battery Limiter](#3-battery-limiter-)
  - [Boot Animation Sound](#4-boot-animation-sound-)
  - [Fan Speed](#5-fan-speed--️)
  - [LCD Override](#6-lcd-override-️)
  - [USB Charging](#7-usb-charging-)
  - [Turbo State & OSD Popup](#8-turbo-state-)
- [Keyboard Configuration](#-keyboard-configuration)
- [GUI Frontends](#gui)
- [Troubleshooting](#-troubleshooting)
- [Roadmap](#-roadmap)
- [License](#license)
- [Donations](#-donations)

## 🚀 Installation

**Requirements:** Linux headers matching your running kernel, and `dkms`. Tested on kernel versions 6.12, 6.13 ([previous code base](https://github.com/0x7375646F/Linuwu-Sense/tree/v6.13)), and 6.14 (zen).

Identify your current kernel version:
```bash
uname -r
```

Install headers and `dkms`:
```bash
# Arch Linux
sudo pacman -S linux-headers dkms

# Debian / Ubuntu
sudo apt install linux-headers-$(uname -r) dkms
```

Clone the repository and build/install the module:
```bash
git clone https://github.com/0x7375646F/Linuwu-Sense.git
cd Linuwu-Sense
sudo make install
```

`make install` will:
1. Blacklist the in-kernel `acer_wmi` module (which otherwise conflicts with this driver).
2. Register `linuwu_sense` with **DKMS** and build/install it for your running kernel.
3. Load the module and configure it to auto-load on every boot.
4. Set up a `linuwu_sense` group and sysfs permissions so you don't need `sudo` for everyday use.

Because the module is registered with DKMS, it is **automatically rebuilt whenever your kernel is upgraded** — you do not need to re-run `make install` after every kernel update. Verify at any time with:
```bash
dkms status
```

To uninstall:
```bash
sudo make uninstall
```

## 🛠️ Usage

Thermal profiles can be switched with a single click. For battery mode, choose between Eco and Balanced; when plugged into AC, choose Quiet, Balanced, Performance, or Turbo. Each profile is remembered separately for battery and AC power, and the thermal/fan settings adjust automatically based on your current power source.

For **Predator** laptops, the following path is used: `/sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense`

For **Nitro** laptops, the following path is used: `/sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/nitro_sense`

`predator_sense` includes all features except the custom boot logo. `four_zoned_kb` provides RGB support for four-zone keyboards (no per-key RGB support). Here's how to interact with the sysfs files in these directories:

### **0. Thermal Profiles (Nitro users especially who don't have switch key) 🚀**

Some Acer Nitro laptops don't have a thermal profile switch button, so it must be set manually:

To probe the current thermal profile:
```bash
cat /sys/firmware/acpi/platform_profile
```

To check the supported thermal profiles:
```bash
cat /sys/firmware/acpi/platform_profile_choices
```

To switch the platform profile (replace `balanced` with a supported profile):
```bash
echo balanced | sudo tee /sys/firmware/acpi/platform_profile
```

### **1. Backlight Timeout ⏰**

Turns off the keyboard RGB after 30 seconds of idle mode.

| Value | Meaning |
|---|---|
| `0` | Disabled |
| `1` | Enabled |

```bash
# Check
cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/backlight_timeout

# Set
echo 1 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/backlight_timeout
```

### **2. Battery Calibration 🔋**

Calibrates your battery to provide a more accurate percentage reading: charges to 100%, drains to 0%, then recharges to 100%. **Do not unplug the laptop from AC power during calibration.**

| Value | Meaning |
|---|---|
| `1` | Start calibration |
| `0` | Stop calibration |

```bash
# Check
cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/battery_calibration

# Set
echo 1 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/battery_calibration
```

### **3. Battery Limiter ⚡**

Limits battery charging to 80%, preserving battery health for laptops primarily used while plugged into AC power.

| Value | Meaning |
|---|---|
| `1` | Enabled |
| `0` | Disabled |

```bash
# Check
cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/battery_limiter

# Set
echo 1 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/battery_limiter
```

### **4. Boot Animation Sound 🎶**

Enables or disables the custom boot animation and sound.

| Value | Meaning |
|---|---|
| `1` | Enabled |
| `0` | Disabled |

```bash
# Check
cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/boot_animation_sound

# Set
echo 0 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/boot_animation_sound
```

### **5. Fan Speed 🌬️**

Controls the CPU and GPU fan speeds.

| Value | Meaning |
|---|---|
| `0` | Auto |
| `1` | Minimum fan speed (not recommended) |
| `100` | Maximum fan speed |
| other | Any value in between, e.g. `50`, `55`, `70` |

Example (set CPU to 50 and GPU to 70):
```bash
echo 50,70 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/fan_speed
```

### **6. LCD Override 🖥️**

Reduces LCD latency and minimizes ghosting.

| Value | Meaning |
|---|---|
| `1` | Enabled |
| `0` | Disabled |

```bash
# Check
cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/lcd_override

# Set
echo 1 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/lcd_override
```

### **7. USB Charging ⚡**

Allows the USB charging port to provide power even when the laptop is off.

| Value | Meaning |
|---|---|
| `0` | Disabled |
| `10` | Provides power until battery reaches 10% |
| `20` | Provides power until battery reaches 20% |
| `30` | Provides power until battery reaches 30% |

```bash
# Check
cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/usb_charging

# Set
echo 20 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense/usb_charging
```

### **8. Turbo State 🌀**

Read-only attribute exposing the current turbo hotkey state, for laptops that use the turbo-toggle fan mechanism (e.g. Predator PH315-53 / PH315-52) instead of the `predator_sense` / `nitro_sense` thermal-profile interface.

| Value | Meaning |
|---|---|
| `0` | Auto fan mode |
| `1` | Turbo fan mode |

```bash
cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/turbo_sense/turbo_state
```

The driver calls `sysfs_notify()` on this file every time the turbo hotkey is toggled, so userspace can `poll()` on it instead of polling in a loop. This is what powers the **Turbo OSD** popup below.

#### Turbo OSD popup 🟢⚪

`osd/linuwu_turbo_osd.py` is a small GTK3 script that shows a transient grey/green popup at the top of the screen for 3 seconds whenever turbo mode is toggled on or off. It blocks on `turbo_state` rather than polling, so it uses no CPU while idle.

Requirements: `python3-gi` with GTK 3 (`sudo apt install python3-gi gir1.2-gtk-3.0` on Debian/Ubuntu/Mint).

To try it directly:
```bash
python3 osd/linuwu_turbo_osd.py
```

To run it automatically on login via a systemd `--user` service — **this step does not happen automatically as part of `make install`, it must be done separately**:
```bash
mkdir -p ~/.local/share/linuwu-sense ~/.config/systemd/user
cp osd/linuwu_turbo_osd.py ~/.local/share/linuwu-sense/
cp osd/linuwu-turbo-osd.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now linuwu-turbo-osd.service
```

Check it's running with `systemctl --user status linuwu-turbo-osd.service`.

## 💻 Keyboard Configuration

### Directory: `four_zoned_kb`

The `four_zoned_kb` directory contains two sysfs files that control the RGB backlight behavior of the four-zone keyboard:

1. **`four_zone_mode`**
2. **`per_zone_mode`**

#### Per-Zone Mode (`per_zone_mode`) 🎨

Sets a specific RGB color for each of the four keyboard zones individually. Each zone is an RGB value in hex (e.g. `4287f5`, where `42` is Red, `87` is Green, `f5` is Blue).

- The file accepts four color parameters (one per zone) plus a brightness value, all comma-separated.
- Each color parameter is in the format `RRGGBB`.

Set all four zones to the same color and full brightness:
```bash
echo 4287f5,4287f5,4287f5,4287f5,100 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/four_zoned_kb/per_zone_mode
```

Set each zone to a unique color:
```bash
echo 4287f5,ff5733,33ff57,ff33a6,100 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/four_zoned_kb/per_zone_mode
```

Reading (`cat`) the file returns the current values in the same format: `4287f5,4287f5,4287f5,4287f5,100`.

#### Four-Zone Mode (`four_zone_mode`) ✨

Controls advanced RGB effects for the keyboard. Takes seven comma-separated parameters:

| Parameter | Range | Meaning |
|---|---|---|
| Mode | 0–7 | Lighting effect type (see table below) |
| Speed | 0–9 | Speed of the effect (if applicable) |
| Brightness | 0–100 | Intensity of the lighting effect |
| Direction | 1–2 | `1` = right to left, `2` = left to right |
| Red | 0–255 | |
| Green | 0–255 | |
| Blue | 0–255 | |

Modes:

| Mode | Name | Notes |
|---|---|---|
| 0 | Static | Fixed color, no animation |
| 1 | Breathing | Color fades in and out |
| 2 | Neon | Fixed neon glow (black), direction ignored |
| 3 | Wave | Color transitions across the keyboard |
| 4 | Shifting | Full control over speed, direction, and color |
| 5 | Zoom | Direction ignored |
| 6 | Meteor | Direction ignored |
| 7 | Twinkling | Direction ignored |

Example — Neon Mode, speed 1, full brightness, top-to-bottom:
```bash
echo 3,1,100,2,0,0,0 | sudo tee /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/four_zoned_kb/four_zone_mode
```
`3` Neon Mode · `1` Speed · `100` Full brightness · `2` Direction (top to bottom) · `0,0,0` Black (ignored by Neon)

Thermal and fan profiles are saved and reloaded on every reboot, so your settings persist across restarts.

## GUI

- [Div Acer Manager Max by PXDiv](https://github.com/PXDiv/Div-Acer-Manager-Max)
- [GUI LinuwuSense by KumarVivek](https://github.com/kumarvivek1752/Linuwu-Sense-GUI/tree/main)

## 🩺 Troubleshooting

**Turbo/hotkeys stopped working after a reboot, or nothing under `/sys/module/linuwu_sense` exists anymore.**
This almost always means the kernel was upgraded and the module was never rebuilt for the new kernel. Check what's actually loaded:
```bash
uname -r                 # kernel you're running now
dkms status               # should list linuwu_sense as "installed" for that kernel
lsmod | grep linuwu_sense
```
If `dkms status` doesn't show your current kernel, DKMS wasn't able to rebuild it automatically (e.g. it was installed before this project switched to DKMS, or the rebuild failed). Re-run `sudo make install` from the repo to fix it, then confirm `dkms status` lists your running kernel next time.

**Pressing the turbo key does nothing, and `dmesg` shows nothing either.**
Confirm the module is actually loaded and receiving the key (`sudo dmesg | grep -i turbo` while pressing the key should print `pressed turbo button - N`). If a press is logged but nothing happens, check `dmesg` for a `linuwu_sense: Failed to ...` error — the driver logs a `pr_err` whenever the underlying WMI call to read or set the turbo LED fails, so a firmware/EC-level failure will now show up there instead of failing silently.

**The Turbo OSD popup never appears.**
The popup is a separate opt-in component (`osd/linuwu_turbo_osd.py`) — adding the driver support for it does *not* install or start it. Check whether it's actually running:
```bash
systemctl --user status linuwu-turbo-osd.service
```
If the unit isn't found, you haven't installed it yet — follow the [Turbo OSD popup](#turbo-osd-popup-) steps above. If it's running but still silent, confirm the sysfs file it watches exists and updates: `cat /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/turbo_sense/turbo_state` should flip between `0`/`1` as you press the key.

## 🚧 Roadmap

- [x] GUI for keyboard RGB controls to make it noob-friendly.
- [x] Module persistence after reboot (via DKMS, surviving kernel upgrades too).
- [ ] Custom boot logo feature support.
- [ ] More device support — currently only PHN16-71 is fully supported.

## License

GNU General Public License v3

## 💖 Donations

Donations are completely optional but show your love for open-source development and motivate me to add more features to this project!

USDT (BEP20 - BNB Smart Chain): `0xDA7aa42B9Fc3041F20f4Ec828A70E9bDD54A6822`
