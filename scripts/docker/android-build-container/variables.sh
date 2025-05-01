#!/bin/bash
# When changing Qt version remember to update the 
# qt-installer-noninteractive file as well.
QT_VERSION=5.15
LATEST_QT=5.15.1
NDK_VERSION=27.2.12479018
ANDROID_BUILDTOOLS_REVISION=34.0.0
ANDROID_PLATFORM_LEVEL=26
ANDROID_PLATFORMS=android-34
ANDROID_NDK=ndk/${NDK_VERSION}
# OpenSSL also has an entry in get-dep-lib.sh line 103
# that needs to be updated as well.
OPENSSL_VERSION=1.1.1w
