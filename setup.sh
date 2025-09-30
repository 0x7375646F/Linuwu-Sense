#!/bin/env bash

# Linuwu Sense Installer Script
# This script installs, uninstalls, or updates the Linuwu Sense driver for Acer laptops on Linux
# Components: Linuwu-Sense (drivers)

# Stop on error
set -e

# Constants
SCRIPT_VERSION="0.1.0"
SERVICE_DIR="/etc/systemd/system"
SERVICE_NAME="linuwu_sense.service"
MODULE_BLACKLIST_FILE="/etc/modprobe.d/blacklist-acer_wmi.conf"
MODULE_BLACKLIST="acer_wmi"
MODULE_LOAD_FILE="/etc/modules-load.d/linuwu_sense.conf"
MODULE_LOAD="linuwu_sense"

# These are set during runtime
MODULE_NAME=""
MODULE_VERSION=0
SYSTEM=""
ARCH=""
LEGACY="Not found"
CURRENT_VERSION="None"
LOCAL_DIR="$(
    cd "$(dirname "$0")"
    pwd -P
)"

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to check if command exists
is_command_available() {
  local name="$1"
  command -v "${name}" &>/dev/null && return 0 || true
  [ "${name}" == "systemctl" ] && return 1 || true
  echo -e "${RED}Command: ${name} is required but was not found!${NC}"
  echo -e "Please make sure ${name} is installed and"
  echo -e "available in \$PATH before running the script."
  return 1
}

# Function to check if module is already installed
is_module_installed() {
  local module_name="$1"
  if [ -z "$module_name" ]; then
    echo -e "${YELLOW}Module name cannot be empty.${NC}"
    return 1
  fi
  modinfo "${module_name}" &> /dev/null && return 0 || return 1
}

# Get current driver name and version
get_name_and_version() {
  local PACKAGE_NAME
  local PACKAGE_VERSION
  local SOURCE_DIR
  local dir

  if ! is_command_available dkms; then
    pause
    exit 1
  fi

  if [ -f "dkms.conf" ]; then
    eval "$(grep PACKAGE dkms.conf)"
    MODULE_VERSION="$PACKAGE_VERSION"
    MODULE_NAME="$PACKAGE_NAME"
  else
    echo -e "${RED}dkms.conf file not found, quitting...${NC}"
    exit 1
  fi

  if is_module_installed "${MODULE_NAME}"; then
    if dkms status | grep -q "${MODULE_NAME}"; then
      SOURCE_DIR="/usr/src/${MODULE_NAME}-"
      for dir in "${SOURCE_DIR}"*; do
        CURRENT_VERSION="${dir#${SOURCE_DIR}}"
      done
    else
      LEGACY="Found"
    fi
  fi
  return 0
}

# Function to pause script execution
pause() {
  echo -e "${BLUE}Press any key to continue...${NC}"
  read -n 1 -s -r
}

# Function to check and elevate privileges
check_root() {
  local exit_code
  if [ "$EUID" -ne 0 ]; then
    echo -e "${YELLOW}This script requires root privileges.${NC}"

    # Check if sudo is available
    if is_command_available sudo; then
      echo -e "${BLUE}Attempting to run with sudo...${NC}"
      exec sudo "$0" && exit_code=0 || exit_code=$?
      exit $exit_code
    fi

    pause
    exit 1
  fi
}

print_banner() {
  clear
  echo -e "${BLUE}==========================================${NC}"
  echo -e "${BLUE}      Linuwu Sense Installer v${SCRIPT_VERSION}       ${NC}"
  echo -e "${BLUE}     Acer Laptop WMI Drivers v${MODULE_VERSION}      ${NC}"
  echo -e "${BLUE}==========================================${NC}"
  echo -e "${YELLOW}System:         ${SYSTEM}${NC}"
  echo -e "${YELLOW}Architecture:   ${ARCH}${NC}"
  echo -e "${YELLOW}Legacy driver:  ${LEGACY}${NC}"
  echo -e "${YELLOW}Current driver: ${CURRENT_VERSION}${NC}"
  echo ""
}

# Function to detect and clean up legacy installations
legacy_uninstall() {

  if [ "${LEGACY}" == "Not found" ]; then
    echo -e "${YELLOW}There is no legacy installation, skipping...${NC}"
    return 0
  fi
  echo -e "${YELLOW}Performing legacy uninstall...${NC}"

  # Check if the module is actually installed
  if is_module_installed "${MODULE_NAME}"; then
    if [ "${CURRENT_VERSION}" != "None" ] ; then
      echo -e "${YELLOW}DKMS driver found, legacy removal is not safe, skipping...${NC}"
    else
      # Check if make is installed
      if is_command_available make; then
        echo -e "Removing legacy driver..."
        make uninstall || true
        echo ""
        echo -e "${GREEN}Legacy installation cleanup completed.${NC}"
        LEGACY="Not found"
        return 0
      fi
    fi
  fi

  echo -e "${YELLOW}No legacy installations cleaned.${NC}"
  return 1
}

# Function to perform comprehensive cleanup for uninstall/reinstall
uninstall() {
  local array
  local is_loaded
  local exit_code

  if [ "${CURRENT_VERSION}" == "None" ]; then
    echo -e "${YELLOW}There is no current installation, skipping...${NC}"
    return 0
  fi
  echo -e "${YELLOW}Performing uninstall...${NC}"

  # Stop and disable current daemon service
  if systemctl is-active --quiet ${SERVICE_NAME} 2>/dev/null; then
    echo "Stopping current daemon service..."
    systemctl stop ${SERVICE_NAME}
  fi

  if systemctl is-enabled --quiet ${SERVICE_NAME} 2>/dev/null; then
    echo "Disabling current daemon service..."
    systemctl disable ${SERVICE_NAME}
  fi

  # Uninstall DKMS drivers
  dkms remove -m "${MODULE_NAME}" -v "${MODULE_VERSION}" --all && exit_code=0 || exit_code=$?

  if [ $exit_code -ne 0 ]; then
    echo -e "${RED}Error: Failed to uninstall DKMS drivers!${NC}"
    return 1
  fi

  array=(
    "${MODULE_BLACKLIST_FILE}"
    "${MODULE_LOAD_FILE}"
    "${SERVICE_DIR}/${SERVICE_NAME}"
    "/usr/src/${MODULE_NAME}-${MODULE_VERSION}"
  )

  # Remove current installed files
  echo "Removing current installation files..."
  for artifact in "${array[@]}"; do
    echo "Removing: ${artifact}"
    [ -e "${artifact}" ] && rm -rf "${artifact}" || true
  done

  # Final systemd daemon reload
  systemctl daemon-reload

  # Load stock driber
  lsmod | grep -q "${MODULE_LOAD}" && is_loaded=0 || is_loaded=1
  if [ $is_loaded -eq 0 ]; then
    modprobe -r "${MODULE_LOAD}" || true
  fi
  [ -n "{MODULE_BLACKLIST}" ] && modprobe "${MODULE_BLACKLIST}" || true

  echo ""
  echo -e "${GREEN}Uninstalll completed successfully.${NC}"
  CURRENT_VERSION="None"
  return 0
}

install_drivers() {
  local is_loaded

  if is_module_installed "${MODULE_NAME}"; then
    echo -e "${RED}An existing driver was found, please uninstall it first.${NC}"
    return 1
  fi

  echo -e "${YELLOW}Installing DKMS drivers...${NC}"

  dkms add $(pwd)
  dkms install -m "${MODULE_NAME}" -v "${MODULE_VERSION}"

  echo ""
  echo -e "${RED}****             ATTENTION!!!            ****${NC}"
  echo -e "${RED}Stock WMI driver will be unloaded and${NC}"
  echo -e "${RED}the new DKMS driver will be loaded.${NC}"
  echo -e "${RED}If the system crashes or freezes then${NC}"
  echo -e "${RED}it is safe to reboot, the new driver will NOT${NC}"
  echo -e "${RED}load automatically unless install finishes.${NC}"
  echo ""
  pause

  [ -n "{MODULE_BLACKLIST}" ] && modprobe -r "${MODULE_BLACKLIST}" || true
  modprobe "${MODULE_LOAD}" || true
  lsmod | grep -q "${MODULE_LOAD}" && is_loaded=0 || is_loaded=1

  if [ $is_loaded -eq 0 ]; then
    echo -e "${GREEN}DKMS drivers loaded successfully!${NC}"
  else
    echo -e "${RED}Error: Failed to load DKMS drivers!${NC}"
    uninstall
    return 1
  fi

  install_service

  echo "blacklist ${MODULE_BLACKLIST}" > "${MODULE_BLACKLIST_FILE}"
  echo "${MODULE_LOAD}" > "${MODULE_LOAD_FILE}"

  echo ""
  echo -e "${YELLOW}*****               ATTENTION               *****${NC}"
  echo -e "${YELLOW}If other kernels are installed, you may need to${NC}"
  echo -e "${YELLOW}install the driver for them manually (or wait for${NC}"
  echo -e "${YELLOW}the next kernel update):${NC}"
  echo -e "${YELLOW}\$ sudo dkms install ${MODULE_NAME}/${MODULE_VERSION} -k other_kernel ${NC}"
  echo ""
  echo -e "${GREEN}DKMS drivers installed successfully!${NC}"
  CURRENT_VERSION="${MODULE_VERSION}"
  return 0
}

install_service() {
  echo -e "${YELLOW}Installing service...${NC}"

  if [ ! -f "${SERVICE_NAME}" ]; then
    echo -e "${RED}Error: service file not found!${NC}"
    return 1
  fi

  cp "${SERVICE_NAME}" "${SERVICE_DIR}"

  # Enable and start the service
  systemctl daemon-reload
  systemctl enable "${SERVICE_NAME}"
  systemctl start "${SERVICE_NAME}"

  # Verify service is running
  if systemctl is-active --quiet ${SERVICE_NAME}; then
    echo -e "${GREEN}Service installed and service started successfully!${NC}"
    return 0
  else
    echo -e "${RED}Warning: service may not have started correctly. Check status.'${NC}"
    return 1
  fi
}

show_service_status() {
    if systemctl list-unit-files | grep -q ${SERVICE_NAME}; then
      echo -e "${YELLOW}Keep the service enabled to retain settings across reboots.${NC}"
      echo ""
      echo -e "${BLUE}Service Status:${NC}"
      systemctl status ${SERVICE_NAME} --no-pager -l
    else
      echo -e "${YELLOW}Service not found. Drivers may not be installed.${NC}"
    fi
}

perform_install() {
  local skip_drivers=$1
  local is_update=$2
  local result=1

  # If this is an update/reinstall, perform cleanup first
  if [ "$is_update" = true ]; then
    echo -e "${BLUE}Performing cleanup before installation...${NC}"
    uninstall && result=0 || result=$?
    echo ""
  else
    # For fresh installs, still check for legacy installations
    legacy_uninstall && result=0 || result=$?
    echo ""
  fi

  if [ $result -eq 0 ]; then
    # Install driver
    if [ "$skip_drivers" = false ]; then
      install_drivers && result=0 || result=$?
    else
      echo -e "${YELLOW}Skipping driver installation as requested.${NC}"
    fi
  fi

  # Check if all installations were successful
  if [ $result -eq 0 ]; then
    echo -e "${GREEN}DKMS installation completed successfully!${NC}"
    echo -e "It should be already loaded and will automatically load in next boots."
    echo ""
    pause
    return 0
  else
    echo -e "${RED}Failed to install. Please check the errors above.${NC}"
    pause
    return 1
  fi
}

# Function to check system compatibility
check_system() {
  echo -e "${BLUE}Checking system compatibility...${NC}"

  # Check if we're on a supported distribution
  SYSTEM="$(uname -s)"
  ARCH="$(uname -m)"
  if [[ "${SYSTEM}" != "Linux" ]] || [[ "${ARCH}" != *"86"* ]]; then
    echo -e "${RED}Error: system is not supported.${NC}"
    pause
    exit 1
  fi

  # Check if systemd is available
  if ! is_command_available systemctl; then
    echo -e "${RED}Error: systemd is required but not found on this system.${NC}"
    pause
    exit 1
  fi

  # Check for dkms.conf
  get_name_and_version

  if [ -f /etc/os-release ]; then
    eval "$(grep NAME /etc/os-release)"
    echo "Detected OS: ${PRETTY_NAME}"
  fi

  echo -e "${GREEN}System compatibility check passed.${NC}"
  sleep 2
}

main_menu() {
  local max

  # Check and elevate privileges if needed
  check_root

  # Perform initial system check
  check_system

  while true; do
    [ "${LEGACY}" == "Found" ] && max=5 || max=4
    print_banner

    echo -e "Please select an option:"
    echo -e "  ${GREEN}1${NC}) Install DKMS drivers"
    echo -e "  ${GREEN}2${NC}) Uninstall DKMS drivers"
    echo -e "  ${GREEN}3${NC}) Reinstall/Update DKMS drivers (recommended for upgrades)"
    echo -e "  ${GREEN}4${NC}) Check service status"
    [ ${max} -eq 5 ] && echo -e "  ${GREEN}5${NC}) Uninstall legacy (non-DKMS) drivers"
    echo -e "  ${GREEN}q${NC}) Quit"
    echo ""

    read -p "Enter your choice [1-${max} or q]: " choice

    case $choice in
      1)
        echo -e "${BLUE}Starting complete installation...${NC}"
        perform_install false false || true
        ;;
      2)
        echo -e "${BLUE}Starting uninstallation...${NC}"
        uninstall || true
        pause
        ;;
      3)
        echo -e "${BLUE}Starting reinstallation/update...${NC}"
        echo -e "${YELLOW}This will completely remove the existing installation first.${NC}"
        perform_install false true
        ;;
      4)
        echo -e "${BLUE}Checking service status...${NC}"
        show_service_status
        echo ""
        pause
        ;;
      5)
        if [ ${max} -eq 5 ]; then
          echo -e "${BLUE}Starting legacy uninstallation...${NC}"
          legacy_uninstall || true
          pause
        else
          echo -e "${RED}Invalid option. Please try again.${NC}"
          sleep 2
        fi
        ;;
      q|Q)
        echo -e "${BLUE}Exiting installer. Goodbye!${NC}"
        exit 0
        ;;
      *)
        echo -e "${RED}Invalid option. Please try again.${NC}"
        sleep 2
        ;;
    esac
  done
}

# Start the installer
main_menu
exit 0
