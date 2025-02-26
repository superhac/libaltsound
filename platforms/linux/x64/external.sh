#!/bin/bash

set -e

SDL_SHA=b5c3eab6b447111d3c7879bb547b80fb4abd9063
SDL_MIXER_SHA=20f342235983911b5077562cd131e0135afa2d20

rm -rf external
mkdir external
cd external

#
# download bass24 and copy to platform/arch
#

mkdir bass
cd bass
curl -s https://www.un4seen.com/files/bass24-linux.zip -o bass.zip
unzip bass.zip
cp bass.h ../../third-party/include
cp libs/x86_64/libbass.so ../../third-party/runtime-libs/linux/x64
cd ..


#
# SDL3/SDL3_mixer
#

rm -rf SDL3
mkdir SDL3
cd SDL3

curl -sL https://github.com/libsdl-org/SDL/archive/${SDL_SHA}.tar.gz -o SDL-${SDL_SHA}.tar.gz
tar xzf SDL-${SDL_SHA}.tar.gz
mv SDL-${SDL_SHA} SDL
cd SDL
cmake \
    -DSDL_SHARED=ON \
    -DSDL_STATIC=OFF \
    -DSDL_TEST_LIBRARY=OFF \
    -DSDL_OPENGLES=OFF \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -B build
cmake --build build -- -j${NUM_PROCS}
cd ..

curl -sL https://github.com/libsdl-org/SDL_mixer/archive/${SDL_MIXER_SHA}.tar.gz -o SDL_mixer-${SDL_MIXER_SHA}.tar.gz
tar xzf SDL_mixer-${SDL_MIXER_SHA}.tar.gz
mv SDL_mixer-${SDL_MIXER_SHA} SDL_mixer
cd SDL_mixer
./external/download.sh
cmake \
    -DBUILD_SHARED_LIBS=ON \
    -DSDLMIXER_SAMPLES=OFF \
    -DSDLMIXER_VENDORED=ON \
    -DSDL3_DIR=../SDL/build \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -B build
cmake --build build -- -j${NUM_PROCS}
cd ..

cd ..

cp -a SDL3/SDL/build/libSDL3.{so,so.*} ../third-party/runtime-libs/linux/x64
cp -r SDL3/SDL/include/SDL3 ../third-party/include/

cp -a SDL3/SDL_mixer/build/libSDL3_mixer.{so,so.*} ../third-party/runtime-libs/linux/x64
cp -r SDL3/SDL_mixer/include/SDL3_mixer ../third-party/include/
