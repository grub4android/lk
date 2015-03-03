TARGET := qcom-msm8960
QCOM_ENABLE_UART := 1
QCOM_MMU_IDENTITY_MAP := 1

MODULES += \
	app/tests \
	app/stringtests \
	app/shell \
	lib/aes \
	lib/aes/test \
	lib/bytes \
	lib/cksum \
	lib/debugcommands \
	lib/libm

