#!/bin/bash

# Linuwu-Sense DKMS Installation Script
# This script installs the Linuwu-Sense module with full DKMS support

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script variables
PACKAGE_NAME="linuwu-sense"
PACKAGE_VERSION="1.0.0"
MODULE_NAME="linuwu_sense"
SERVICE_NAME="linuwu_sense.service"
REAL_USER=$(echo ${SUDO_USER:-$(whoami)})

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    local missing_deps=()
    
    # Check for DKMS
    if ! command -v dkms &> /dev/null; then
        missing_deps+=("dkms")
    fi
    
    # Check for build tools
    if ! command -v make &> /dev/null; then
        missing_deps+=("build-essential")
    fi
    
    # Check for kernel headers
    local kernel_version=$(uname -r)
    if [ ! -d "/lib/modules/$kernel_version/build" ]; then
        missing_deps+=("linux-headers-$kernel_version")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_status "Please install them using your package manager:"
        echo "  sudo apt install ${missing_deps[*]}  # For Debian/Ubuntu"
        echo "  sudo dnf install ${missing_deps[*]}  # For Fedora/RHEL"
        echo "  sudo pacman -S ${missing_deps[*]}    # For Arch Linux"
        exit 1
    fi
    
    print_success "All dependencies are satisfied"
}

# Function to backup existing installation
backup_existing() {
    print_status "Checking for existing installation..."
    
    if dkms status | grep -q "$PACKAGE_NAME"; then
        print_warning "Existing DKMS installation found"
        read -p "Do you want to remove the existing installation? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            print_status "Removing existing DKMS installation..."
            dkms remove "$PACKAGE_NAME/$PACKAGE_VERSION" --all
            print_success "Existing installation removed"
        else
            print_error "Installation aborted"
            exit 1
        fi
    fi
    
    # Check for manual installation
    if [ -f "/lib/modules/$(uname -r)/kernel/drivers/platform/x86/$MODULE_NAME.ko" ]; then
        print_warning "Manual installation found"
        read -p "Do you want to remove the manual installation? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            print_status "Removing manual installation..."
            rm -f "/lib/modules/$(uname -r)/kernel/drivers/platform/x86/$MODULE_NAME.ko"
            depmod -a
            print_success "Manual installation removed"
        fi
    fi
}

# Function to unload conflicting modules
unload_conflicting_modules() {
    print_status "Unloading conflicting modules..."
    
    # Unload acer_wmi if loaded
    if lsmod | grep -q "acer_wmi"; then
        print_status "Unloading acer_wmi module..."
        modprobe -r acer_wmi 2>/dev/null || true
    fi
    
    # Unload linuwu_sense if loaded
    if lsmod | grep -q "$MODULE_NAME"; then
        print_status "Unloading existing $MODULE_NAME module..."
        modprobe -r "$MODULE_NAME" 2>/dev/null || true
    fi
    
    print_success "Conflicting modules unloaded"
}

# Function to install DKMS module
install_dkms_module() {
    print_status "Installing DKMS module..."
    
    # Add module to DKMS
    dkms add "$(pwd)"
    
    # Build and install module
    dkms build "$PACKAGE_NAME/$PACKAGE_VERSION"
    dkms install "$PACKAGE_NAME/$PACKAGE_VERSION"
    
    print_success "DKMS module installed successfully"
}

# Function to configure module loading
configure_module_loading() {
    print_status "Configuring module loading..."
    
    # Create blacklist for acer_wmi
    cat > /etc/modprobe.d/blacklist-acer_wmi.conf << EOF
# Blacklist acer_wmi to prevent conflicts with linuwu_sense
blacklist acer_wmi
EOF
    
    # Create modules-load.d configuration
    cat > /etc/modules-load.d/$MODULE_NAME.conf << EOF
# Load linuwu_sense module at boot
$MODULE_NAME
EOF
    
    print_success "Module loading configured"
}

# Function to setup systemd service
setup_systemd_service() {
    print_status "Setting up systemd service..."
    
    # Copy service file
    cp "$SERVICE_NAME" /etc/systemd/system/
    
    # Reload systemd and enable service
    systemctl daemon-reload
    systemctl enable "$SERVICE_NAME"
    systemctl start "$SERVICE_NAME"
    
    print_success "Systemd service configured"
}

# Function to setup user permissions
setup_permissions() {
    print_status "Setting up user permissions..."
    
    # Create linuwu_sense group if it doesn't exist
    if ! getent group linuwu_sense >/dev/null; then
        groupadd linuwu_sense
        print_status "Created linuwu_sense group"
    fi
    
    # Add user to group
    usermod -aG linuwu_sense "$REAL_USER"
    print_status "Added user $REAL_USER to linuwu_sense group"
    
    # Setup tmpfiles configuration for sysfs permissions
    setup_tmpfiles_config
    
    print_success "User permissions configured"
}

# Function to setup tmpfiles configuration
setup_tmpfiles_config() {
    print_status "Setting up sysfs permissions..."
    
    local tmpfiles_conf="/etc/tmpfiles.d/$MODULE_NAME.conf"
    
    # Wait for module to be loaded
    sleep 2
    
    # Detect model path
    local model_path=$(ls /sys/module/$MODULE_NAME/drivers/platform:acer-wmi/acer-wmi/ 2>/dev/null | grep -E 'predator_sense|nitro_sense' || true)
    
    if [ -n "$model_path" ]; then
        print_status "Detected model directory: $model_path"
        
        # Create tmpfiles configuration
        cat > "$tmpfiles_conf" << EOF
# Linuwu-Sense sysfs permissions
EOF
        
        # Add permissions for supported fields based on model
        if echo "$model_path" | grep -q "nitro_sense"; then
            local supported_fields="fan_speed battery_limiter battery_calibration usb_charging"
        else
            local supported_fields="backlight_timeout battery_calibration battery_limiter boot_animation_sound fan_speed lcd_override usb_charging"
        fi
        
        for field in $supported_fields; do
            echo "f /sys/module/$MODULE_NAME/drivers/platform:acer-wmi/acer-wmi/$model_path/$field 0660 root linuwu_sense" >> "$tmpfiles_conf"
        done
        
        # Add keyboard permissions if available
        local kb_base="/sys/module/$MODULE_NAME/drivers/platform:acer-wmi/acer-wmi/four_zoned_kb"
        if [ -d "$kb_base" ]; then
            for zone in four_zone_mode per_zone_mode; do
                echo "f $kb_base/$zone 0660 root linuwu_sense" >> "$tmpfiles_conf"
            done
        fi
        
        # Apply tmpfiles configuration
        systemd-tmpfiles --create "$tmpfiles_conf"
        print_success "Sysfs permissions configured"
    else
        print_warning "Could not detect predator_sense or nitro_sense in sysfs"
    fi
}

# Function to load the module
load_module() {
    print_status "Loading $MODULE_NAME module..."
    
    modprobe "$MODULE_NAME"
    
    # Wait a moment for module to initialize
    sleep 2
    
    if lsmod | grep -q "$MODULE_NAME"; then
        print_success "Module loaded successfully"
    else
        print_error "Failed to load module"
        exit 1
    fi
}

# Function to verify installation
verify_installation() {
    print_status "Verifying installation..."
    
    # Check if module is loaded
    if lsmod | grep -q "$MODULE_NAME"; then
        print_success "Module is loaded"
    else
        print_error "Module is not loaded"
        return 1
    fi
    
    # Check DKMS status
    if dkms status | grep -q "$PACKAGE_NAME.*installed"; then
        print_success "DKMS installation verified"
    else
        print_error "DKMS installation verification failed"
        return 1
    fi
    
    # Check service status
    if systemctl is-active --quiet "$SERVICE_NAME"; then
        print_success "Systemd service is running"
    else
        print_warning "Systemd service is not running"
    fi
    
    print_success "Installation verification completed"
}

# Function to display post-installation information
display_post_install_info() {
    echo
    print_success "Linuwu-Sense DKMS installation completed successfully!"
    echo
    echo "Installation Summary:"
    echo "  - DKMS module: $PACKAGE_NAME/$PACKAGE_VERSION"
    echo "  - Module name: $MODULE_NAME"
    echo "  - User group: linuwu_sense"
    echo "  - Service: $SERVICE_NAME"
    echo
    echo "Next steps:"
    echo "  1. Log out and log back in for group membership to take effect"
    echo "  2. The module will automatically rebuild on kernel updates"
    echo "  3. Check /sys/module/$MODULE_NAME/ for available controls"
    echo
    echo "Useful commands:"
    echo "  - Check DKMS status: dkms status"
    echo "  - Check module status: lsmod | grep $MODULE_NAME"
    echo "  - Check service status: systemctl status $SERVICE_NAME"
    echo "  - Uninstall: sudo ./uninstall.sh"
    echo
}

# Main installation function
main() {
    echo "=========================================="
    echo "  Linuwu-Sense DKMS Installation Script"
    echo "=========================================="
    echo
    
    check_root
    check_dependencies
    backup_existing
    unload_conflicting_modules
    install_dkms_module
    configure_module_loading
    setup_systemd_service
    setup_permissions
    load_module
    verify_installation
    display_post_install_info
}

# Run main function
main "$@"
