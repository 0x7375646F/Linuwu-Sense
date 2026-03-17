#!/bin/bash
# Install profile notifier to auto-start on login

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AUTOSTART_DIR="$HOME/.config/autostart"
DESKTOP_FILE="$AUTOSTART_DIR/profile-notifier.desktop"

echo "Installing Performance Profile Notifier..."

# Create autostart directory if it doesn't exist
mkdir -p "$AUTOSTART_DIR"

# Create desktop entry
cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Type=Application
Name=Performance Profile Notifier
Comment=Show notification when thermal profile changes
Exec=$SCRIPT_DIR/profile-notifier.py
Terminal=false
Hidden=false
X-GNOME-Autostart-enabled=true
Icon=preferences-system
Categories=System;
EOF

echo "✅ Profile notifier installed to autostart!"
echo "Location: $DESKTOP_FILE"
echo ""
echo "It will start automatically on next login."
echo "To start it now, run: ./profile-notifier.py"
echo ""
echo "To uninstall, run: rm $DESKTOP_FILE"
