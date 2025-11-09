namespace AcerSense.Core.Constants;

/// <summary>
/// Acer WMI GUIDs and method IDs
/// Ported from linuwu_sense.c
/// </summary>
public static class AcerWmiConstants
{
    // WMI GUIDs for Acer ACPI methods
    public const string AMW0_GUID1 = "67C3371D-95A3-4C37-BB61-DD47B491DAAB";
    public const string AMW0_GUID2 = "431F16ED-0C2B-444C-B267-27DEB140CF9C";
    public const string WMID_GUID1 = "6AF4F258-B401-42FD-BE91-3D4AC2D7C0D3";
    public const string WMID_GUID2 = "95764E09-FB56-4E83-B31A-37761F60994A";
    public const string WMID_GUID3 = "61EF69EA-865C-4BC3-A502-A0DEBA0CB531";
    public const string WMID_GUID4 = "7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56";
    public const string WMID_GUID5 = "79772EC5-04B1-4bfd-843C-61E7F77B6CC9";
    public const string ACERWMID_EVENT_GUID = "676AA15E-6A47-4D9F-A2CC-1E6D18D14026";

    // Magic number for AMW0 ACPI writes
    public const int ACER_AMW0_WRITE = 0x9610;

    // WMID Method IDs
    public const int WMID_GET_WIRELESS = 1;
    public const int WMID_GET_BLUETOOTH = 2;
    public const int WMID_GET_BRIGHTNESS = 3;
    public const int WMID_SET_WIRELESS = 4;
    public const int WMID_SET_BLUETOOTH = 5;
    public const int WMID_SET_BRIGHTNESS = 6;
    public const int WMID_GET_THREEG = 10;
    public const int WMID_SET_THREEG = 11;

    // Gaming/Predator/Nitro Method IDs
    public const int WMID_GET_GAMING_PROFILE = 3;
    public const int WMID_SET_GAMING_PROFILE = 1;
    public const int WMID_SET_GAMING_LED = 2;
    public const int WMID_GET_GAMING_LED = 4;
    public const int WMID_GET_GAMING_SYS_INFO = 5;
    public const int WMID_SET_GAMING_RGB_KB = 6;
    public const int WMID_GET_GAMING_RGB_KB = 7;
    public const int WMID_SET_GAMING_FAN_BEHAVIOR = 14;
    public const int WMID_SET_GAMING_FAN_SPEED = 16;
    public const int WMID_GET_BATTERY_HEALTH_STATUS = 20;
    public const int WMID_SET_BATTERY_HEALTH_CONTROL = 21;
    public const int WMID_SET_GAMING_MISC_SETTING = 22;
    public const int WMID_GET_GAMING_MISC_SETTING = 23;
    public const int WMID_SET_GAMING_KB_BACKLIGHT = 20;
    public const int WMID_GET_GAMING_KB_BACKLIGHT = 21;

    // Bit masks
    public const int PREDATOR_V4_FAN_SPEED_MASK = 0x1FFF00; // Bits 20-8
    public const int RETURN_STATUS_MASK = 0xFF; // Bits 7-0
    public const int SENSOR_INDEX_MASK = 0xFF00; // Bits 15-8
    public const int SENSOR_READING_MASK = 0xFFFF00; // Bits 23-8

    // Sensor IDs
    public const int SENSOR_CPU_TEMPERATURE = 0x01;
    public const int SENSOR_CPU_FAN_SPEED = 0x02;
    public const int SENSOR_GPU_FAN_SPEED = 0x06;
    public const int SENSOR_GPU_TEMPERATURE = 0x0A;

    // System info commands
    public const int CMD_GET_SUPPORTED_SENSORS = 0x0000;
    public const int CMD_GET_BAT_STATUS = 0x02;
    public const int CMD_GET_SENSOR_READING = 0x0001;
    public const int CMD_GET_CPU_FAN_SPEED = 0x0201;
    public const int CMD_GET_GPU_FAN_SPEED = 0x0601;

    // Overclock/Turbo modes
    public const int OC_NORMAL = 0x0000;
    public const int OC_TURBO = 0x0002;

    // Misc settings
    public const int MISC_SETTING_OC_1 = 0x0005;
    public const int MISC_SETTING_OC_2 = 0x0007;
    public const int MISC_SETTING_SUPPORTED_PROFILES = 0x000A;
    public const int MISC_SETTING_PLATFORM_PROFILE = 0x000B;
}
