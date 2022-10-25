set(RASPBERRY ON)
set(RASPBERRY_DIR_BASE $ENV{HOME}/x-tools/armv7-rpi2-linux-musleabihf)
set(RASPBERRY_DIR_COMPILER ${RASPBERRY_DIR_BASE}/bin)

set(CMAKE_C_COMPILER ${RASPBERRY_DIR_COMPILER}/armv7-rpi2-linux-musleabihf-gcc)
set(CMAKE_CXX_COMPILER ${RASPBERRY_DIR_COMPILER}/armv7-rpi2-linux-musleabihf-g++)
