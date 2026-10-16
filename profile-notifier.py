#!/usr/bin/env python3
"""
Performance Profile Notifier for Linuwu-Sense
Universal - works on GNOME, KDE, XFCE, and other desktop environments
"""

import subprocess
import time
import os

# Profile display names
PROFILES = {
    'quiet': ('🔇 Quiet Mode', 'Minimal fan noise, lower performance'),
    'balanced': ('⚖️ Balanced Mode', 'Balanced performance and efficiency'),
    'balanced-performance': ('⚡ Performance Mode', 'High performance'),
    'performance': ('🚀 Turbo Mode', 'Maximum performance'),
    'low-power': ('🌿 Eco Mode', 'Power saving (battery only)')
}

def detect_desktop_environment():
    """Detect which desktop environment is running"""
    desktop = os.environ.get('XDG_CURRENT_DESKTOP', '').lower()
    
    if 'kde' in desktop or 'plasma' in desktop:
        return 'kde'
    elif 'gnome' in desktop:
        return 'gnome'
    elif 'xfce' in desktop:
        return 'xfce'
    else:
        return 'generic'

def command_exists(cmd):
    """Check if a command exists in PATH"""
    return subprocess.call(['which', cmd], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) == 0

def get_current_profile():
    """Read current platform profile"""
    try:
        with open('/sys/firmware/acpi/platform_profile', 'r') as f:
            return f.read().strip()
    except:
        return None

def show_notification_kde(title, message):
    """KDE/Plasma notification using kdialog"""
    if command_exists('kdialog'):
        subprocess.Popen([
            'kdialog',
            '--title', 'Performance Profile',
            '--passivepopup', f'{title}\n{message}',
            '3'  # 3 seconds
        ])
    else:
        # Fallback to notify-send
        show_notification_generic(title, message)

def show_notification_gnome(title, message):
    """GNOME notification using zenity"""
    if command_exists('zenity'):
        subprocess.Popen([
            'zenity',
            '--notification',
            '--text', f'{title}\n{message}',
            '--timeout', '3'
        ])
    else:
        # Fallback to notify-send
        show_notification_generic(title, message)

def show_notification_generic(title, message):
    """Generic notification using notify-send (works on most DEs)"""
    subprocess.Popen([
        'notify-send',
        '-u', 'normal',
        '-t', '3000',
        '-a', 'Linuwu-Sense',
        title,
        message
    ])

def show_notification(profile):
    """Show notification using appropriate method for desktop environment"""
    if profile not in PROFILES:
        return
    
    title, message = PROFILES[profile]
    desktop = detect_desktop_environment()
    
    if desktop == 'kde':
        show_notification_kde(title, message)
    elif desktop == 'gnome':
        show_notification_gnome(title, message)
    else:
        # XFCE and others use notify-send
        show_notification_generic(title, message)

def main():
    """Monitor profile changes and show notifications"""
    desktop = detect_desktop_environment()
    print(f"Performance Profile Notifier started")
    print(f"Detected desktop environment: {desktop.upper()}")
    print(f"Monitoring thermal profile changes...")
    
    last_profile = get_current_profile()
    
    while True:
        time.sleep(0.5)
        
        current_profile = get_current_profile()
        
        if current_profile and current_profile != last_profile:
            print(f"Profile changed: {last_profile} -> {current_profile}")
            show_notification(current_profile)
            last_profile = current_profile

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\nNotifier stopped")
