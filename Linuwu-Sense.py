#!/usr/bin/env python3
"""
Linuwu-Sense Control Script
A Python script to control the Linuwu-Sense kernel module for Acer Predator/Nitro laptops.

This script provides an easy-to-use interface for controlling:
- Thermal profiles and fan speeds
- Battery management (limiter, calibration)
- RGB keyboard lighting (four-zone keyboards)
- Miscellaneous settings (backlight timeout, boot animation, LCD override, USB charging)

Author: Inspired by acer-predator-turbo-and-rgb-keyboard-linux-module
License: GPL-3.0
"""

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Optional, Tuple, Dict, Any

# Configuration
CONFIG_DIR = Path.home() / ".config" / "linuwu-sense" / "saved_profiles"
CONFIG_DIR.mkdir(parents=True, exist_ok=True)

# Sysfs paths - these will be detected automatically
PREDATOR_SENSE_PATH = "/sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/predator_sense"
NITRO_SENSE_PATH = "/sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/nitro_sense"
FOUR_ZONED_KB_PATH = "/sys/module/linuwu_sense/drivers/platform:acer-wmi/acer-wmi/four_zoned_kb"

class LinuwuSenseController:
    """Main controller class for Linuwu-Sense module."""
    
    def __init__(self):
        self.sense_path = self._detect_sense_path()
        self.four_zone_path = FOUR_ZONED_KB_PATH if os.path.exists(FOUR_ZONED_KB_PATH) else None
        
    def _detect_sense_path(self) -> Optional[str]:
        """Detect whether this is a Predator or Nitro laptop."""
        if os.path.exists(PREDATOR_SENSE_PATH):
            return PREDATOR_SENSE_PATH
        elif os.path.exists(NITRO_SENSE_PATH):
            return NITRO_SENSE_PATH
        return None
    
    def _read_sysfs(self, attribute: str) -> Optional[str]:
        """Read from sysfs attribute."""
        if not self.sense_path:
            return None
        try:
            with open(os.path.join(self.sense_path, attribute), 'r') as f:
                return f.read().strip()
        except (FileNotFoundError, PermissionError, OSError):
            return None
    
    def _write_sysfs(self, attribute: str, value: str) -> bool:
        """Write to sysfs attribute."""
        if not self.sense_path:
            return False
        try:
            with open(os.path.join(self.sense_path, attribute), 'w') as f:
                f.write(value)
            return True
        except (FileNotFoundError, PermissionError, OSError):
            return False
    
    def _read_four_zone_sysfs(self, attribute: str) -> Optional[str]:
        """Read from four-zone keyboard sysfs attribute."""
        if not self.four_zone_path:
            return None
        try:
            with open(os.path.join(self.four_zone_path, attribute), 'r') as f:
                return f.read().strip()
        except (FileNotFoundError, PermissionError, OSError):
            return None
    
    def _write_four_zone_sysfs(self, attribute: str, value: str) -> bool:
        """Write to four-zone keyboard sysfs attribute."""
        if not self.four_zone_path:
            return False
        try:
            with open(os.path.join(self.four_zone_path, attribute), 'w') as f:
                f.write(value)
            return True
        except (FileNotFoundError, PermissionError, OSError):
            return False
    
    def get_status(self) -> Dict[str, Any]:
        """Get current status of all available controls."""
        status = {}
        
        if not self.sense_path:
            status['error'] = "Linuwu-Sense module not detected or not loaded"
            return status
        
        # Thermal and fan controls
        fan_speed = self._read_sysfs('fan_speed')
        if fan_speed:
            status['fan_speed'] = fan_speed
        
        # Battery controls
        battery_limiter = self._read_sysfs('battery_limiter')
        if battery_limiter is not None:
            status['battery_limiter'] = battery_limiter
        
        battery_calibration = self._read_sysfs('battery_calibration')
        if battery_calibration is not None:
            status['battery_calibration'] = battery_calibration
        
        # Miscellaneous controls
        backlight_timeout = self._read_sysfs('backlight_timeout')
        if backlight_timeout is not None:
            status['backlight_timeout'] = backlight_timeout
        
        boot_animation_sound = self._read_sysfs('boot_animation_sound')
        if boot_animation_sound is not None:
            status['boot_animation_sound'] = boot_animation_sound
        
        lcd_override = self._read_sysfs('lcd_override')
        if lcd_override is not None:
            status['lcd_override'] = lcd_override
        
        usb_charging = self._read_sysfs('usb_charging')
        if usb_charging is not None:
            status['usb_charging'] = usb_charging
        
        # RGB keyboard controls
        if self.four_zone_path:
            four_zone_mode = self._read_four_zone_sysfs('four_zone_mode')
            if four_zone_mode:
                status['four_zone_mode'] = four_zone_mode
            
            per_zone_mode = self._read_four_zone_sysfs('per_zone_mode')
            if per_zone_mode:
                status['per_zone_mode'] = per_zone_mode
        
        return status
    
    def set_fan_speed(self, cpu_speed: int, gpu_speed: int) -> bool:
        """Set CPU and GPU fan speeds (0-100, 0 = auto)."""
        if not (0 <= cpu_speed <= 100 and 0 <= gpu_speed <= 100):
            print("Error: Fan speeds must be between 0 and 100")
            return False
        
        return self._write_sysfs('fan_speed', f"{cpu_speed},{gpu_speed}")
    
    def set_battery_limiter(self, enabled: bool) -> bool:
        """Enable or disable battery charging limiter (80% limit)."""
        return self._write_sysfs('battery_limiter', '1' if enabled else '0')
    
    def set_battery_calibration(self, start: bool) -> bool:
        """Start or stop battery calibration."""
        return self._write_sysfs('battery_calibration', '1' if start else '0')
    
    def set_backlight_timeout(self, enabled: bool) -> bool:
        """Enable or disable keyboard backlight timeout (30 seconds)."""
        return self._write_sysfs('backlight_timeout', '1' if enabled else '0')
    
    def set_boot_animation_sound(self, enabled: bool) -> bool:
        """Enable or disable custom boot animation and sound."""
        return self._write_sysfs('boot_animation_sound', '1' if enabled else '0')
    
    def set_lcd_override(self, enabled: bool) -> bool:
        """Enable or disable LCD override (reduces latency and ghosting)."""
        return self._write_sysfs('lcd_override', '1' if enabled else '0')
    
    def set_usb_charging(self, level: int) -> bool:
        """Set USB charging level (0=disabled, 10/20/30=power until battery reaches that %)."""
        if level not in [0, 10, 20, 30]:
            print("Error: USB charging level must be 0, 10, 20, or 30")
            return False
        
        return self._write_sysfs('usb_charging', str(level))
    
    def set_four_zone_mode(self, mode: int, speed: int, brightness: int, 
                          direction: int, red: int, green: int, blue: int) -> bool:
        """Set four-zone RGB keyboard mode.
        
        Modes:
        0: Static - Fixed color, no animation
        1: Breathing - Color fades in and out
        2: Neon - Neon glow effect (black color)
        3: Wave - Wave-like effect across keyboard
        4: Shifting - Shifting light effect
        5: Zoom - Zoom effect
        6: Meteor - Meteor-like effect
        7: Twinkling - Twinkling light effect
        """
        if not (0 <= mode <= 7):
            print("Error: Mode must be between 0 and 7")
            return False
        if not (0 <= speed <= 9):
            print("Error: Speed must be between 0 and 9")
            return False
        if not (0 <= brightness <= 100):
            print("Error: Brightness must be between 0 and 100")
            return False
        # Direction validation - some modes ignore direction, so 0 is valid
        if not (0 <= direction <= 2):
            print("Error: Direction must be 0 (ignored), 1 (right to left), or 2 (left to right)")
            return False
        if not all(0 <= c <= 255 for c in [red, green, blue]):
            print("Error: RGB values must be between 0 and 255")
            return False
        
        return self._write_four_zone_sysfs('four_zone_mode', 
                                         f"{mode},{speed},{brightness},{direction},{red},{green},{blue}")
    
    def set_per_zone_mode(self, zone1_color: str, zone2_color: str, 
                         zone3_color: str, zone4_color: str, brightness: int) -> bool:
        """Set individual colors for each of the four keyboard zones.
        
        Colors should be in hex format (e.g., '4287f5' for RGB values).
        """
        if not (0 <= brightness <= 100):
            print("Error: Brightness must be between 0 and 100")
            return False
        
        # Validate hex colors
        for i, color in enumerate([zone1_color, zone2_color, zone3_color, zone4_color], 1):
            if len(color) != 6 or not all(c in '0123456789abcdefABCDEF' for c in color):
                print(f"Error: Zone {i} color must be a 6-digit hex value (e.g., '4287f5')")
                return False
        
        return self._write_four_zone_sysfs('per_zone_mode', 
                                         f"{zone1_color},{zone2_color},{zone3_color},{zone4_color},{brightness}")
    
    def save_profile(self, name: str) -> bool:
        """Save current settings as a profile."""
        try:
            current_status = self.get_status()
            if 'error' in current_status:
                print(f"Error: {current_status['error']}")
                return False
            
            profile_path = CONFIG_DIR / f"{name}.json"
            with open(profile_path, 'w') as f:
                json.dump(current_status, f, indent=2)
            print(f"Profile '{name}' saved successfully")
            return True
        except Exception as e:
            print(f"Error saving profile: {e}")
            return False
    
    def load_profile(self, name: str) -> bool:
        """Load settings from a profile."""
        try:
            profile_path = CONFIG_DIR / f"{name}.json"
            if not profile_path.exists():
                print(f"Profile '{name}' not found")
                return False
            
            with open(profile_path, 'r') as f:
                profile_data = json.load(f)
            
            # Apply settings from profile
            success = True
            
            if 'fan_speed' in profile_data:
                try:
                    cpu, gpu = map(int, profile_data['fan_speed'].split(','))
                    if not self.set_fan_speed(cpu, gpu):
                        success = False
                except ValueError:
                    print("Error: Invalid fan speed format in profile")
                    success = False
            
            if 'battery_limiter' in profile_data:
                if not self.set_battery_limiter(profile_data['battery_limiter'] == '1'):
                    success = False
            
            if 'backlight_timeout' in profile_data:
                if not self.set_backlight_timeout(profile_data['backlight_timeout'] == '1'):
                    success = False
            
            if 'boot_animation_sound' in profile_data:
                if not self.set_boot_animation_sound(profile_data['boot_animation_sound'] == '1'):
                    success = False
            
            if 'lcd_override' in profile_data:
                if not self.set_lcd_override(profile_data['lcd_override'] == '1'):
                    success = False
            
            if 'usb_charging' in profile_data:
                if not self.set_usb_charging(int(profile_data['usb_charging'])):
                    success = False
            
            if 'four_zone_mode' in profile_data and self.four_zone_path:
                try:
                    parts = profile_data['four_zone_mode'].split(',')
                    if len(parts) == 7:
                        mode, speed, brightness, direction, red, green, blue = map(int, parts)
                        if not self.set_four_zone_mode(mode, speed, brightness, direction, red, green, blue):
                            success = False
                except ValueError:
                    print("Error: Invalid four-zone mode format in profile")
                    success = False
            
            if 'per_zone_mode' in profile_data and self.four_zone_path:
                try:
                    parts = profile_data['per_zone_mode'].split(',')
                    if len(parts) == 5:
                        zone1, zone2, zone3, zone4, brightness = parts
                        if not self.set_per_zone_mode(zone1, zone2, zone3, zone4, int(brightness)):
                            success = False
                except ValueError:
                    print("Error: Invalid per-zone mode format in profile")
                    success = False
            
            if success:
                print(f"Profile '{name}' loaded successfully")
            else:
                print(f"Profile '{name}' loaded with some errors")
            
            return success
            
        except Exception as e:
            print(f"Error loading profile: {e}")
            return False
    
    def list_profiles(self) -> None:
        """List all saved profiles."""
        profiles = list(CONFIG_DIR.glob("*.json"))
        if not profiles:
            print("No saved profiles found")
            return
        
        print("Saved profiles:")
        for profile in sorted(profiles):
            print(f"  {profile.stem}")


def main():
    """Main function with argument parsing."""
    parser = argparse.ArgumentParser(
        description="Linuwu-Sense Control Script - Control Acer Predator/Nitro laptop features",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Show current status
  linuwu-sense --status

  # Set fan speeds (CPU=50%, GPU=70%)
  linuwu-sense --fan-speed 50 70

  # Enable battery limiter
  linuwu-sense --battery-limiter

  # Set four-zone RGB to breathing mode (purple, speed=4, brightness=100)
  linuwu-sense --four-zone-mode 1 4 100 1 255 0 255

  # Set per-zone colors (all zones blue, brightness=100)
  linuwu-sense --per-zone-mode 4287f5 4287f5 4287f5 4287f5 100

  # Save current settings as 'gaming'
  linuwu-sense --save-profile gaming

  # Load 'gaming' profile
  linuwu-sense --load-profile gaming

  # List all saved profiles
  linuwu-sense --list-profiles
        """
    )
    
    # Status and info
    parser.add_argument('--status', action='store_true', 
                       help='Show current status of all controls')
    parser.add_argument('--list-profiles', action='store_true',
                       help='List all saved profiles')
    
    # Fan control
    parser.add_argument('--fan-speed', nargs=2, type=int, metavar='CPU GPU',
                       help='Set CPU and GPU fan speeds (0-100, 0=auto)')
    
    # Battery control
    parser.add_argument('--battery-limiter', action='store_true',
                       help='Enable battery charging limiter (80%% limit)')
    parser.add_argument('--no-battery-limiter', action='store_true',
                       help='Disable battery charging limiter')
    parser.add_argument('--battery-calibration', action='store_true',
                       help='Start battery calibration')
    parser.add_argument('--stop-battery-calibration', action='store_true',
                       help='Stop battery calibration')
    
    # RGB keyboard control
    parser.add_argument('--four-zone-mode', nargs=7, type=int, 
                       metavar='MODE SPEED BRIGHTNESS DIRECTION RED GREEN BLUE',
                       help='Set four-zone RGB mode (0-7, 0-9, 0-100, 0-2, 0-255, 0-255, 0-255)')
    parser.add_argument('--per-zone-mode', nargs=5, 
                       metavar='ZONE1 ZONE2 ZONE3 ZONE4 BRIGHTNESS',
                       help='Set per-zone colors (hex colors, 0-100 brightness)')
    
    # Miscellaneous controls
    parser.add_argument('--backlight-timeout', action='store_true',
                       help='Enable keyboard backlight timeout (30 seconds)')
    parser.add_argument('--no-backlight-timeout', action='store_true',
                       help='Disable keyboard backlight timeout')
    parser.add_argument('--boot-animation', action='store_true',
                       help='Enable custom boot animation and sound')
    parser.add_argument('--no-boot-animation', action='store_true',
                       help='Disable custom boot animation and sound')
    parser.add_argument('--lcd-override', action='store_true',
                       help='Enable LCD override (reduces latency)')
    parser.add_argument('--no-lcd-override', action='store_true',
                       help='Disable LCD override')
    parser.add_argument('--usb-charging', type=int, choices=[0, 10, 20, 30],
                       metavar='LEVEL',
                       help='Set USB charging level (0=disabled, 10/20/30=power until battery reaches that %%)')
    
    # Profile management
    parser.add_argument('--save-profile', type=str, metavar='NAME',
                       help='Save current settings as a profile')
    parser.add_argument('--load-profile', type=str, metavar='NAME',
                       help='Load settings from a profile')
    
    args = parser.parse_args()
    
    # Create controller
    controller = LinuwuSenseController()
    
    # Handle status request
    if args.status:
        status = controller.get_status()
        if 'error' in status:
            print(f"Error: {status['error']}")
            sys.exit(1)
        
        print("Linuwu-Sense Status:")
        print("=" * 50)
        
        if 'fan_speed' in status:
            print(f"Fan Speed: {status['fan_speed']}")
        
        if 'battery_limiter' in status:
            print(f"Battery Limiter: {'Enabled' if status['battery_limiter'] == '1' else 'Disabled'}")
        
        if 'battery_calibration' in status:
            print(f"Battery Calibration: {'Active' if status['battery_calibration'] == '1' else 'Inactive'}")
        
        if 'backlight_timeout' in status:
            print(f"Backlight Timeout: {'Enabled' if status['backlight_timeout'] == '1' else 'Disabled'}")
        
        if 'boot_animation_sound' in status:
            print(f"Boot Animation: {'Enabled' if status['boot_animation_sound'] == '1' else 'Disabled'}")
        
        if 'lcd_override' in status:
            print(f"LCD Override: {'Enabled' if status['lcd_override'] == '1' else 'Disabled'}")
        
        if 'usb_charging' in status:
            print(f"USB Charging: {status['usb_charging']}%")
        
        if 'four_zone_mode' in status:
            print(f"Four-Zone Mode: {status['four_zone_mode']}")
        
        if 'per_zone_mode' in status:
            print(f"Per-Zone Mode: {status['per_zone_mode']}")
        
        return
    
    # Handle profile listing
    if args.list_profiles:
        controller.list_profiles()
        return
    
    # Handle profile operations
    if args.save_profile:
        controller.save_profile(args.save_profile)
        return
    
    if args.load_profile:
        controller.load_profile(args.load_profile)
        return
    
    # Handle fan speed
    if args.fan_speed:
        cpu, gpu = args.fan_speed
        if controller.set_fan_speed(cpu, gpu):
            print(f"Fan speeds set: CPU={cpu}%, GPU={gpu}%")
        else:
            print("Failed to set fan speeds")
            sys.exit(1)
    
    # Handle battery controls
    if args.battery_limiter:
        if controller.set_battery_limiter(True):
            print("Battery limiter enabled")
        else:
            print("Failed to enable battery limiter")
            sys.exit(1)
    
    if args.no_battery_limiter:
        if controller.set_battery_limiter(False):
            print("Battery limiter disabled")
        else:
            print("Failed to disable battery limiter")
            sys.exit(1)
    
    if args.battery_calibration:
        if controller.set_battery_calibration(True):
            print("Battery calibration started")
        else:
            print("Failed to start battery calibration")
            sys.exit(1)
    
    if args.stop_battery_calibration:
        if controller.set_battery_calibration(False):
            print("Battery calibration stopped")
        else:
            print("Failed to stop battery calibration")
            sys.exit(1)
    
    # Handle RGB controls
    if args.four_zone_mode:
        mode, speed, brightness, direction, red, green, blue = args.four_zone_mode
        if controller.set_four_zone_mode(mode, speed, brightness, direction, red, green, blue):
            print(f"Four-zone mode set: mode={mode}, speed={speed}, brightness={brightness}%")
        else:
            print("Failed to set four-zone mode")
            sys.exit(1)
    
    if args.per_zone_mode:
        zone1, zone2, zone3, zone4, brightness = args.per_zone_mode
        if controller.set_per_zone_mode(zone1, zone2, zone3, zone4, int(brightness)):
            print(f"Per-zone mode set: zones={zone1},{zone2},{zone3},{zone4}, brightness={brightness}%")
        else:
            print("Failed to set per-zone mode")
            sys.exit(1)
    
    # Handle miscellaneous controls
    if args.backlight_timeout:
        if controller.set_backlight_timeout(True):
            print("Backlight timeout enabled")
        else:
            print("Failed to enable backlight timeout")
            sys.exit(1)
    
    if args.no_backlight_timeout:
        if controller.set_backlight_timeout(False):
            print("Backlight timeout disabled")
        else:
            print("Failed to disable backlight timeout")
            sys.exit(1)
    
    if args.boot_animation:
        if controller.set_boot_animation_sound(True):
            print("Boot animation enabled")
        else:
            print("Failed to enable boot animation")
            sys.exit(1)
    
    if args.no_boot_animation:
        if controller.set_boot_animation_sound(False):
            print("Boot animation disabled")
        else:
            print("Failed to disable boot animation")
            sys.exit(1)
    
    if args.lcd_override:
        if controller.set_lcd_override(True):
            print("LCD override enabled")
        else:
            print("Failed to enable LCD override")
            sys.exit(1)
    
    if args.no_lcd_override:
        if controller.set_lcd_override(False):
            print("LCD override disabled")
        else:
            print("Failed to disable LCD override")
            sys.exit(1)
    
    if args.usb_charging is not None:
        if controller.set_usb_charging(args.usb_charging):
            if args.usb_charging == 0:
                print("USB charging disabled")
            else:
                print(f"USB charging set to {args.usb_charging}%")
        else:
            print("Failed to set USB charging")
            sys.exit(1)
    
    # If no arguments provided, show help
    if len(sys.argv) == 1:
        parser.print_help()


if __name__ == "__main__":
    main()
