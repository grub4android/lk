LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

EXT4_SRCS = \
	ext4_fs.c \
	ext4.c \
	ext4_bcache.c \
	ext4_dir_idx.c \
	ext4_hash.c \
	ext4_extent.c \
	ext4_super.c \
	ext4_debug.c \
	ext4_bitmap.c \
	ext4_dir.c \
	ext4_block_group.c \
	ext4_ialloc.c \
	ext4_balloc.c \
	ext4_blockdev.c \
	ext4_inode.c \
	blockdev/ext4_mmcdev.c

GLOBAL_INCLUDES += \
	$(LOCAL_DIR) \
	$(LOCAL_DIR)/blockdev

MODULE_SRCS += $(addprefix $(LOCAL_DIR)/, $(EXT4_SRCS))

include make/module.mk
