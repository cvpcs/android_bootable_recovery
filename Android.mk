ifneq ($(TARGET_SIMULATOR),true)
ifeq ($(TARGET_ARCH),arm)

# if building on froyo, we need the internal ext4_utils library
ifeq ($(PLATFORM_SDK_VERSION),8)
    USE_INTERNAL_EXT4UTILS := true
endif

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

commands_recovery_local_path := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
    recovery.c \
    recovery_lib.c \
    recovery_menu.c \
    recovery_config.c \
    bootloader.c \
    install.c \
    roots.c \
    mounts.c \
    ui.c \
    verifier.c \
    encryptedfs_provisioning.c

# add nandroid
LOCAL_SRC_FILES += \
    nandroid/nandroid.c \
    nandroid/nandroid_raw.c \
    nandroid/nandroid_scan.c \
    nandroid/nandroid_tar.c \
    nandroid/nandroid_yaffs.c

# add our menus
LOCAL_SRC_FILES += \
    menus/nandroid_menu.c \
    menus/install_menu.c \
    menus/mount_menu.c \
    menus/options_menu.c \
    menus/wipe_menu.c

LOCAL_MODULE := recovery

LOCAL_FORCE_STATIC_EXECUTABLE := true

# determine our API version based on whether we're in froyo
ifeq ($(PLATFORM_SDK_VERSION),8)
RECOVERY_API_VERSION := 2
else
RECOVERY_API_VERSION := 3
endif
LOCAL_CFLAGS += -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)

BOARD_RECOVERY_DEFINES := \
	BOARD_HAS_PHYSICAL_KEYBOARD \
	BOARD_HAS_NO_SELECT_BUTTON \
	BOARD_HAS_NO_MISC_PARTITION

$(foreach board_define,$(BOARD_RECOVERY_DEFINES), \
  $(if $($(board_define)), \
    $(eval LOCAL_CFLAGS += -D$(board_define)=\"$($(board_define))\") \
  ) \
  )

# This binary is in the recovery ramdisk, which is otherwise a copy of root.
# It gets copied there in config/Makefile.  LOCAL_MODULE_TAGS suppresses
# a (redundant) copy of the binary in /system/bin for user builds.
# TODO: Build the ramdisk image in a more principled way.

LOCAL_MODULE_TAGS := eng

LOCAL_STATIC_LIBRARIES :=
ifeq ($(TARGET_RECOVERY_UI_LIB),)
  LOCAL_SRC_FILES += default_recovery_ui.c
else
  LOCAL_STATIC_LIBRARIES += $(TARGET_RECOVERY_UI_LIB)
endif
ifeq ($(USE_INTERNAL_EXT4UTILS),true)
    LOCAL_STATIC_LIBRARIES += libext4_recovery_utils
else
    LOCAL_STATIC_LIBRARIES += libext4_utils
endif
LOCAL_STATIC_LIBRARIES += libe2fsck libtune2fs libmke2fs libext2fs libext2_blkid libext2_uuid libext2_profile libext2_com_err libext2_e2p
LOCAL_STATIC_LIBRARIES += libz libbusybox libclearsilverregex libunyaffs libmkyaffs2image
LOCAL_STATIC_LIBRARIES += libflash_image libdump_image liberase_image libxz liblzma
LOCAL_STATIC_LIBRARIES += libminzip libunz libflashutils libmtdutils libmmcutils libbmlutils libmincrypt
LOCAL_STATIC_LIBRARIES += libminui libpixelflinger_static libpng libcutils
LOCAL_STATIC_LIBRARIES += libstdc++ libc

ifeq ($(USE_INTERNAL_EXT4UTILS),true)
    LOCAL_C_INCLUDES += bootable/recovery/ext4_utils
else
    LOCAL_C_INCLUDES += system/extras/ext4_utils
endif
LOCAL_C_INCLUDES += external/yaffs2/yaffs2/utils

include $(BUILD_EXECUTABLE)

# make some recovery symlinks
RECOVERY_LINKS := busybox e2fsck flash_image dump_image erase_image mke2fs tune2fs
RECOVERY_LINKS += lzcat lzma unlzma unxz xz xzcat
RECOVERY_SYMLINKS := $(addprefix $(TARGET_RECOVERY_ROOT_OUT)/sbin/,$(RECOVERY_LINKS))
$(RECOVERY_SYMLINKS): RECOVERY_BINARY := $(LOCAL_MODULE)
$(RECOVERY_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Symlink: $@ -> $(RECOVERY_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf $(RECOVERY_BINARY) $@
ALL_DEFAULT_INSTALLED_MODULES += $(RECOVERY_SYMLINKS)

# Now let's do busybox symlinks
BUSYBOX_LINKS := $(shell cat external/busybox/busybox-minimal.links)
RECOVERY_BUSYBOX_SYMLINKS := $(addprefix $(TARGET_RECOVERY_ROOT_OUT)/sbin/,$(filter-out $(RECOVERY_LINKS),$(notdir $(BUSYBOX_LINKS))))
$(RECOVERY_BUSYBOX_SYMLINKS): BUSYBOX_BINARY := busybox
$(RECOVERY_BUSYBOX_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Symlink: $@ -> $(BUSYBOX_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf $(BUSYBOX_BINARY) $@
ALL_DEFAULT_INSTALLED_MODULES += $(RECOVERY_BUSYBOX_SYMLINKS)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := verifier_test.c verifier.c
LOCAL_MODULE := verifier_test
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_TAGS := tests
LOCAL_STATIC_LIBRARIES := libmincrypt libcutils libstdc++ libc
include $(BUILD_EXECUTABLE)

ifeq ($(USE_INTERNAL_EXT4UTILS),true)
    include $(commands_recovery_local_path)/ext4_utils/Android.mk
endif
include $(commands_recovery_local_path)/minui/Android.mk
include $(commands_recovery_local_path)/minzip/Android.mk
include $(commands_recovery_local_path)/flashutils/Android.mk
include $(commands_recovery_local_path)/mtdutils/Android.mk
include $(commands_recovery_local_path)/mmcutils/Android.mk
include $(commands_recovery_local_path)/bmlutils/Android.mk
include $(commands_recovery_local_path)/tools/Android.mk
include $(commands_recovery_local_path)/edify/Android.mk
include $(commands_recovery_local_path)/updater2/Android.mk
include $(commands_recovery_local_path)/updater3/Android.mk
include $(commands_recovery_local_path)/applypatch/Android.mk
commands_recovery_local_path :=

endif   # TARGET_ARCH == arm
endif    # !TARGET_SIMULATOR

