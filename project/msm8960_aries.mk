# top level project rules for the msm8960_virtio project
#
LOCAL_DIR := $(GET_LOCAL_DIR)
include $(LOCAL_DIR)/msm8960.mk

TARGET := msm8960_aries
DEFINES += WITH_XIAOMI_DUALBOOT=1
SPLASH_PARTITION_NAME:=\"logo\"
