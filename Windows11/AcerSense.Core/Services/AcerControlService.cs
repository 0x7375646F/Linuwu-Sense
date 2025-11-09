using AcerSense.Core.Detection;
using AcerSense.Core.Models;
using AcerSense.Core.WMI;

namespace AcerSense.Core.Services;

/// <summary>
/// High-level service for controlling Acer laptop features
/// </summary>
public class AcerControlService : IDisposable
{
    private readonly AcerWmiInterface _wmi;
    private readonly LaptopQuirk _quirk;
    private bool _disposed;

    public LaptopQuirk DeviceQuirk => _quirk;
    public bool IsSupported => _quirk.IsPredatorV4 || _quirk.IsNitroV4 || _quirk.IsNitroSense;

    public AcerControlService()
    {
        _wmi = new AcerWmiInterface();
        _quirk = DeviceDetector.DetectDevice();
    }

    /// <summary>
    /// Initialize the service
    /// </summary>
    public bool Initialize()
    {
        if (!IsSupported)
        {
            Console.WriteLine($"Unsupported device: {_quirk.ModelName}");
            return false;
        }

        if (!_wmi.Initialize())
        {
            Console.WriteLine("Failed to initialize WMI interface");
            return false;
        }

        Console.WriteLine($"Initialized Acer Control for: {_quirk.ModelName}");
        return true;
    }

    #region Fan Control

    /// <summary>
    /// Get CPU fan speed in RPM
    /// </summary>
    public int GetCpuFanSpeed()
    {
        if (!_quirk.IsPredatorV4 && !_quirk.IsNitroV4)
            return 0;

        return _wmi.GetCpuFanSpeed();
    }

    /// <summary>
    /// Get GPU fan speed in RPM
    /// </summary>
    public int GetGpuFanSpeed()
    {
        if (!_quirk.IsPredatorV4 && !_quirk.IsNitroV4)
            return 0;

        return _wmi.GetGpuFanSpeed();
    }

    /// <summary>
    /// Set CPU fan speed (0-100%)
    /// </summary>
    public bool SetCpuFanSpeed(int percent)
    {
        if (!_quirk.IsPredatorV4 && !_quirk.IsNitroV4)
            return false;

        return _wmi.SetFanSpeed(true, percent);
    }

    /// <summary>
    /// Set GPU fan speed (0-100%)
    /// </summary>
    public bool SetGpuFanSpeed(int percent)
    {
        if (!_quirk.IsPredatorV4 && !_quirk.IsNitroV4)
            return false;

        return _wmi.SetFanSpeed(false, percent);
    }

    #endregion

    #region Performance Profiles

    /// <summary>
    /// Set performance profile
    /// </summary>
    public bool SetPerformanceProfile(PerformanceProfile profile)
    {
        if (!_quirk.IsPredatorV4 && !_quirk.IsNitroV4 && !_quirk.IsNitroSense)
            return false;

        return _wmi.SetPerformanceProfile((int)profile);
    }

    /// <summary>
    /// Get current performance profile
    /// </summary>
    public PerformanceProfile GetPerformanceProfile()
    {
        if (!_quirk.IsPredatorV4 && !_quirk.IsNitroV4 && !_quirk.IsNitroSense)
            return PerformanceProfile.Balanced;

        int profile = _wmi.GetPerformanceProfile();
        return (PerformanceProfile)Math.Clamp(profile, 0, 3);
    }

    #endregion

    #region RGB Keyboard

    /// <summary>
    /// Set RGB color for a keyboard zone
    /// </summary>
    public bool SetKeyboardZoneColor(KeyboardZone zone, byte red, byte green, byte blue)
    {
        if (!_quirk.HasFourZoneKeyboard)
            return false;

        return _wmi.SetKeyboardRgb((int)zone, red, green, blue);
    }

    /// <summary>
    /// Set all keyboard zones to the same color
    /// </summary>
    public bool SetKeyboardColor(byte red, byte green, byte blue)
    {
        if (!_quirk.HasFourZoneKeyboard)
            return false;

        bool success = true;
        for (int zone = 0; zone < 4; zone++)
        {
            success &= _wmi.SetKeyboardRgb(zone, red, green, blue);
        }
        return success;
    }

    /// <summary>
    /// Set rainbow effect (cycles through colors)
    /// </summary>
    public void SetKeyboardRainbow()
    {
        if (!_quirk.HasFourZoneKeyboard)
            return;

        // Zone 1: Red
        _wmi.SetKeyboardRgb(0, 255, 0, 0);
        // Zone 2: Green
        _wmi.SetKeyboardRgb(1, 0, 255, 0);
        // Zone 3: Blue
        _wmi.SetKeyboardRgb(2, 0, 0, 255);
        // Zone 4: Purple
        _wmi.SetKeyboardRgb(3, 255, 0, 255);
    }

    #endregion

    #region Battery Management

    /// <summary>
    /// Enable battery health mode (limits charge to extend battery life)
    /// </summary>
    public bool EnableBatteryHealthMode(int chargeLimit = 80)
    {
        if (!_quirk.IsPredatorV4 && !_quirk.IsNitroV4)
            return false;

        return _wmi.SetBatteryHealthMode((int)BatteryMode.HealthMode, chargeLimit);
    }

    /// <summary>
    /// Disable battery health mode (normal charging)
    /// </summary>
    public bool DisableBatteryHealthMode()
    {
        if (!_quirk.IsPredatorV4 && !_quirk.IsNitroV4)
            return false;

        return _wmi.SetBatteryHealthMode((int)BatteryMode.Normal, 100);
    }

    /// <summary>
    /// Start battery calibration
    /// </summary>
    public bool StartBatteryCalibration()
    {
        if (!_quirk.IsPredatorV4 && !_quirk.IsNitroV4)
            return false;

        return _wmi.SetBatteryHealthMode((int)BatteryMode.CalibrationMode, 100);
    }

    /// <summary>
    /// Get battery health status
    /// </summary>
    public (BatteryMode mode, int limit) GetBatteryHealthStatus()
    {
        if (!_quirk.IsPredatorV4 && !_quirk.IsNitroV4)
            return (BatteryMode.Normal, 100);

        var (mode, limit) = _wmi.GetBatteryHealthStatus();
        return ((BatteryMode)mode, limit);
    }

    #endregion

    #region Turbo/Overclock

    /// <summary>
    /// Enable turbo/overclock mode
    /// </summary>
    public bool EnableTurboMode()
    {
        if (!_quirk.Turbo && !_quirk.IsPredatorV4)
            return false;

        return _wmi.SetTurboMode(true);
    }

    /// <summary>
    /// Disable turbo/overclock mode
    /// </summary>
    public bool DisableTurboMode()
    {
        if (!_quirk.Turbo && !_quirk.IsPredatorV4)
            return false;

        return _wmi.SetTurboMode(false);
    }

    #endregion

    public void Dispose()
    {
        if (!_disposed)
        {
            _wmi?.Dispose();
            _disposed = true;
        }
        GC.SuppressFinalize(this);
    }
}
