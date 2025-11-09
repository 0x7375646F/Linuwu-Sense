using System.Management;
using AcerSense.Core.Models;

namespace AcerSense.Core.Detection;

/// <summary>
/// Detects laptop model and sets appropriate quirks/capabilities
/// Uses SMBIOS/WMI to identify the device (similar to DMI matching in Linux)
/// </summary>
public class DeviceDetector
{
    private static readonly Dictionary<string, Func<LaptopQuirk>> QuirkMap = new()
    {
        // Predator Series
        ["Predator PH315-53"] = () => new LaptopQuirk
        {
            ModelName = "Predator PH315-53",
            Turbo = true,
            HasCpuFans = true,
            HasGpuFans = true
        },
        ["Predator PHN16-71"] = () => new LaptopQuirk
        {
            ModelName = "Predator PHN16-71",
            IsPredatorV4 = true,
            HasFourZoneKeyboard = true
        },
        ["Predator PHN16-72"] = () => new LaptopQuirk
        {
            ModelName = "Predator PHN16-72",
            IsPredatorV4 = true,
            HasFourZoneKeyboard = true
        },
        ["Predator PH16-71"] = () => new LaptopQuirk
        {
            ModelName = "Predator PH16-71",
            IsPredatorV4 = true
        },
        ["Predator PH18-71"] = () => new LaptopQuirk
        {
            ModelName = "Predator PH18-71",
            IsPredatorV4 = true
        },

        // Nitro Series
        ["Nitro AN16-43"] = () => new LaptopQuirk
        {
            ModelName = "Nitro AN16-43",
            IsNitroV4 = true,
            HasFourZoneKeyboard = true
        },
        ["Nitro AN16-41"] = () => new LaptopQuirk
        {
            ModelName = "Nitro AN16-41",
            IsNitroV4 = true,
            HasFourZoneKeyboard = true
        },
        ["Nitro ANV16-41"] = () => new LaptopQuirk
        {
            ModelName = "Nitro ANV16-41",
            IsNitroV4 = true,
            HasFourZoneKeyboard = false
        },
        ["Nitro ANV15-41"] = () => new LaptopQuirk
        {
            ModelName = "Nitro ANV15-41",
            IsNitroSense = true
        },
        ["Nitro ANV15-51"] = () => new LaptopQuirk
        {
            ModelName = "Nitro ANV15-51",
            IsNitroSense = true
        },
        ["Nitro AN515-58"] = () => new LaptopQuirk
        {
            ModelName = "Nitro AN515-58",
            IsNitroV4 = true,
            HasFourZoneKeyboard = true
        },
        ["Nitro AN515-55"] = () => new LaptopQuirk
        {
            ModelName = "Nitro AN515-55",
            IsNitroSense = true
        }
    };

    /// <summary>
    /// Detect the current laptop model and return its quirks
    /// </summary>
    public static LaptopQuirk DetectDevice()
    {
        try
        {
            string manufacturer = GetSystemInfo("Manufacturer");
            string productName = GetSystemInfo("Model");

            Console.WriteLine($"Detected: {manufacturer} {productName}");

            // Check if it's an Acer device
            if (!manufacturer.Contains("Acer", StringComparison.OrdinalIgnoreCase))
            {
                return new LaptopQuirk
                {
                    ModelName = "Unknown (Non-Acer)",
                    Manufacturer = manufacturer
                };
            }

            // Try to match known models
            foreach (var kvp in QuirkMap)
            {
                if (productName.Contains(kvp.Key, StringComparison.OrdinalIgnoreCase))
                {
                    var quirk = kvp.Value();
                    quirk.Manufacturer = manufacturer;
                    return quirk;
                }
            }

            // Check for generic Predator/Nitro
            if (productName.Contains("Predator", StringComparison.OrdinalIgnoreCase))
            {
                return new LaptopQuirk
                {
                    ModelName = productName,
                    Manufacturer = manufacturer,
                    IsPredatorV4 = true // Assume v4 for unknown Predators
                };
            }

            if (productName.Contains("Nitro", StringComparison.OrdinalIgnoreCase))
            {
                return new LaptopQuirk
                {
                    ModelName = productName,
                    Manufacturer = manufacturer,
                    IsNitroV4 = true // Assume v4 for unknown Nitros
                };
            }

            // Unknown Acer device
            return new LaptopQuirk
            {
                ModelName = productName,
                Manufacturer = manufacturer
            };
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Device detection failed: {ex.Message}");
            return new LaptopQuirk { ModelName = "Unknown" };
        }
    }

    /// <summary>
    /// Get system information from WMI
    /// </summary>
    private static string GetSystemInfo(string property)
    {
        try
        {
            using var searcher = new ManagementObjectSearcher(
                "SELECT * FROM Win32_ComputerSystem");

            foreach (ManagementObject obj in searcher.Get())
            {
                return obj[property]?.ToString() ?? "Unknown";
            }
        }
        catch
        {
            // Ignore
        }

        return "Unknown";
    }

    /// <summary>
    /// Check if WMI interface is available
    /// </summary>
    public static bool IsAcerWmiAvailable()
    {
        try
        {
            // Try to enumerate WMI classes in root\WMI namespace
            using var searcher = new ManagementObjectSearcher("root\\WMI",
                "SELECT * FROM meta_class");

            foreach (ManagementObject obj in searcher.Get())
            {
                string className = obj["__CLASS"]?.ToString() ?? "";
                if (className.Contains("Acer", StringComparison.OrdinalIgnoreCase))
                {
                    Console.WriteLine($"Found Acer WMI class: {className}");
                    return true;
                }
            }

            return false;
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Get list of available features based on quirks
    /// </summary>
    public static List<string> GetAvailableFeatures(LaptopQuirk quirk)
    {
        var features = new List<string>();

        if (quirk.IsPredatorV4 || quirk.IsNitroV4)
        {
            features.Add("Performance Profiles");
            features.Add("Fan Speed Control");
            features.Add("Battery Health Management");
            features.Add("Temperature Monitoring");
        }

        if (quirk.IsPredatorV4)
        {
            features.Add("LCD Override");
            features.Add("Boot Animation Control");
            features.Add("Backlight Timeout");
        }

        if (quirk.HasFourZoneKeyboard)
        {
            features.Add("4-Zone RGB Keyboard");
        }

        if (quirk.Turbo)
        {
            features.Add("Turbo/Overclock Mode");
        }

        if (quirk.IsNitroSense)
        {
            features.Add("Basic Fan Monitoring");
            features.Add("Platform Profile Switching");
        }

        return features;
    }
}
