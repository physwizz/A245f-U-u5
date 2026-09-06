dtb-y += a24_eur_open_beni_6.12_w00_r06.dtbo
dtb-y += a24_eur_open_beni_6.12_w00_r05.dtbo
dtb-y += a24_eur_open_beni_6.12_w00_r04.dtbo
dtb-y += a24_eur_open_beni_6.12_w00_r03.dtbo
dtb-y += a24_eur_open_beni_6.12_w00_r00.dtbo

# SPDX-License-Identifier: GPL-2.0

# for the include dt-bindings header files in the kernel_device_modules-X.Y
# common, for include/dt-bindings
DTC_INCLUDE += $(DEVICE_MODULES_PATH)/include
# arm64
DTC_INCLUDE += $(DEVICE_MODULES_PATH)/arch/$(SRCARCH)/boot/dts $(DEVICE_MODULES_PATH)/arch/$(SRCARCH)/boot/dts/mediatek

# for cust.dtsi
#DTC_INCLUDE += $(DEVICE_MODULES_REL_PATH)/arch/$(SRCARCH)/boot/dts
DTC_INCLUDE += $(dir $(obj))

# for devicetree overlay
DTC_FLAGS += -@

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb

