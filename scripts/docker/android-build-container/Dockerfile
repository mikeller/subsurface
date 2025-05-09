FROM ubuntu:24.04 as base

ARG QT_EMAIL
ARG QT_PASSWORD

RUN apt-get update  && \
    apt-get dist-upgrade -y && \
    apt-get install -y \
    unzip \
    git \
    cmake \
    autoconf \
    libtool-bin \
    openjdk-21-jdk \
    python3-pip \
    curl && \
    apt-get clean

FROM base as build

WORKDIR /android

RUN apt-get install -y \
    libxkbcommon-x11-0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-render-util0 && \
    apt-get clean

# run the build setup
# install, NDK and SDK there, plus the three files from the Subsurface
ADD android-build-setup.sh variables.sh .
RUN env QT_EMAIL=${QT_EMAIL} QT_PASSWORD=${QT_PASSWORD} bash -x android-build-setup.sh

# clean up the files that we don't need to keep the container smaller
RUN rm -rf \
	   android-build-setup.sh \
	   5*/android/lib/*x86* \
	   5*/android/doc \
	   5*/android/include/QtHelp \
	   5*/android/include/QtFbSupport \
	   5*/android/include/QtFontDatabaseSupport \
	   5*/android/include/QtNfc \
	   5*/android/include/QtPrintSupport \
	   5*/android/include/QtTest \
	   5*/android/include/QtXml \
	   5*/android/plugins/geoservices/*qtgeoservices_mapboxgl_*.so \
	   5*/android/lib/cmake/Qt5Test/Qt5TestConfig.cmake \
	   ndk/*/sources/cxx-stl/llvm-libc++/libs/x* \
	   ndk/*/sources/cxx-stl/llvm-libc++/libs/*/*static* \
	   ndk/*/sources/cxx-stl/llvm-libc++abi \
	   ndk/*/sources/cxx-stl/system \
	   ndk/*/sources/third_party

FROM base as final

RUN apt-get install -y \
    autoconf \
    automake \
    cmake \
    git \
    make \
    wget \
    zip \
    unzip \
    bzip2 \
    pkg-config \
    libx11-xcb1 \
    libglib2.0-0 \
    coreutils \
    p7zip-full && \
    apt-get clean

WORKDIR /android

COPY --from=build /android/ .

RUN git config --global --add safe.directory /android/subsurface && \
    git config --global --add safe.directory /android/subsurface/libdivecomputer && \
    git config --global --add safe.directory /android/subsurface/nightly-builds
