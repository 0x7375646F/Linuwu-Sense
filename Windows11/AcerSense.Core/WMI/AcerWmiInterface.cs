using System.Management;
using AcerSense.Core.Constants;

namespace AcerSense.Core.WMI;

/// <summary>
/// Low-level WMI interface for Acer laptops
/// Communicates with ACPI/WMI methods exposed by Acer BIOS
/// </summary>
public class AcerWmiInterface : IDisposable
{
    private ManagementObject? _wmiObject;
    private bool _disposed;

    /// <summary>
    /// Initialize WMI connection to Acer WMI interface
    /// </summary>
    public bool Initialize()
    {
        try
        {
            // Try to find Acer WMI instance
            // In Windows, WMI methods are exposed through root\WMI namespace
            var searcher = new ManagementObjectSearcher("root\\WMI",
                $"SELECT * FROM AcerWMI");

            foreach (ManagementObject obj in searcher.Get())
            {
                _wmiObject = obj;
                return true;
            }

            // If not found, try alternative namespace
            searcher = new ManagementObjectSearcher("root\\WMI",
                $"SELECT * FROM WmiMonitorBrightness");

            foreach (ManagementObject obj in searcher.Get())
            {
                // At least we have some WMI capability
                return true;
            }

            return false;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Failed to initialize WMI: {ex.Message}");
            return false;
        }
    }

    /// <summary>
    /// Call a WMI method with parameters
    /// </summary>
    public uint CallMethod(string guid, int methodId, params object[] parameters)
    {
        try
        {
            if (_wmiObject == null)
                return 0xFFFFFFFF; // Error

            // Create input parameters
            ManagementBaseObject inParams = _wmiObject.GetMethodParameters("WMIMethod");
            inParams["MethodId"] = methodId;
            inParams["GUID"] = guid;

            if (parameters.Length > 0)
                inParams["InBuffer"] = parameters;

            // Execute method
            ManagementBaseObject outParams = _wmiObject.InvokeMethod("WMIMethod", inParams, null);

            if (outParams != null && outParams["OutBuffer"] != null)
            {
                byte[] result = (byte[])outParams["OutBuffer"];
                if (result.Length >= 4)
                {
                    return BitConverter.ToUInt32(result, 0);
                }
            }

            return 0;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"WMI method call failed: {ex.Message}");
            return 0xFFFFFFFF;
        }
    }

    /// <summary>
    /// Call gaming-specific WMI method (WMID_GUID4)
    /// </summary>
    public uint CallGamingMethod(int methodId, params uint[] parameters)
    {
        return CallMethod(AcerWmiConstants.WMID_GUID4, methodId, parameters.Cast<object>().ToArray());
    }

    /// <summary>
    /// Get gaming system info (fan speeds, temperatures, etc.)
    /// </summary>
    public uint GetGamingSysInfo(int command)
    {
        return CallGamingMethod(AcerWmiConstants.WMID_GET_GAMING_SYS_INFO, (uint)command);
    }

    /// <summary>
    /// Set gaming misc setting (profiles, overclock, etc.)
    /// </summary>
    public uint SetGamingMiscSetting(int settingIndex, int value)
    {
        uint param = (uint)((value << 8) | settingIndex);
        return CallGamingMethod(AcerWmiConstants.WMID_SET_GAMING_MISC_SETTING, param);
    }

    /// <summary>
    /// Get gaming misc setting
    /// </summary>
    public uint GetGamingMiscSetting(int settingIndex)
    {
        return CallGamingMethod(AcerWmiConstants.WMID_GET_GAMING_MISC_SETTING, (uint)settingIndex);
    }

    /// <summary>
    /// Set fan speed (0-100%)
    /// </summary>
    public bool SetFanSpeed(bool isCpuFan, int speedPercent)
    {
        if (speedPercent < 0 || speedPercent > 100)
            return false;

        int fanId = isCpuFan ? 1 : 2;
        uint param = (uint)((speedPercent << 8) | fanId);

        uint result = CallGamingMethod(AcerWmiConstants.WMID_SET_GAMING_FAN_SPEED, param);
        return (result & 0xFF) == 0; // Check status byte
    }

    /// <summary>
    /// Get CPU fan speed (RPM)
    /// </summary>
    public int GetCpuFanSpeed()
    {
        uint result = GetGamingSysInfo(AcerWmiConstants.CMD_GET_CPU_FAN_SPEED);
        return (int)((result & AcerWmiConstants.PREDATOR_V4_FAN_SPEED_MASK) >> 8);
    }

    /// <summary>
    /// Get GPU fan speed (RPM)
    /// </summary>
    public int GetGpuFanSpeed()
    {
        uint result = GetGamingSysInfo(AcerWmiConstants.CMD_GET_GPU_FAN_SPEED);
        return (int)((result & AcerWmiConstants.PREDATOR_V4_FAN_SPEED_MASK) >> 8);
    }

    /// <summary>
    /// Set RGB keyboard color for a specific zone
    /// </summary>
    public bool SetKeyboardRgb(int zone, byte red, byte green, byte blue)
    {
        // Format: [zone][mode][red][green][blue][brightness]
        uint param1 = (uint)(zone | (1 << 8)); // Static mode
        uint param2 = (uint)(red | (green << 8) | (blue << 16) | (255 << 24));

        uint result = CallGamingMethod(AcerWmiConstants.WMID_SET_GAMING_RGB_KB, param1, param2);
        return (result & 0xFF) == 0;
    }

    /// <summary>
    /// Set performance profile
    /// </summary>
    public bool SetPerformanceProfile(int profileIndex)
    {
        uint result = SetGamingMiscSetting(
            AcerWmiConstants.MISC_SETTING_PLATFORM_PROFILE,
            profileIndex);
        return (result & 0xFF) == 0;
    }

    /// <summary>
    /// Get current performance profile
    /// </summary>
    public int GetPerformanceProfile()
    {
        uint result = GetGamingMiscSetting(AcerWmiConstants.MISC_SETTING_PLATFORM_PROFILE);
        return (int)((result >> 8) & 0xFF);
    }

    /// <summary>
    /// Set turbo/overclock mode
    /// </summary>
    public bool SetTurboMode(bool enabled)
    {
        int value = enabled ? AcerWmiConstants.OC_TURBO : AcerWmiConstants.OC_NORMAL;
        uint result = SetGamingMiscSetting(AcerWmiConstants.MISC_SETTING_OC_1, value);
        return (result & 0xFF) == 0;
    }

    /// <summary>
    /// Set battery health mode
    /// </summary>
    public bool SetBatteryHealthMode(int mode, int limit = 80)
    {
        uint param = (uint)((limit << 8) | mode);
        uint result = CallGamingMethod(AcerWmiConstants.WMID_SET_BATTERY_HEALTH_CONTROL, param);
        return (result & 0xFF) == 0;
    }

    /// <summary>
    /// Get battery health status
    /// </summary>
    public (int mode, int limit) GetBatteryHealthStatus()
    {
        uint result = CallGamingMethod(AcerWmiConstants.WMID_GET_BATTERY_HEALTH_STATUS);
        int mode = (int)(result & 0xFF);
        int limit = (int)((result >> 8) & 0xFF);
        return (mode, limit);
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            _wmiObject?.Dispose();
            _disposed = true;
        }
        GC.SuppressFinalize(this);
    }
}
