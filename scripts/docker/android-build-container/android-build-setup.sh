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
export ANDROID_HOME=$(pwd)
export PATH=$ANDROID_HOME/cmdline-tools/bin:/usr/local/bin:/bin:/usr/bin
rm -rf cmdline-tools/latest
yes | sdkmanager --sdk_root="$ANDROID_HOME" "ndk;$NDK_VERSION" "cmdline-tools;latest" "platform-tools" "platforms;$ANDROID_PLATFORMS" "build-tools;$ANDROID_BUILDTOOLS_REVISION"
yes | sdkmanager --sdk_root="$ANDROID_HOME" --licenses

# next check that Qt is installed
if [ ! -d "$LATEST_QT" ] ; then
	pip3 install --break-system-packages --user aqtinstall
	$HOME/.local/bin/aqt install-qt -O "$ANDROID_HOME" linux android "$LATEST_QT"
fi

# Need to use a newer version of gradle
sed -i 's/^distributionUrl=.*$/distributionUrl=https\\:\/\/services.gradle.org\/distributions\/gradle-8.14-bin.zip/g' "$ANDROID_HOME/$LATEST_QT/android/src/3rdparty/gradle/gradle/wrapper/gradle-wrapper.properties"

# set up the gradle.properties file to use AndroidX
echo "android.useAndroidX=true" >> "$ANDROID_HOME/$LATEST_QT/android/src/3rdparty/gradle/gradle.properties"

# now that we have an NDK, copy the font that we need for OnePlus phones
# due to https://bugreports.qt.io/browse/QTBUG-69494
#cp "$ANDROID_HOME"/platforms/"$ANDROID_PLATFORMS"/data/fonts/Roboto-Regular.ttf "$SCRIPTDIR"/../../android-mobile || exit 1

echo "things are set up for the Android build"
