# Performance Profile Notifier

A lightweight desktop notifier that shows a popup when you change thermal profiles using the mode switch button.

**Universal compatibility** - automatically detects and works on:
- 🐧 GNOME (Ubuntu, Fedora Workstation, etc.)
- 🔷 KDE Plasma (Fedora KDE, Kubuntu, etc.)
- 🖱️ XFCE (Xubuntu, etc.)
- 📦 Other desktop environments via notify-send

## Features

- 🔔 Desktop notifications when switching profiles
- 🎨 Clean, minimal popup (top-right corner)
- 🚀 Lightweight - uses minimal CPU
- ⚡ Instant response to profile changes

## Installation

1. Make sure you have `notify-send` installed:
```bash
# Ubuntu/Debian
sudo apt install libnotify-bin

# Arch Linux
sudo pacman -S libnotify
```

2. Make the script executable:
```bash
chmod +x profile-notifier.py
```

3. Run it:
```bash
./profile-notifier.py
```

## Auto-start on Boot

To run automatically when you login, create a desktop entry:

```bash
mkdir -p ~/.config/autostart
cat > ~/.config/autostart/profile-notifier.desktop << 'EOF'
[Desktop Entry]
Type=Application
Name=Performance Profile Notifier
Comment=Show notification when thermal profile changes
Exec=/home/YOUR_USERNAME/path/to/Linuwu-Sense/profile-notifier.py
Terminal=false
Hidden=false
X-GNOME-Autostart-enabled=true
EOF
```

**Remember to update the `Exec=` path to match your installation!**

## Profile Notifications

The notifier shows these messages when on **AC power**:

| Profile | Icon | Title | Description |
|---------|------|-------|-------------|
| Quiet | 🔇 | Quiet Mode | Minimal fan noise |
| Balanced | ⚖️ | Balanced Mode | Balanced performance |
| Performance | ⚡ | Performance Mode | High performance |
| Turbo | 🚀 | Turbo Mode | Maximum performance |

When on **battery power**:

| Profile | Icon | Title | Description |
|---------|------|-------|-------------|
| Eco | 🌿 | Eco Mode | Power saving (auto) |
| Balanced | ⚖️ | Balanced Mode | Normal operation |

## Customization

Edit `profile-notifier.py` to customize:

- **Notification duration**: Change `-t` value (milliseconds)
- **Icons**: Modify the icon names
- **Messages**: Edit the `PROFILES` dictionary

## Troubleshooting

**Notifications not showing?**
- Check if `notify-send` works: `notify-send "Test" "Message"`
- Make sure your desktop environment supports notifications
- Try restarting the script

**Script not auto-starting?**
- Verify the path in `~/.config/autostart/profile-notifier.desktop`
- Check if autostart is enabled in your desktop environment

## How It Works

1. Monitors `/sys/firmware/acpi/platform_profile` for changes
2. Detects when you press the mode switch button
3. Shows a desktop notification with the new profile
4. Notification appears in top-right corner (system tray area)

Perfect for knowing which mode you switched to without opening any apps!
