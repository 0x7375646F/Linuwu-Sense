// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Acer WMI Laptop Extras
 *
 *  Copyright (C) 2007-2009	Carlos Corbacho <carlos@strangeworlds.co.uk>
 *
 *  Based on acer_acpi:
 *    Copyright (C) 2005-2007	E.M. Smith
 *    Copyright (C) 2007-2008	Carlos Corbacho <cathectic@gmail.com>
 */

 #define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
 
 #include <linux/delay.h>
 #include <linux/kernel.h>
 #include <linux/module.h>
 #include <linux/init.h>
 #include <linux/types.h>
 #include <linux/dmi.h>
 #include <linux/backlight.h>
 #include <linux/leds.h>
 #include <linux/platform_device.h>
 #include <linux/platform_profile.h>
 #include <linux/acpi.h>
 #include <linux/i8042.h>
 #include <linux/rfkill.h>
 #include <linux/workqueue.h>
 #include <linux/debugfs.h>
 #include <linux/slab.h>
 #include <linux/input.h>
 #include <linux/input/sparse-keymap.h>
 #include <acpi/video.h>
 #include <linux/hwmon.h>
 #include <linux/fs.h>
 #include <linux/units.h>
 #include <linux/unaligned.h>
 #include <linux/bitfield.h>
 #include <linux/bitmap.h>
 
 MODULE_AUTHOR("Carlos Corbacho");
 MODULE_DESCRIPTION("Acer Laptop WMI Extras Driver");
 MODULE_LICENSE("GPL");
 
 /*
  * Magic Number
  * Meaning is unknown - this number is required for writing to ACPI for AMW0
  * (it's also used in acerhk when directly accessing the BIOS)
  */
 #define ACER_AMW0_WRITE	0x9610
 
 /*
  * Bit masks for the AMW0 interface
  */
 #define ACER_AMW0_WIRELESS_MASK  0x35
 #define ACER_AMW0_BLUETOOTH_MASK 0x34
 #define ACER_AMW0_MAILLED_MASK   0x31
 
 /*
  * Method IDs for WMID interface
  */
 #define ACER_WMID_GET_WIRELESS_METHODID		1
 #define ACER_WMID_GET_BLUETOOTH_METHODID	2
 #define ACER_WMID_GET_BRIGHTNESS_METHODID	3
 #define ACER_WMID_SET_WIRELESS_METHODID		4
 #define ACER_WMID_SET_BLUETOOTH_METHODID	5
 #define ACER_WMID_SET_BRIGHTNESS_METHODID	6
 #define ACER_WMID_GET_THREEG_METHODID		10
 #define ACER_WMID_SET_THREEG_METHODID		11
 #define ACER_WMID_SET_FUNCTION 1
 #define ACER_WMID_GET_FUNCTION 2
 
 
 #define ACER_WMID_GET_GAMING_PROFILE_METHODID 3
 #define ACER_WMID_SET_GAMING_PROFILE_METHODID 1
 #define ACER_WMID_SET_GAMING_LED_METHODID 2
 #define ACER_WMID_GET_GAMING_LED_METHODID 4
 #define ACER_WMID_GET_GAMING_SYS_INFO_METHODID 5
 #define ACER_WMID_SET_GAMING_FAN_BEHAVIOR_METHODID 14
 #define ACER_WMID_SET_GAMING_FAN_SPEED_METHODID 16
 #define ACER_WMID_SET_GAMING_MISC_SETTING_METHODID 22
 #define ACER_WMID_GET_GAMING_MISC_SETTING_METHODID 23
 #define ACER_WMID_GET_BATTERY_HEALTH_CONTROL_STATUS_METHODID 20
 #define ACER_WMID_SET_BATTERY_HEALTH_CONTROL_METHODID 21
 #define ACER_WMID_GET_GAMING_KB_BACKLIGHT_METHODID 21 
 #define ACER_WMID_SET_GAMING_KB_BACKLIGHT_METHODID 20
 #define ACER_WMID_SET_GAMING_RGB_KB_METHODID 6
 #define ACER_WMID_GET_GAMING_RGB_KB_METHODID 7
 
 #define ACER_PREDATOR_V4_FAN_SPEED_READ_BIT_MASK GENMASK(20, 8)
 #define ACER_GAMING_MISC_SETTING_STATUS_MASK GENMASK_ULL(7, 0)
 #define ACER_GAMING_MISC_SETTING_INDEX_MASK GENMASK_ULL(7, 0)
 #define ACER_GAMING_MISC_SETTING_VALUE_MASK GENMASK_ULL(15, 8)
 
 #define ACER_PREDATOR_V4_RETURN_STATUS_BIT_MASK GENMASK_ULL(7, 0)
 #define ACER_PREDATOR_V4_SENSOR_INDEX_BIT_MASK GENMASK_ULL(15, 8)
 #define ACER_PREDATOR_V4_SENSOR_READING_BIT_MASK GENMASK_ULL(23, 8)
 #define ACER_PREDATOR_V4_SUPPORTED_SENSORS_BIT_MASK GENMASK_ULL(39, 24)
 
 /*
  * Acer ACPI method GUIDs
  */
 #define AMW0_GUID1		"67C3371D-95A3-4C37-BB61-DD47B491DAAB"
 #define AMW0_GUID2		"431F16ED-0C2B-444C-B267-27DEB140CF9C"
 #define WMID_GUID1		"6AF4F258-B401-42FD-BE91-3D4AC2D7C0D3"
 #define WMID_GUID2		"95764E09-FB56-4E83-B31A-37761F60994A"
 #define WMID_GUID3		"61EF69EA-865C-4BC3-A502-A0DEBA0CB531"
 #define WMID_GUID4		"7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56"
 #define WMID_GUID5		"79772EC5-04B1-4bfd-843C-61E7F77B6CC9"
 
 /*
  * Predator State
  */
 #define STATE_FILE "/etc/predator_state" 
 #define KB_STATE_FILE "/etc/four_zone_kb_state"
 /*
  * Acer ACPI event GUIDs
  */
 #define ACERWMID_EVENT_GUID "676AA15E-6A47-4D9F-A2CC-1E6D18D14026"
 
 MODULE_ALIAS("wmi:67C3371D-95A3-4C37-BB61-DD47B491DAAB");
 MODULE_ALIAS("wmi:6AF4F258-B401-42FD-BE91-3D4AC2D7C0D3");
 MODULE_ALIAS("wmi:676AA15E-6A47-4D9F-A2CC-1E6D18D14026");
 
 enum acer_wmi_event_ids {
     WMID_HOTKEY_EVENT = 0x1,
     WMID_ACCEL_OR_KBD_DOCK_EVENT = 0x5,
     WMID_GAMING_TURBO_KEY_EVENT = 0x7,	
     WMID_AC_EVENT = 0x8,
     WMID_BATTERY_BOOST_EVENT = 0x9,
     WMID_CALIBRATION_EVENT = 0x0B,
 };
 
 enum battery_mode {
     HEALTH_MODE = 1,
     CALIBRATION_MODE = 2 
 };
 
 enum acer_wmi_predator_v4_sys_info_command {
    ACER_WMID_CMD_GET_PREDATOR_V4_SUPPORTED_SENSORS = 0x0000,
    ACER_WMID_CMD_GET_PREDATOR_V4_BAT_STATUS = 0x02,
    ACER_WMID_CMD_GET_PREDATOR_V4_SENSOR_READING	= 0x0001,
    ACER_WMID_CMD_GET_PREDATOR_V4_CPU_FAN_SPEED = 0x0201,
    ACER_WMID_CMD_GET_PREDATOR_V4_GPU_FAN_SPEED = 0x0601,
 };

 enum acer_wmi_predator_v4_sensor_id {
    ACER_WMID_SENSOR_CPU_TEMPERATURE	= 0x01,
    ACER_WMID_SENSOR_CPU_FAN_SPEED		= 0x02,
    ACER_WMID_SENSOR_EXTERNAL_TEMPERATURE_2 = 0x03,
    ACER_WMID_SENSOR_GPU_FAN_SPEED		= 0x06,
    ACER_WMID_SENSOR_GPU_TEMPERATURE	= 0x0A,
};

enum acer_wmi_predator_v4_oc {
    ACER_WMID_OC_NORMAL			= 0x0000,
    ACER_WMID_OC_TURBO			= 0x0002,
};

 enum acer_wmi_gaming_misc_setting {
	ACER_WMID_MISC_SETTING_OC_1			= 0x0005,
	ACER_WMID_MISC_SETTING_OC_2			= 0x0007,
	ACER_WMID_MISC_SETTING_SUPPORTED_PROFILES	= 0x000A,
	ACER_WMID_MISC_SETTING_PLATFORM_PROFILE		= 0x000B,
 };
 
 static const struct key_entry acer_wmi_keymap[] __initconst = {
     {KE_KEY, 0x01, {KEY_WLAN} },     /* WiFi */
     {KE_KEY, 0x03, {KEY_WLAN} },     /* WiFi */
     {KE_KEY, 0x04, {KEY_WLAN} },     /* WiFi */
     {KE_KEY, 0x12, {KEY_BLUETOOTH} },	/* BT */
     {KE_KEY, 0x21, {KEY_PROG1} },    /* Backup */
     {KE_KEY, 0x22, {KEY_PROG2} },    /* Arcade */
     {KE_KEY, 0x23, {KEY_PROG3} },    /* P_Key */
     {KE_KEY, 0x24, {KEY_PROG4} },    /* Social networking_Key */
     {KE_KEY, 0x27, {KEY_HELP} },
     {KE_KEY, 0x29, {KEY_PROG3} },    /* P_Key for TM8372 */
     {KE_IGNORE, 0x41, {KEY_MUTE} },
     {KE_IGNORE, 0x42, {KEY_PREVIOUSSONG} },
     {KE_IGNORE, 0x4d, {KEY_PREVIOUSSONG} },
     {KE_IGNORE, 0x43, {KEY_NEXTSONG} },
     {KE_IGNORE, 0x4e, {KEY_NEXTSONG} },
     {KE_IGNORE, 0x44, {KEY_PLAYPAUSE} },
     {KE_IGNORE, 0x4f, {KEY_PLAYPAUSE} },
     {KE_IGNORE, 0x45, {KEY_STOP} },
     {KE_IGNORE, 0x50, {KEY_STOP} },
     {KE_IGNORE, 0x48, {KEY_VOLUMEUP} },
     {KE_IGNORE, 0x49, {KEY_VOLUMEDOWN} },
     {KE_IGNORE, 0x4a, {KEY_VOLUMEDOWN} },
     /*
      * 0x61 is KEY_SWITCHVIDEOMODE. Usually this is a duplicate input event
      * with the "Video Bus" input device events. But sometimes it is not
      * a dup. Map it to KEY_UNKNOWN instead of using KE_IGNORE so that
      * udev/hwdb can override it on systems where it is not a dup.
      */
     {KE_KEY, 0x61, {KEY_UNKNOWN} },
     {KE_IGNORE, 0x62, {KEY_BRIGHTNESSUP} },
     {KE_IGNORE, 0x63, {KEY_BRIGHTNESSDOWN} },
     {KE_KEY, 0x64, {KEY_SWITCHVIDEOMODE} },	/* Display Switch */
     {KE_IGNORE, 0x81, {KEY_SLEEP} },
     {KE_KEY, 0x82, {KEY_TOUCHPAD_TOGGLE} },	/* Touch Pad Toggle */
     {KE_IGNORE, 0x84, {KEY_KBDILLUMTOGGLE} }, /* Automatic Keyboard background light toggle */
     {KE_KEY, KEY_TOUCHPAD_ON, {KEY_TOUCHPAD_ON} },
     {KE_KEY, KEY_TOUCHPAD_OFF, {KEY_TOUCHPAD_OFF} },
     {KE_IGNORE, 0x83, {KEY_TOUCHPAD_TOGGLE} },
     {KE_KEY, 0x85, {KEY_TOUCHPAD_TOGGLE} },
     {KE_KEY, 0x86, {KEY_WLAN} },
     {KE_KEY, 0x87, {KEY_POWER} },
     {KE_END, 0}
 };
 
 static struct input_dev *acer_wmi_input_dev;
 static struct input_dev *acer_wmi_accel_dev;
 
 struct event_return_value {
     u8 function;
     u8 key_num;
     u16 device_state;
     u16 reserved1;
     u8 kbd_dock_state;
     u8 reserved2;
 } __packed;
 
 /*
  * GUID3 Get Device Status device flags
  */
 #define ACER_WMID3_GDS_WIRELESS		(1<<0)	/* WiFi */
 #define ACER_WMID3_GDS_THREEG		(1<<6)	/* 3G */
 #define ACER_WMID3_GDS_WIMAX		(1<<7)	/* WiMAX */
 #define ACER_WMID3_GDS_BLUETOOTH	(1<<11)	/* BT */
 #define ACER_WMID3_GDS_RFBTN		(1<<14)	/* RF Button */
 
 #define ACER_WMID3_GDS_TOUCHPAD		(1<<1)	/* Touchpad */
 
 /* Hotkey Customized Setting and Acer Application Status.
  * Set Device Default Value and Report Acer Application Status.
  * When Acer Application starts, it will run this method to inform
  * BIOS/EC that Acer Application is on.
  * App Status
  *	Bit[0]: Launch Manager Status
  *	Bit[1]: ePM Status
  *	Bit[2]: Device Control Status
  *	Bit[3]: Acer Power Button Utility Status
  *	Bit[4]: RF Button Status
  *	Bit[5]: ODD PM Status
  *	Bit[6]: Device Default Value Control
  *	Bit[7]: Hall Sensor Application Status
  */
 struct func_input_params {
     u8 function_num;        /* Function Number */
     u16 commun_devices;     /* Communication type devices default status */
     u16 devices;            /* Other type devices default status */
     u8 app_status;          /* Acer Device Status. LM, ePM, RF Button... */
     u8 app_mask;		/* Bit mask to app_status */
     u8 reserved;
 } __packed;
 
 struct func_return_value {
     u8 error_code;          /* Error Code */
     u8 ec_return_value;     /* EC Return Value */
     u16 reserved;
 } __packed;
 
 struct wmid3_gds_set_input_param {     /* Set Device Status input parameter */
     u8 function_num;        /* Function Number */
     u8 hotkey_number;       /* Hotkey Number */
     u16 devices;            /* Set Device */
     u8 volume_value;        /* Volume Value */
 } __packed;
 
 struct wmid3_gds_get_input_param {     /* Get Device Status input parameter */
     u8 function_num;	/* Function Number */
     u8 hotkey_number;	/* Hotkey Number */
     u16 devices;		/* Get Device */
 } __packed;
 
 struct wmid3_gds_return_value {	/* Get Device Status return value*/
     u8 error_code;		/* Error Code */
     u8 ec_return_value;	/* EC Return Value */
     u16 devices;		/* Current Device Status */
     u32 reserved;
 } __packed;
 
 struct hotkey_function_type_aa {
     u8 type;
     u8 length;
     u16 handle;
     u16 commun_func_bitmap;
     u16 application_func_bitmap;
     u16 media_func_bitmap;
     u16 display_func_bitmap;
     u16 others_func_bitmap;
     u8 commun_fn_key_number;
 } __packed;
 
 /*
  * Interface capability flags
  */
 #define ACER_CAP_MAILLED		BIT(0)
 #define ACER_CAP_WIRELESS		BIT(1)
 #define ACER_CAP_BLUETOOTH		BIT(2)
 #define ACER_CAP_BRIGHTNESS		BIT(3)
 #define ACER_CAP_THREEG			BIT(4)
 #define ACER_CAP_SET_FUNCTION_MODE	BIT(5)
 #define ACER_CAP_KBD_DOCK		BIT(6)
 #define ACER_CAP_TURBO_OC		BIT(7)
 #define ACER_CAP_TURBO_LED		BIT(8)
 #define ACER_CAP_TURBO_FAN		BIT(9)
 #define ACER_CAP_PLATFORM_PROFILE	BIT(10)
 #define ACER_CAP_FAN_SPEED_READ		BIT(11)
 #define ACER_CAP_PREDATOR_SENSE		BIT(12)
 #define ACER_CAP_NITRO_SENSE BIT(13)
 #define ACER_CAP_NITRO_SENSE_V4		BIT(14)

 /*
  * Interface type flags
  */
 enum interface_flags {
     ACER_AMW0,
     ACER_AMW0_V2,
     ACER_WMID,
     ACER_WMID_v2,
 };
 
 static int max_brightness = 0xF;
 
 static int mailled = -1;
 static int brightness = -1;
 static int threeg = -1;
 static int force_series;
 static int force_caps = -1;
 static bool ec_raw_mode;
 static bool has_type_aa;
 static u16 commun_func_bitmap;
 static u8 commun_fn_key_number;
 static bool cycle_gaming_thermal_profile = true;
 static bool predator_v4;
 static bool nitro_v4;
 static u64 supported_sensors;
 
 module_param(mailled, int, 0444);
 module_param(brightness, int, 0444);
 module_param(threeg, int, 0444);
 module_param(force_series, int, 0444);
 module_param(force_caps, int, 0444);
 module_param(ec_raw_mode, bool, 0444);
 module_param(cycle_gaming_thermal_profile, bool, 0644);
 module_param(predator_v4, bool, 0444);
 module_param(nitro_v4, bool, 0444);
 MODULE_PARM_DESC(mailled, "Set initial state of Mail LED");
 MODULE_PARM_DESC(brightness, "Set initial LCD backlight brightness");
 MODULE_PARM_DESC(threeg, "Set initial state of 3G hardware");
 MODULE_PARM_DESC(force_series, "Force a different laptop series");
 MODULE_PARM_DESC(force_caps, "Force the capability bitmask to this value");
 MODULE_PARM_DESC(ec_raw_mode, "Enable EC raw mode");
 MODULE_PARM_DESC(cycle_gaming_thermal_profile,
     "Set thermal mode key in cycle mode. Disabling it sets the mode key in turbo toggle mode");
 MODULE_PARM_DESC(predator_v4,
     "Enable features for predator laptops that use predator sense v4");
 MODULE_PARM_DESC(nitro_v4,
    "Enable features for nitro laptops that use nitro sense v4");
 
 struct acer_data {
     int mailled;
     int threeg;
     int brightness;
 };
 
 struct acer_debug {
     struct dentry *root;
     u32 wmid_devices;
 };
 
 static struct rfkill *wireless_rfkill;
 static struct rfkill *bluetooth_rfkill;
 static struct rfkill *threeg_rfkill;
 static bool rfkill_inited;
 
 /* Each low-level interface must define at least some of the following */
 struct wmi_interface {
     /* The WMI device type */
     u32 type;
 
     /* The capabilities this interface provides */
     u32 capability;
 
     /* Private data for the current interface */
     struct acer_data data;
 
     /* debugfs entries associated with this interface */
     struct acer_debug debug;
 };
 
 /* The static interface pointer, points to the currently detected interface */
 static struct wmi_interface *interface;
 
 /*
  * Embedded Controller quirks
  * Some laptops require us to directly access the EC to either enable or query
  * features that are not available through WMI.
  */
 
 struct quirk_entry {
     u8 wireless;
     u8 mailled;
     s8 brightness;
     u8 bluetooth;
     u8 turbo;
     u8 cpu_fans;
     u8 gpu_fans;
     u8 predator_v4;
     u8 nitro_v4;
     u8 nitro_sense;
     u8 four_zone_kb;
 };
 
 static struct quirk_entry *quirks;
 
 static void __init set_quirks(void)
 {
     if (quirks->mailled)
         interface->capability |= ACER_CAP_MAILLED;
 
     if (quirks->brightness)
         interface->capability |= ACER_CAP_BRIGHTNESS;
 
     if (quirks->turbo)
         interface->capability |= ACER_CAP_TURBO_OC | ACER_CAP_TURBO_LED
                      | ACER_CAP_TURBO_FAN;
 
    /* Some acer nitro laptops don't have features like lcd override , boot animation sound so this is used. Think wisely before using any quirks validate your features. */
     if (quirks->nitro_sense)
         interface->capability |= ACER_CAP_PLATFORM_PROFILE | ACER_CAP_FAN_SPEED_READ | ACER_CAP_NITRO_SENSE;
 
     if (quirks->predator_v4)
         interface->capability |= ACER_CAP_PLATFORM_PROFILE |
                      ACER_CAP_FAN_SPEED_READ | ACER_CAP_PREDATOR_SENSE;
     
     /* Includes all feature that predatorv4 have*/
     if (quirks->nitro_v4)
         interface->capability |= ACER_CAP_PLATFORM_PROFILE |
                 ACER_CAP_FAN_SPEED_READ  | ACER_CAP_NITRO_SENSE_V4;
 }
 
 static int __init dmi_matched(const struct dmi_system_id *dmi)
 {
     quirks = dmi->driver_data;
     return 1;
 }
 
 static int __init set_force_caps(const struct dmi_system_id *dmi)
 {
     if (force_caps == -1) {
         force_caps = (uintptr_t)dmi->driver_data;
         pr_info("Found %s, set force_caps to 0x%x\n", dmi->ident, force_caps);
     }
     return 1;
 }
 
 static struct quirk_entry quirk_unknown = {
 };
 
 static struct quirk_entry quirk_acer_aspire_1520 = {
     .brightness = -1,
 };
 
 static struct quirk_entry quirk_acer_travelmate_2490 = {
     .mailled = 1,
 };
 
 static struct quirk_entry quirk_acer_predator_ph315_53 = {
     .turbo = 1,
     .cpu_fans = 1,
     .gpu_fans = 1,
 };
 
 static struct quirk_entry quirk_acer_predator_phn16_71 = {
     .predator_v4 = 1,
     .four_zone_kb = 1,
 };
 
 static struct quirk_entry quirk_acer_predator_phn16_72 = {
    .predator_v4 = 1,
    .four_zone_kb = 1,
 };
 
 static struct quirk_entry quirk_acer_nitro_an16_41 = {
    .nitro_v4 = 1,
    .four_zone_kb = 1,
 };

  static struct quirk_entry quirk_acer_nitro_an16_43 = {
    .nitro_v4 = 1,
    .four_zone_kb = 1,
 };
 
 static struct quirk_entry quirk_acer_nitro_an515_58 = {
    .nitro_v4 = 1,
    .four_zone_kb = 1,
 };


 static struct quirk_entry quirk_acer_nitro = {
     .nitro_sense = 1,
 };
 
 static struct quirk_entry quirk_acer_predator_v4 = {
     .predator_v4 = 1,
 };
 
 /* This AMW0 laptop has no bluetooth */
 static struct quirk_entry quirk_medion_md_98300 = {
     .wireless = 1,
 };
 
 static struct quirk_entry quirk_fujitsu_amilo_li_1718 = {
     .wireless = 2,
 };
 
 static struct quirk_entry quirk_lenovo_ideapad_s205 = {
     .wireless = 3,
 };
 
 static struct quirk_entry quirk_acer_nitro_v4 = {
    .nitro_v4 = 1,
 };

 /* The Aspire One has a dummy ACPI-WMI interface - disable it */
 static const struct dmi_system_id acer_blacklist[] __initconst = {
     {
         .ident = "Acer Aspire One (SSD)",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "AOA110"),
         },
     },
     {
         .ident = "Acer Aspire One (HDD)",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "AOA150"),
         },
     },
     {}
 };
 
 static const struct dmi_system_id amw0_whitelist[] __initconst = {
     {
         .ident = "Acer",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
         },
     },
     {
         .ident = "Gateway",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Gateway"),
         },
     },
     {
         .ident = "Packard Bell",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Packard Bell"),
         },
     },
     {}
 };
 
 /*
  * This quirk table is only for Acer/Gateway/Packard Bell family
  * that those machines are supported by acer-wmi driver.
  */
 static const struct dmi_system_id acer_quirks[] __initconst = {
     {
         .callback = dmi_matched,
         .ident = "Acer Nitro AN16-43",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Nitro AN16-43"),
         },
         .driver_data = &quirk_acer_nitro_an16_43,
     },
     {
        .callback = dmi_matched,
        .ident = "Acer Nitro AN515-58",
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
            DMI_MATCH(DMI_PRODUCT_NAME, "Nitro AN515-58"),
        },
        .driver_data = &quirk_acer_nitro_an515_58,
    },
     {
         .callback = dmi_matched,
         .ident = "Acer Nitro AN16-41",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Nitro AN16-41"),
         },
         .driver_data = &quirk_acer_nitro_an16_41,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Nitro ANV15-41",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Nitro ANV15-41"),
         },
         .driver_data = &quirk_acer_nitro,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Nitro ANV15-51",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Nitro ANV15-51"),
         },
         .driver_data = &quirk_acer_nitro,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Aspire 1360",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Aspire 1360"),
         },
         .driver_data = &quirk_acer_aspire_1520,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Aspire 1520",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Aspire 1520"),
         },
         .driver_data = &quirk_acer_aspire_1520,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Aspire 3100",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Aspire 3100"),
         },
         .driver_data = &quirk_acer_travelmate_2490,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Aspire 3610",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Aspire 3610"),
         },
         .driver_data = &quirk_acer_travelmate_2490,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Aspire 5100",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Aspire 5100"),
         },
         .driver_data = &quirk_acer_travelmate_2490,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Aspire 5610",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Aspire 5610"),
         },
         .driver_data = &quirk_acer_travelmate_2490,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Aspire 5630",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Aspire 5630"),
         },
         .driver_data = &quirk_acer_travelmate_2490,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Aspire 5650",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Aspire 5650"),
         },
         .driver_data = &quirk_acer_travelmate_2490,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Aspire 5680",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Aspire 5680"),
         },
         .driver_data = &quirk_acer_travelmate_2490,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Aspire 9110",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Aspire 9110"),
         },
         .driver_data = &quirk_acer_travelmate_2490,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer TravelMate 2490",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "TravelMate 2490"),
         },
         .driver_data = &quirk_acer_travelmate_2490,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer TravelMate 4200",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "TravelMate 4200"),
         },
         .driver_data = &quirk_acer_travelmate_2490,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Predator PH315-53",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Predator PH315-53"),
         },
         .driver_data = &quirk_acer_predator_ph315_53,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Predator PHN16-71",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Predator PHN16-71"),
         },
         .driver_data = &quirk_acer_predator_phn16_71,
     },
     {
        .callback = dmi_matched,
        .ident = "Acer Predator PHN16-72",
        .matches = {
            DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
            DMI_MATCH(DMI_PRODUCT_NAME, "Predator PHN16-72"),
        },
        .driver_data = &quirk_acer_predator_phn16_72,
    },
     {
         .callback = dmi_matched,
         .ident = "Acer Predator PH16-71",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Predator PH16-71"),
         },
         .driver_data = &quirk_acer_predator_v4,
     },
     {
         .callback = dmi_matched,
         .ident = "Acer Predator PH18-71",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Predator PH18-71"),
         },
         .driver_data = &quirk_acer_predator_v4,
     },
     {
         .callback = set_force_caps,
         .ident = "Acer Aspire Switch 10E SW3-016",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Aspire SW3-016"),
         },
         .driver_data = (void *)ACER_CAP_KBD_DOCK,
     },
     {
         .callback = set_force_caps,
         .ident = "Acer Aspire Switch 10 SW5-012",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Aspire SW5-012"),
         },
         .driver_data = (void *)ACER_CAP_KBD_DOCK,
     },
     {
         .callback = set_force_caps,
         .ident = "Acer Aspire Switch V 10 SW5-017",
         .matches = {
             DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "SW5-017"),
         },
         .driver_data = (void *)ACER_CAP_KBD_DOCK,
     },
     {
         .callback = set_force_caps,
         .ident = "Acer One 10 (S1003)",
         .matches = {
             DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Acer"),
             DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "One S1003"),
         },
         .driver_data = (void *)ACER_CAP_KBD_DOCK,
     },
     {}
 };
 
 /*
  * This quirk list is for those non-acer machines that have AMW0_GUID1
  * but supported by acer-wmi in past days. Keeping this quirk list here
  * is only for backward compatible. Please do not add new machine to
  * here anymore. Those non-acer machines should be supported by
  * appropriate wmi drivers.
  */
 static const struct dmi_system_id non_acer_quirks[] __initconst = {
     {
         .callback = dmi_matched,
         .ident = "Fujitsu Siemens Amilo Li 1718",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "FUJITSU SIEMENS"),
             DMI_MATCH(DMI_PRODUCT_NAME, "AMILO Li 1718"),
         },
         .driver_data = &quirk_fujitsu_amilo_li_1718,
     },
     {
         .callback = dmi_matched,
         .ident = "Medion MD 98300",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "MEDION"),
             DMI_MATCH(DMI_PRODUCT_NAME, "WAM2030"),
         },
         .driver_data = &quirk_medion_md_98300,
     },
     {
         .callback = dmi_matched,
         .ident = "Lenovo Ideapad S205",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "LENOVO"),
             DMI_MATCH(DMI_PRODUCT_NAME, "10382LG"),
         },
         .driver_data = &quirk_lenovo_ideapad_s205,
     },
     {
         .callback = dmi_matched,
         .ident = "Lenovo Ideapad S205 (Brazos)",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "LENOVO"),
             DMI_MATCH(DMI_PRODUCT_NAME, "Brazos"),
         },
         .driver_data = &quirk_lenovo_ideapad_s205,
     },
     {
         .callback = dmi_matched,
         .ident = "Lenovo 3000 N200",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "LENOVO"),
             DMI_MATCH(DMI_PRODUCT_NAME, "0687A31"),
         },
         .driver_data = &quirk_fujitsu_amilo_li_1718,
     },
     {
         .callback = dmi_matched,
         .ident = "Lenovo Ideapad S205-10382JG",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "LENOVO"),
             DMI_MATCH(DMI_PRODUCT_NAME, "10382JG"),
         },
         .driver_data = &quirk_lenovo_ideapad_s205,
     },
     {
         .callback = dmi_matched,
         .ident = "Lenovo Ideapad S205-1038DPG",
         .matches = {
             DMI_MATCH(DMI_SYS_VENDOR, "LENOVO"),
             DMI_MATCH(DMI_PRODUCT_NAME, "1038DPG"),
         },
         .driver_data = &quirk_lenovo_ideapad_s205,
     },
     {}
 };
 
 static struct device *platform_profile_device;
 static bool platform_profile_support;
 
 /*
  * The profile used before turbo mode. This variable is needed for
  * returning from turbo mode when the mode key is in toggle mode.
  */
  static int last_non_turbo_profile = INT_MIN;

  /* The most performant supported profile */
  static int acer_predator_v4_max_perf;
 
 enum acer_predator_v4_thermal_profile {
    ACER_PREDATOR_V4_THERMAL_PROFILE_QUIET		= 0x00,
    ACER_PREDATOR_V4_THERMAL_PROFILE_BALANCED	= 0x01,
    ACER_PREDATOR_V4_THERMAL_PROFILE_PERFORMANCE	= 0x04,
    ACER_PREDATOR_V4_THERMAL_PROFILE_TURBO		= 0x05,
    ACER_PREDATOR_V4_THERMAL_PROFILE_ECO		= 0x06,
};
 
 /* Find which quirks are needed for a particular vendor/ model pair */
 static void __init find_quirks(void)
 {
     if (predator_v4) {
         quirks = &quirk_acer_predator_v4;
     } else if (nitro_v4) {
         quirks = &quirk_acer_nitro_v4;
     } else if (!force_series) {
         dmi_check_system(acer_quirks);
         dmi_check_system(non_acer_quirks);
     } else if (force_series == 2490) {
         quirks = &quirk_acer_travelmate_2490;
     }
 
     if (quirks == NULL)
         quirks = &quirk_unknown;
 }
 
 /*
  * General interface convenience methods
  */
 
 static bool has_cap(u32 cap)
 {
     return interface->capability & cap;
 }
 
 /*
  * AMW0 (V1) interface
  */
 struct wmab_args {
     u32 eax;
     u32 ebx;
     u32 ecx;
     u32 edx;
 };
 
 struct wmab_ret {
     u32 eax;
     u32 ebx;
     u32 ecx;
     u32 edx;
     u32 eex;
 };
 
 static acpi_status wmab_execute(struct wmab_args *regbuf,
 struct acpi_buffer *result)
 {
     struct acpi_buffer input;
     acpi_status status;
     input.length = sizeof(struct wmab_args);
     input.pointer = (u8 *)regbuf;
 
     status = wmi_evaluate_method(AMW0_GUID1, 0, 1, &input, result);
 
     return status;
 }
 
 static acpi_status AMW0_get_u32(u32 *value, u32 cap)
 {
     int err;
     u8 result;
 
     switch (cap) {
     case ACER_CAP_MAILLED:
         switch (quirks->mailled) {
         default:
             err = ec_read(0xA, &result);
             if (err)
                 return AE_ERROR;
             *value = (result >> 7) & 0x1;
             return AE_OK;
         }
         break;
     case ACER_CAP_WIRELESS:
         switch (quirks->wireless) {
         case 1:
             err = ec_read(0x7B, &result);
             if (err)
                 return AE_ERROR;
             *value = result & 0x1;
             return AE_OK;
         case 2:
             err = ec_read(0x71, &result);
             if (err)
                 return AE_ERROR;
             *value = result & 0x1;
             return AE_OK;
         case 3:
             err = ec_read(0x78, &result);
             if (err)
                 return AE_ERROR;
             *value = result & 0x1;
             return AE_OK;
         default:
             err = ec_read(0xA, &result);
             if (err)
                 return AE_ERROR;
             *value = (result >> 2) & 0x1;
             return AE_OK;
         }
         break;
     case ACER_CAP_BLUETOOTH:
         switch (quirks->bluetooth) {
         default:
             err = ec_read(0xA, &result);
             if (err)
                 return AE_ERROR;
             *value = (result >> 4) & 0x1;
             return AE_OK;
         }
         break;
     case ACER_CAP_BRIGHTNESS:
         switch (quirks->brightness) {
         default:
             err = ec_read(0x83, &result);
             if (err)
                 return AE_ERROR;
             *value = result;
             return AE_OK;
         }
         break;
     default:
         return AE_ERROR;
     }
     return AE_OK;
 }
 
 static acpi_status AMW0_set_u32(u32 value, u32 cap)
 {
     struct wmab_args args;
 
     args.eax = ACER_AMW0_WRITE;
     args.ebx = value ? (1<<8) : 0;
     args.ecx = args.edx = 0;
 
     switch (cap) {
     case ACER_CAP_MAILLED:
         if (value > 1)
             return AE_BAD_PARAMETER;
         args.ebx |= ACER_AMW0_MAILLED_MASK;
         break;
     case ACER_CAP_WIRELESS:
         if (value > 1)
             return AE_BAD_PARAMETER;
         args.ebx |= ACER_AMW0_WIRELESS_MASK;
         break;
     case ACER_CAP_BLUETOOTH:
         if (value > 1)
             return AE_BAD_PARAMETER;
         args.ebx |= ACER_AMW0_BLUETOOTH_MASK;
         break;
     case ACER_CAP_BRIGHTNESS:
         if (value > max_brightness)
             return AE_BAD_PARAMETER;
         switch (quirks->brightness) {
         default:
             return ec_write(0x83, value);
         }
     default:
         return AE_ERROR;
     }
 
     /* Actually do the set */
     return wmab_execute(&args, NULL);
 }
 
 static acpi_status __init AMW0_find_mailled(void)
 {
     struct wmab_args args;
     struct wmab_ret ret;
     acpi_status status = AE_OK;
     struct acpi_buffer out = { ACPI_ALLOCATE_BUFFER, NULL };
     union acpi_object *obj;
 
     args.eax = 0x86;
     args.ebx = args.ecx = args.edx = 0;
 
     status = wmab_execute(&args, &out);
     if (ACPI_FAILURE(status))
         return status;
 
     obj = (union acpi_object *) out.pointer;
     if (obj && obj->type == ACPI_TYPE_BUFFER &&
     obj->buffer.length == sizeof(struct wmab_ret)) {
         ret = *((struct wmab_ret *) obj->buffer.pointer);
     } else {
         kfree(out.pointer);
         return AE_ERROR;
     }
 
     if (ret.eex & 0x1)
         interface->capability |= ACER_CAP_MAILLED;
 
     kfree(out.pointer);
 
     return AE_OK;
 }
 
 static const struct acpi_device_id norfkill_ids[] __initconst = {
     { "VPC2004", 0},
     { "IBM0068", 0},
     { "LEN0068", 0},
     { "SNY5001", 0},	/* sony-laptop in charge */
     { "HPQ6601", 0},
     { "", 0},
 };
 
 static int __init AMW0_set_cap_acpi_check_device(void)
 {
     const struct acpi_device_id *id;
 
     for (id = norfkill_ids; id->id[0]; id++)
         if (acpi_dev_found(id->id))
             return true;
 
     return false;
 }
 
 static acpi_status __init AMW0_set_capabilities(void)
 {
     struct wmab_args args;
     struct wmab_ret ret;
     acpi_status status;
     struct acpi_buffer out = { ACPI_ALLOCATE_BUFFER, NULL };
     union acpi_object *obj;
 
     /*
      * On laptops with this strange GUID (non Acer), normal probing doesn't
      * work.
      */
     if (wmi_has_guid(AMW0_GUID2)) {
         if ((quirks != &quirk_unknown) ||
             !AMW0_set_cap_acpi_check_device())
             interface->capability |= ACER_CAP_WIRELESS;
         return AE_OK;
     }
 
     args.eax = ACER_AMW0_WRITE;
     args.ecx = args.edx = 0;
 
     args.ebx = 0xa2 << 8;
     args.ebx |= ACER_AMW0_WIRELESS_MASK;
 
     status = wmab_execute(&args, &out);
     if (ACPI_FAILURE(status))
         return status;
 
     obj = out.pointer;
     if (obj && obj->type == ACPI_TYPE_BUFFER &&
     obj->buffer.length == sizeof(struct wmab_ret)) {
         ret = *((struct wmab_ret *) obj->buffer.pointer);
     } else {
         status = AE_ERROR;
         goto out;
     }
 
     if (ret.eax & 0x1)
         interface->capability |= ACER_CAP_WIRELESS;
 
     args.ebx = 2 << 8;
     args.ebx |= ACER_AMW0_BLUETOOTH_MASK;
 
     /*
      * It's ok to use existing buffer for next wmab_execute call.
      * But we need to kfree(out.pointer) if next wmab_execute fail.
      */
     status = wmab_execute(&args, &out);
     if (ACPI_FAILURE(status))
         goto out;
 
     obj = (union acpi_object *) out.pointer;
     if (obj && obj->type == ACPI_TYPE_BUFFER
     && obj->buffer.length == sizeof(struct wmab_ret)) {
         ret = *((struct wmab_ret *) obj->buffer.pointer);
     } else {
         status = AE_ERROR;
         goto out;
     }
 
     if (ret.eax & 0x1)
         interface->capability |= ACER_CAP_BLUETOOTH;
 
     /*
      * This appears to be safe to enable, since all Wistron based laptops
      * appear to use the same EC register for brightness, even if they
      * differ for wireless, etc
      */
     if (quirks->brightness >= 0)
         interface->capability |= ACER_CAP_BRIGHTNESS;
 
     status = AE_OK;
 out:
     kfree(out.pointer);
     return status;
 }
 
 static struct wmi_interface AMW0_interface = {
     .type = ACER_AMW0,
 };
 
 static struct wmi_interface AMW0_V2_interface = {
     .type = ACER_AMW0_V2,
 };
 
 /*
  * New interface (The WMID interface)
  */
 static acpi_status
 WMI_execute_u32(u32 method_id, u32 in, u32 *out)
 {
     struct acpi_buffer input = { (acpi_size) sizeof(u32), (void *)(&in) };
     struct acpi_buffer result = { ACPI_ALLOCATE_BUFFER, NULL };
     union acpi_object *obj;
     u32 tmp = 0;
     acpi_status status;
 
     status = wmi_evaluate_method(WMID_GUID1, 0, method_id, &input, &result);
 
     if (ACPI_FAILURE(status))
         return status;
 
     obj = (union acpi_object *) result.pointer;
     if (obj) {
         if (obj->type == ACPI_TYPE_BUFFER &&
             (obj->buffer.length == sizeof(u32) ||
             obj->buffer.length == sizeof(u64))) {
             tmp = *((u32 *) obj->buffer.pointer);
         } else if (obj->type == ACPI_TYPE_INTEGER) {
             tmp = (u32) obj->integer.value;
         }
     }
 
     if (out)
         *out = tmp;
 
     kfree(result.pointer);
 
     return status;
 }
 
 static acpi_status WMID_get_u32(u32 *value, u32 cap)
 {
     acpi_status status;
     u8 tmp;
     u32 result, method_id = 0;
 
     switch (cap) {
     case ACER_CAP_WIRELESS:
         method_id = ACER_WMID_GET_WIRELESS_METHODID;
         break;
     case ACER_CAP_BLUETOOTH:
         method_id = ACER_WMID_GET_BLUETOOTH_METHODID;
         break;
     case ACER_CAP_BRIGHTNESS:
         method_id = ACER_WMID_GET_BRIGHTNESS_METHODID;
         break;
     case ACER_CAP_THREEG:
         method_id = ACER_WMID_GET_THREEG_METHODID;
         break;
     case ACER_CAP_MAILLED:
         if (quirks->mailled == 1) {
             ec_read(0x9f, &tmp);
             *value = tmp & 0x1;
             return 0;
         }
         fallthrough;
     default:
         return AE_ERROR;
     }
     status = WMI_execute_u32(method_id, 0, &result);
 
     if (ACPI_SUCCESS(status))
         *value = (u8)result;
 
     return status;
 }
 
 static acpi_status WMID_set_u32(u32 value, u32 cap)
 {
     u32 method_id = 0;
     char param;
 
     switch (cap) {
     case ACER_CAP_BRIGHTNESS:
         if (value > max_brightness)
             return AE_BAD_PARAMETER;
         method_id = ACER_WMID_SET_BRIGHTNESS_METHODID;
         break;
     case ACER_CAP_WIRELESS:
         if (value > 1)
             return AE_BAD_PARAMETER;
         method_id = ACER_WMID_SET_WIRELESS_METHODID;
         break;
     case ACER_CAP_BLUETOOTH:
         if (value > 1)
             return AE_BAD_PARAMETER;
         method_id = ACER_WMID_SET_BLUETOOTH_METHODID;
         break;
     case ACER_CAP_THREEG:
         if (value > 1)
             return AE_BAD_PARAMETER;
         method_id = ACER_WMID_SET_THREEG_METHODID;
         break;
     case ACER_CAP_MAILLED:
         if (value > 1)
             return AE_BAD_PARAMETER;
         if (quirks->mailled == 1) {
             param = value ? 0x92 : 0x93;
             i8042_lock_chip();
             i8042_command(&param, 0x1059);
             i8042_unlock_chip();
             return 0;
         }
         break;
     default:
         return AE_ERROR;
     }
     return WMI_execute_u32(method_id, (u32)value, NULL);
 }
 
 static acpi_status wmid3_get_device_status(u32 *value, u16 device)
 {
     struct wmid3_gds_return_value return_value;
     acpi_status status;
     union acpi_object *obj;
     struct wmid3_gds_get_input_param params = {
         .function_num = 0x1,
         .hotkey_number = commun_fn_key_number,
         .devices = device,
     };
     struct acpi_buffer input = {
         sizeof(struct wmid3_gds_get_input_param),
         &params
     };
     struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
 
     status = wmi_evaluate_method(WMID_GUID3, 0, 0x2, &input, &output);
     if (ACPI_FAILURE(status))
         return status;
 
     obj = output.pointer;
 
     if (!obj)
         return AE_ERROR;
     else if (obj->type != ACPI_TYPE_BUFFER) {
         kfree(obj);
         return AE_ERROR;
     }
     if (obj->buffer.length != 8) {
         pr_warn("Unknown buffer length %d\n", obj->buffer.length);
         kfree(obj);
         return AE_ERROR;
     }
 
     return_value = *((struct wmid3_gds_return_value *)obj->buffer.pointer);
     kfree(obj);
 
     if (return_value.error_code || return_value.ec_return_value)
         pr_warn("Get 0x%x Device Status failed: 0x%x - 0x%x\n",
             device,
             return_value.error_code,
             return_value.ec_return_value);
     else
         *value = !!(return_value.devices & device);
 
     return status;
 }
 
 static acpi_status wmid_v2_get_u32(u32 *value, u32 cap)
 {
     u16 device;
 
     switch (cap) {
     case ACER_CAP_WIRELESS:
         device = ACER_WMID3_GDS_WIRELESS;
         break;
     case ACER_CAP_BLUETOOTH:
         device = ACER_WMID3_GDS_BLUETOOTH;
         break;
     case ACER_CAP_THREEG:
         device = ACER_WMID3_GDS_THREEG;
         break;
     default:
         return AE_ERROR;
     }
     return wmid3_get_device_status(value, device);
 }
 
 static acpi_status wmid3_set_device_status(u32 value, u16 device)
 {
     struct wmid3_gds_return_value return_value;
     acpi_status status;
     union acpi_object *obj;
     u16 devices;
     struct wmid3_gds_get_input_param get_params = {
         .function_num = 0x1,
         .hotkey_number = commun_fn_key_number,
         .devices = commun_func_bitmap,
     };
     struct acpi_buffer get_input = {
         sizeof(struct wmid3_gds_get_input_param),
         &get_params
     };
     struct wmid3_gds_set_input_param set_params = {
         .function_num = 0x2,
         .hotkey_number = commun_fn_key_number,
         .devices = commun_func_bitmap,
     };
     struct acpi_buffer set_input = {
         sizeof(struct wmid3_gds_set_input_param),
         &set_params
     };
     struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
     struct acpi_buffer output2 = { ACPI_ALLOCATE_BUFFER, NULL };
 
     status = wmi_evaluate_method(WMID_GUID3, 0, 0x2, &get_input, &output);
     if (ACPI_FAILURE(status))
         return status;
 
     obj = output.pointer;
 
     if (!obj)
         return AE_ERROR;
     else if (obj->type != ACPI_TYPE_BUFFER) {
         kfree(obj);
         return AE_ERROR;
     }
     if (obj->buffer.length != 8) {
         pr_warn("Unknown buffer length %d\n", obj->buffer.length);
         kfree(obj);
         return AE_ERROR;
     }
 
     return_value = *((struct wmid3_gds_return_value *)obj->buffer.pointer);
     kfree(obj);
 
     if (return_value.error_code || return_value.ec_return_value) {
         pr_warn("Get Current Device Status failed: 0x%x - 0x%x\n",
             return_value.error_code,
             return_value.ec_return_value);
         return status;
     }
 
     devices = return_value.devices;
     set_params.devices = (value) ? (devices | device) : (devices & ~device);
 
     status = wmi_evaluate_method(WMID_GUID3, 0, 0x1, &set_input, &output2);
     if (ACPI_FAILURE(status))
         return status;
 
     obj = output2.pointer;
 
     if (!obj)
         return AE_ERROR;
     else if (obj->type != ACPI_TYPE_BUFFER) {
         kfree(obj);
         return AE_ERROR;
     }
     if (obj->buffer.length != 4) {
         pr_warn("Unknown buffer length %d\n", obj->buffer.length);
         kfree(obj);
         return AE_ERROR;
     }
 
     return_value = *((struct wmid3_gds_return_value *)obj->buffer.pointer);
     kfree(obj);
 
     if (return_value.error_code || return_value.ec_return_value)
         pr_warn("Set Device Status failed: 0x%x - 0x%x\n",
             return_value.error_code,
             return_value.ec_return_value);
 
     return status;
 }
 
 static acpi_status wmid_v2_set_u32(u32 value, u32 cap)
 {
     u16 device;
 
     switch (cap) {
     case ACER_CAP_WIRELESS:
         device = ACER_WMID3_GDS_WIRELESS;
         break;
     case ACER_CAP_BLUETOOTH:
         device = ACER_WMID3_GDS_BLUETOOTH;
         break;
     case ACER_CAP_THREEG:
         device = ACER_WMID3_GDS_THREEG;
         break;
     default:
         return AE_ERROR;
     }
     return wmid3_set_device_status(value, device);
 }
 
 static void __init type_aa_dmi_decode(const struct dmi_header *header, void *d)
 {
     struct hotkey_function_type_aa *type_aa;
 
     /* We are looking for OEM-specific Type AAh */
     if (header->type != 0xAA)
         return;
 
     has_type_aa = true;
     type_aa = (struct hotkey_function_type_aa *) header;
 
     pr_info("Function bitmap for Communication Button: 0x%x\n",
         type_aa->commun_func_bitmap);
     commun_func_bitmap = type_aa->commun_func_bitmap;
 
     if (type_aa->commun_func_bitmap & ACER_WMID3_GDS_WIRELESS)
         interface->capability |= ACER_CAP_WIRELESS;
     if (type_aa->commun_func_bitmap & ACER_WMID3_GDS_THREEG)
         interface->capability |= ACER_CAP_THREEG;
     if (type_aa->commun_func_bitmap & ACER_WMID3_GDS_BLUETOOTH)
         interface->capability |= ACER_CAP_BLUETOOTH;
     if (type_aa->commun_func_bitmap & ACER_WMID3_GDS_RFBTN)
         commun_func_bitmap &= ~ACER_WMID3_GDS_RFBTN;
 
     commun_fn_key_number = type_aa->commun_fn_key_number;
 }
 
 static acpi_status __init WMID_set_capabilities(void)
 {
     struct acpi_buffer out = {ACPI_ALLOCATE_BUFFER, NULL};
     union acpi_object *obj;
     acpi_status status;
     u32 devices;
 
     status = wmi_query_block(WMID_GUID2, 0, &out);
     if (ACPI_FAILURE(status))
         return status;
 
     obj = (union acpi_object *) out.pointer;
     if (obj) {
         if (obj->type == ACPI_TYPE_BUFFER &&
             (obj->buffer.length == sizeof(u32) ||
             obj->buffer.length == sizeof(u64))) {
             devices = *((u32 *) obj->buffer.pointer);
         } else if (obj->type == ACPI_TYPE_INTEGER) {
             devices = (u32) obj->integer.value;
         } else {
             kfree(out.pointer);
             return AE_ERROR;
         }
     } else {
         kfree(out.pointer);
         return AE_ERROR;
     }
 
     pr_info("Function bitmap for Communication Device: 0x%x\n", devices);
     if (devices & 0x07)
         interface->capability |= ACER_CAP_WIRELESS;
     if (devices & 0x40)
         interface->capability |= ACER_CAP_THREEG;
     if (devices & 0x10)
         interface->capability |= ACER_CAP_BLUETOOTH;
 
     if (!(devices & 0x20))
         max_brightness = 0x9;
 
     kfree(out.pointer);
     return status;
 }
 
 static struct wmi_interface wmid_interface = {
     .type = ACER_WMID,
 };
 
 static struct wmi_interface wmid_v2_interface = {
     .type = ACER_WMID_v2,
 };
 
 /*
  * WMID ApgeAction interface
  */
 
 static acpi_status
 WMI_apgeaction_execute_u64(u32 method_id, u64 in, u64 *out){
     struct acpi_buffer input = { (acpi_size) sizeof(u64), (void *)(&in) };
     struct acpi_buffer result = { ACPI_ALLOCATE_BUFFER, NULL };
     union acpi_object *obj;
     u64 tmp = 0;
     acpi_status status;
     status = wmi_evaluate_method(WMID_GUID3, 0, method_id, &input, &result);
 
     if (ACPI_FAILURE(status))
         return status;
     obj = (union acpi_object *) result.pointer;
 
     if (obj) {
         if (obj->type == ACPI_TYPE_BUFFER) {
             if (obj->buffer.length == sizeof(u32))
                 tmp = *((u32 *) obj->buffer.pointer);
             else if (obj->buffer.length == sizeof(u64))
                 tmp = *((u64 *) obj->buffer.pointer);
         } else if (obj->type == ACPI_TYPE_INTEGER) {
             tmp = (u64) obj->integer.value;
         }
     }
 
     if (out)
         *out = tmp;
 
     kfree(result.pointer);
 
     return status;
 }
 /*
  * WMID Gaming interface
  */
 
 static acpi_status
 WMI_gaming_execute_u64(u32 method_id, u64 in, u64 *out)
 {
     struct acpi_buffer input = { (acpi_size) sizeof(u64), (void *)(&in) };
     struct acpi_buffer result = { ACPI_ALLOCATE_BUFFER, NULL };
     union acpi_object *obj;
     u64 tmp = 0;
     acpi_status status;
 
     status = wmi_evaluate_method(WMID_GUID4, 0, method_id, &input, &result);
 
     if (ACPI_FAILURE(status))
         return status;
     obj = (union acpi_object *) result.pointer;
 
     if (obj) {
         if (obj->type == ACPI_TYPE_BUFFER) {
             if (obj->buffer.length == sizeof(u32))
                 tmp = *((u32 *) obj->buffer.pointer);
             else if (obj->buffer.length == sizeof(u64))
                 tmp = *((u64 *) obj->buffer.pointer);
         } else if (obj->type == ACPI_TYPE_INTEGER) {
             tmp = (u64) obj->integer.value;
         }
     }
 
     if (out)
         *out = tmp;
 
     kfree(result.pointer);
 
     return status;
 }
 
 static int WMI_gaming_execute_u32_u64(u32 method_id, u32 in, u64 *out)
 {
     struct acpi_buffer result = { ACPI_ALLOCATE_BUFFER, NULL };
     struct acpi_buffer input = {
         .length = sizeof(in),
         .pointer = &in,
     };
     union acpi_object *obj;
     acpi_status status;
     int ret = 0;
 
     status = wmi_evaluate_method(WMID_GUID4, 0, method_id, &input, &result);
     if (ACPI_FAILURE(status))
         return -EIO;
 
     obj = result.pointer;
     if (obj && out) {
         switch (obj->type) {
         case ACPI_TYPE_INTEGER:
             *out = obj->integer.value;
             break;
         case ACPI_TYPE_BUFFER:
             if (obj->buffer.length < sizeof(*out))
                 ret = -ENOMSG;
             else
                 *out = get_unaligned_le64(obj->buffer.pointer);
 
             break;
         default:
             ret = -ENOMSG;
             break;
         }
     }
 
     kfree(obj);
 
     return ret;
 }

 static acpi_status WMID_gaming_set_u64(u64 value, u32 cap)
 {
     u32 method_id = 0;
 
     if (!(interface->capability & cap))
         return AE_BAD_PARAMETER;
 
     switch (cap) {
     case ACER_CAP_TURBO_LED:
         method_id = ACER_WMID_SET_GAMING_LED_METHODID;
         break;
     case ACER_CAP_TURBO_FAN:
         method_id = ACER_WMID_SET_GAMING_FAN_BEHAVIOR_METHODID;
         break;
     default:
         return AE_BAD_PARAMETER;
     }
 
     return WMI_gaming_execute_u64(method_id, value, NULL);
 }
 
 static acpi_status WMID_gaming_get_u64(u64 *value, u32 cap)
 {
     acpi_status status;
     u64 result;
     u64 input;
     u32 method_id;
 
     if (!(interface->capability & cap))
         return AE_BAD_PARAMETER;
 
     switch (cap) {
     case ACER_CAP_TURBO_LED:
         method_id = ACER_WMID_GET_GAMING_LED_METHODID;
         input = 0x1;
         break;
     default:
         return AE_BAD_PARAMETER;
     }
     status = WMI_gaming_execute_u64(method_id, input, &result);
     if (ACPI_SUCCESS(status))
         *value = (u64) result;
 
     return status;
 }
 
 static int WMID_gaming_get_sys_info(u32 command, u64 *out)
 {
     acpi_status status;
     u64 result;
 
     status = WMI_gaming_execute_u64(ACER_WMID_GET_GAMING_SYS_INFO_METHODID, command, &result);
     if (ACPI_FAILURE(status))
         return -EIO;
 
     /* The return status must be zero for the operation to have succeeded */
     if (FIELD_GET(ACER_PREDATOR_V4_RETURN_STATUS_BIT_MASK, result))
         return -EIO;
 
     *out = result;
 
     return 0;
 }

 static void WMID_gaming_set_fan_mode(u8 fan_mode)
 {
     /* fan_mode = 1 is used for auto, fan_mode = 2 used for turbo*/
     u64 gpu_fan_config1 = 0, gpu_fan_config2 = 0;
     int i;
 
     if (quirks->cpu_fans > 0)
         gpu_fan_config2 |= 1;
     for (i = 0; i < (quirks->cpu_fans + quirks->gpu_fans); ++i)
         gpu_fan_config2 |= 1 << (i + 1);
     for (i = 0; i < quirks->gpu_fans; ++i)
         gpu_fan_config2 |= 1 << (i + 3);
     if (quirks->cpu_fans > 0)
         gpu_fan_config1 |= fan_mode;
     for (i = 0; i < (quirks->cpu_fans + quirks->gpu_fans); ++i)
         gpu_fan_config1 |= fan_mode << (2 * i + 2);
     for (i = 0; i < quirks->gpu_fans; ++i)
         gpu_fan_config1 |= fan_mode << (2 * i + 6);
     WMID_gaming_set_u64(gpu_fan_config2 | gpu_fan_config1 << 16, ACER_CAP_TURBO_FAN);
 }

 static int WMID_gaming_set_misc_setting(enum acer_wmi_gaming_misc_setting setting, u8 value)
 {
     acpi_status status;
     u64 input = 0;
     u64 result;
 
     input |= FIELD_PREP(ACER_GAMING_MISC_SETTING_INDEX_MASK, setting);
     input |= FIELD_PREP(ACER_GAMING_MISC_SETTING_VALUE_MASK, value);
 
     status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_MISC_SETTING_METHODID, input, &result);
     if (ACPI_FAILURE(status))
         return -EIO;
 
     /* The return status must be zero for the operation to have succeeded */
     if (FIELD_GET(ACER_GAMING_MISC_SETTING_STATUS_MASK, result))
         return -EIO;
 
     return 0;
 }
 
 static int WMID_gaming_get_misc_setting(enum acer_wmi_gaming_misc_setting setting, u8 *value)
 {
     u64 input = 0;
     u64 result;
     int ret;
 
     input |= FIELD_PREP(ACER_GAMING_MISC_SETTING_INDEX_MASK, setting);
 
     ret = WMI_gaming_execute_u32_u64(ACER_WMID_GET_GAMING_MISC_SETTING_METHODID, input,
                      &result);
     if (ret < 0)
         return ret;
 
     /* The return status must be zero for the operation to have succeeded */
     if (FIELD_GET(ACER_GAMING_MISC_SETTING_STATUS_MASK, result))
         return -EIO;
 
     *value = FIELD_GET(ACER_GAMING_MISC_SETTING_VALUE_MASK, result);
 
     return 0;
 }

 /*
  * Generic Device (interface-independent)
  */
 
 static acpi_status get_u32(u32 *value, u32 cap)
 {
     acpi_status status = AE_ERROR;
 
     switch (interface->type) {
     case ACER_AMW0:
         status = AMW0_get_u32(value, cap);
         break;
     case ACER_AMW0_V2:
         if (cap == ACER_CAP_MAILLED) {
             status = AMW0_get_u32(value, cap);
             break;
         }
         fallthrough;
     case ACER_WMID:
         status = WMID_get_u32(value, cap);
         break;
     case ACER_WMID_v2:
         if (cap & (ACER_CAP_WIRELESS |
                ACER_CAP_BLUETOOTH |
                ACER_CAP_THREEG))
             status = wmid_v2_get_u32(value, cap);
         else if (wmi_has_guid(WMID_GUID2))
             status = WMID_get_u32(value, cap);
         break;
     }
 
     return status;
 }
 
 static acpi_status set_u32(u32 value, u32 cap)
 {
     acpi_status status;
 
     if (interface->capability & cap) {
         switch (interface->type) {
         case ACER_AMW0:
             return AMW0_set_u32(value, cap);
         case ACER_AMW0_V2:
             if (cap == ACER_CAP_MAILLED)
                 return AMW0_set_u32(value, cap);
 
             /*
              * On some models, some WMID methods don't toggle
              * properly. For those cases, we want to run the AMW0
              * method afterwards to be certain we've really toggled
              * the device state.
              */
             if (cap == ACER_CAP_WIRELESS ||
                 cap == ACER_CAP_BLUETOOTH) {
                 status = WMID_set_u32(value, cap);
                 if (ACPI_FAILURE(status))
                     return status;
 
                 return AMW0_set_u32(value, cap);
             }
             fallthrough;
         case ACER_WMID:
             return WMID_set_u32(value, cap);
         case ACER_WMID_v2:
             if (cap & (ACER_CAP_WIRELESS |
                    ACER_CAP_BLUETOOTH |
                    ACER_CAP_THREEG))
                 return wmid_v2_set_u32(value, cap);
             else if (wmi_has_guid(WMID_GUID2))
                 return WMID_set_u32(value, cap);
             fallthrough;
         default:
             return AE_BAD_PARAMETER;
         }
     }
     return AE_BAD_PARAMETER;
 }
 
 static void __init acer_commandline_init(void)
 {
     /*
      * These will all fail silently if the value given is invalid, or the
      * capability isn't available on the given interface
      */
     if (mailled >= 0)
         set_u32(mailled, ACER_CAP_MAILLED);
     if (!has_type_aa && threeg >= 0)
         set_u32(threeg, ACER_CAP_THREEG);
     if (brightness >= 0)
         set_u32(brightness, ACER_CAP_BRIGHTNESS);
 }
 
 /*
  * LED device (Mail LED only, no other LEDs known yet)
  */
 static void mail_led_set(struct led_classdev *led_cdev,
 enum led_brightness value)
 {
     set_u32(value, ACER_CAP_MAILLED);
 }
 
 static struct led_classdev mail_led = {
     .name = "acer-wmi::mail",
     .brightness_set = mail_led_set,
 };
 
 static int acer_led_init(struct device *dev)
 {
     return led_classdev_register(dev, &mail_led);
 }
 
 static void acer_led_exit(void)
 {
     set_u32(LED_OFF, ACER_CAP_MAILLED);
     led_classdev_unregister(&mail_led);
 }
 
 /*
  * Backlight device
  */
 static struct backlight_device *acer_backlight_device;
 
 static int read_brightness(struct backlight_device *bd)
 {
     u32 value;
     get_u32(&value, ACER_CAP_BRIGHTNESS);
     return value;
 }
 
 static int update_bl_status(struct backlight_device *bd)
 {
     int intensity = backlight_get_brightness(bd);
 
     set_u32(intensity, ACER_CAP_BRIGHTNESS);
 
     return 0;
 }
 
 static const struct backlight_ops acer_bl_ops = {
     .get_brightness = read_brightness,
     .update_status = update_bl_status,
 };
 
 static int acer_backlight_init(struct device *dev)
 {
     struct backlight_properties props;
     struct backlight_device *bd;
 
     memset(&props, 0, sizeof(struct backlight_properties));
     props.type = BACKLIGHT_PLATFORM;
     props.max_brightness = max_brightness;
     bd = backlight_device_register("acer-wmi", dev, NULL, &acer_bl_ops,
                        &props);
     if (IS_ERR(bd)) {
         pr_err("Could not register Acer backlight device\n");
         acer_backlight_device = NULL;
         return PTR_ERR(bd);
     }
 
     acer_backlight_device = bd;
 
     bd->props.power = BACKLIGHT_POWER_ON;
     bd->props.brightness = read_brightness(bd);
     backlight_update_status(bd);
     return 0;
 }
 
 static void acer_backlight_exit(void)
 {
     backlight_device_unregister(acer_backlight_device);
 }
 
 /*
  * Accelerometer device
  */
 static acpi_handle gsensor_handle;
 
 static int acer_gsensor_init(void)
 {
     acpi_status status;
     struct acpi_buffer output;
     union acpi_object out_obj;
 
     output.length = sizeof(out_obj);
     output.pointer = &out_obj;
     status = acpi_evaluate_object(gsensor_handle, "_INI", NULL, &output);
     if (ACPI_FAILURE(status))
         return -1;
 
     return 0;
 }
 
 static int acer_gsensor_open(struct input_dev *input)
 {
     return acer_gsensor_init();
 }
 
 static int acer_gsensor_event(void)
 {
     acpi_status status;
     struct acpi_buffer output;
     union acpi_object out_obj[5];
 
     if (!acer_wmi_accel_dev)
         return -1;
 
     output.length = sizeof(out_obj);
     output.pointer = out_obj;
 
     status = acpi_evaluate_object(gsensor_handle, "RDVL", NULL, &output);
     if (ACPI_FAILURE(status))
         return -1;
 
     if (out_obj->package.count != 4)
         return -1;
 
     input_report_abs(acer_wmi_accel_dev, ABS_X,
         (s16)out_obj->package.elements[0].integer.value);
     input_report_abs(acer_wmi_accel_dev, ABS_Y,
         (s16)out_obj->package.elements[1].integer.value);
     input_report_abs(acer_wmi_accel_dev, ABS_Z,
         (s16)out_obj->package.elements[2].integer.value);
     input_sync(acer_wmi_accel_dev);
     return 0;
 }
 /* Fan Speed */
 static acpi_status acer_set_fan_speed(int t_cpu_fan_speed, int t_gpu_fan_speed);
 
//  static int acer_get_fan_speed(int fan) {
//      if (quirks->predator_v4 || quirks->nitro_sense) {
//          acpi_status status;
//          u64 fanspeed;
 
//          status = WMI_gaming_execute_u64(
//              ACER_WMID_GET_GAMING_SYS_INFO_METHODID,
//              fan == 0 ? ACER_WMID_CMD_GET_PREDATOR_V4_CPU_FAN_SPEED :
//                     ACER_WMID_CMD_GET_PREDATOR_V4_GPU_FAN_SPEED,
//              &fanspeed);
 
//          if (ACPI_FAILURE(status))
//              return -EIO;
 
//          return FIELD_GET(ACER_PREDATOR_V4_FAN_SPEED_READ_BIT_MASK, fanspeed);
//      }
//      return -EOPNOTSUPP;
//  }
 
 /*
  *  Predator series turbo button
  */
 static int acer_toggle_turbo(void)
 {
     u64 turbo_led_state;
 
     /* Get current state from turbo button */
     if (ACPI_FAILURE(WMID_gaming_get_u64(&turbo_led_state, ACER_CAP_TURBO_LED)))
         return -1;
 
     if (turbo_led_state) {
         /* Turn off turbo led */
         WMID_gaming_set_u64(0x1, ACER_CAP_TURBO_LED);
 
         /* Set FAN mode to auto */
         WMID_gaming_set_fan_mode(0x1);
 
         /* Set OC to normal */
         if (has_cap(ACER_CAP_TURBO_OC)) {
             WMID_gaming_set_misc_setting(ACER_WMID_MISC_SETTING_OC_1,
                              ACER_WMID_OC_NORMAL);
             WMID_gaming_set_misc_setting(ACER_WMID_MISC_SETTING_OC_2,
                              ACER_WMID_OC_NORMAL);
         }
     } else {
         /* Turn on turbo led */
         WMID_gaming_set_u64(0x10001, ACER_CAP_TURBO_LED);
 
         /* Set FAN mode to turbo */
         WMID_gaming_set_fan_mode(0x2);
 
         /* Set OC to turbo mode */
         if (has_cap(ACER_CAP_TURBO_OC)) {
             WMID_gaming_set_misc_setting(ACER_WMID_MISC_SETTING_OC_1,
                              ACER_WMID_OC_TURBO);
             WMID_gaming_set_misc_setting(ACER_WMID_MISC_SETTING_OC_2,
                              ACER_WMID_OC_TURBO);
         }
     }
     return turbo_led_state;
 }
 
 static int
 acer_predator_v4_platform_profile_get(struct device *dev,
                       enum platform_profile_option *profile)
 {
     u8 tp;
     int err;
 
     err = WMID_gaming_get_misc_setting(ACER_WMID_MISC_SETTING_PLATFORM_PROFILE, &tp);
     if (err)
         return err;
 
     switch (tp) {
     case ACER_PREDATOR_V4_THERMAL_PROFILE_TURBO:
         *profile = PLATFORM_PROFILE_PERFORMANCE;
         break;
     case ACER_PREDATOR_V4_THERMAL_PROFILE_PERFORMANCE:
         *profile = PLATFORM_PROFILE_BALANCED_PERFORMANCE;
         break;
     case ACER_PREDATOR_V4_THERMAL_PROFILE_BALANCED:
         *profile = PLATFORM_PROFILE_BALANCED;
         break;
     case ACER_PREDATOR_V4_THERMAL_PROFILE_QUIET:
         *profile = PLATFORM_PROFILE_QUIET;
         break;
     case ACER_PREDATOR_V4_THERMAL_PROFILE_ECO:
         *profile = PLATFORM_PROFILE_LOW_POWER;
         break;
     default:
         return -EOPNOTSUPP;
     }
 
     return 0;
 }
 
 static int
 acer_predator_v4_platform_profile_set(struct device *dev,
                       enum platform_profile_option profile)
 {
     int err,tp;
     acpi_status status;
     u64 on_AC;
 
     /* Check Power Source */
     status = WMI_gaming_execute_u64(
         ACER_WMID_GET_GAMING_SYS_INFO_METHODID,
         ACER_WMID_CMD_GET_PREDATOR_V4_BAT_STATUS, &on_AC);
 
     if (ACPI_FAILURE(status))
         return -EIO;
 
     /* Check power source */
     /* Blocking these modes since in official version this is not supported when its not plugged in AC! */
     if(!on_AC && (profile == PLATFORM_PROFILE_PERFORMANCE || profile == PLATFORM_PROFILE_BALANCED_PERFORMANCE || profile == PLATFORM_PROFILE_QUIET)){
         return -EOPNOTSUPP;
     }
 
     /* turn the fan down i mean its quiet mode | eco mode after all*/
     if(profile == PLATFORM_PROFILE_QUIET || profile == PLATFORM_PROFILE_LOW_POWER) {
         acpi_status stat = acer_set_fan_speed(0,0);
         if(ACPI_FAILURE(stat)){
             return -EIO;
         }
     }
 
     switch (profile) {
     case PLATFORM_PROFILE_PERFORMANCE:
         tp = ACER_PREDATOR_V4_THERMAL_PROFILE_TURBO;
         break;
     case PLATFORM_PROFILE_BALANCED_PERFORMANCE:
         tp = ACER_PREDATOR_V4_THERMAL_PROFILE_PERFORMANCE;
         break;
     case PLATFORM_PROFILE_BALANCED:
         tp = ACER_PREDATOR_V4_THERMAL_PROFILE_BALANCED;
         break;
     case PLATFORM_PROFILE_QUIET:
         tp = ACER_PREDATOR_V4_THERMAL_PROFILE_QUIET;
         break;
     case PLATFORM_PROFILE_LOW_POWER:
         tp = ACER_PREDATOR_V4_THERMAL_PROFILE_ECO;
         break;
     default:
         return -EOPNOTSUPP;
     }
 
     err = WMID_gaming_set_misc_setting(ACER_WMID_MISC_SETTING_PLATFORM_PROFILE, tp);
     if (err)
         return err;
 
     if (tp != acer_predator_v4_max_perf)
         last_non_turbo_profile = tp;
 
     return 0;
 }
 
 static int
 acer_predator_v4_platform_profile_probe(void *drvdata, unsigned long *choices)
 {
     unsigned long supported_profiles;
     int err;
 
     err = WMID_gaming_get_misc_setting(ACER_WMID_MISC_SETTING_SUPPORTED_PROFILES,
                        (u8 *)&supported_profiles);
     if (err)
         return err;
 
     /* Iterate through supported profiles in order of increasing performance */
     if (test_bit(ACER_PREDATOR_V4_THERMAL_PROFILE_ECO, &supported_profiles)) {
         set_bit(PLATFORM_PROFILE_LOW_POWER, choices);
         acer_predator_v4_max_perf = ACER_PREDATOR_V4_THERMAL_PROFILE_ECO;
         last_non_turbo_profile = ACER_PREDATOR_V4_THERMAL_PROFILE_ECO;
     }
 
     if (test_bit(ACER_PREDATOR_V4_THERMAL_PROFILE_QUIET, &supported_profiles)) {
         set_bit(PLATFORM_PROFILE_QUIET, choices);
         acer_predator_v4_max_perf = ACER_PREDATOR_V4_THERMAL_PROFILE_QUIET;
         last_non_turbo_profile = ACER_PREDATOR_V4_THERMAL_PROFILE_QUIET;
     }
 
     if (test_bit(ACER_PREDATOR_V4_THERMAL_PROFILE_BALANCED, &supported_profiles)) {
         set_bit(PLATFORM_PROFILE_BALANCED, choices);
         acer_predator_v4_max_perf = ACER_PREDATOR_V4_THERMAL_PROFILE_BALANCED;
         last_non_turbo_profile = ACER_PREDATOR_V4_THERMAL_PROFILE_BALANCED;
     }
 
     if (test_bit(ACER_PREDATOR_V4_THERMAL_PROFILE_PERFORMANCE, &supported_profiles)) {
         set_bit(PLATFORM_PROFILE_BALANCED_PERFORMANCE, choices);
         acer_predator_v4_max_perf = ACER_PREDATOR_V4_THERMAL_PROFILE_PERFORMANCE;
 
         /* We only use this profile as a fallback option in case no prior
          * profile is supported.
          */
         if (last_non_turbo_profile < 0)
             last_non_turbo_profile = ACER_PREDATOR_V4_THERMAL_PROFILE_PERFORMANCE;
     }
 
     if (test_bit(ACER_PREDATOR_V4_THERMAL_PROFILE_TURBO, &supported_profiles)) {
         set_bit(PLATFORM_PROFILE_PERFORMANCE, choices);
         acer_predator_v4_max_perf = ACER_PREDATOR_V4_THERMAL_PROFILE_TURBO;
 
         /* We need to handle the hypothetical case where only the turbo profile
          * is supported. In this case the turbo toggle will essentially be a
          * no-op.
          */
         if (last_non_turbo_profile < 0)
             last_non_turbo_profile = ACER_PREDATOR_V4_THERMAL_PROFILE_TURBO;
     }
 
     return 0;
 }
 
 static int acer_predator_state_update(int value);
 
 static acpi_status acer_predator_state_restore(int value);
 
 static acpi_status battery_health_set(u8 function, u8 function_status);
 
 static const struct platform_profile_ops acer_predator_v4_platform_profile_ops = {
     .probe = acer_predator_v4_platform_profile_probe,
     .profile_get = acer_predator_v4_platform_profile_get,
     .profile_set = acer_predator_v4_platform_profile_set,
 };
 
 static int acer_platform_profile_setup(struct platform_device *pdev)
 {
     const int max_retries = 10;
     int delay_ms = 100;
     if (!quirks->predator_v4 && !quirks->nitro_sense && !quirks->nitro_v4)
         return 0;
     for (int attempt = 1; attempt <= max_retries; attempt++) {
         platform_profile_device = devm_platform_profile_register(
             &pdev->dev, "acer-wmi", NULL, &acer_predator_v4_platform_profile_ops);
         if (!IS_ERR(platform_profile_device)) {
             platform_profile_support = true;
             pr_info("Platform profile registered successfully (attempt %d)\n", attempt);
             return 0;
         }
         pr_warn("Platform profile registration failed (attempt %d/%d), error: %ld\n",
                 attempt, max_retries, PTR_ERR(platform_profile_device));
         if (attempt < max_retries) {
             msleep(delay_ms);
             delay_ms = min(delay_ms * 2, 1000);
         }
     }
     return PTR_ERR(platform_profile_device);
 }
 
 static int acer_thermal_profile_change(void)
 {
     /*
      * This mode key can rotate each mode or toggle turbo mode.
      * On battery, only ECO and BALANCED mode are available.
      */
     if (quirks->predator_v4 || quirks->nitro_sense || quirks->nitro_v4) {
         u8 current_tp;
         int tp, err;
         u64 on_AC;
         acpi_status status;
         err = WMID_gaming_get_misc_setting(ACER_WMID_MISC_SETTING_PLATFORM_PROFILE, &current_tp);
         if (err)
             return err;
         /* Check power source */
         status = WMI_gaming_execute_u64(
             ACER_WMID_GET_GAMING_SYS_INFO_METHODID,
             ACER_WMID_CMD_GET_PREDATOR_V4_BAT_STATUS, &on_AC);
         
         if (ACPI_FAILURE(status))
             return -EIO;
         
         /* On AC - define next profile transitions */
         if (!on_AC) {
            if (current_tp == ACER_PREDATOR_V4_THERMAL_PROFILE_ECO)
                tp = ACER_PREDATOR_V4_THERMAL_PROFILE_BALANCED;
            else
                tp = ACER_PREDATOR_V4_THERMAL_PROFILE_ECO;
        } else {
            switch (current_tp) {
            case ACER_PREDATOR_V4_THERMAL_PROFILE_TURBO:
                tp = cycle_gaming_thermal_profile
                     ? ACER_PREDATOR_V4_THERMAL_PROFILE_QUIET
                     : last_non_turbo_profile;
                break;
            case ACER_PREDATOR_V4_THERMAL_PROFILE_PERFORMANCE:
                tp = (acer_predator_v4_max_perf == current_tp)
                     ? last_non_turbo_profile
                     : acer_predator_v4_max_perf;
                break;
            case ACER_PREDATOR_V4_THERMAL_PROFILE_BALANCED:
                tp = cycle_gaming_thermal_profile
                     ? ACER_PREDATOR_V4_THERMAL_PROFILE_PERFORMANCE
                     : acer_predator_v4_max_perf;
                break;
            case ACER_PREDATOR_V4_THERMAL_PROFILE_QUIET:
                tp = cycle_gaming_thermal_profile
                     ? ACER_PREDATOR_V4_THERMAL_PROFILE_BALANCED
                     : acer_predator_v4_max_perf;
                break;
            case ACER_PREDATOR_V4_THERMAL_PROFILE_ECO:
                tp = cycle_gaming_thermal_profile
                     ? ACER_PREDATOR_V4_THERMAL_PROFILE_QUIET
                     : acer_predator_v4_max_perf;
                break;
            default:
                return -EOPNOTSUPP;
            }
        }

         err = WMID_gaming_set_misc_setting(ACER_WMID_MISC_SETTING_PLATFORM_PROFILE, tp);
         if (err)
             return err;
 
         /* the quiter you become the more you'll be able to hear! */
         if(tp == ACER_PREDATOR_V4_THERMAL_PROFILE_QUIET || tp == ACER_PREDATOR_V4_THERMAL_PROFILE_ECO) {
             acpi_status stat = acer_set_fan_speed(0,0);
             if(ACPI_FAILURE(stat)){
                 return -EIO;
             }
         }
         /* Store non-turbo profile for turbo mode toggle*/
         if (tp != acer_predator_v4_max_perf)
             last_non_turbo_profile = tp;
 
         platform_profile_notify(platform_profile_device);
     }
 
     return 0;
 }
 
 /*
  * Switch series keyboard dock status
  */
 static int acer_kbd_dock_state_to_sw_tablet_mode(u8 kbd_dock_state)
 {
     switch (kbd_dock_state) {
     case 0x01: /* Docked, traditional clamshell laptop mode */
         return 0;
     case 0x04: /* Stand-alone tablet */
     case 0x40: /* Docked, tent mode, keyboard not usable */
         return 1;
     default:
         pr_warn("Unknown kbd_dock_state 0x%02x\n", kbd_dock_state);
     }
 
     return 0;
 }
 
 static void acer_kbd_dock_get_initial_state(void)
 {
     u8 *output, input[8] = { 0x05, 0x00, };
     struct acpi_buffer input_buf = { sizeof(input), input };
     struct acpi_buffer output_buf = { ACPI_ALLOCATE_BUFFER, NULL };
     union acpi_object *obj;
     acpi_status status;
     int sw_tablet_mode;
 
     status = wmi_evaluate_method(WMID_GUID3, 0, 0x2, &input_buf, &output_buf);
     if (ACPI_FAILURE(status)) {
         pr_err("Error getting keyboard-dock initial status: %s\n",
                acpi_format_exception(status));
         return;
     }
 
     obj = output_buf.pointer;
     if (!obj || obj->type != ACPI_TYPE_BUFFER || obj->buffer.length != 8) {
         pr_err("Unexpected output format getting keyboard-dock initial status\n");
         goto out_free_obj;
     }
 
     output = obj->buffer.pointer;
     if (output[0] != 0x00 || (output[3] != 0x05 && output[3] != 0x45)) {
         pr_err("Unexpected output [0]=0x%02x [3]=0x%02x getting keyboard-dock initial status\n",
                output[0], output[3]);
         goto out_free_obj;
     }
 
     sw_tablet_mode = acer_kbd_dock_state_to_sw_tablet_mode(output[4]);
     input_report_switch(acer_wmi_input_dev, SW_TABLET_MODE, sw_tablet_mode);
 
 out_free_obj:
     kfree(obj);
 }
 
 static void acer_kbd_dock_event(const struct event_return_value *event)
 {
     int sw_tablet_mode;
 
     if (!has_cap(ACER_CAP_KBD_DOCK))
         return;
 
     sw_tablet_mode = acer_kbd_dock_state_to_sw_tablet_mode(event->kbd_dock_state);
     input_report_switch(acer_wmi_input_dev, SW_TABLET_MODE, sw_tablet_mode);
     input_sync(acer_wmi_input_dev);
 }
 
 /*
  * Rfkill devices
  */
 static void acer_rfkill_update(struct work_struct *ignored);
 static DECLARE_DELAYED_WORK(acer_rfkill_work, acer_rfkill_update);
 static void acer_rfkill_update(struct work_struct *ignored)
 {
     u32 state;
     acpi_status status;
 
     if (has_cap(ACER_CAP_WIRELESS)) {
         status = get_u32(&state, ACER_CAP_WIRELESS);
         if (ACPI_SUCCESS(status)) {
             if (quirks->wireless == 3)
                 rfkill_set_hw_state(wireless_rfkill, !state);
             else
                 rfkill_set_sw_state(wireless_rfkill, !state);
         }
     }
 
     if (has_cap(ACER_CAP_BLUETOOTH)) {
         status = get_u32(&state, ACER_CAP_BLUETOOTH);
         if (ACPI_SUCCESS(status))
             rfkill_set_sw_state(bluetooth_rfkill, !state);
     }
 
     if (has_cap(ACER_CAP_THREEG) && wmi_has_guid(WMID_GUID3)) {
         status = get_u32(&state, ACER_WMID3_GDS_THREEG);
         if (ACPI_SUCCESS(status))
             rfkill_set_sw_state(threeg_rfkill, !state);
     }
 
     schedule_delayed_work(&acer_rfkill_work, round_jiffies_relative(HZ));
 }
 
 static int acer_rfkill_set(void *data, bool blocked)
 {
     acpi_status status;
     u32 cap = (unsigned long)data;
 
     if (rfkill_inited) {
         status = set_u32(!blocked, cap);
         if (ACPI_FAILURE(status))
             return -ENODEV;
     }
 
     return 0;
 }
 
 static const struct rfkill_ops acer_rfkill_ops = {
     .set_block = acer_rfkill_set,
 };
 
 static struct rfkill *acer_rfkill_register(struct device *dev,
                        enum rfkill_type type,
                        char *name, u32 cap)
 {
     int err;
     struct rfkill *rfkill_dev;
     u32 state;
     acpi_status status;
 
     rfkill_dev = rfkill_alloc(name, dev, type,
                   &acer_rfkill_ops,
                   (void *)(unsigned long)cap);
     if (!rfkill_dev)
         return ERR_PTR(-ENOMEM);
 
     status = get_u32(&state, cap);
 
     err = rfkill_register(rfkill_dev);
     if (err) {
         rfkill_destroy(rfkill_dev);
         return ERR_PTR(err);
     }
 
     if (ACPI_SUCCESS(status))
         rfkill_set_sw_state(rfkill_dev, !state);
 
     return rfkill_dev;
 }
 
 static int acer_rfkill_init(struct device *dev)
 {
     int err;
 
     if (has_cap(ACER_CAP_WIRELESS)) {
         wireless_rfkill = acer_rfkill_register(dev, RFKILL_TYPE_WLAN,
             "acer-wireless", ACER_CAP_WIRELESS);
         if (IS_ERR(wireless_rfkill)) {
             err = PTR_ERR(wireless_rfkill);
             goto error_wireless;
         }
     }
 
     if (has_cap(ACER_CAP_BLUETOOTH)) {
         bluetooth_rfkill = acer_rfkill_register(dev,
             RFKILL_TYPE_BLUETOOTH, "acer-bluetooth",
             ACER_CAP_BLUETOOTH);
         if (IS_ERR(bluetooth_rfkill)) {
             err = PTR_ERR(bluetooth_rfkill);
             goto error_bluetooth;
         }
     }
 
     if (has_cap(ACER_CAP_THREEG)) {
         threeg_rfkill = acer_rfkill_register(dev,
             RFKILL_TYPE_WWAN, "acer-threeg",
             ACER_CAP_THREEG);
         if (IS_ERR(threeg_rfkill)) {
             err = PTR_ERR(threeg_rfkill);
             goto error_threeg;
         }
     }
 
     rfkill_inited = true;
 
     if ((ec_raw_mode || !wmi_has_guid(ACERWMID_EVENT_GUID)) &&
         has_cap(ACER_CAP_WIRELESS | ACER_CAP_BLUETOOTH | ACER_CAP_THREEG))
         schedule_delayed_work(&acer_rfkill_work,
             round_jiffies_relative(HZ));
 
     return 0;
 
 error_threeg:
     if (has_cap(ACER_CAP_BLUETOOTH)) {
         rfkill_unregister(bluetooth_rfkill);
         rfkill_destroy(bluetooth_rfkill);
     }
 error_bluetooth:
     if (has_cap(ACER_CAP_WIRELESS)) {
         rfkill_unregister(wireless_rfkill);
         rfkill_destroy(wireless_rfkill);
     }
 error_wireless:
     return err;
 }
 
 static void acer_rfkill_exit(void)
 {
     if ((ec_raw_mode || !wmi_has_guid(ACERWMID_EVENT_GUID)) &&
         has_cap(ACER_CAP_WIRELESS | ACER_CAP_BLUETOOTH | ACER_CAP_THREEG))
         cancel_delayed_work_sync(&acer_rfkill_work);
 
     if (has_cap(ACER_CAP_WIRELESS)) {
         rfkill_unregister(wireless_rfkill);
         rfkill_destroy(wireless_rfkill);
     }
 
     if (has_cap(ACER_CAP_BLUETOOTH)) {
         rfkill_unregister(bluetooth_rfkill);
         rfkill_destroy(bluetooth_rfkill);
     }
 
     if (has_cap(ACER_CAP_THREEG)) {
         rfkill_unregister(threeg_rfkill);
         rfkill_destroy(threeg_rfkill);
     }
 }
 
 static void acer_wmi_notify(union acpi_object *obj, void *context)
 {
     struct event_return_value return_value;
     u16 device_state;
     const struct key_entry *key;
     u32 scancode;
 
     if (!obj)
         return;
     if (obj->type != ACPI_TYPE_BUFFER) {
         pr_warn("Unknown response received %d\n", obj->type);
         return;
     }
     if (obj->buffer.length != 8) {
         pr_warn("Unknown buffer length %d\n", obj->buffer.length);
         return;
     }
 
     return_value = *((struct event_return_value *)obj->buffer.pointer);
 
     switch (return_value.function) {
     case WMID_HOTKEY_EVENT:
         device_state = return_value.device_state;
         pr_info("device state: 0x%x\n", device_state);
 
         key = sparse_keymap_entry_from_scancode(acer_wmi_input_dev,
                             return_value.key_num);
         if (!key) {
             pr_warn("Unknown key number - 0x%x\n",
                 return_value.key_num);
         } else {
             scancode = return_value.key_num;
             switch (key->keycode) {
             case KEY_WLAN:
             case KEY_BLUETOOTH:
                 if (has_cap(ACER_CAP_WIRELESS))
                     rfkill_set_sw_state(wireless_rfkill,
                         !(device_state & ACER_WMID3_GDS_WIRELESS));
                 if (has_cap(ACER_CAP_THREEG))
                     rfkill_set_sw_state(threeg_rfkill,
                         !(device_state & ACER_WMID3_GDS_THREEG));
                 if (has_cap(ACER_CAP_BLUETOOTH))
                     rfkill_set_sw_state(bluetooth_rfkill,
                         !(device_state & ACER_WMID3_GDS_BLUETOOTH));
                 break;
             case KEY_TOUCHPAD_TOGGLE:
                 scancode = (device_state & ACER_WMID3_GDS_TOUCHPAD) ?
                         KEY_TOUCHPAD_ON : KEY_TOUCHPAD_OFF;
             }
             sparse_keymap_report_event(acer_wmi_input_dev, scancode, 1, true);
         }
         break;
     case WMID_ACCEL_OR_KBD_DOCK_EVENT:
         acer_gsensor_event();
         acer_kbd_dock_event(&return_value);
         break;
     case WMID_GAMING_TURBO_KEY_EVENT:
        pr_info("pressed turbo button - %d\n", return_value.key_num);
         if (return_value.key_num == 0x4  && !has_cap(ACER_CAP_NITRO_SENSE_V4))
             acer_toggle_turbo();
         if ((return_value.key_num == 0x5 || (return_value.key_num == 0x4 && has_cap(ACER_CAP_NITRO_SENSE_V4))) && has_cap(ACER_CAP_PLATFORM_PROFILE))
             acer_thermal_profile_change();
         break;
     case WMID_AC_EVENT:
         if(has_cap(ACER_CAP_PREDATOR_SENSE) || has_cap(ACER_CAP_NITRO_SENSE_V4)){
             if(return_value.key_num == 0){
                 /* store the current state when it is connected to AC*/
                 acer_predator_state_update(1);
                 /* restore to the state when it was disconnected from AC*/
                 acer_predator_state_restore(0);
             } else if (return_value.key_num == 1){
                 /* store the current state when it is disconnected from AC*/
                 acer_predator_state_update(0);
                 /* restore to the state when it was connected to AC*/
                 acer_predator_state_restore(1);
             } else {
                 pr_info("Unknown key number - %d\n", return_value.key_num);
             }
         }
         break;
     case WMID_BATTERY_BOOST_EVENT:
         break;
     case WMID_CALIBRATION_EVENT:
         if(has_cap(ACER_CAP_PREDATOR_SENSE) || has_cap(ACER_CAP_NITRO_SENSE) || has_cap(ACER_CAP_NITRO_SENSE_V4)){
             if (battery_health_set(CALIBRATION_MODE,return_value.key_num) != AE_OK){
                 pr_err("Error changing calibration state\n");
             }
         }
         break;
     default:
         pr_warn("Unknown function number - %d - %d\n",
             return_value.function, return_value.key_num);
         break;
     }
 }
 
 static acpi_status __init
 wmid3_set_function_mode(struct func_input_params *params,
             struct func_return_value *return_value)
 {
     acpi_status status;
     union acpi_object *obj;
 
     struct acpi_buffer input = { sizeof(struct func_input_params), params };
     struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
 
     status = wmi_evaluate_method(WMID_GUID3, 0, 0x1, &input, &output);
     if (ACPI_FAILURE(status))
         return status;
 
     obj = output.pointer;
 
     if (!obj)
         return AE_ERROR;
     else if (obj->type != ACPI_TYPE_BUFFER) {
         kfree(obj);
         return AE_ERROR;
     }
     if (obj->buffer.length != 4) {
         pr_warn("Unknown buffer length %d\n", obj->buffer.length);
         kfree(obj);
         return AE_ERROR;
     }
 
     *return_value = *((struct func_return_value *)obj->buffer.pointer);
     kfree(obj);
 
     return status;
 }
 
 static int __init acer_wmi_enable_ec_raw(void)
 {
     struct func_return_value return_value;
     acpi_status status;
     struct func_input_params params = {
         .function_num = 0x1,
         .commun_devices = 0xFFFF,
         .devices = 0xFFFF,
         .app_status = 0x00,		/* Launch Manager Deactive */
         .app_mask = 0x01,
     };
 
     status = wmid3_set_function_mode(&params, &return_value);
 
     if (return_value.error_code || return_value.ec_return_value)
         pr_warn("Enabling EC raw mode failed: 0x%x - 0x%x\n",
             return_value.error_code,
             return_value.ec_return_value);
     else
         pr_info("Enabled EC raw mode\n");
 
     return status;
 }
 
 static int __init acer_wmi_enable_lm(void)
 {
     struct func_return_value return_value;
     acpi_status status;
     struct func_input_params params = {
         .function_num = 0x1,
         .commun_devices = 0xFFFF,
         .devices = 0xFFFF,
         .app_status = 0x01,            /* Launch Manager Active */
         .app_mask = 0x01,
     };
 
     status = wmid3_set_function_mode(&params, &return_value);
 
     if (return_value.error_code || return_value.ec_return_value)
         pr_warn("Enabling Launch Manager failed: 0x%x - 0x%x\n",
             return_value.error_code,
             return_value.ec_return_value);
 
     return status;
 }
 
 static int __init acer_wmi_enable_rf_button(void)
 {
     struct func_return_value return_value;
     acpi_status status;
     struct func_input_params params = {
         .function_num = 0x1,
         .commun_devices = 0xFFFF,
         .devices = 0xFFFF,
         .app_status = 0x10,            /* RF Button Active */
         .app_mask = 0x10,
     };
 
     status = wmid3_set_function_mode(&params, &return_value);
 
     if (return_value.error_code || return_value.ec_return_value)
         pr_warn("Enabling RF Button failed: 0x%x - 0x%x\n",
             return_value.error_code,
             return_value.ec_return_value);
 
     return status;
 }
 
 static int __init acer_wmi_accel_setup(void)
 {
     struct acpi_device *adev;
     int err;
 
     adev = acpi_dev_get_first_match_dev("BST0001", NULL, -1);
     if (!adev)
         return -ENODEV;
 
     gsensor_handle = acpi_device_handle(adev);
     acpi_dev_put(adev);
 
     acer_wmi_accel_dev = input_allocate_device();
     if (!acer_wmi_accel_dev)
         return -ENOMEM;
 
     acer_wmi_accel_dev->open = acer_gsensor_open;
 
     acer_wmi_accel_dev->name = "Acer BMA150 accelerometer";
     acer_wmi_accel_dev->phys = "wmi/input1";
     acer_wmi_accel_dev->id.bustype = BUS_HOST;
     acer_wmi_accel_dev->evbit[0] = BIT_MASK(EV_ABS);
     input_set_abs_params(acer_wmi_accel_dev, ABS_X, -16384, 16384, 0, 0);
     input_set_abs_params(acer_wmi_accel_dev, ABS_Y, -16384, 16384, 0, 0);
     input_set_abs_params(acer_wmi_accel_dev, ABS_Z, -16384, 16384, 0, 0);
 
     err = input_register_device(acer_wmi_accel_dev);
     if (err)
         goto err_free_dev;
 
     return 0;
 
 err_free_dev:
     input_free_device(acer_wmi_accel_dev);
     return err;
 }
 
 static int __init acer_wmi_input_setup(void)
 {
     acpi_status status;
     int err;
 
     acer_wmi_input_dev = input_allocate_device();
     if (!acer_wmi_input_dev)
         return -ENOMEM;
 
     acer_wmi_input_dev->name = "Acer WMI hotkeys";
     acer_wmi_input_dev->phys = "wmi/input0";
     acer_wmi_input_dev->id.bustype = BUS_HOST;
 
     err = sparse_keymap_setup(acer_wmi_input_dev, acer_wmi_keymap, NULL);
     if (err)
         goto err_free_dev;
 
     if (has_cap(ACER_CAP_KBD_DOCK))
         input_set_capability(acer_wmi_input_dev, EV_SW, SW_TABLET_MODE);
 
     status = wmi_install_notify_handler(ACERWMID_EVENT_GUID,
                         acer_wmi_notify, NULL);
     if (ACPI_FAILURE(status)) {
         err = -EIO;
         goto err_free_dev;
     }
 
     if (has_cap(ACER_CAP_KBD_DOCK))
         acer_kbd_dock_get_initial_state();
 
     err = input_register_device(acer_wmi_input_dev);
     if (err)
         goto err_uninstall_notifier;
 
     return 0;
 
 err_uninstall_notifier:
     wmi_remove_notify_handler(ACERWMID_EVENT_GUID);
 err_free_dev:
     input_free_device(acer_wmi_input_dev);
     return err;
 }
 
 static void acer_wmi_input_destroy(void)
 {
     wmi_remove_notify_handler(ACERWMID_EVENT_GUID);
     input_unregister_device(acer_wmi_input_dev);
 }
 
 /*
  * debugfs functions
  */
 static u32 get_wmid_devices(void)
 {
     struct acpi_buffer out = {ACPI_ALLOCATE_BUFFER, NULL};
     union acpi_object *obj;
     acpi_status status;
     u32 devices = 0;
 
     status = wmi_query_block(WMID_GUID2, 0, &out);
     if (ACPI_FAILURE(status))
         return 0;
 
     obj = (union acpi_object *) out.pointer;
     if (obj) {
         if (obj->type == ACPI_TYPE_BUFFER &&
             (obj->buffer.length == sizeof(u32) ||
             obj->buffer.length == sizeof(u64))) {
             devices = *((u32 *) obj->buffer.pointer);
         } else if (obj->type == ACPI_TYPE_INTEGER) {
             devices = (u32) obj->integer.value;
         }
     }
 
     kfree(out.pointer);
     return devices;
 }
 
 static int acer_wmi_hwmon_init(void);
 
 
 /*
  * USB Charging
  */
 static ssize_t predator_usb_charging_show(struct device *dev, struct device_attribute *attr,char *buf){
     acpi_status status;
     u64 result;
     status = WMI_apgeaction_execute_u64(ACER_WMID_GET_FUNCTION,0x4,&result);
     if(ACPI_FAILURE(status)){
         pr_err("Error getting usb charging status: %s\n",acpi_format_exception(status));
         return -ENODEV;
     }
     pr_info("usb charging get status: %llu\n",result);
     return sprintf(buf, "%d\n", result == 663296 ? 0 : result == 659200 ? 10 : result == 1314560 ? 20 : result == 1969920 ? 30 : -1); //-1 means unknown value
 }
 
 static ssize_t predator_usb_charging_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count){
     acpi_status status;
     u64 result;
     u8 val;
       if (sscanf(buf, "%hhd", &val) != 1)
         return -EINVAL;
       if ((val != 0) && (val != 10) && (val != 20) && (val != 30))
         return -EINVAL;
     pr_info("usb charging set value: %d\n",val);
     status = WMI_apgeaction_execute_u64(ACER_WMID_SET_FUNCTION,val == 0 ? 663300 : val == 10 ? 659204 : val == 20 ? 1314564 : val == 30 ? 1969924 : 663300, &result); //if unkown value then turn it off.
     if(ACPI_FAILURE(status)){
         pr_err("Error setting usb charging status: %s\n",acpi_format_exception(status));
         return -ENODEV;
     }
     pr_info("usb charging set status: %llu\n",result);
     return count;
 }
 
 /*
  * Battery Limit (80%)
  * Battery Calibration
  */
 struct get_battery_health_control_status_input {
     u8 uBatteryNo;
     u8 uFunctionQuery;
     u8 uReserved[2];
 } __packed;
 
 struct get_battery_health_control_status_output {
     u8 uFunctionList;
     u8 uReturn[2];
     u8 uFunctionStatus[5];
 } __packed;
 
 struct set_battery_health_control_input {
     u8 uBatteryNo;
     u8 uFunctionMask;
     u8 uFunctionStatus;
     u8 uReservedIn[5];
 } __packed;
 
 struct set_battery_health_control_output {
     u8 uReturn;
     u8 uReservedOut;
 } __packed;
 
 
 static acpi_status battery_health_query(int mode, int *enabled){
     pr_info("battery health query: %d\n",mode);
     acpi_status status;
     union acpi_object *obj;
     struct get_battery_health_control_status_input params = {
         .uBatteryNo = 0x1,
         .uFunctionQuery = 0x1,
         .uReserved = { 0x0, 0x0 }
     };
     struct get_battery_health_control_status_output ret;
 
     struct acpi_buffer input = {
         sizeof(struct get_battery_health_control_status_input), &params
     };
 
     struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
 
     status = wmi_evaluate_method(WMID_GUID5, 0, ACER_WMID_GET_BATTERY_HEALTH_CONTROL_STATUS_METHODID, &input, &output);
     if (ACPI_FAILURE(status))
         return status;
     
     obj = output.pointer;
 
       if (!obj || obj->type != ACPI_TYPE_BUFFER || obj->buffer.length != 8) {
         pr_err("Unexpected output format getting battery health status, buffer "
                "length:%d\n",
                obj->buffer.length);
         goto failed;
       }
     
       ret = *((struct get_battery_health_control_status_output *)obj->buffer.pointer);
     
     if(mode == HEALTH_MODE){
         *enabled = ret.uFunctionStatus[0];
     } else if(mode == CALIBRATION_MODE){
         *enabled = ret.uFunctionStatus[1];
     } else {
         goto failed;
     }
 
     kfree(obj);
     return AE_OK;
 
     failed:
           kfree(obj);
           return AE_ERROR;
 }
 
 static acpi_status battery_health_set(u8 function, u8 function_status){
 
     pr_info("battery_health_set: %d | %d\n",function,function_status);
     
     acpi_status status;
     union acpi_object *obj;
     struct set_battery_health_control_input params = {
         .uBatteryNo = 0x1,
         .uFunctionMask = function,
         .uFunctionStatus = function_status,
         .uReservedIn = { 0x0, 0x0, 0x0, 0x0, 0x0 }
     };
     struct set_battery_health_control_output  ret;
 
     struct acpi_buffer input = {
         sizeof(struct set_battery_health_control_input), &params
     };
 
     struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
 
     status = wmi_evaluate_method(WMID_GUID5, 0, ACER_WMID_SET_BATTERY_HEALTH_CONTROL_METHODID, &input, &output);
     if (ACPI_FAILURE(status))
         return status;
     
     obj = output.pointer;
 
       if (!obj || obj->type != ACPI_TYPE_BUFFER || obj->buffer.length != 4) {
         pr_err("Unexpected output format getting battery health status, buffer "
                "length:%d\n",
                obj->buffer.length);
         goto failed;
       }
     
       ret = *((struct set_battery_health_control_output  *)obj->buffer.pointer);
     
     if(ret.uReturn != 0 && ret.uReservedOut != 0){
         pr_err("Failed to set battery health status\n");
         goto failed;
     }
 
     kfree(obj);
     return AE_OK;
 
     failed:
           kfree(obj);
           return AE_ERROR;
 }
 
 
 static ssize_t predator_battery_limit_show(struct device *dev,
                                            struct device_attribute *attr,
                                            char *buf) {
 
     int enabled;
     acpi_status status = battery_health_query(HEALTH_MODE, &enabled);
 
     if (ACPI_FAILURE(status))
         return -ENODEV;
 
     return sprintf(buf, "%d\n", enabled);
 }
 
 static ssize_t predator_battery_limit_store(struct device *dev,
                                             struct device_attribute *attr,
                                             const char *buf, size_t count) {
     u8 val;
       if (sscanf(buf, "%hhd", &val) != 1)
         return -EINVAL;
 
       if ((val != 0) && (val != 1))
         return -EINVAL;
       
     if (battery_health_set(HEALTH_MODE,val) != AE_OK)
         return -ENODEV;
       
     return count;
 }
 
 static ssize_t predator_battery_calibration_show(struct device *dev,
                                            struct device_attribute *attr,
                                            char *buf) {
 
     int enabled;
     acpi_status status = battery_health_query(CALIBRATION_MODE, &enabled);
 
     if (ACPI_FAILURE(status))
         return -ENODEV;
 
     return sprintf(buf, "%d\n", enabled);
 }
 
 static ssize_t preadtor_battery_calibration_store(struct device *dev,
                                             struct device_attribute *attr,
                                             const char *buf, size_t count) {
     u8 val;
       if (sscanf(buf, "%hhd", &val) != 1)
         return -EINVAL;
       
     if ((val != 0) && (val != 1))
         return -EINVAL;
       
     if (battery_health_set(CALIBRATION_MODE,val) != AE_OK)
         return -ENODEV;
       
     return count;
 }
 
 
 /*
  * FAN CONTROLS
  */
 static int cpu_fan_speed = 0;
 static int gpu_fan_speed = 0;
 
 static u64 fan_val_calc(int percentage, int fan_index) {
     return (((percentage * 25600) / 100) & 0xFF00) + fan_index;
 }
 static acpi_status acer_set_fan_speed(int t_cpu_fan_speed, int t_gpu_fan_speed){
     
     acpi_status status;
 
     if (t_cpu_fan_speed == 100 && t_gpu_fan_speed == 100) {
         pr_info("MAX FAN MODE!\n");
         status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_FAN_BEHAVIOR_METHODID, 0x820009, NULL);
         if(ACPI_FAILURE(status)){
             pr_err("Error setting fan speed status: %s\n",acpi_format_exception(status));
             return AE_ERROR;
         }
     } else if (t_cpu_fan_speed == 0 && t_gpu_fan_speed == 0) {
         pr_info("AUTO FAN MODE!\n");
         status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_FAN_BEHAVIOR_METHODID, 0x410009, NULL);
         if(ACPI_FAILURE(status)){
             pr_err("Error setting fan speed status: %s\n",acpi_format_exception(status));
             return AE_ERROR;
         }
     } else if (t_cpu_fan_speed <= 100 && t_gpu_fan_speed <= 100) {
         if (t_cpu_fan_speed == 0) {
             pr_info("CUSTOM FAN MODE (GPU)\n");
             status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_FAN_BEHAVIOR_METHODID, 0x10001, NULL);
             if(ACPI_FAILURE(status)){
                 pr_err("Error setting fan speed status: %s\n",acpi_format_exception(status));
                 return AE_ERROR;
             }
             status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_FAN_BEHAVIOR_METHODID, 0xC00008, NULL);
             if(ACPI_FAILURE(status)){
                 pr_err("Error setting fan speed status: %s\n",acpi_format_exception(status));
                 return AE_ERROR;
             }
             status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_FAN_SPEED_METHODID, fan_val_calc(t_gpu_fan_speed,4), NULL);
             if(ACPI_FAILURE(status)){
                 pr_err("Error setting fan speed status: %s\n",acpi_format_exception(status));
                 return AE_ERROR;
             }
         } else if (t_gpu_fan_speed == 0) {
             pr_info("CUSTOM FAN MODE (CPU)\n");
             status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_FAN_BEHAVIOR_METHODID, 0x400008, NULL);
             if(ACPI_FAILURE(status)){
                 pr_err("Error setting fan speed status: %s\n",acpi_format_exception(status));
                 return AE_ERROR;
             }
             status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_FAN_BEHAVIOR_METHODID, 0x30001, NULL);
             if(ACPI_FAILURE(status)){
                 pr_err("Error setting fan speed status: %s\n",acpi_format_exception(status));
                 return AE_ERROR;
             }
             status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_FAN_SPEED_METHODID, fan_val_calc(t_cpu_fan_speed,1), NULL);
             if(ACPI_FAILURE(status)){
                 pr_err("Error setting fan speed status: %s\n",acpi_format_exception(status));
                 return AE_ERROR;
             }
         } else {
             pr_info("CUSTOM FAN MODE (MIXED)!\n");
             //set gaming behvaiour mode to custom
             status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_FAN_BEHAVIOR_METHODID, 0xC30009, NULL);
             if(ACPI_FAILURE(status)){
                 pr_err("Error setting fan speed status: %s\n",acpi_format_exception(status));
                 return AE_ERROR;
             }
             //set cpu fan speed
             status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_FAN_SPEED_METHODID, fan_val_calc(t_cpu_fan_speed,1), NULL);
             if(ACPI_FAILURE(status)){
                 pr_err("Error setting fan speed status: %s\n",acpi_format_exception(status));
                 return AE_ERROR;
             }
             //set gpu fan speed
             status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_FAN_SPEED_METHODID, fan_val_calc(t_gpu_fan_speed,4), NULL);
             if(ACPI_FAILURE(status)){
                 pr_err("Error setting fan speed status: %s\n",acpi_format_exception(status));
                 return AE_ERROR;
             }
         }
     } else {
         return AE_ERROR;
     }
 
     cpu_fan_speed = t_cpu_fan_speed;
     gpu_fan_speed = t_gpu_fan_speed;
     pr_info("Fan speeds updated: CPU=%d, GPU=%d\n", cpu_fan_speed, gpu_fan_speed);
 
     return AE_OK;	
 }
 
 static ssize_t predator_fan_speed_show(struct device *dev,
                                            struct device_attribute *attr,
                                            char *buf) {
   return sprintf(buf, "%d,%d\n", cpu_fan_speed, gpu_fan_speed);                           
 }
 
 
 static ssize_t predator_fan_speed_store(struct device *dev,
                                             struct device_attribute *attr,
                                             const char *buf, size_t count) {
     int t_cpu_fan_speed, t_gpu_fan_speed;
     
     char input[9];
     char *token;
     char* input_ptr = input;
     size_t len = min(count, sizeof(input) - 1);
     strncpy(input, buf, len);
 
     if(input[len-1] == '\n'){
         input[len-1] = '\0';
     } else {
         input[len] = '\0';
     }
 
 
     token = strsep(&input_ptr, ",");
     if (!token || kstrtoint(token, 10, &t_cpu_fan_speed) || t_cpu_fan_speed < 0 || t_cpu_fan_speed > 100) {
         pr_err("Invalid CPU speed value.\n");
         return -EINVAL;
     }
 
     token = strsep(&input_ptr, ",");
     if (!token || kstrtoint(token, 10, &t_gpu_fan_speed) || t_gpu_fan_speed < 0 || t_gpu_fan_speed > 100) {
         pr_err("Invalid GPU speed value.\n");
         return -EINVAL;
     }
 
     acpi_status status = acer_set_fan_speed(t_cpu_fan_speed, t_gpu_fan_speed);
     if(ACPI_FAILURE(status)){
         return -ENODEV;
     } 
 
     return count;
 }
 /*
  * persistent predator states.
  */
 struct acer_predator_state {
     int cpu_fan_speed;
     int gpu_fan_speed;
     int thermal_profile;
 };
 
 struct power_states {
     struct acer_predator_state battery_state;
     struct acer_predator_state ac_state;
 } __attribute__((packed));
 
 static struct power_states current_states = {
     .battery_state = {0, 0, ACER_PREDATOR_V4_THERMAL_PROFILE_ECO},
     .ac_state = {0, 0, ACER_PREDATOR_V4_THERMAL_PROFILE_BALANCED}
 };
 
 static int acer_predator_state_update(int value){
     u8 current_tp;
     int tp, err;
     err = WMID_gaming_get_misc_setting(
        ACER_WMID_MISC_SETTING_PLATFORM_PROFILE, 
        &current_tp);
     if (err)
         return err;
     switch (current_tp) {
         case ACER_PREDATOR_V4_THERMAL_PROFILE_TURBO:
             tp = ACER_PREDATOR_V4_THERMAL_PROFILE_TURBO;
             break;
         case ACER_PREDATOR_V4_THERMAL_PROFILE_PERFORMANCE:
             tp = ACER_PREDATOR_V4_THERMAL_PROFILE_PERFORMANCE;
             break;
         case ACER_PREDATOR_V4_THERMAL_PROFILE_BALANCED:
             tp = ACER_PREDATOR_V4_THERMAL_PROFILE_BALANCED;
             break;
         case ACER_PREDATOR_V4_THERMAL_PROFILE_QUIET:
             tp = ACER_PREDATOR_V4_THERMAL_PROFILE_QUIET;
             break;
         case ACER_PREDATOR_V4_THERMAL_PROFILE_ECO:
             tp = ACER_PREDATOR_V4_THERMAL_PROFILE_ECO;
             break;
         default:
             return -1;
     }
     /* When AC is connected */
     if(value == 1){
         current_states.ac_state.thermal_profile = tp;
         current_states.ac_state.cpu_fan_speed = cpu_fan_speed;
         current_states.ac_state.gpu_fan_speed = gpu_fan_speed;
     /* When AC isn't connected */
     } else if(value == 0){
         current_states.battery_state.thermal_profile = tp;
         current_states.battery_state.cpu_fan_speed = cpu_fan_speed;
         current_states.battery_state.gpu_fan_speed = gpu_fan_speed;
     } else {
         pr_err("invalid value received: %d\n", value);
         return -1;
     }
     return 0;
 }
 
 static acpi_status acer_predator_state_restore(int value){
     int err = WMID_gaming_set_misc_setting(ACER_WMID_MISC_SETTING_PLATFORM_PROFILE, 
                                        value == 0 ? current_states.battery_state.thermal_profile : current_states.ac_state.thermal_profile);
     if (err)
         return err;
 
     acpi_status status = acer_set_fan_speed(value == 0 ? current_states.battery_state.cpu_fan_speed : current_states.ac_state.cpu_fan_speed, 
                                 value == 0 ? current_states.battery_state.gpu_fan_speed : current_states.ac_state.gpu_fan_speed);
     if(ACPI_FAILURE(status)){
         return AE_ERROR;
     } 
 
     return AE_OK;
 }
 
 static int acer_predator_state_load(void)
 {
     u64 on_AC;
     struct file *file;
     ssize_t len;
     acpi_status status;
 
     file = filp_open(STATE_FILE, O_RDONLY, 0);
     if (!IS_ERR(file)) {
 
         len = kernel_read(file, (char *)&current_states, sizeof(current_states), &file->f_pos);
         filp_close(file, NULL);
 
         if (len != sizeof(current_states)) {
             pr_err("Incomplete state read, using defaults\n");
         } else {
             pr_info("Thermal states loaded\n");
         }
     } else {
         pr_info("State file not found, loading defaults\n");
     }
 
     /* Always proceed to restore state based on power source */
     status = WMI_gaming_execute_u64(
         ACER_WMID_GET_GAMING_SYS_INFO_METHODID,
         ACER_WMID_CMD_GET_PREDATOR_V4_BAT_STATUS, &on_AC);
 
     if (ACPI_FAILURE(status)) {
         pr_err("Failed to query power source state\n");
         return -1;
     }
 
     /* Restore state based on power source (0 for battery, 1 for AC) */
     status = acer_predator_state_restore(on_AC == 0 ? 0 : 1);
     if (ACPI_FAILURE(status)) {
         pr_err("Failed to restore thermal state\n");
         return -1;
     }
 
     pr_info("Thermal states restored successfully\n");
     return 0;
 }
 
 
 static int acer_predator_state_save(void){
     u64 on_AC;
     acpi_status status;
     struct file *file;
     ssize_t len;
 
     status = WMI_gaming_execute_u64(
         ACER_WMID_GET_GAMING_SYS_INFO_METHODID,
         ACER_WMID_CMD_GET_PREDATOR_V4_BAT_STATUS, &on_AC);
     if (ACPI_FAILURE(status))
         return -1;
 
     /* update to the latest state based on power source */
     status = acer_predator_state_update(on_AC == 0 ? 0 : 1);
     if (ACPI_FAILURE(status)){
         return -1;
     }
 
     file = filp_open(STATE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
     if(!file) {
         pr_info("state_access - Error opening file\n");
         return -1;
     }
 
     len = kernel_write(file, (char *)&current_states, sizeof(current_states), &file->f_pos);
     if(len < 0) {
         pr_info("state_access - Error writing to file: %ld\n", len);
         filp_close(file, NULL);
     }
 
     filp_close(file, NULL);
 
     if (len != sizeof(current_states)) {
         pr_err("Failed to write complete state to file\n");
         return -1;
     }
 
     pr_info("Thermal states saved successfully\n");
     return 0;
 }
 
 /*
  *LCD OVERRIDE CONTROLS
  */
 static ssize_t predator_lcd_override_show(struct device *dev, struct device_attribute *attr,char *buf){
     acpi_status status;
     u64 result;
     status = WMI_gaming_execute_u64(ACER_WMID_GET_GAMING_PROFILE_METHODID,0x00,&result);
     if(ACPI_FAILURE(status)){
         pr_err("Error getting lcd override status: %s\n",acpi_format_exception(status));
         return -ENODEV;
     }
     pr_info("lcd override get status: %llu\n",result);
     return sprintf(buf, "%d\n", result == 0x1000001000000 ? 1 : result == 0x1000000 ? 0 : -1);
 }
 
 static ssize_t predator_lcd_override_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count){
     acpi_status status;
     u64 result;
     u8 val;
       if (sscanf(buf, "%hhd", &val) != 1)
         return -EINVAL;
       if ((val != 0) && (val != 1))
         return -EINVAL;
     pr_info("lcd_override set value: %d\n",val);
     status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_PROFILE_METHODID,val == 1 ? 0x1000000000010 : 0x10, &result);
     if(ACPI_FAILURE(status)){
         pr_err("Error setting lcd override status: %s\n",acpi_format_exception(status));
         return -ENODEV;
     }
     pr_info("lcd override set status: %llu\n",result);
     return count;
 }
 
 /*
  * BACKLIGHT 30 SEC TIMEOUT
  */
 
 static ssize_t predator_backlight_timeout_show(struct device *dev, struct device_attribute *attr,char *buf){
     acpi_status status;
     u64 result;
     status = WMI_apgeaction_execute_u64(ACER_WMID_GET_FUNCTION,0x88401,&result);
     if(ACPI_FAILURE(status)){
         pr_err("Error getting backlight_timeout status: %s\n",acpi_format_exception(status));
         return -ENODEV;
     }
     pr_info("backlight_timeout get status: %llu\n",result);
     return sprintf(buf, "%d\n", result == 0x1E0000080000 ? 1 : result == 0x80000 ? 0 : -1);
 }
 
 static ssize_t predator_backlight_timeout_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count){
     acpi_status status;
     u64 result;
     u8 val;
       if (sscanf(buf, "%hhd", &val) != 1)
         return -EINVAL;
       if ((val != 0) && (val != 1))
         return -EINVAL;
     pr_info("bascklight_timeout set value: %d\n",val);
     status = WMI_apgeaction_execute_u64(ACER_WMID_SET_FUNCTION,val == 1 ? 0x1E0000088402 : 0x88402, &result);
     if(ACPI_FAILURE(status)){
         pr_err("Error setting backlight_timeout status: %s\n",acpi_format_exception(status));
         return -ENODEV;
     }
     pr_info("backlight_timeout set status: %llu\n",result);
     return count;
 }
 
 /*
  * System Boot Animation & Sound 
  */
 static ssize_t predator_boot_animation_sound_show(struct device *dev, struct device_attribute *attr,char *buf){
     acpi_status status;
     u64 result;
     status = WMI_gaming_execute_u64(ACER_WMID_GET_GAMING_MISC_SETTING_METHODID,0x6,&result);
     if(ACPI_FAILURE(status)){
         pr_err("Error getting boot_animation_sound status: %s\n",acpi_format_exception(status));
         return -ENODEV;
     }
     pr_info("boot_animation_sound get status: %llu\n",result);
     return sprintf(buf, "%d\n", result == 0x100 ? 1 : result == 0 ? 0 : -1);
 }
 
 static ssize_t predator_boot_animation_sound_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count){
     acpi_status status;
     u64 result;
     u8 val;
       if (sscanf(buf, "%hhd", &val) != 1)
         return -EINVAL;
       if ((val != 0) && (val != 1))
         return -EINVAL;
     pr_info("boot_animation_sound set value: %d\n",val);
     status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_MISC_SETTING_METHODID,val == 1 ? 0x106 : 0x6, &result);
     if(ACPI_FAILURE(status)){
         pr_err("Error setting boot_animation_sound status: %s\n",acpi_format_exception(status));
         return -ENODEV;
     }
     pr_info("boot_animation_sound set status: %llu\n",result);
     return count;
 }
 
 /*
  * predator sense attributes
  */
 static struct device_attribute boot_animation_sound = __ATTR(boot_animation_sound, 0644, predator_boot_animation_sound_show, predator_boot_animation_sound_store);
 static struct device_attribute backlight_timeout = __ATTR(backlight_timeout, 0644, predator_backlight_timeout_show, predator_backlight_timeout_store);
 static struct device_attribute usb_charging = __ATTR(usb_charging, 0644, predator_usb_charging_show, predator_usb_charging_store);
 static struct device_attribute battery_calibration = __ATTR(battery_calibration, 0644, predator_battery_calibration_show, preadtor_battery_calibration_store);
 static struct device_attribute battery_limiter = __ATTR(battery_limiter, 0644, predator_battery_limit_show, predator_battery_limit_store);
 static struct device_attribute fan_speed = __ATTR(fan_speed, 0644, predator_fan_speed_show, predator_fan_speed_store);
 static struct device_attribute lcd_override = __ATTR(lcd_override, 0644, predator_lcd_override_show, predator_lcd_override_store);
 static struct attribute *predator_sense_attrs[] = {
     &lcd_override.attr,
     &fan_speed.attr,
     &battery_limiter.attr,
     &battery_calibration.attr,
     &usb_charging.attr,
     &backlight_timeout.attr,
     &boot_animation_sound.attr,
     NULL
 };
 
 static struct attribute_group preadtor_sense_attr_group = {
     .name = "predator_sense", .attrs = predator_sense_attrs
 };
 
 static struct attribute_group nitro_sense_v4_attr_group = {
     .name = "nitro_sense", .attrs = predator_sense_attrs
 };
 
 /* nitro sense attributes */
 static struct attribute *nitro_sense_attrs[] = {
     &fan_speed.attr,
     &battery_limiter.attr,
     &battery_calibration.attr,
     &usb_charging.attr,
     &backlight_timeout.attr,
     NULL
 }; 
 static struct attribute_group nitro_sense_attr_group = {
     .name = "nitro_sense", .attrs = nitro_sense_attrs
 };
 
 /* Four Zoned Keyboard  */
 
 struct get_four_zoned_kb_output {
     u8 gmReturn;
     u8 gmOutput[15];
 } __packed;
 
 static acpi_status set_kb_status(int mode, int speed, int brightness,
                                  int direction, int red, int green, int blue){
     u64 resp = 0;
     u8 gmInput[16] = {mode, speed, brightness, 0, direction, red, green, blue, 3, 1, 0, 0, 0, 0, 0, 0};
     
     acpi_status status;
     union acpi_object *obj;
     struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
     struct acpi_buffer input = { (acpi_size)sizeof(gmInput), (void *)(gmInput) };
     
     status = wmi_evaluate_method(WMID_GUID4, 0, ACER_WMID_SET_GAMING_KB_BACKLIGHT_METHODID, &input, &output);
     if (ACPI_FAILURE(status))
         return status;
 
     obj = (union acpi_object *) output.pointer;
 
     if (obj) {
         if (obj->type == ACPI_TYPE_BUFFER) {
             if (obj->buffer.length == sizeof(u32))
                 resp = *((u32 *) obj->buffer.pointer);
             else if (obj->buffer.length == sizeof(u64))
                 resp = *((u64 *) obj->buffer.pointer);
         } else if (obj->type == ACPI_TYPE_INTEGER) {
             resp = (u64) obj->integer.value;
         }
     }
 
     if(resp != 0){
         pr_err("failed to set keyboard rgb: %llu\n",resp);
         kfree(obj);
         return AE_ERROR;
     }
 
     kfree(obj);
     return status;
 }
 
 static acpi_status get_kb_status(struct get_four_zoned_kb_output *out){
     u64 in = 1;
     acpi_status status;
     union acpi_object *obj;
     struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
     struct acpi_buffer input = { (acpi_size) sizeof(u64), (void *)(&in) };
 
     status = wmi_evaluate_method(WMID_GUID4, 0, ACER_WMID_GET_GAMING_KB_BACKLIGHT_METHODID, &input, &output);
     if (ACPI_FAILURE(status))
         return status;
     
     obj = output.pointer;
 
       if (!obj || obj->type != ACPI_TYPE_BUFFER || obj->buffer.length != 16) {
         pr_err("Unexpected output format getting kb zone status, buffer "
                "length:%d\n",
                obj->buffer.length);
         goto failed;
       }
 
       *out = *((struct get_four_zoned_kb_output  *)obj->buffer.pointer);
 
     kfree(obj);
     return AE_OK;
 
     failed:
           kfree(obj);
           return AE_ERROR;
 }
 
 
 /* KB Backlight State  */;
 
 struct per_zone_color {
     u64 zone1, zone2, zone3, zone4;
     int brightness;
 } __packed;
 
 struct kb_state {
     u8 per_zone;
     u8 mode;
     u8 speed;
     u8 brightness;
     u8 direction;
     u8 red;
     u8 green;
     u8 blue;
     struct per_zone_color zones;
 } __packed;
 
 static struct kb_state current_kb_state;
 
 
 /* four zone mode */
 static ssize_t four_zoned_rgb_kb_show(struct device *dev, struct device_attribute *attr,char *buf){
     acpi_status status;
     struct get_four_zoned_kb_output output;
     status = get_kb_status(&output);
     if(ACPI_FAILURE(status)){
         pr_err("Error getting kb status: %s\n",acpi_format_exception(status));
         return -ENODEV;
     }
     return sprintf(buf, "%d,%d,%d,%d,%d,%d,%d\n",output.gmOutput[0],output.gmOutput[1],output.gmOutput[2],output.gmOutput[4],output.gmOutput[5],output.gmOutput[6],output.gmOutput[7]);
 }
 
 static ssize_t four_zoned_rgb_kb_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
     acpi_status status;
 
     int mode, speed, brightness, direction, red, green, blue;
     char input_buf[30];
     char *token;
     char *input_ptr = input_buf;
     size_t len = min(count, sizeof(input_buf) - 1);
 
     strncpy(input_buf, buf, len);
 
     if(input_buf[len-1] == '\n'){
         input_buf[len-1] = '\0';
     } else {
         input_buf[len] = '\0';
     }
 
     token = strsep(&input_ptr, ",");
     if (!token || kstrtoint(token, 10, &mode) || mode < 0 || mode > 7) {
         pr_err("Invalid mode value.\n");
         return -EINVAL;
     }
 
     token = strsep(&input_ptr, ",");
     if (!token || kstrtoint(token, 10, &speed) || speed < 0 || speed > 9) {
         pr_err("Invalid speed value.\n");
         return -EINVAL;
     }
 
     token = strsep(&input_ptr, ",");
     if (!token || kstrtoint(token, 10, &brightness) || brightness < 0 || brightness > 100) {
         pr_err("Invalid brightness value.\n");
         return -EINVAL;
     }
 
     token = strsep(&input_ptr, ",");
     if (!token || kstrtoint(token, 10, &direction) || ((direction <= 0) && (mode == 0x3 || mode == 0x4 )) || direction < 0 || direction > 2) {
         pr_err("Invalid direction value.\n");
         return -EINVAL;
     }
 
     token = strsep(&input_ptr, ",");
     if (!token || kstrtoint(token, 10, &red) || red < 0 || red > 255) {
         pr_err("Invalid red value.\n");
         return -EINVAL;
     }
 
     token = strsep(&input_ptr, ",");
     if (!token || kstrtoint(token, 10, &green) || green < 0 || green > 255) {
         pr_err("Invalid green value.\n");
         return -EINVAL;
     }
 
     token = strsep(&input_ptr, ",");
     if (!token || kstrtoint(token, 10, &blue) || blue < 0 || blue > 255) {
         pr_err("Invalid blue value.\n");
         return -EINVAL;
     }
 
     switch (mode) {
         case 0x0:  // Static mode: Ignore speed and direction
             speed = 0;
             direction = 0;
             break;
         case 0x1:  // Breathing mode: Ignore speed
             speed = 0;
             direction = 0;
             break;
         case 0x2:  // Neon mode: Ignore red, green, blue, and direction
             red = 0;
             green = 0;
             blue = 0;
             direction = 0;
             break;
         case 0x3:  // Wave mode: Ignore red, green, and blue
             red = 0;
             green = 0;
             blue = 0;
             break;
         case 0x4:  // Shifting mode: No restrictions (all values allowed)
             break;
         case 0x5:  // Zoom mode: Ignore direction
             direction = 0;
             break;
         case 0x6:  // Meteor mode: Ignore direction
             direction = 0;
             break;
         case 0x7:  // Twinkling mode: Ignore direction
             direction = 0;
             break;
         default:
             pr_err("Invalid mode value.\n");
             return -EINVAL;
     }
 
     status = set_kb_status(mode,speed,brightness,direction,red,green,blue);
     if (ACPI_FAILURE(status)) {
         pr_err("Error setting RGB KB status.\n");
         return -ENODEV;
     }
 
     /* Set per_zone to 0 */
     current_kb_state.per_zone = 0;
 
     return count;
 }
 
 
 /* Per Zone Mode */
 
 static acpi_status get_per_zone_color(struct per_zone_color *output) {
     acpi_status status;
     u64 *zones[] = { &output->zone1, &output->zone2, &output->zone3, &output->zone4 };
     u8 zone_ids[] = { 0x1, 0x2, 0x4, 0x8 };
 
     for (int i = 0; i < 4; i++) {
         status = WMI_gaming_execute_u64(ACER_WMID_GET_GAMING_RGB_KB_METHODID, zone_ids[i], zones[i]);
         if (ACPI_FAILURE(status)) {
             pr_err("Error getting kb status (zone %d): %s\n", i + 1, acpi_format_exception(status));
             return status;
         }
         *zones[i] = cpu_to_be64(*zones[i]) >> 32;
     }
 
     /* Fetching Brighness Value */
     struct get_four_zoned_kb_output out;
     status = get_kb_status(&out);
     if (ACPI_FAILURE(status)) {
         pr_err("get kb status failed!");
         return status;
     }
     output->brightness = out.gmOutput[2];
     
     return AE_OK;  
 }
 
 
 
 static acpi_status set_per_zone_color(struct per_zone_color *input) {
     acpi_status status;
     u64 *zones[] = { &input->zone1, &input->zone2, &input->zone3, &input->zone4 };
     u8 zone_ids[] = { 0x1, 0x2, 0x4, 0x8 };
 
     status = set_kb_status(0, 0, input->brightness, 0, 0, 0, 0);
     if (ACPI_FAILURE(status)) {
         pr_err("Error setting KB status.\n");
         return -ENODEV;
     }
 
     for (int i = 0; i < 4; i++) {
         *zones[i] = (cpu_to_be64(*zones[i]) >> 32) | zone_ids[i];
         status = WMI_gaming_execute_u64(ACER_WMID_SET_GAMING_RGB_KB_METHODID, *zones[i], NULL);
         if (ACPI_FAILURE(status)) {
             pr_err("Error setting KB color (zone %d): %s\n", i + 1, acpi_format_exception(status));
             return status;
         }
     }
     /* set per_zone to 1*/
 
     current_kb_state.per_zone = 1;
 
     return status;  
 }
 
 static ssize_t per_zoned_rgb_kb_show(struct device *dev, struct device_attribute *attr,char *buf){
     struct per_zone_color output;
     acpi_status status;
     status = get_per_zone_color(&output);
     if(ACPI_FAILURE(status)){
         return -ENODEV;
     }
     return sprintf(buf,"%06llx,%06llx,%06llx,%06llx,%d\n",output.zone1,output.zone2,output.zone3,output.zone4,output.brightness);
 }
 
 static ssize_t per_zoned_rgb_kb_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
     int i = 0;
     size_t len;
     char *token;
     char str_buf[34];
     struct per_zone_color colors;
     char *input_ptr = str_buf;
     len = min(count, sizeof(str_buf) - 1);
     strncpy(str_buf, buf, len);
     if(str_buf[len-1] == '\n'){
         str_buf[len-1] = '\0';
     } else {
         str_buf[len] = '\0';
     }
 
     acpi_status status;
 
     /* zone1,zone2,zone3,zone4 */
     
     while ((token = strsep(&input_ptr, ",")) && i < 4) {
         if (strlen(token) != 6) {
             pr_err("Invalid rgb length: %s (%lu) (must be 3 bytes)\n", token, strlen(token));
             return -EINVAL;
         }
         if (kstrtoull(token, 16, &((u64 *)&colors)[i])) {
             pr_err("Invalid hex value: %s\n", token);
             return -EINVAL;
         }
         i++;
     }
 
     if (!token || kstrtoint(token, 10, &colors.brightness) || colors.brightness < 0 || colors.brightness > 100) {
         pr_err("Invalid brightness value.\n");
         return -EINVAL;
     }
 
     /* set per zone colors */
     status = set_per_zone_color(&colors);
     if(ACPI_FAILURE(status)){
         pr_err("Error setting RGB KB status.\n");
         return -ENODEV;
     }
     return count;
 }
 
 /* BackLight State */
 
 static int four_zone_kb_state_update(void) {
     acpi_status status;
     struct get_four_zoned_kb_output out;
 
     // Get keyboard status
     status = get_kb_status(&out);
     if (ACPI_FAILURE(status)) {
         pr_err("get kb status failed!");
         return -1;
     }
 
     current_kb_state.mode = out.gmOutput[0];
     current_kb_state.speed = out.gmOutput[1];
     current_kb_state.brightness = out.gmOutput[2];
     current_kb_state.direction = out.gmOutput[4]; 
     current_kb_state.red = out.gmOutput[5];
     current_kb_state.green = out.gmOutput[6];
     current_kb_state.blue = out.gmOutput[7];
 
     // Get per-zone color data
     status = get_per_zone_color(&current_kb_state.zones);
     if (ACPI_FAILURE(status)) {
         pr_err("get_per_zone_color failed!");
         return -1;
     }
     return 0;
 }
 
 static int four_zone_kb_state_save(void){
     struct file *file;
     ssize_t len;
     
     four_zone_kb_state_update();
 
     file = filp_open(KB_STATE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
     if(!file) {
         pr_err("kb_state_access - Error opening file\n");
         return -1;
     }
 
     len = kernel_write(file, (char *)&current_kb_state, sizeof(current_kb_state), &file->f_pos);
     if(len < 0) {
         pr_err("kb_state_access - Error writing to file: %ld\n", len);
         filp_close(file, NULL);
     }
     
     filp_close(file, NULL);
 
     if (len != sizeof(current_kb_state)) {
         pr_err("Failed to write complete state to file\n");
         return -1;
     }
 
     pr_info("kb states saved successfully\n");
     return 0;
 }
 
 
 static int four_zone_kb_state_load(void)
 {
     struct file *file;
     ssize_t len;
     acpi_status status;
 
     file = filp_open(KB_STATE_FILE, O_RDONLY, 0);
     if (!IS_ERR(file)) {
 
         len = kernel_read(file, (char *)&current_kb_state, sizeof(current_kb_state), &file->f_pos);
         filp_close(file, NULL);
 
         if (len != sizeof(current_kb_state)) {
             pr_err("Incomplete state read\n");
             return -1;
         } else {
             pr_info("KB states loaded\n");
         }
     } else {
         pr_info("KB state file not found!\n");
         return -1;
     }
 
     if(current_kb_state.per_zone){
         status = set_per_zone_color(&current_kb_state.zones);
         if(ACPI_FAILURE(status)){
             pr_err("Error setting RGB KB status.\n");
             return -1;
         }
     } else {
         status = set_kb_status(current_kb_state.mode,current_kb_state.speed,current_kb_state.brightness,current_kb_state.direction,current_kb_state.red,current_kb_state.green,current_kb_state.blue);
         if(ACPI_FAILURE(status)){
             pr_err("Error setting KB status.\n");
             return -1;
         }
     }
 
     pr_info("KB states restored successfully\n");
     return 0;
 }
 
 /* Four Zoned Keyboard Attributes */
 static struct device_attribute four_zoned_rgb_mode = __ATTR(four_zone_mode, 0644, four_zoned_rgb_kb_show, four_zoned_rgb_kb_store);
 static struct device_attribute per_zoned_rgb_mode = __ATTR(per_zone_mode, 0644, per_zoned_rgb_kb_show, per_zoned_rgb_kb_store);
 static struct attribute *four_zoned_kb_attrs[] = {
     &four_zoned_rgb_mode.attr,
     &per_zoned_rgb_mode.attr,
     NULL
 };
 
 /* Four Zoned RGB Keyboard */
 static struct attribute_group four_zoned_kb_attr_group = {
     .name = "four_zoned_kb", .attrs = four_zoned_kb_attrs
 };
 /*
  * Platform device
  */
 
 static int acer_platform_probe(struct platform_device *device)
 {
     int err;
 
     if (has_cap(ACER_CAP_MAILLED)) {
         err = acer_led_init(&device->dev);
         if (err)
             goto error_mailled;
     }
 
     if (has_cap(ACER_CAP_BRIGHTNESS)) {
         err = acer_backlight_init(&device->dev);
         if (err)
             goto error_brightness;
     }
 
     err = acer_rfkill_init(&device->dev);
     if (err)
         goto error_rfkill;
 
     if (has_cap(ACER_CAP_PLATFORM_PROFILE)) {
         err = acer_platform_profile_setup(device);
         if (err)
             goto error_platform_profile;
     }
 
     if (has_cap(ACER_CAP_PREDATOR_SENSE)) {
         err = sysfs_create_group(&device->dev.kobj, &preadtor_sense_attr_group);
         if (err)
             goto error_predator_sense;
         acer_predator_state_load();
     }
     if (has_cap(ACER_CAP_NITRO_SENSE_V4)) {
         err = sysfs_create_group(&device->dev.kobj, &nitro_sense_v4_attr_group);
         if (err)
             goto error_predator_sense;
         acer_predator_state_load();
     }
     if (has_cap(ACER_CAP_NITRO_SENSE)){
         err = sysfs_create_group(&device->dev.kobj, &nitro_sense_attr_group);
         if (err)
             goto error_nitro_sense;
     }
 
     if(quirks->four_zone_kb){
         err = sysfs_create_group(&device->dev.kobj, &four_zoned_kb_attr_group);
         if (err)
             goto error_four_zone;
         four_zone_kb_state_load();
     }
 
     if (has_cap(ACER_CAP_FAN_SPEED_READ)) {
         err = acer_wmi_hwmon_init();
         if (err)
             goto error_hwmon;
     }
 
     return 0;
 
 error_hwmon:
 error_platform_profile:
     acer_rfkill_exit();
 error_rfkill:
     if (has_cap(ACER_CAP_BRIGHTNESS))
         acer_backlight_exit();
 error_brightness:
     if (has_cap(ACER_CAP_MAILLED))
         acer_led_exit();
 error_four_zone:
     return err;
 error_nitro_sense:
     return err;
 error_predator_sense:
     return err;
 error_mailled:
     return err;
 }
 
 
 static void acer_platform_remove(struct platform_device *device)
 {
     if (has_cap(ACER_CAP_MAILLED))
         acer_led_exit();
     if (has_cap(ACER_CAP_BRIGHTNESS))
         acer_backlight_exit();
     if (has_cap(ACER_CAP_PREDATOR_SENSE)) {
         sysfs_remove_group(&device->dev.kobj, &preadtor_sense_attr_group);
         acer_predator_state_save();
     }
     if (has_cap(ACER_CAP_NITRO_SENSE)) {
        sysfs_remove_group(&device->dev.kobj, &nitro_sense_v4_attr_group);
        acer_predator_state_save();
     }
     if (has_cap(ACER_CAP_NITRO_SENSE_V4)) {
         sysfs_remove_group(&device->dev.kobj, &nitro_sense_v4_attr_group);
         acer_predator_state_save();
     }
     if(quirks->four_zone_kb){
         sysfs_remove_group(&device->dev.kobj, &four_zoned_kb_attr_group);
         four_zone_kb_state_save();
     }
 
     acer_rfkill_exit();
 }
 
 #ifdef CONFIG_PM_SLEEP
 static int acer_suspend(struct device *dev)
 {
     u32 value;
     struct acer_data *data = &interface->data;
 
     if (!data)
         return -ENOMEM;
 
     if (has_cap(ACER_CAP_MAILLED)) {
         get_u32(&value, ACER_CAP_MAILLED);
         set_u32(LED_OFF, ACER_CAP_MAILLED);
         data->mailled = value;
     }
 
     if (has_cap(ACER_CAP_BRIGHTNESS)) {
         get_u32(&value, ACER_CAP_BRIGHTNESS);
         data->brightness = value;
     }
 
     return 0;
 }
 
 static int acer_resume(struct device *dev)
 {
     struct acer_data *data = &interface->data;
 
     if (!data)
         return -ENOMEM;
 
     if (has_cap(ACER_CAP_MAILLED))
         set_u32(data->mailled, ACER_CAP_MAILLED);
 
     if (has_cap(ACER_CAP_BRIGHTNESS))
         set_u32(data->brightness, ACER_CAP_BRIGHTNESS);
 
     if (acer_wmi_accel_dev)
         acer_gsensor_init();
 
     return 0;
 }
 #else
 #define acer_suspend	NULL
 #define acer_resume	NULL
 #endif
 
 static SIMPLE_DEV_PM_OPS(acer_pm, acer_suspend, acer_resume);
 
 static void acer_platform_shutdown(struct platform_device *device)
 {
     struct acer_data *data = &interface->data;
 
     if (!data)
         return;
 
     if (has_cap(ACER_CAP_MAILLED))
         set_u32(LED_OFF, ACER_CAP_MAILLED);
 }
 
 static struct platform_driver acer_platform_driver = {
     .driver = {
         .name = "acer-wmi",
         .pm = &acer_pm,
     },
     .probe = acer_platform_probe,
     .remove = acer_platform_remove,
     .shutdown = acer_platform_shutdown,
 };
 
 static struct platform_device *acer_platform_device;
 
 static void remove_debugfs(void)
 {
     debugfs_remove_recursive(interface->debug.root);
 }
 
 static void __init create_debugfs(void)
 {
     interface->debug.root = debugfs_create_dir("acer-wmi", NULL);
 
     debugfs_create_u32("devices", S_IRUGO, interface->debug.root,
                &interface->debug.wmid_devices);
 }

 static const enum acer_wmi_predator_v4_sensor_id acer_wmi_temp_channel_to_sensor_id[] = {
    [0] = ACER_WMID_SENSOR_CPU_TEMPERATURE,
    [1] = ACER_WMID_SENSOR_GPU_TEMPERATURE,
    [2] = ACER_WMID_SENSOR_EXTERNAL_TEMPERATURE_2,
};

static const enum acer_wmi_predator_v4_sensor_id acer_wmi_fan_channel_to_sensor_id[] = {
    [0] = ACER_WMID_SENSOR_CPU_FAN_SPEED,
    [1] = ACER_WMID_SENSOR_GPU_FAN_SPEED,
};
 
 static umode_t acer_wmi_hwmon_is_visible(const void *data,
                      enum hwmon_sensor_types type, u32 attr,
                      int channel)
 {
     enum acer_wmi_predator_v4_sensor_id sensor_id;
     const u64 *supported_sensors = data;
 
     switch (type) {
     case hwmon_temp:
         sensor_id = acer_wmi_temp_channel_to_sensor_id[channel];
         break;
     case hwmon_fan:
         sensor_id = acer_wmi_fan_channel_to_sensor_id[channel];
         break;
     default:
         return 0;
     }
 
     if (*supported_sensors & BIT(sensor_id - 1))
         return 0444;
 
     return 0;
 }
 
 static int acer_wmi_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
                    u32 attr, int channel, long *val)
 {
     u64 command = ACER_WMID_CMD_GET_PREDATOR_V4_SENSOR_READING;
     u64 result;
     int ret;
 
     switch (type) {
     case hwmon_temp:
         command |= FIELD_PREP(ACER_PREDATOR_V4_SENSOR_INDEX_BIT_MASK,
                       acer_wmi_temp_channel_to_sensor_id[channel]);
 
         ret = WMID_gaming_get_sys_info(command, &result);
         if (ret < 0)
             return ret;
 
         result = FIELD_GET(ACER_PREDATOR_V4_SENSOR_READING_BIT_MASK, result);
         *val = result * MILLIDEGREE_PER_DEGREE;
         return 0;
     case hwmon_fan:
         command |= FIELD_PREP(ACER_PREDATOR_V4_SENSOR_INDEX_BIT_MASK,
                       acer_wmi_fan_channel_to_sensor_id[channel]);
 
         ret = WMID_gaming_get_sys_info(command, &result);
         if (ret < 0)
             return ret;
 
         *val = FIELD_GET(ACER_PREDATOR_V4_SENSOR_READING_BIT_MASK, result);
         return 0;
     default:
         return -EOPNOTSUPP;
     }
 } 
 
 static const struct hwmon_channel_info *const acer_wmi_hwmon_info[] = {
     HWMON_CHANNEL_INFO(temp,
                HWMON_T_INPUT,
                HWMON_T_INPUT,
                HWMON_T_INPUT
                ),
     HWMON_CHANNEL_INFO(fan,
                HWMON_F_INPUT,
                HWMON_F_INPUT
                ),
     NULL
 };
 
 static const struct hwmon_ops acer_wmi_hwmon_ops = {
     .read = acer_wmi_hwmon_read,
     .is_visible = acer_wmi_hwmon_is_visible,
 };
 
 static const struct hwmon_chip_info acer_wmi_hwmon_chip_info = {
     .ops = &acer_wmi_hwmon_ops,
     .info = acer_wmi_hwmon_info,
 };
 
 static int acer_wmi_hwmon_init(void)
 {
     struct device *dev = &acer_platform_device->dev;
     struct device *hwmon;
     u64 result;
     int ret;
 
     ret = WMID_gaming_get_sys_info(ACER_WMID_CMD_GET_PREDATOR_V4_SUPPORTED_SENSORS, &result);
     if (ret < 0)
         return ret;
 
     /* Return early if no sensors are available */
     supported_sensors = FIELD_GET(ACER_PREDATOR_V4_SUPPORTED_SENSORS_BIT_MASK, result);
     if (!supported_sensors)
         return 0;
 
     hwmon = devm_hwmon_device_register_with_info(dev, "acer",
                              &supported_sensors,
                              &acer_wmi_hwmon_chip_info,
                              NULL);
 
     if (IS_ERR(hwmon)) {
         dev_err(dev, "Could not register acer hwmon device\n");
         return PTR_ERR(hwmon);
     }
 
     return 0;
 }
 
 static int __init acer_wmi_init(void)
 {
     int err;
 
     pr_info("Acer Laptop ACPI-WMI Extras\n");
 
     if (dmi_check_system(acer_blacklist)) {
         pr_info("Blacklisted hardware detected - not loading\n");
         return -ENODEV;
     }
 
     find_quirks();
 
     /*
      * The AMW0_GUID1 wmi is not only found on Acer family but also other
      * machines like Lenovo, Fujitsu and Medion. In the past days,
      * acer-wmi driver handled those non-Acer machines by quirks list.
      * But actually acer-wmi driver was loaded on any machines that have
      * AMW0_GUID1. This behavior is strange because those machines should
      * be supported by appropriate wmi drivers. e.g. fujitsu-laptop,
      * ideapad-laptop. So, here checks the machine that has AMW0_GUID1
      * should be in Acer/Gateway/Packard Bell white list, or it's already
      * in the past quirk list.
      */
     if (wmi_has_guid(AMW0_GUID1) &&
         !dmi_check_system(amw0_whitelist) &&
         quirks == &quirk_unknown) {
         pr_debug("Unsupported machine has AMW0_GUID1, unable to load\n");
         return -ENODEV;
     }
 
     /*
      * Detect which ACPI-WMI interface we're using.
      */
     if (wmi_has_guid(AMW0_GUID1) && wmi_has_guid(WMID_GUID1))
         interface = &AMW0_V2_interface;
 
     if (!wmi_has_guid(AMW0_GUID1) && wmi_has_guid(WMID_GUID1))
         interface = &wmid_interface;
 
     if (wmi_has_guid(WMID_GUID3))
         interface = &wmid_v2_interface;
 
     if (interface)
         dmi_walk(type_aa_dmi_decode, NULL);
 
     if (wmi_has_guid(WMID_GUID2) && interface) {
         if (!has_type_aa && ACPI_FAILURE(WMID_set_capabilities())) {
             pr_err("Unable to detect available WMID devices\n");
             return -ENODEV;
         }
         /* WMID always provides brightness methods */
         interface->capability |= ACER_CAP_BRIGHTNESS;
     } else if (!wmi_has_guid(WMID_GUID2) && interface && !has_type_aa && force_caps == -1) {
         pr_err("No WMID device detection method found\n");
         return -ENODEV;
     }
 
     if (wmi_has_guid(AMW0_GUID1) && !wmi_has_guid(WMID_GUID1)) {
         interface = &AMW0_interface;
 
         if (ACPI_FAILURE(AMW0_set_capabilities())) {
             pr_err("Unable to detect available AMW0 devices\n");
             return -ENODEV;
         }
     }
 
     if (wmi_has_guid(AMW0_GUID1))
         AMW0_find_mailled();
 
     if (!interface) {
         pr_err("No or unsupported WMI interface, unable to load\n");
         return -ENODEV;
     }
 
     set_quirks();
 
     if (acpi_video_get_backlight_type() != acpi_backlight_vendor)
         interface->capability &= ~ACER_CAP_BRIGHTNESS;
 
     if (wmi_has_guid(WMID_GUID3))
         interface->capability |= ACER_CAP_SET_FUNCTION_MODE;
 
     if (force_caps != -1)
         interface->capability = force_caps;
 
     if (wmi_has_guid(WMID_GUID3) &&
         (interface->capability & ACER_CAP_SET_FUNCTION_MODE)) {
         if (ACPI_FAILURE(acer_wmi_enable_rf_button()))
             pr_warn("Cannot enable RF Button Driver\n");
 
         if (ec_raw_mode) {
             if (ACPI_FAILURE(acer_wmi_enable_ec_raw())) {
                 pr_err("Cannot enable EC raw mode\n");
                 return -ENODEV;
             }
         } else if (ACPI_FAILURE(acer_wmi_enable_lm())) {
             pr_err("Cannot enable Launch Manager mode\n");
             return -ENODEV;
         }
     } else if (ec_raw_mode) {
         pr_info("No WMID EC raw mode enable method\n");
     }
 
     if (wmi_has_guid(ACERWMID_EVENT_GUID)) {
         err = acer_wmi_input_setup();
         if (err)
             return err;
         err = acer_wmi_accel_setup();
         if (err && err != -ENODEV)
             pr_warn("Cannot enable accelerometer\n");
     }
 
     err = platform_driver_register(&acer_platform_driver);
     if (err) {
         pr_err("Unable to register platform driver\n");
         goto error_platform_register;
     }
 
     acer_platform_device = platform_device_alloc("acer-wmi", PLATFORM_DEVID_NONE);
     if (!acer_platform_device) {
         err = -ENOMEM;
         goto error_device_alloc;
     }
 
     err = platform_device_add(acer_platform_device);
     if (err)
         goto error_device_add;
 
     if (wmi_has_guid(WMID_GUID2)) {
         interface->debug.wmid_devices = get_wmid_devices();
         create_debugfs();
     }
 
     /* Override any initial settings with values from the commandline */
     acer_commandline_init();
 
     return 0;
 
 error_device_add:
     platform_device_put(acer_platform_device);
 error_device_alloc:
     platform_driver_unregister(&acer_platform_driver);
 error_platform_register:
     if (wmi_has_guid(ACERWMID_EVENT_GUID))
         acer_wmi_input_destroy();
     if (acer_wmi_accel_dev)
         input_unregister_device(acer_wmi_accel_dev);
 
     return err;
 }
 
 static void __exit acer_wmi_exit(void)
 {
     if (wmi_has_guid(ACERWMID_EVENT_GUID))
         acer_wmi_input_destroy();
 
     if (acer_wmi_accel_dev)
         input_unregister_device(acer_wmi_accel_dev);
 
     remove_debugfs();
     platform_device_unregister(acer_platform_device);
     platform_driver_unregister(&acer_platform_driver);
 
     pr_info("Acer Laptop WMI Extras unloaded\n");
 }
 
 module_init(acer_wmi_init);
 module_exit(acer_wmi_exit);
