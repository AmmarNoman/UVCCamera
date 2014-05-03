/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014 saki t_saki@serenegiant.com
 *
 * File name: localdefines.h
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * All files in the folder are under this Apache License, Version 2.0.
 * Files in the jni/libjpeg, jni/libusb and jin/libuvc folder may have a different license, see the respective files.
*/

#ifndef LOCALDEFINES_H_
#define LOCALDEFINES_H_

#include <jni.h>

#define LOG_TAG "libUVCCamera"

// write back array that got by getXXXArrayElements into original Java object and release its array
#define	ARRAYELEMENTS_COPYBACK_AND_RELEASE 0
// write back array that got by getXXXArrayElements into origianl Java object but do not release its array
#define	ARRAYELEMENTS_COPYBACK_ONLY JNI_COMMIT
// never write back array that got by getXXXArrayElements but release its array
#define ARRAYELEMENTS_ABORT_AND_RELEASE JNI_ABORT

//#define USE_LOGALL	// If you don't need to all LOG, comment out this line and select follows
//#define USE_LOGV
//#define USE_LOGD
//#define USE_LOGI
#define USE_LOGW
#define USE_LOGE
#define USE_LOGF


// Absolute class name of Java object
// if you change the package name of AndroBulletGlue library, you must fix these
#define		JTYPE_SYSTEM				"Ljava/lang/System;"

//
typedef		jlong						ID_TYPE;

#endif /* LOCALDEFINES_H_ */
