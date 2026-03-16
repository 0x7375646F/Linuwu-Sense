#!/bin/bash

# Linuwu-Sense DKMS Uninstallation Script
# This script removes the Linuwu-Sense module and cleans up all related files

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

# Function to stop and disable service
stop_service() {
    print_status "Stopping and disabling systemd service..."
    
    if systemctl is-active --quiet "$SERVICE_NAME"; then
        systemctl stop "$SERVICE_NAME"
        print_status "Service stopped"
    fi
    
    if systemctl is-enabled --quiet "$SERVICE_NAME"; then
        systemctl disable "$SERVICE_NAME"
        print_status "Service disabled"
    fi
    
    print_success "Service management completed"
}

# Function to unload modules
unload_modules() {
    print_status "Unloading modules..."
    
    # Unload linuwu_sense if loaded
    if lsmod | grep -q "$MODULE_NAME"; then
        modprobe -r "$MODULE_NAME" 2>/dev/null || true
        print_status "Unloaded $MODULE_NAME module"
    fi
    
    # Reload acer_wmi if it was blacklisted
    if [ -f "/etc/modprobe.d/blacklist-acer_wmi.conf" ]; then
        modprobe acer_wmi 2>/dev/null || true
        print_status "Reloaded acer_wmi module"
    fi
    
    print_success "Module unloading completed"
}

# Function to remove DKMS installation
remove_dkms() {
    print_status "Removing DKMS installation..."
    
    if dkms status | grep -q "$PACKAGE_NAME"; then
        dkms remove "$PACKAGE_NAME/$PACKAGE_VERSION" --all
        print_success "DKMS installation removed"
    else
        print_warning "No DKMS installation found"
    fi
}

# Function to remove manual installation
remove_manual_installation() {
    print_status "Removing manual installation files..."
    
    # Remove module file
    if [ -f "/lib/modules/$(uname -r)/kernel/drivers/platform/x86/$MODULE_NAME.ko" ]; then
        rm -f "/lib/modules/$(uname -r)/kernel/drivers/platform/x86/$MODULE_NAME.ko"
        print_status "Removed module file"
    fi
    
    # Update module dependencies
    depmod -a
    print_status "Updated module dependencies"
    
    print_success "Manual installation files removed"
}

# Function to remove configuration files
remove_config_files() {
    print_status "Removing configuration files..."
    
    # Remove systemd service file
    if [ -f "/etc/systemd/system/$SERVICE_NAME" ]; then
        rm -f "/etc/systemd/system/$SERVICE_NAME"
        print_status "Removed systemd service file"
    fi
    
    # Remove modules-load.d configuration
    if [ -f "/etc/modules-load.d/$MODULE_NAME.conf" ]; then
        rm -f "/etc/modules-load.d/$MODULE_NAME.conf"
        print_status "Removed modules-load.d configuration"
    fi
    
    # Remove blacklist configuration
    if [ -f "/etc/modprobe.d/blacklist-acer_wmi.conf" ]; then
        rm -f "/etc/modprobe.d/blacklist-acer_wmi.conf"
        print_status "Removed acer_wmi blacklist"
    fi
    
    # Remove tmpfiles configuration
    if [ -f "/etc/tmpfiles.d/$MODULE_NAME.conf" ]; then
        rm -f "/etc/tmpfiles.d/$MODULE_NAME.conf"
        print_status "Removed tmpfiles configuration"
    fi
    
    # Reload systemd
    systemctl daemon-reload
    print_status "Reloaded systemd"
    
    print_success "Configuration files removed"
}

# Function to cleanup user groups
cleanup_user_groups() {
    print_status "Cleaning up user groups..."
    
    # Remove user from linuwu_sense group
    if getent group linuwu_sense >/dev/null; then
        gpasswd -d "$REAL_USER" linuwu_sense 2>/dev/null || true
        print_status "Removed user $REAL_USER from linuwu_sense group"
        
        # Check if group is empty and remove it
        if [ "$(getent group linuwu_sense | cut -d: -f4)" = "" ]; then
            groupdel linuwu_sense 2>/dev/null || true
            print_status "Removed empty linuwu_sense group"
        fi
    else
        print_warning "linuwu_sense group does not exist"
    fi
    
    print_success "User group cleanup completed"
}

# Function to verify uninstallation
verify_uninstallation() {
    print_status "Verifying uninstallation..."
    
    # Check if module is unloaded
    if ! lsmod | grep -q "$MODULE_NAME"; then
        print_success "Module is unloaded"
    else
        print_error "Module is still loaded"
        return 1
    fi
    
    # Check if DKMS installation is removed
    if ! dkms status | grep -q "$PACKAGE_NAME"; then
        print_success "DKMS installation is removed"
    else
        print_error "DKMS installation still exists"
        return 1
    fi
    
    # Check if service is stopped
    if ! systemctl is-active --quiet "$SERVICE_NAME"; then
        print_success "Service is stopped"
    else
        print_warning "Service is still running"
    fi
    
    print_success "Uninstallation verification completed"
}

# Function to display post-uninstallation information
display_post_uninstall_info() {
    echo
    print_success "Linuwu-Sense DKMS uninstallation completed successfully!"
    echo
    echo "Uninstallation Summary:"
    echo "  - DKMS module: Removed"
    echo "  - Module: Unloaded"
    echo "  - Service: Stopped and disabled"
    echo "  - Configuration files: Removed"
    echo "  - User groups: Cleaned up"
    echo
    echo "Note:"
    echo "  - The acer_wmi module has been reloaded"
    echo "  - You may need to reboot for all changes to take effect"
    echo
}

# Main uninstallation function
main() {
    echo "=========================================="
    echo "  Linuwu-Sense DKMS Uninstallation Script"
    echo "=========================================="
    echo
    
    check_root
    stop_service
    unload_modules
    remove_dkms
    remove_manual_installation
    remove_config_files
    cleanup_user_groups
    verify_uninstallation
    display_post_uninstall_info
}

# Run main function
main "$@"
