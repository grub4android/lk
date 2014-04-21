ZLIB_LOCAL_PATH := $(GET_LOCAL_DIR)

ZLIB_SRCS = \
	gunzip.c \
	gzip.c \
	src/inffast.c \
	src/inftrees.c \
	src/trees.c \
	src/inflate.c \
	src/deflate.c \
	src/uncompr.c \
	src/compress.c \
	src/adler32.c \
	src/crc32.c \
	src/zutil.c


ZLIB_OBJS = $(ZLIB_SRCS:%.c=%.o)

INCLUDES += \
	-I. \
	-I$(ZLIB_LOCAL_PATH) \
	-I$(ZLIB_LOCAL_PATH)/src

OBJS += $(addprefix $(ZLIB_LOCAL_PATH)/, $(ZLIB_OBJS))


