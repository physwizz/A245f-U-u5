load("@mgk_info//:kernel_version.bzl",
     "kernel_version",
)

product_device_modules_srcs = [
    # keep sorted
    # "//kernel_device_modules-{}/drivers/samsung:ddk_srcs".format(kernel_version),
    # "//kernel_device_modules-{}/drivers/samsung/debug:ddk_srcs".format(kernel_version),
    "//kernel_device_modules-{}/sound/soc/codecs/sma1305:srcs".format(kernel_version),
    "//kernel_device_modules-{}/drivers/gpu/drm/panel/mcd_panel:srcs".format(kernel_version),
]

product_device_modules_kconfig = [
    # "//kernel_device_modules-{}/drivers/samsung:ddk_kconfigs".format(kernel_version),
    # "//kernel_device_modules-{}/drivers/samsung/debug:ddk_kconfigs".format(kernel_version),
    "//kernel_device_modules-{}/sound/soc/codecs/sma1305:ddk_kconfigs".format(kernel_version),
]

product_device_modules = [
    # "//kernel_device_modules-{}/drivers/samsung/debug:sec_debug".format(kernel_version),
    # "//kernel_device_modules-{}/drivers/samsung/sec_reboot".format(kernel_version),
    "//kernel_device_modules-{}/sound/soc/codecs/sma1305:snd-soc-sma1305".format(kernel_version),
    "//kernel_device_modules-{}/drivers/gpu/drm/panel/mcd_panel:mcd_panel_adapter".format(kernel_version),
    "//kernel_device_modules-{}/drivers/leds:leds-s2mf301".format(kernel_version),
    "//kernel_device_modules-{}/drivers/misc/mediatek/flashlight:flashlights-s2mf301".format(kernel_version),
    "//kernel_device_modules-{}/drivers/samsung/pm:sec_wakeup_cpu_allocator".format(kernel_version),
]

product_gki_modules = [
    # "drivers/watchdog/softdog.ko",
    "drivers/i2c/busses/i2c-gpio.ko",
    "drivers/i2c/algos/i2c-algo-bit.ko",
]
