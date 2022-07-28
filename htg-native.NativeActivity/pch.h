//
// pch.h
// Header for standard system include files.
//
// Used by the build system to generate the precompiled header. Note that no
// pch.cpp is needed and the pch.h is automatically included in all cpp files
// that are part of the project
//

#include <jni.h>
#include <errno.h>

#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>

#include <android/log.h>
#include "android_native_app_glue.h"
#include <android/native_window_jni.h>
#include <android/rect.h>
#include <android/surface_control.h>
#include <android/window.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/obb.h>
#include <android/native_activity.h>
#include <android/storage_manager.h>
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
