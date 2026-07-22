#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x7b80af92, "debugfs_create_u32" },
	{ 0xb3a1d6ac, "platform_device_put" },
	{ 0x6563b35d, "input_set_capability" },
	{ 0x21eac63b, "acpi_dev_found" },
	{ 0x30eb81ed, "__dynamic_pr_debug" },
	{ 0xd272d446, "dump_stack" },
	{ 0xb47aafff, "kernel_read" },
	{ 0x1d755664, "backlight_device_register" },
	{ 0x9aa6980d, "mutex_lock" },
	{ 0x5a844b26, "__x86_indirect_thunk_rax" },
	{ 0x9aa6980d, "mutex_unlock" },
	{ 0x8c98796d, "devm_platform_profile_register" },
	{ 0xb3162e2f, "sysfs_create_group" },
	{ 0x72ba702e, "led_classdev_register_ext" },
	{ 0x8c1a6af6, "devm_hwmon_device_register_with_info" },
	{ 0xb485f613, "_dev_err" },
	{ 0x67628f51, "msleep" },
	{ 0x1377f9e8, "sparse_keymap_entry_from_scancode" },
	{ 0x32f5d160, "sparse_keymap_report_event" },
	{ 0x47ed45b8, "platform_profile_notify" },
	{ 0x71798f7e, "delayed_work_timer_fn" },
	{ 0x0f2f972e, "param_ops_bool" },
	{ 0x0f2f972e, "param_ops_int" },
	{ 0xd272d446, "__fentry__" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xe8213e80, "_printk" },
	{ 0xbd03ed67, "__ref_stack_chk_guard" },
	{ 0xb1aae510, "wmi_evaluate_method" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0xd272d446, "i8042_lock_chip" },
	{ 0x282ecbcc, "i8042_command" },
	{ 0xd272d446, "i8042_unlock_chip" },
	{ 0x361eec3f, "ec_read" },
	{ 0xc609ff70, "strncpy" },
	{ 0xa5c7582d, "strsep" },
	{ 0xd09b06f5, "kstrtoint" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0x97d7321b, "acpi_format_exception" },
	{ 0x43a349ca, "strlen" },
	{ 0xbd069841, "kstrtoull" },
	{ 0xdd6830c7, "sprintf" },
	{ 0x173ec8da, "sscanf" },
	{ 0x82fd7238, "__ubsan_handle_shift_out_of_bounds" },
	{ 0xe6112ad6, "wmi_has_guid" },
	{ 0x09885971, "rfkill_unregister" },
	{ 0x09885971, "rfkill_destroy" },
	{ 0x85acaba2, "cancel_delayed_work_sync" },
	{ 0xe4de56b4, "__ubsan_handle_load_invalid_value" },
	{ 0x70e9fdd0, "filp_open" },
	{ 0xea1a4412, "kernel_write" },
	{ 0xf19f3c24, "filp_close" },
	{ 0x651bbffe, "acpi_evaluate_object" },
	{ 0x9d87d3d3, "input_event" },
	{ 0x8fa876bd, "input_unregister_device" },
	{ 0x1fb799fc, "debugfs_remove" },
	{ 0xb3a1d6ac, "platform_device_unregister" },
	{ 0x0064884b, "platform_driver_unregister" },
	{ 0x19d5e86c, "wmi_remove_notify_handler" },
	{ 0xbd9aacde, "round_jiffies_relative" },
	{ 0xaef1f20d, "system_percpu_wq" },
	{ 0x8ce83585, "queue_delayed_work_on" },
	{ 0x3dcb6aa5, "rfkill_set_sw_state" },
	{ 0x23da0b63, "rfkill_set_hw_state_reason" },
	{ 0xa08923d7, "rfkill_alloc" },
	{ 0xb8742e01, "rfkill_register" },
	{ 0xf1f059ea, "ec_write" },
	{ 0x8e035422, "led_classdev_unregister" },
	{ 0x5c5a86e0, "backlight_device_unregister" },
	{ 0x83bd6419, "sysfs_remove_group" },
	{ 0x9c25c67b, "dmi_check_system" },
	{ 0x602be3cc, "dmi_walk" },
	{ 0x727e57a3, "__acpi_video_get_backlight_type" },
	{ 0x9659ecf4, "input_allocate_device" },
	{ 0x723af98e, "sparse_keymap_setup" },
	{ 0x040b45e0, "wmi_install_notify_handler" },
	{ 0x50ee5e89, "input_register_device" },
	{ 0xf9e4505f, "acpi_dev_get_first_match_dev" },
	{ 0x8df607d2, "put_device" },
	{ 0xaab063d1, "input_set_abs_params" },
	{ 0xfb598b3a, "__platform_driver_register" },
	{ 0x31b1a988, "platform_device_alloc" },
	{ 0xe29d0252, "platform_device_add" },
	{ 0x8fa876bd, "input_free_device" },
	{ 0x063cc594, "wmi_query_block" },
	{ 0x4583040d, "debugfs_create_dir" },
	{ 0xd954c786, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x7b80af92,
	0xb3a1d6ac,
	0x6563b35d,
	0x21eac63b,
	0x30eb81ed,
	0xd272d446,
	0xb47aafff,
	0x1d755664,
	0x9aa6980d,
	0x5a844b26,
	0x9aa6980d,
	0x8c98796d,
	0xb3162e2f,
	0x72ba702e,
	0x8c1a6af6,
	0xb485f613,
	0x67628f51,
	0x1377f9e8,
	0x32f5d160,
	0x47ed45b8,
	0x71798f7e,
	0x0f2f972e,
	0x0f2f972e,
	0xd272d446,
	0xd272d446,
	0xe8213e80,
	0xbd03ed67,
	0xb1aae510,
	0xcb8b6ec6,
	0xd272d446,
	0xd272d446,
	0x282ecbcc,
	0xd272d446,
	0x361eec3f,
	0xc609ff70,
	0xa5c7582d,
	0xd09b06f5,
	0x90a48d82,
	0x97d7321b,
	0x43a349ca,
	0xbd069841,
	0xdd6830c7,
	0x173ec8da,
	0x82fd7238,
	0xe6112ad6,
	0x09885971,
	0x09885971,
	0x85acaba2,
	0xe4de56b4,
	0x70e9fdd0,
	0xea1a4412,
	0xf19f3c24,
	0x651bbffe,
	0x9d87d3d3,
	0x8fa876bd,
	0x1fb799fc,
	0xb3a1d6ac,
	0x0064884b,
	0x19d5e86c,
	0xbd9aacde,
	0xaef1f20d,
	0x8ce83585,
	0x3dcb6aa5,
	0x23da0b63,
	0xa08923d7,
	0xb8742e01,
	0xf1f059ea,
	0x8e035422,
	0x5c5a86e0,
	0x83bd6419,
	0x9c25c67b,
	0x602be3cc,
	0x727e57a3,
	0x9659ecf4,
	0x723af98e,
	0x040b45e0,
	0x50ee5e89,
	0xf9e4505f,
	0x8df607d2,
	0xaab063d1,
	0xfb598b3a,
	0x31b1a988,
	0xe29d0252,
	0x8fa876bd,
	0x063cc594,
	0x4583040d,
	0xd954c786,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"debugfs_create_u32\0"
	"platform_device_put\0"
	"input_set_capability\0"
	"acpi_dev_found\0"
	"__dynamic_pr_debug\0"
	"dump_stack\0"
	"kernel_read\0"
	"backlight_device_register\0"
	"mutex_lock\0"
	"__x86_indirect_thunk_rax\0"
	"mutex_unlock\0"
	"devm_platform_profile_register\0"
	"sysfs_create_group\0"
	"led_classdev_register_ext\0"
	"devm_hwmon_device_register_with_info\0"
	"_dev_err\0"
	"msleep\0"
	"sparse_keymap_entry_from_scancode\0"
	"sparse_keymap_report_event\0"
	"platform_profile_notify\0"
	"delayed_work_timer_fn\0"
	"param_ops_bool\0"
	"param_ops_int\0"
	"__fentry__\0"
	"__x86_return_thunk\0"
	"_printk\0"
	"__ref_stack_chk_guard\0"
	"wmi_evaluate_method\0"
	"kfree\0"
	"__stack_chk_fail\0"
	"i8042_lock_chip\0"
	"i8042_command\0"
	"i8042_unlock_chip\0"
	"ec_read\0"
	"strncpy\0"
	"strsep\0"
	"kstrtoint\0"
	"__ubsan_handle_out_of_bounds\0"
	"acpi_format_exception\0"
	"strlen\0"
	"kstrtoull\0"
	"sprintf\0"
	"sscanf\0"
	"__ubsan_handle_shift_out_of_bounds\0"
	"wmi_has_guid\0"
	"rfkill_unregister\0"
	"rfkill_destroy\0"
	"cancel_delayed_work_sync\0"
	"__ubsan_handle_load_invalid_value\0"
	"filp_open\0"
	"kernel_write\0"
	"filp_close\0"
	"acpi_evaluate_object\0"
	"input_event\0"
	"input_unregister_device\0"
	"debugfs_remove\0"
	"platform_device_unregister\0"
	"platform_driver_unregister\0"
	"wmi_remove_notify_handler\0"
	"round_jiffies_relative\0"
	"system_percpu_wq\0"
	"queue_delayed_work_on\0"
	"rfkill_set_sw_state\0"
	"rfkill_set_hw_state_reason\0"
	"rfkill_alloc\0"
	"rfkill_register\0"
	"ec_write\0"
	"led_classdev_unregister\0"
	"backlight_device_unregister\0"
	"sysfs_remove_group\0"
	"dmi_check_system\0"
	"dmi_walk\0"
	"__acpi_video_get_backlight_type\0"
	"input_allocate_device\0"
	"sparse_keymap_setup\0"
	"wmi_install_notify_handler\0"
	"input_register_device\0"
	"acpi_dev_get_first_match_dev\0"
	"put_device\0"
	"input_set_abs_params\0"
	"__platform_driver_register\0"
	"platform_device_alloc\0"
	"platform_device_add\0"
	"input_free_device\0"
	"wmi_query_block\0"
	"debugfs_create_dir\0"
	"module_layout\0"
;

MODULE_INFO(depends, "platform_profile,sparse-keymap,wmi,video");


MODULE_INFO(srcversion, "361395F188F6E262706CC8A");
