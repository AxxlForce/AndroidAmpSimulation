LOCAL_PATH := $(call my-dir)
 
include $(CLEAR_VARS)
 
LOCAL_LDLIBS += libOpenSLES
 
# Here we give our module name and source file(s)
LOCAL_MODULE    := native
LOCAL_SRC_FILES := native.c
 
include $(BUILD_SHARED_LIBRARY)