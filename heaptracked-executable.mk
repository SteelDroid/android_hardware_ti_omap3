LOCAL_SRC_FILES += $(OMAP3_DEBUG_SRC)
LOCAL_SHARED_LIBRARIES += $(OMAP3_DEBUG_SHARED_LIBRARIES)
LOCAL_STATIC_LIBRARIES:= libheaptracker
LOCAL_CFLAGS += $(OMAP3_DEBUG_CFLAGS)
LOCAL_LDFLAGS += $(OMAP3_DEBUG_LDFLAGS)

include $(BUILD_EXECUTABLE)