# Linuwu-Sense DKMS Installation Guide

This guide explains how to install Linuwu-Sense using DKMS (Dynamic Kernel Module Support) for automatic kernel module rebuilding on kernel updates.

## What is DKMS?

DKMS (Dynamic Kernel Module Support) is a framework that allows kernel modules to be automatically rebuilt when a new kernel is installed. This ensures that your Linuwu-Sense module will continue to work after kernel updates without manual intervention.

## Prerequisites

Before installing Linuwu-Sense with DKMS, ensure you have the following dependencies:

### For Debian/Ubuntu:
```bash
sudo apt update
sudo apt install dkms build-essential linux-headers-$(uname -r)
```

### For Fedora/RHEL/CentOS:
```bash
sudo dnf install dkms gcc make kernel-devel
```

### For Arch Linux:
```bash
sudo pacman -S dkms linux-headers
```

## Installation

### Quick Installation

1. **Clone or download the Linuwu-Sense repository:**
   ```bash
   git clone <repository-url>
   cd Linuwu-Sense
   ```

2. **Make the installation script executable:**
   ```bash
   chmod +x install.sh
   ```

3. **Run the installation script:**
   ```bash
   sudo ./install.sh
   ```

The installation script will:
- Check for required dependencies
- Remove any existing installations
- Install the module through DKMS
- Configure automatic module loading
- Set up systemd service
- Configure user permissions
- Load the module

### Manual Installation

If you prefer to install manually:

1. **Add the module to DKMS:**
   ```bash
   sudo dkms add $(pwd)
   ```

2. **Build and install the module:**
   ```bash
   sudo dkms build linuwu-sense/1.0.0
   sudo dkms install linuwu-sense/1.0.0
   ```

3. **Configure module loading:**
   ```bash
   echo "linuwu_sense" | sudo tee /etc/modules-load.d/linuwu_sense.conf
   echo "blacklist acer_wmi" | sudo tee /etc/modprobe.d/blacklist-acer_wmi.conf
   ```

4. **Load the module:**
   ```bash
   sudo modprobe linuwu_sense
   ```

## Verification

After installation, verify that everything is working correctly:

### Check DKMS Status
```bash
dkms status
```
You should see output like:
```
linuwu-sense, 1.0.0, 5.15.0-52-generic, x86_64: installed
```

### Check Module Status
```bash
lsmod | grep linuwu_sense
```

### Check Service Status
```bash
systemctl status linuwu_sense.service
```

### Check Available Controls
```bash
ls /sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/
```

## Uninstallation

### Quick Uninstallation

To remove the DKMS installation:

```bash
sudo ./uninstall.sh
```

### Manual Uninstallation

1. **Remove DKMS installation:**
   ```bash
   sudo dkms remove linuwu-sense/1.0.0 --all
   ```

2. **Remove configuration files:**
   ```bash
   sudo rm -f /etc/modules-load.d/linuwu_sense.conf
   sudo rm -f /etc/modprobe.d/blacklist-acer_wmi.conf
   sudo rm -f /etc/systemd/system/linuwu_sense.service
   sudo rm -f /etc/tmpfiles.d/linuwu_sense.conf
   ```

3. **Reload systemd:**
   ```bash
   sudo systemctl daemon-reload
   ```

## DKMS Management Commands

### View DKMS Status
```bash
dkms status
```

### List All DKMS Modules
```bash
dkms list
```

### Rebuild Module for Current Kernel
```bash
sudo dkms autoinstall
```

### Remove Specific Version
```bash
sudo dkms remove linuwu-sense/1.0.0
```

### Remove All Versions
```bash
sudo dkms remove linuwu-sense/1.0.0 --all
```

## Troubleshooting

### Module Not Loading After Kernel Update

If the module doesn't load after a kernel update:

1. **Check if DKMS built the module for the new kernel:**
   ```bash
   dkms status
   ```

2. **Manually rebuild if needed:**
   ```bash
   sudo dkms autoinstall
   ```

3. **Check for build errors:**
   ```bash
   sudo dkms build linuwu-sense/1.0.0
   ```

### Build Errors

If you encounter build errors:

1. **Check kernel headers:**
   ```bash
   ls /lib/modules/$(uname -r)/build
   ```

2. **Install missing headers:**
   ```bash
   # For Debian/Ubuntu
   sudo apt install linux-headers-$(uname -r)
   
   # For Fedora/RHEL
   sudo dnf install kernel-devel
   
   # For Arch Linux
   sudo pacman -S linux-headers
   ```

3. **Check build logs:**
   ```bash
   cat /var/lib/dkms/linuwu-sense/1.0.0/build/make.log
   ```

### Module Conflicts

If you experience conflicts with the original acer_wmi module:

1. **Ensure acer_wmi is blacklisted:**
   ```bash
   cat /etc/modprobe.d/blacklist-acer_wmi.conf
   ```

2. **Unload conflicting modules:**
   ```bash
   sudo modprobe -r acer_wmi
   sudo modprobe linuwu_sense
   ```

## Automatic Kernel Updates

With DKMS installed, the module will automatically rebuild when:

- A new kernel is installed via package manager
- The system is updated and a new kernel is available
- You manually trigger a rebuild with `dkms autoinstall`

### Systemd Integration

The installation includes a systemd service that ensures the module is properly unloaded during shutdown. This prevents potential issues during system updates.

## File Locations

After installation, the following files are created:

- **DKMS Source:** `/usr/src/linuwu-sense-1.0.0/`
- **Module:** `/lib/modules/$(uname -r)/kernel/drivers/platform/x86/linuwu_sense.ko`
- **Configuration:**
  - `/etc/modules-load.d/linuwu_sense.conf`
  - `/etc/modprobe.d/blacklist-acer_wmi.conf`
  - `/etc/systemd/system/linuwu_sense.service`
  - `/etc/tmpfiles.d/linuwu_sense.conf`

## Support

If you encounter issues with the DKMS installation:

1. Check the troubleshooting section above
2. Review the build logs in `/var/lib/dkms/linuwu-sense/1.0.0/build/`
3. Check system logs: `journalctl -u linuwu_sense.service`
4. Verify your system meets the prerequisites

## Benefits of DKMS Installation

- **Automatic Updates:** Module rebuilds automatically on kernel updates
- **Consistent Installation:** Standardized installation process
- **Easy Management:** Simple commands to manage the module
- **Clean Uninstallation:** Complete removal of all components
- **System Integration:** Proper integration with systemd and module loading

This DKMS installation method ensures that your Linuwu-Sense module will continue to work seamlessly across kernel updates, providing a hassle-free experience for managing your Acer laptop's advanced features.
