export ARCH=arm
export CROSS_COMPILE=/home/jake/cm10.1/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-
make geehrc_defconfig
make -j12
