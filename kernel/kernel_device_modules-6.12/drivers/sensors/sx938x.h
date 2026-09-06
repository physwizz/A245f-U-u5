/*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*/
#ifndef SX938x_H
#define SX938x_H


#define VENDOR_NAME              "SEMTECH"
#define NOTI_MODULE_NAME         "grip_notifier"


/* IrqStat 0:Inactive 1:Active */
#define SX938x_IRQSTAT_RESET_FLAG		0x10
#define SX938x_IRQSTAT_TOUCH_FLAG		0x08
#define SX938x_IRQSTAT_RELEASE_FLAG		0x04
#define SX938x_IRQSTAT_COMPDONE_FLAG		0x02
#define SX938x_IRQSTAT_CONV_FLAG		0x01

#define SX938x_STAT_COMPSTAT_ALL_FLAG (0x02 | 0x04)

/*
*  I2C Registers
*/
//-Interrupt and status
#define SX938x_IRQSTAT_REG		0x00
#define SX938x_STAT_REG			0x01
#define SX938x_IRQ_ENABLE_REG	0x02
#define SX938x_IRQCFG_REG		0x03
//-General control
#define SX938x_GNRL_CTRL0_REG	0x10
#define SX938x_GNRL_CTRL1_REG	0x11
#define SX938x_GNRL_CTRL2_REG	0x12
//-AFE Control - Main - Ref
#define SX938x_AFE_CTRL1_REG		0x21
#define SX938x_AFE_PARAM0_PHR_REG	0x22
#define SX938x_AFE_PARAM1_PHR_REG	0x23
#define SX938x_AFE_PARAM0_PHM_REG	0x24
#define SX938x_AFE_PARAM1_PHM_REG	0x25

//-Advanced Digital Processing control
#define SX938x_PROXCTRL0_PHR_REG	0x40
#define SX938x_PROXCTRL0_PHM_REG	0x41
#define SX938x_PROXCTRL1_REG	0x42
#define SX938x_PROXCTRL2_REG	0x43
#define SX938x_PROXCTRL3_REG	0x44
#define SX938x_PROXCTRL4_REG	0x45
#define SX938x_PROXCTRL5_REG	0x46
//Ref
#define SX938x_REFCORR0_REG		0x60
#define SX938x_REFCORR1_REG		0x61
//Use Filter
#define SX938x_USEFILTER0_REG	0x70
#define SX938x_USEFILTER1_REG	0x71
#define SX938x_USEFILTER2_REG	0x72
#define SX938x_USEFILTER3_REG	0x73
#define SX938x_USEFILTER4_REG	0x74

/*      Sensor Readback */
#define SX938x_USEMSB_PHR			0x90
#define SX938x_USELSB_PHR			0x91
#define SX938x_OFFSETMSB_PHR		0x92
#define SX938x_OFFSETLSB_PHR		0x93

#define SX938x_USEMSB_PHM			0x94
#define SX938x_USELSB_PHM			0x95
#define SX938x_AVGMSB_PHM			0x96
#define SX938x_AVGLSB_PHM			0x97
#define SX938x_DIFFMSB_PHM			0x98
#define SX938x_DIFFLSB_PHM			0x99
#define SX938x_OFFSETMSB_PHM		0x9A
#define SX938x_OFFSETLSB_PHM		0x9B
#define SX938x_USEFILTMSB			0x9E
#define SX938x_USEFILTLSB			0x9F

#define SX938x_SOFTRESET_REG	0xCF
#define SX938x_WHOAMI_REG		0xFA
#define SX938x_REV_REG			0xFE

#define SX938x_IRQSTAT_PROG2_FLAG		0x04
#define SX938x_IRQSTAT_PROG1_FLAG		0x02
#define SX938x_IRQSTAT_PROG0_FLAG		0x01


/* RegStat0  */
#define SX938x_PROXSTAT_FLAG			0x08

/*      SoftReset */
#define SX938x_SOFTRESET				0xDE
#define SX938x_WHOAMI_VALUE				0x82
#define SX938x_REV_VALUE				0x13

struct sx938x_p {
	struct i2c_client *client;
	struct input_dev *input;
	struct input_dev *noti_input_dev;
	struct device *factory_device;
	struct delayed_work init_work;
	struct delayed_work irq_work;
	struct delayed_work debug_work;
	struct wakeup_source *grip_ws;
	struct mutex mode_mutex;
	struct mutex read_mutex;
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	struct notifier_block pdic_nb;
#endif
#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
	struct notifier_block hall_nb;
#endif
	struct regulator *dvdd_vreg;	/* regulator */
	const char *dvdd_vreg_name;	/* regulator name */

#if IS_ENABLED(CONFIG_SENSORS_COMMON_VDD_SUB)
	int gpio_nirq_sub;
#endif
	int pre_attach;
	int debug_count;
	int debug_zero_count;
	int fail_status_code;
	int is_unknown_mode;
	int motion;
	int irq_count;
	int abnormal_mode;
	int ldo_en;
	int irq;
	int gpio_nirq;
	int state;
	int init_done;
	int noti_enable;
	int again_m;
	int dgain_m;
	int diff_cnt;

	atomic_t enable;

	u32 unknown_sel;

	s32 useful_avg;
	s32 capMain;
	s32 useful;

	u16 detect_threshold;
	u16 offset;

	s16 avg;
	s16 diff;
	s16 diff_avg;
	s16 max_diff;
	s16 max_normal_diff;

	u8 ic_num;

	bool first_working;
	bool skip_data;
};

#define MAX_NUM_STATUS_BITS (8)

enum {
	OFF = 0,
	ON = 1
};

#define GRIP_ERR(fmt, ...) pr_err("[GRIP_%s] %s "fmt, grip_name[data->ic_num], __func__, ##__VA_ARGS__)
#define GRIP_INFO(fmt, ...) pr_info("[GRIP_%s] %s "fmt, grip_name[data->ic_num], __func__, ##__VA_ARGS__)
#define GRIP_WARN(fmt, ...) pr_warn("[GRIP_%s] %s "fmt, grip_name[data->ic_num], __func__, ##__VA_ARGS__)

#if IS_ENABLED(CONFIG_SENSORS_GRIP_FAILURE_DEBUG)
extern void update_grip_error(u8 idx, u32 error_state);
#endif

#if !IS_ENABLED(CONFIG_SENSORS_CORE_AP)
extern int sensors_create_symlink(struct input_dev *inputdev);
extern void sensors_remove_symlink(struct input_dev *inputdev);
extern int sensors_register(struct device **dev, void *drvdata,
			    struct device_attribute *attributes[], char *name);
extern void sensors_unregister(struct device *dev,
			       struct device_attribute *attributes[]);
#endif

int sx938x_get_nirq_state(struct sx938x_p *data);

#endif
