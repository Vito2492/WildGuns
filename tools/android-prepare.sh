#!/bin/sh
# Puts what the Android build needs beside android/: SDL2's sources (C into
# app/jni/SDL, Java into app/src/main/java/org/libsdl/app) and the snes9x
# core built for each ABI (app/src/main/jniLibs/<abi>/libsnes9x_libretro.so).
#   tools/android-prepare.sh SDL_SOURCE_DIR SNES9X_LIBS_DIR
set -e
sdl=$1
libs=$2
here=$(cd "$(dirname "$0")/.." && pwd)
app=$here/android/app
rm -rf "$app/jni/SDL"
mkdir -p "$app/jni/SDL"
cp -r "$sdl"/. "$app/jni/SDL/"
mkdir -p "$app/src/main/java/org/libsdl/app"
cp "$sdl"/android-project/app/src/main/java/org/libsdl/app/*.java "$app/src/main/java/org/libsdl/app/"
for abi in arm64-v8a armeabi-v7a x86_64; do
    mkdir -p "$app/src/main/jniLibs/$abi"
    cp "$libs/$abi/libretro.so" "$app/src/main/jniLibs/$abi/libsnes9x_libretro.so"
done
echo "android: ready to build (cd android && gradle assembleRelease)"
