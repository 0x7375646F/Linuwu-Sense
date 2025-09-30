# Linuwu Sense DKMS Installation Guide

This guide explains how to install or uninstall the Linuwu Sense DKMS module. The DKMS module makes sure the driver is updated when newer kernels are installed.

## Prerequisites

Before installing, ensure you have:
- `dkms` package installed
- kernel headers 

  on Ubuntu/Debian:
  ```bash
  sudo apt-get install linux-headers-generic
  # or
  sudo apt-get install linux-headers-$(uname -r)
  ```
- `systemd` (for service management)
- Root privileges

## Installation Methods

### Method 1: Using the Installer Script (Recommended)

The easiest way to install is by using the provided installer script:

```bash
# Make sure the script is executable
chmod u+x setup.sh

# Run the installer
./setup.sh # Select option 1 or 3
```

The script will guide you through the installation process and provide options for:
- Installing the DKMS drivers (it will remove legacy driver first if installed)
- Uninstalling existing drivers
- Reinstalling/updating drivers (_use this for driver updates, it will remove the current driver first_)
- Checking service status

### Method 2: Manual Installation

If you prefer to install manually, follow these steps while in the directtory that contains `dkms.conf`:

1. **Add the module to DKMS:**
   ```bash
   sudo dkms add "$(pwd)" 
   ```
   In the output, note the version in the end, for instance if the output is:
   ```bash
   Creating symlink /var/lib/dkms/linuwu-sense/0.1.0/source -> /usr/src/linuwu-sense-0.1.0
   ```
   then version is `0.1.0`.

2. **Install the module:**
   ```bash
   sudo dkms install -m linuwu-sense -v 0.1.0
   ```

3. **Unload the stock driver and load the new one:**

   _It is safe to do it first, if the computer freezes or crash, rebooting will not load the driver automatically yet._
   ```bash
   sudo modprobe -r acer_wmi
   sudo modprobe linuwu_sense
   ```

4. **Configure automatic load the module on boot:**
   ```bash
   # Blacklist the stock driver
   echo "blacklist acer_wmi" | sudo tee /etc/modprobe.d/blacklist-acer_wmi.conf
   
   # Load the new driver
   echo "linuwu_sense" | sudo tee /etc/modules-load.d/linuwu_sense.conf
   ```

5. **Install the service:**
   ```bash
   # Copy the service file
   sudo cp linuwu_sense.service /etc/systemd/system/

   # Apply changes to systemd
   sudo systemctl daemon-reload

   # Enable and start the service
   sudo systemctl enable linuwu_sense.service
   sudo systemctl start linuwu_sense.service
   ```

### Verification

After installation, verify that the driver is loaded:

```bash
# Check if the module is loaded
lsmod | grep linuwu_sense

# Check service status
systemctl status linuwu_sense.service
```

## Uninstallation

To uninstall the drivers:

1. **Using the script:**
   ```bash
   ./setup.sh # Select option 2 (Uninstall)
   ```

2. **Manual uninstallation:**
   ```bash
   # Remove the module from DKMS
   sudo dkms remove -m linuwu_sense -v 0.1.0 --all
   
   # Stop and disable the service
   sudo systemctl stop linuwu_sense.service
   sudo systemctl disable linuwu_sense.service

   # Remove installed files
   sudo rm -f /etc/modprobe.d/blacklist-acer_wmi.conf
   sudo rm -f /etc/modules-load.d/linuwu_sense.conf
   sudo rm -f /etc/systemd/system/linuwu_sense.service
   sudo rm -rf /usr/src/linuwu_sense-0.1.0

   # Apply changes to systemd
   sudo systemctl daemon-reload
   
   # Unload then load the stock driver
   sudo modprobe -r linuwu_sense
   sudo modprobe acer_wmi
   ```

## Troubleshooting

If you encounter issues:
1. Check that `dkms` is installed: `sudo apt install dkms` (Ubuntu/Debian)
2. Ensure you're running `setup.sh` with root privileges
3. Review the installer script output for specific error messages
4. Check DKMS modules status with `dkms status`
5. Check the service logs: `journalctl -u linuwu_sense.service`

## Note about extra kernels installed

If there are other kernels installed, you may need to install the driver for those kernels manually:
```bash
sudo dkms install linuwu_sense/0.1.0 -k <kernel_version>
```
Or wait for a new system update to the kernels, it should trigger the installation of all DKMS modules automatically.

***
For more information about DKMS, refer to [dkms(8)](https://linux.die.net/man/8/dkms).