
# SPDX-License-Identifier: GPL-2.0
# COPYRIGHT(C) 2025 Samsung Electronics Co., Ltd. All Right Reserved.

lego_module_list_first = [

]

lego_module_list_second = [
"drivers/input/input_boost/input_booster_lkm.ko"
]

lego_module_list = [
"drivers/battery/charger/sm5714_charger/sm5714-charger.ko",
"drivers/battery/common/sb_wireless.ko",
"drivers/battery/common/sec-battery.ko",
"drivers/battery/common/sec-direct-charger.ko",
"drivers/battery/common/sec_pd.ko",
"drivers/battery/core/sb-core.ko",
"drivers/battery/fuelgauge/sm5714_fuelgauge/sm5714_fuelgauge.ko",
"drivers/gpu/drm/samsung/panel/mcd-panel.ko",
"drivers/gpu/drm/samsung/panel/oled_common/usdm-panel-oled-common.ko",
"drivers/knox/hdm/hdm.ko",
"drivers/knox/kzt/kzt.ko",
"drivers/knox/ngksm/ngksm.ko",
"drivers/mstdrv/mstdrv.ko",
"drivers/muic/common/common_muic.ko",
"drivers/samsung/pm/sec_thermistor/sec_thermistor.ko",
"drivers/sec_panel_notifier_v2/sec_panel_notifier_v2.ko",
"drivers/sensorhub/shub.ko",
"drivers/sensorhub/utility/sensor_core.ko",
"drivers/sensors/sx938x.ko",
"drivers/staging/android/switch/sec_switch_class.ko",
"drivers/sti/abc/abc.ko",
"drivers/sti/abc/abc_hub.ko",
"drivers/usb/common/vbus_notifier/vbus_notifier.ko",
"drivers/usb/typec/common/pdic_notifier_module.ko",
"drivers/usb/typec/sm/sm5714/pdic_sm5714.ko",
"drivers/vibrator/common/sec_vibrator.ko",
"drivers/vibrator/common/vib_info/vibrator_vib_info.ko",
"drivers/vibrator/dc/dc_vibrator.ko",
"if_cb_manager.ko",
"s2mpb03.ko",
"usb_typec_manager.ko"
]

lego_dtbo_list = [
"a24_eur_open_beni_6.12_w00_r06.dtbo",
"a24_eur_open_beni_6.12_w00_r05.dtbo",
"a24_eur_open_beni_6.12_w00_r04.dtbo",
"a24_eur_open_beni_6.12_w00_r03.dtbo",
"a24_eur_open_beni_6.12_w00_r00.dtbo"
]
lego_model = 'a24'