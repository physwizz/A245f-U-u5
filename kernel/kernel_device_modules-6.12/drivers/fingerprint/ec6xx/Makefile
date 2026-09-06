#
# Makefile for the Fingerprint sensor driver.
#

# Each configuration option enables a list of files.

ccflags-y := -Wformat

obj-$(CONFIG_SENSORS_EC6XX)	+= fingerprint_module.o
fingerprint_module-$(CONFIG_SENSORS_EC6XX)	+= fingerprint_common.o fingerprint_common_ap.o ec6xx-spi.o ec6xx-spi_data_transfer.o