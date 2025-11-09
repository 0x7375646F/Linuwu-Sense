# Getting Started with Acer Sense for Windows 11

This guide will help you get up and running with Acer Sense on your Windows 11 laptop.

## Quick Start

### 1. Check System Requirements

Before you begin, ensure:
- ✅ You have a supported Acer Predator or Nitro laptop
- ✅ You're running Windows 10 (1809+) or Windows 11
- ✅ You have Administrator privileges
- ✅ .NET 8.0 Runtime is installed (or will install it in next steps)

### 2. Install .NET 8.0 Runtime

If you don't have .NET 8.0 installed:

1. Download from: https://dotnet.microsoft.com/download/dotnet/8.0
2. Choose "Download .NET Desktop Runtime 8.0.x (x64)"
3. Run the installer
4. Restart your computer

### 3. Build the Application

**Option A: Using Visual Studio 2022**

1. Open `AcerSense.sln` in Visual Studio
2. Select "Release" configuration
3. Press F5 or click "Build → Build Solution"
4. Navigate to `AcerSense.UI\bin\Release\net8.0-windows`
5. Right-click `AcerSense.UI.exe` → "Run as Administrator"

**Option B: Using PowerShell**

1. Open PowerShell as Administrator
2. Navigate to the Windows11 folder:
   ```powershell
   cd path\to\Linuwu-Sense-win\Windows11
   ```
3. Run the build script:
   ```powershell
   .\build.ps1
   ```
4. Navigate to output folder:
   ```powershell
   cd AcerSense.UI\bin\Release\net8.0-windows
   ```
5. Run as Administrator:
   ```powershell
   Start-Process .\AcerSense.UI.exe -Verb RunAs
   ```

**Option C: Using Command Line**

```cmd
cd Windows11
dotnet restore
dotnet build --configuration Release
cd AcerSense.UI\bin\Release\net8.0-windows
:: Right-click AcerSense.UI.exe and select "Run as Administrator"
```

### 4. First Run

1. The application will launch with Administrator privileges
2. It will automatically detect your laptop model
3. You'll see the main interface with multiple tabs

**What you should see**:
- Header showing your laptop model name
- Green status indicator (🟢) if connected successfully
- Multiple tabs: Performance, RGB Keyboard, Battery, About

**If you see a red status indicator (🔴)**:
- Your laptop may not be supported
- WMI service may not be running
- Try restarting the application as Administrator

### 5. Explore Features

**Performance Tab**:
- Try changing the Performance Profile
- Adjust fan speeds with the sliders
- Watch real-time RPM readings

**RGB Keyboard Tab** (if supported):
- Click "Rainbow" for a quick test
- Customize each zone individually
- Try different presets

**Battery Tab**:
- Enable Battery Health Mode
- Set charge limit to 80% for daily use
- Only use Calibration when needed

## Supported Features by Model

### Predator PHN16-71/72
✅ All features including:
- Performance profiles
- Fan control
- 4-Zone RGB keyboard
- Battery health
- Turbo mode
- LCD override

### Nitro AN16-43/41, AN515-58
✅ Most features including:
- Performance profiles
- Fan control
- 4-Zone RGB keyboard
- Battery health
- Turbo mode

### Nitro AN515-55, ANV15-41/51
✅ Basic features:
- Performance profiles
- Fan speed reading (no control)
- Battery health

## Usage Tips

### Performance Profiles

- **Quiet**: Lowest fan noise, reduced performance (good for office work)
- **Balanced**: Default setting, balances performance and noise
- **Performance**: Higher performance, louder fans (good for gaming)
- **Turbo**: Maximum performance, loudest fans (for competitive gaming)

### Fan Control

- **Auto Mode**: Let the performance profile control fans
- **Manual Mode**: Use sliders for precise control
- **Warning**: Don't set fans too low during heavy use!

### RGB Keyboard

- **Quick Presets**: Fastest way to change colors
- **Per-Zone**: Customize for a unique look
- **Save Power**: Turn off RGB when on battery

### Battery Health

- **Daily Use**: Enable health mode at 80% if always plugged in
- **Travel**: Disable health mode for full capacity
- **Calibration**: Only needed every 3-6 months

## Troubleshooting

### "Failed to initialize WMI"

**Solution**:
1. Ensure you're running as Administrator
2. Open Services (Win+R → `services.msc`)
3. Find "Windows Management Instrumentation"
4. Ensure it's running (Status: Running)
5. If stopped, right-click → Start

### "Unsupported device"

**Possible causes**:
- Non-Acer laptop
- Unsupported Acer model
- Missing WMI methods in BIOS

**Try**:
1. Check if your model is in the supported list (README.md)
2. Update your BIOS from Acer's support site
3. Check the About tab for detected model name

### Features grayed out

**This is normal**:
- Features are enabled based on your laptop model
- Not all laptops support all features
- Check the About tab to see available features

### Fan speeds show 0 RPM

**Possible causes**:
- Fans are actually at 0 RPM (normal when idle)
- WMI method not working
- Need to wait a few seconds for first reading

**Try**:
1. Click Refresh button
2. Adjust fan slider
3. Wait 5-10 seconds
4. Check if RPM updates

### RGB keyboard not changing

**Check**:
1. Does your laptop have RGB keyboard?
2. Is "4-Zone RGB Keyboard" listed in About tab?
3. Try clicking a Quick Preset first
4. Check if Acer's official software is running (close it)

## Advanced Usage

### Command Line Arguments (Future)

Coming soon:
```cmd
AcerSense.UI.exe --profile Performance
AcerSense.UI.exe --fan-cpu 75 --fan-gpu 75
AcerSense.UI.exe --rgb-rainbow
```

### Auto-Start on Boot (Manual Setup)

1. Create a shortcut to `AcerSense.UI.exe`
2. Right-click shortcut → Properties
3. Click "Advanced..." → Check "Run as administrator"
4. Move shortcut to:
   ```
   %APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup
   ```

**Note**: Windows will prompt for UAC on each startup

### Profile Saving (Future Feature)

Coming soon: Save and load custom profiles

## Getting Help

### Check Documentation
- Read the full README.md
- Check FAQ section
- Review troubleshooting guide

### Report Issues
1. Go to GitHub Issues
2. Search for existing similar issues
3. Create new issue with:
   - Laptop model
   - Windows version
   - Error message
   - Steps to reproduce

### Community Support
- Check existing GitHub discussions
- Share your experience with other users
- Help test new features

## Safety Reminders

⚠️ **Important**:
- Always monitor temperatures when adjusting fans
- Don't disable fans during heavy load
- Use turbo mode sparingly
- Battery calibration takes several hours
- Keep laptop ventilated

## What's Next?

After getting comfortable with the basics:
1. Experiment with custom RGB patterns
2. Find your optimal fan curve
3. Set up battery health mode if applicable
4. Try different performance profiles for different tasks
5. Report any issues or suggest features on GitHub

## Need More Help?

- 📖 Full documentation: [README.md](README.md)
- 🐛 Report bugs: [GitHub Issues](https://github.com/yourusername/Linuwu-Sense-win/issues)
- 💬 Discussions: [GitHub Discussions](https://github.com/yourusername/Linuwu-Sense-win/discussions)

---

**Enjoy controlling your Acer laptop on Windows 11!** 🎮
