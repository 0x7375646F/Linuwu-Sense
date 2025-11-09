namespace AcerSense.Core.Models;

/// <summary>
/// Represents device-specific feature support flags
/// Ported from quirk_entry struct in linuwu_sense.c
/// </summary>
public class LaptopQuirk
{
    public bool Wireless { get; set; }
    public bool Mailled { get; set; }
    public bool Bluetooth { get; set; }
    public bool Turbo { get; set; }
    public bool HasCpuFans { get; set; }
    public bool HasGpuFans { get; set; }
    public bool IsPredatorV4 { get; set; }
    public bool IsNitroV4 { get; set; }
    public bool IsNitroSense { get; set; }
    public bool HasFourZoneKeyboard { get; set; }

    public string ModelName { get; set; } = string.Empty;
    public string Manufacturer { get; set; } = string.Empty;
}

/// <summary>
/// Laptop model enumeration
/// </summary>
public enum LaptopModel
{
    Unknown,

    // Predator Series
    PredatorPH315_53,
    PredatorPHN16_71,
    PredatorPHN16_72,
    PredatorPH16_71,
    PredatorPH18_71,

    // Nitro Series
    NitroAN16_43,
    NitroAN16_41,
    NitroANV16_41,
    NitroANV15_41,
    NitroANV15_51,
    NitroAN515_58,
    NitroAN515_55,

    // Generic
    GenericPredatorV4,
    GenericNitroV4,
    GenericNitroSense
}

/// <summary>
/// Performance/thermal profile modes
/// </summary>
public enum PerformanceProfile
{
    Quiet = 0,
    Balanced = 1,
    Performance = 2,
    Turbo = 3
}

/// <summary>
/// Battery health modes
/// </summary>
public enum BatteryMode
{
    Normal = 0,
    HealthMode = 1,
    CalibrationMode = 2
}

/// <summary>
/// RGB keyboard zone
/// </summary>
public enum KeyboardZone
{
    Zone1 = 0,
    Zone2 = 1,
    Zone3 = 2,
    Zone4 = 3,
    All = 4
}
