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
    bootloader.c \
    install.c \
    roots.c \
    ui.c \
    verifier.c \
    encryptedfs_provisioning.c

# add source files for extra commands
LOCAL_SRC_FILES += \
    extracommands/flash_image.c \
    extracommands/format.c

# add our menus
LOCAL_SRC_FILES += \
    menus/nandroid_menu.c \
    menus/install_menu.c \
    menus/mount_menu.c \
    menus/wipe_menu.c

LOCAL_MODULE := recovery

LOCAL_FORCE_STATIC_EXECUTABLE := true

ifeq ($(PLATFORM_SDK_VERSION),8)
	RECOVERY_API_VERSION := 2
else
	RECOVERY_API_VERSION := 3
endif
LOCAL_CFLAGS += -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)

BOARD_RECOVERY_DEFINES := \
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
LOCAL_STATIC_LIBRARIES += libz libbusybox libclearsilverregex
LOCAL_STATIC_LIBRARIES += libminzip libunz libmtdutils libmincrypt
LOCAL_STATIC_LIBRARIES += libminui libpixelflinger_static libpng libcutils
LOCAL_STATIC_LIBRARIES += libstdc++ libc

ifeq ($(USE_INTERNAL_EXT4UTILS),true)
    LOCAL_C_INCLUDES += bootable/recovery/ext4_utils
else
    LOCAL_C_INCLUDES += system/extras/ext4_utils
endif

include $(BUILD_EXECUTABLE)

# make some recovery symlinks
RECOVERY_LINKS := busybox flash_image format
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
RECOVERY_BUSYBOX_SYMLINKS := $(addprefix $(TARGET_RECOVERY_ROOT_OUT)/sbin/,$(filter-out $(exclude),$(notdir $(BUSYBOX_LINKS))))
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
include $(commands_recovery_local_path)/mtdutils/Android.mk
include $(commands_recovery_local_path)/tools/Android.mk
include $(commands_recovery_local_path)/edify/Android.mk
include $(commands_recovery_local_path)/updater/Android.mk
include $(commands_recovery_local_path)/applypatch/Android.mk
commands_recovery_local_path :=

endif   # TARGET_ARCH == arm
endif    # !TARGET_SIMULATOR

