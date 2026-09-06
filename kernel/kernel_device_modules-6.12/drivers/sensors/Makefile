#
# Makefile for grip sensors
#

obj-$(CONFIG_SENSORS_SX9380)  		+= sx938x.o


ifeq ($(CONFIG_SEC_KUNIT), y)
obj-$(CONFIG_SEC_KUNIT)			+= kunit_test/
GCOV_PROFILE_sx938x_new.o		:= $(CONFIG_SEC_KUNIT)
endif