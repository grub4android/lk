TARGET := qcom-msm8610
#QCOM_ENABLE_UART := 1

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
	lib/partition

