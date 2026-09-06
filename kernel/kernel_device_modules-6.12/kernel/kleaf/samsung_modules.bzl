load("//kernel_device_modules-6.12:kernel/kleaf/samsung_product_modules.bzl",
     "product_device_modules_srcs",
     "product_device_modules_kconfig",
     "product_device_modules",
     "product_gki_modules",
)

load("@mgk_info//:kernel_version.bzl",
     "kernel_version",
)

load("//kernel_device_modules-6.12:lego_ddk_modules.bzl",
     "lego_kconfig",
     "lego_modules",
)

_device_modules_srcs = [
    # keep sorted
    # "//kernel_device_modules-{}/drivers/samsung:ddk_srcs".format(kernel_version),
    # "//kernel_device_modules-{}/drivers/samsung/debug:ddk_srcs".format(kernel_version),
]

samsung_device_modules_srcs = _device_modules_srcs + product_device_modules_srcs

# kconfigs in android/kernel/kernel_device_modules-6.x
_device_modules_kconfig = [
    "//kernel_device_modules-{}/drivers/block/zram:ddk_kconfigs".format(kernel_version),
    "//kernel_device_modules-{}/drivers/samsung:ddk_kconfigs".format(kernel_version),
    "//kernel_device_modules-{}/drivers/samsung/debug:ddk_kconfigs".format(kernel_version),
    "//kernel_device_modules-{}/drivers/samsung/pm:ddk_kconfigs".format(kernel_version),
    "//kernel_device_modules-{}/drivers/usb/gadget/function:ddk_kconfigs".format(kernel_version),
    "//kernel_device_modules-{}/drivers/usb/notify:ddk_kconfigs".format(kernel_version),
    "//kernel_device_modules-{}/drivers/misc/drb:ddk_kconfigs".format(kernel_version),
    "//kernel_device_modules-{}/block:ddk_kconfigs".format(kernel_version),
    "//kernel_device_modules-{}/mm/sec_mm:ddk_kconfigs".format(kernel_version),
    "//kernel_device_modules-{}/drivers/gpu/drm/panel/mcd_panel:ddk_kconfigs".format(kernel_version),
]

samsung_device_modules_kconfigs = _device_modules_kconfig + product_device_modules_kconfig + lego_kconfig

# modules in android/kernel/kernel_device_modules-6.x
_device_modules = [
    "//kernel_device_modules-{}/drivers/block/zram:zram".format(kernel_version),
    "//kernel_device_modules-{}/drivers/samsung/debug:sec_debug".format(kernel_version),
    "//kernel_device_modules-{}/drivers/samsung/debug:sec_extra_info".format(kernel_version),
    "//kernel_device_modules-{}/drivers/samsung:sec_reboot".format(kernel_version),
    "//kernel_device_modules-{}/drivers/samsung:sec_reset".format(kernel_version),
    "//kernel_device_modules-{}/drivers/samsung:sec_ext".format(kernel_version),
    "//kernel_device_modules-{}/drivers/samsung:sec_bootstat".format(kernel_version),
    "//kernel_device_modules-{}/drivers/usb/gadget/function:usb_f_conn_gadget".format(kernel_version),
    "//kernel_device_modules-{}/drivers/usb/gadget/function:usb_f_ss_mon_gadget".format(kernel_version),
    "//kernel_device_modules-{}/drivers/usb/gadget/function:usb_f_dm".format(kernel_version),
    "//kernel_device_modules-{}/drivers/usb/notify:usb_notifier_module".format(kernel_version),
    "//kernel_device_modules-{}/drivers/usb/notify:usb_notify_layer".format(kernel_version),
    "//kernel_device_modules-{}/drivers/misc/drb:drb".format(kernel_version),
    "//kernel_device_modules-{}/block:blk-sec-common".format(kernel_version),
    "//kernel_device_modules-{}/block:blk-sec-stats".format(kernel_version),
	"//kernel_device_modules-{}/block:blk-sec-wb".format(kernel_version),
	"//kernel_device_modules-{}/block:ssg".format(kernel_version),
    "//kernel_device_modules-{}/mm/sec_mm:sec_mm".format(kernel_version),
    "//kernel_device_modules-{}/drivers/cpufreq:cpufreq_limit".format(kernel_version),
    "//kernel_device_modules-{}/drivers/samsung/pm:sec_pm_debug".format(kernel_version),
    "//kernel_device_modules-{}/drivers/ufs:ufs_sec".format(kernel_version),
]

samsung_device_modules = _device_modules + product_device_modules + lego_modules

# gki modules in android/kernel-6.x
_gki_modules = [
    "crypto/twofish_common.ko",
    "crypto/twofish_generic.ko",
    "drivers/watchdog/softdog.ko",
]

samsung_gki_modules = _gki_modules + product_gki_modules
