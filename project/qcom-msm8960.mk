TARGET := qcom-msm8960
QCOM_ENABLE_UART := 1

MODULES += \
	app/tests \
	app/stringtests \
	app/fastboot \
	lib/aes \
	lib/aes/test \
	lib/bytes \
	lib/cksum \
	lib/debugcommands \
	lib/libm \
	lib/bio \
	lib/partition \
	lib/uefi

GLOBAL_DEFINES += ANDROID_BOOTTYPE_EMMC=1
GLOBAL_DEFINES += ANDROID_FORCE_KERNEL_ADDR=0x80208000
GLOBAL_DEFINES += ANDROID_FORCE_RAMDISK_ADDR=0x82200000
GLOBAL_DEFINES += ANDROID_FORCE_TAGS_ADDR=0x80200100

