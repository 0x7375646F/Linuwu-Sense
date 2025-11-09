# Acer Sense for Windows 11

A modern Windows 11 application for controlling Acer Predator and Nitro gaming laptops. Control your laptop's performance, RGB keyboard, fan speeds, battery health, and more through an intuitive GUI.

## Features

### Performance Control
- **Performance Profiles**: Switch between Quiet, Balanced, Performance, and Turbo modes
- **Turbo/Overclock Mode**: Enable CPU overclocking for maximum performance
- **Real-time Monitoring**: Monitor system performance in real-time

### Fan Management
- **CPU & GPU Fan Control**: Manually adjust fan speeds (0-100%)
- **Real-time RPM Display**: Monitor fan speeds in real-time
- **Automatic Fan Profiles**: Fans adjust automatically based on performance profile

### RGB Keyboard Control (4-Zone)
- **Per-Zone Customization**: Control each of the 4 keyboard zones independently
- **Quick Presets**: Rainbow, Red, Green, Blue, White, and Off presets
- **Full RGB Color Picker**: Choose any color (0-255 for R, G, B)
- **Instant Apply**: Changes apply immediately to your keyboard

### Battery Management
- **Battery Health Mode**: Limit charging to extend battery lifespan (recommended 80%)
- **Adjustable Charge Limit**: Set charge limit from 60% to 100%
- **Battery Calibration**: Recalibrate battery gauge for accurate readings
- **Health Protection**: Protect your battery when laptop stays plugged in

### Additional Features
- **Device Auto-Detection**: Automatically detects your Acer laptop model
- **Modern Windows 11 UI**: Clean, modern interface using WPF with ModernWPF
- **Administrator Privileges**: Proper elevation for hardware access
- **Real-time Updates**: Live monitoring of hardware status

## Supported Devices

### Predator Series
- Predator PH315-53
- Predator PHN16-71 (4-zone RGB)
- Predator PHN16-72 (4-zone RGB)
- Predator PH16-71
- Predator PH18-71

### Nitro Series
- Nitro AN16-43 (4-zone RGB)
- Nitro AN16-41 (4-zone RGB)
- Nitro ANV16-41
- Nitro ANV15-41
- Nitro ANV15-51
- Nitro AN515-58 (4-zone RGB)
- Nitro AN515-55

**Note**: Other Predator and Nitro models may work but are untested. The application will attempt to detect generic Predator/Nitro laptops and enable appropriate features.

## Requirements

- **Operating System**: Windows 10 (1809+) or Windows 11
- **Architecture**: x64 (64-bit)
- **Framework**: .NET 8.0 Runtime
- **Privileges**: Administrator rights (required for WMI/ACPI access)
- **Hardware**: Supported Acer Predator or Nitro laptop

## Installation

### Option 1: Build from Source

1. **Install Prerequisites**:
   - [Visual Studio 2022](https://visualstudio.microsoft.com/) or [.NET 8.0 SDK](https://dotnet.microsoft.com/download/dotnet/8.0)
   - Git

2. **Clone the Repository**:
   ```powershell
   git clone https://github.com/yourusername/Linuwu-Sense-win.git
   cd Linuwu-Sense-win/Windows11
   ```

3. **Build the Solution**:
   ```powershell
   dotnet restore
   dotnet build --configuration Release
   ```

4. **Run the Application**:
   ```powershell
   cd AcerSense.UI/bin/Release/net8.0-windows
   # Right-click AcerSense.UI.exe and select "Run as Administrator"
   ```

### Option 2: Pre-built Binary (Coming Soon)

Download the latest release from the [Releases](https://github.com/yourusername/Linuwu-Sense-win/releases) page.

## Usage

### First Launch

1. **Run as Administrator**: Right-click `AcerSense.UI.exe` and select "Run as Administrator"
2. The application will detect your laptop model automatically
3. Available features will be enabled based on your laptop model

### Performance Tab

- **Select Performance Profile**: Choose from Quiet, Balanced, Performance, or Turbo
- **Enable Turbo Mode**: Toggle overclock mode for maximum CPU performance
- **Adjust Fan Speeds**: Use sliders to manually control CPU and GPU fan speeds
- **Monitor Fan RPM**: Real-time display of fan speeds in RPM

### RGB Keyboard Tab

- **Quick Presets**: Click preset buttons for instant color changes
- **Per-Zone Control**: Adjust RGB values for each of the 4 keyboard zones
- **Apply Changes**: Click "Apply to Zone X" to apply colors to specific zones

### Battery Tab

- **Enable Health Mode**: Toggle battery health mode on/off
- **Set Charge Limit**: Use slider to set maximum charge percentage (60-100%)
- **Start Calibration**: Run battery calibration (takes several hours)

### About Tab

- View detected device information
- See list of available features
- Check application version

## How It Works

### WMI/ACPI Communication

The application uses Windows Management Instrumentation (WMI) to communicate with your laptop's ACPI methods. These are the same methods used by Acer's official software, ported from the Linux kernel module implementation.

**Key Components**:
- **AcerWmiInterface**: Low-level WMI communication layer
- **DeviceDetector**: Identifies laptop model using SMBIOS
- **AcerControlService**: High-level API for all features
- **WPF UI**: Modern Windows 11 user interface

### Architecture

```
┌─────────────────────────┐
│    WPF User Interface   │
│     (MainWindow.xaml)   │
└───────────┬─────────────┘
            │
            ▼
┌─────────────────────────┐
│  AcerControlService     │
│  (High-level API)       │
└───────────┬─────────────┘
            │
            ▼
┌─────────────────────────┐
│   AcerWmiInterface      │
│   (WMI Communication)   │
└───────────┬─────────────┘
            │
            ▼
┌─────────────────────────┐
│   Windows WMI/ACPI      │
│   (Hardware Layer)      │
└─────────────────────────┘
```

## Troubleshooting

### Application Won't Start

- **Ensure you're running as Administrator**: Right-click → "Run as Administrator"
- **Install .NET 8.0 Runtime**: Download from [Microsoft](https://dotnet.microsoft.com/download/dotnet/8.0)
- **Check WMI Service**: Open Services (`services.msc`) and ensure "Windows Management Instrumentation" is running

### Features Not Working

- **Verify Device Support**: Check if your laptop model is in the supported list
- **Update BIOS**: Ensure you have the latest BIOS from Acer's support site
- **Check WMI Availability**: Some features require specific WMI methods exposed by Acer BIOS

### Fan Control Not Responding

- Fan control requires Predator v4 or Nitro v4 laptops
- Some older Nitro models only support fan speed reading, not control
- Try switching performance profiles first

### RGB Keyboard Not Changing

- Ensure your laptop has a 4-zone RGB keyboard
- Some Nitro models don't have 4-zone RGB support
- Check the "About" tab to see if "4-Zone RGB Keyboard" is listed

### Battery Health Mode Not Saving

- Some laptops require AC power to change battery settings
- Ensure you're plugged in when changing battery settings
- Try restarting the application with Administrator privileges

## Known Limitations

### Windows WMI vs Linux ACPI

This is a port of the Linux kernel module, but Windows WMI may not expose all the same ACPI methods. Some features from the Linux version may not be available:

- **Linux-only features**:
  - Direct kernel-level hardware access
  - Some advanced WMI methods may not be available in Windows
  - Boot animation sound control (may not work on all models)

### Hardware Differences

- **WMI Method Availability**: Some Acer laptops don't expose all WMI methods on Windows
- **BIOS Versions**: Older BIOS versions may not support all features
- **Model Variations**: Features vary by exact laptop model and configuration

### Testing Status

This is a **complete rewrite** for Windows based on the Linux kernel module. While the core WMI method IDs and logic are ported directly from the working Linux version, **testing on real hardware is required** to verify full functionality.

**If you have a supported Acer laptop and can test this application, please report your findings!**

## Development

### Project Structure

```
Windows11/
├── AcerSense.sln                    # Visual Studio solution
├── AcerSense.Core/                  # Core library
│   ├── Constants/
│   │   └── AcerWmiConstants.cs      # WMI GUIDs and method IDs
│   ├── Detection/
│   │   └── DeviceDetector.cs        # Laptop model detection
│   ├── Models/
│   │   └── LaptopQuirk.cs          # Device quirks and enums
│   ├── Services/
│   │   └── AcerControlService.cs    # High-level API
│   └── WMI/
│       └── AcerWmiInterface.cs      # Low-level WMI layer
└── AcerSense.UI/                    # WPF application
    ├── App.xaml                     # Application resources
    ├── MainWindow.xaml              # Main UI
    ├── MainWindow.xaml.cs           # UI code-behind
    └── app.manifest                 # Admin privileges manifest
```

### Adding New Features

1. **Add WMI Constants**: Update `AcerWmiConstants.cs` with new method IDs
2. **Add WMI Method**: Implement low-level call in `AcerWmiInterface.cs`
3. **Add Service Method**: Add high-level API in `AcerControlService.cs`
4. **Update UI**: Add controls to `MainWindow.xaml` and handlers to `MainWindow.xaml.cs`

### Contributing

Contributions are welcome! Please:
1. Test on real hardware if possible
2. Document which laptop model you tested on
3. Report any WMI method differences from the Linux implementation
4. Submit pull requests with clear descriptions

## Credits

### Based On
- **Linuwu-Sense**: Original Linux kernel module
- **acer-wmi**: Linux kernel driver for Acer laptops
- **Contributors**: All contributors to the original Linux projects

### Port Author
- Windows 11 port created using AI-assisted development
- WMI implementation based on Linux ACPI method analysis

### Libraries Used
- **.NET 8.0**: Microsoft's cross-platform framework
- **ModernWPF**: Modern UI library for WPF
- **System.Management**: WMI access library

## License

This project is licensed under the **GNU General Public License v3.0** - see the [LICENSE](../LICENSE) file for details.

This is a derivative work of the Linuwu-Sense Linux kernel module, which is also GPL-3.0 licensed.

## Disclaimer

⚠️ **USE AT YOUR OWN RISK**

This software directly controls laptop hardware. Improper use may:
- Cause system instability
- Affect hardware lifespan
- Void warranties
- Cause overheating if fans are disabled

**The authors are not responsible for any damage caused by using this software.**

**This is an unofficial, community-created tool and is not endorsed by Acer Inc.**

## Support

For issues, questions, or feature requests:
- Open an issue on [GitHub Issues](https://github.com/yourusername/Linuwu-Sense-win/issues)
- Check existing issues for solutions
- Provide laptop model, Windows version, and detailed error messages

## FAQ

### Q: Is this official Acer software?
**A**: No, this is an unofficial community project. It is not affiliated with or endorsed by Acer Inc.

### Q: Will this void my warranty?
**A**: Possibly. Using third-party hardware control software may void warranties. Check your warranty terms.

### Q: Why does it need Administrator privileges?
**A**: WMI/ACPI access requires elevated privileges to communicate with hardware.

### Q: Can I run this alongside Acer's official software?
**A**: Not recommended. Both applications control the same hardware and may conflict.

### Q: Does this work on non-Acer laptops?
**A**: No, this is specifically for Acer Predator and Nitro series laptops.

### Q: Why are some features grayed out?
**A**: Features are enabled based on your laptop model. Not all features are available on all models.

### Q: How is this different from the Linux version?
**A**: This is a complete port to Windows using WMI instead of kernel-level ACPI access. The functionality should be similar, but some features may differ due to Windows limitations.

## Roadmap

- [ ] Test on real hardware
- [ ] Add system tray integration
- [ ] Add auto-start on Windows boot
- [ ] Create installer package
- [ ] Add temperature monitoring
- [ ] Add more RGB effects
- [ ] Add profile saving/loading
- [ ] Add logging for debugging
- [ ] Support for more Acer models

## Version History

### Version 1.0.0 (Initial Release)
- Complete port from Linux kernel module
- WMI/ACPI interface implementation
- Modern Windows 11 UI
- Performance profile control
- Fan speed control
- RGB keyboard control (4-zone)
- Battery health management
- Device auto-detection
- Real-time monitoring
