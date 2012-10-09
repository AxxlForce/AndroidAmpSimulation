LOCAL_PATH := $(call my-dir)
 
include $(CLEAR_VARS)
 
# Here we give our module name and source file(s)
LOCAL_MODULE    := native
LOCAL_SRC_FILES := native.c

# for native audio
LOCAL_LDLIBS    += -lOpenSLES

# for logging
LOCAL_LDLIBS    += -llog

# for native asset manager
LOCAL_LDLIBS    += -landroid
 
include $(BUILD_SHARED_LIBRARY)