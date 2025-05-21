#!/bin/bash -eu

# run this in the top level folder you want to create Android binaries in
#
# the script requires the Android Command Line Tools to be in cmdline-tools/bin
#

exec 1> >(tee ./build.log) 2>&1

if [ "$(uname)" != Linux ] ; then
	echo "only on Linux so far"
	exit 1
fi

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"

# these are the current versions for Qt, Android SDK & NDK:
source "$SCRIPTDIR"/variables.sh

# make sure we have the required commands installed
MISSING=
for i in git cmake autoconf libtool java curl unzip; do
	command -v $i >/dev/null ||
		if [ $i = libtool ] ; then
			MISSING="${MISSING}libtool-bin "
		elif [ $i = java ] ; then
			MISSING="${MISSING}openjdk-8-jdk "
		else
			MISSING="${MISSING}${i} "
		fi
done
if [ "$MISSING" ] ; then
	echo "The following packages are missing: $MISSING"
	echo "Please install via your package manager."
	exit 1
fi

# install the Android Command Line Tools
curl -O -L https://dl.google.com/android/repository/commandlinetools-linux-13114758_latest.zip
unzip commandlinetools-linux-*.zip
rm commandlinetools-linux-*.zip

# we need to get the Android SDK and NDK
export JAVA_HOME=/usr
export INSTALL_DIR=$(pwd)
export PATH=$INSTALL_DIR/cmdline-tools/bin:/usr/local/bin:/bin:/usr/bin
rm -rf cmdline-tools/latest
yes | sdkmanager --sdk_root="$INSTALL_DIR" "ndk;$NDK_VERSION" "cmdline-tools;latest" "platform-tools" "platforms;$ANDROID_PLATFORMS" "build-tools;$ANDROID_BUILDTOOLS_REVISION"
yes | sdkmanager --sdk_root="$INSTALL_DIR" --licenses

# next check that Qt is installed
if [ ! -d "$INSTALL_DIR/$QT_DIR" ] ; then
	curl -L -O https://download.qt.io/official_releases/online_installers/qt-online-installer-linux-x64-online.run
	chmod +x qt-online-installer-linux-x64-online.run
    QT_PACKAGE_VERSION=$(echo $QT_VERSION | sed 's/\.//g')
	./qt-online-installer-linux-x64-online.run --root $INSTALL_DIR/Qt --accept-licenses --accept-obligations --confirm-command --default-answer --email $QT_EMAIL --password $QT_PASSWORD --no-save-account install qt.qt5.$QT_PACKAGE_VERSION
	rm qt-online-installer-linux-x64-online.run
fi

echo "things are set up for the Android build"
