#!/bin/bash

#JAVA_HOME=/usr/local/arm/jdk1.6.0/
JAVA_HOME=/home/fighter/work/usr
ANDROID_JAVA_HOME=$JAVA_HOME
PATH=$JAVA_HOME/bin:$PATH

COMPILE_USES_32BIT=true
export COMPILE_USES_32BIT

#export USE_CCACHE=1
#export CCACHE_DIR=/home/cuiyongtai/CACHE_andrid4.0.3

CPU_JOB_NUM=$(grep processor /proc/cpuinfo | awk '{field=$NF};END{print field+1}')
CLIENT=$(whoami)
CPU_JOB_NUM=2

ROOT_DIR=$(pwd)
#KERNEL_DIR=/root/s5pv210/kernel/android_kernel_2.6.35_skdv210
#APP_DIR=/home/fighter/work/android4.0/disk2/s5pv210_ics/android4.0.4/device/samsung/smdkv210/app

#SEC_PRODUCT='generic' #Enable for generic build
SEC_PRODUCT='smdkv210' #Enable for smdk build
#SEC_PRODUCT='A9' #Enable for sekede build

#Modify the below path suitably as per your requirements

# OUT_DIR="$ROOT_DIR/out/target/product/$SEC_PRODUCT"
OUT_DIR="$ROOT_DIR/out/target/product/smdkv210"
OUT_HOSTBIN_DIR="$ROOT_DIR/out/host/linux-x86/bin"

function check_exit()
{
	if [ $? != 0 ]
	then
		exit $?
	fi
}

function build_android()
{
	echo
	echo '[[[[[[[ Build android platform ]]]]]]]'
	echo
	
	START_TIME=`date +%s`

	rm -rf $OUT_DIR/system/lib/*ril.so
	rm -rf $OUT_DIR/system/vendor
	rm -rf $OUT_DIR/system/etc/
	rm -rf $OUT_DIR/system/lib/
	rm -rf $OUT_DIR/system/app/
	rm -rf $OUT_DIR/system/modules/
	rm -rf $OUT_DIR/system/build.prop
	rm -rf $OUT_DIR/root/
	
 	echo Begin copy app
	mkdir $OUT_DIR/system/app/
	#mkdir $OUT_DIR/system/framework/
	mkdir $OUT_DIR/system/lib/
	
	cp $APP_DIR/*.apk  									$OUT_DIR/system/app/
	cp $APP_DIR/google/app/*.apk  			$OUT_DIR/system/app/
	cp $APP_DIR/google/etc/  						$OUT_DIR/system/etc/  -rf
	cp $APP_DIR/google/framework/*  		$OUT_DIR/system/framework/
	cp $APP_DIR/google/lib/*  					$OUT_DIR/system/lib/
	cp $APP_DIR/google/tts/ 						$OUT_DIR/system/tts/  -rf

	if [ $SEC_PRODUCT = "generic" ]
	then
		echo make -j$CPU_JOB_NUM
		echo
		make -j$CPU_JOB_NUM
	else
		echo make -j$CPU_JOB_NUM PRODUCT-full_$SEC_PRODUCT-eng
		echo
		make -j$CPU_JOB_NUM PRODUCT-full_$SEC_PRODUCT-eng
	fi
	check_exit
 
	END_TIME=`date +%s`
	let "ELAPSED_TIME=($END_TIME-$START_TIME)/60"
	echo "Total compile time is $ELAPSED_TIME minutes"
}

function make_uboot_img()
{
	cd $OUT_DIR

	echo
	echo '[[[[[[[ Make ramdisk image for u-boot ]]]]]]]'
	echo

#	mkimage -A arm -O linux -T ramdisk -C none -a 0x30800000 -n "ramdisk" -d ramdisk.img ramdisk-uboot.img
	$OUT_HOSTBIN_DIR/mkyaffs2image root ramdisk-yaffs.img
	check_exit

	rm -f ramdisk.img

	echo
	cd ../../../..
}

function make_fastboot_img()
{
	echo
	echo '[[[[[[[ Make additional images for fastboot ]]]]]]]'
	echo

#	if [ ! -f $KERNEL_DIR/arch/arm/boot/zImage ]
#	then
#		echo "No zImage is found at $KERNEL_DIR/arch/arm/boot"
#		echo '  Please set KERNEL_DIR if you want to make additional images'
#		echo "  Ex.) export KERNEL_DIR=~ID/android_kernel_$SEC_PRODUCT"
#		echo
#		return
#	fi

#	echo 'boot.img ->' $OUT_DIR
#	cp $KERNEL_DIR/arch/arm/boot/zImage $OUT_DIR/zImage
#	$OUT_HOSTBIN_DIR/mkbootimg --kernel $OUT_DIR/zImage --ramdisk $OUT_DIR/ramdisk-uboot.img -o $OUT_DIR/boot.img
#	check_exit

	echo 'update.zip ->' $OUT_DIR
	zip -j $OUT_DIR/update.zip $OUT_DIR/android-info.txt $OUT_DIR/ramdisk-uboot.img $OUT_DIR/system.img
	check_exit

	echo
}

echo
echo '                Build android for '$SEC_PRODUCT''
echo

case "$SEC_PRODUCT" in
	smdkc110)
		build_android
		make_uboot_img
		make_fastboot_img
		;;
	smdkv210)
		build_android
		make_uboot_img
#		make_fastboot_img
		;;
	A9)
		build_android
		make_uboot_img
		make_fastboot_img
		;;
	generic)
		build_android
		make_uboot_img
		;;
	*)
		echo "Please, set SEC_PRODUCT"
		echo "  export SEC_PRODUCT=smdkc100 or SEC_PRODUCT=smdkc110 or SEC_PRODUCT=smdkv210 or SEC_PRODUCT=smdk6440"
		echo "     or "
		echo "  export SEC_PRODUCT=generic"
		exit 1
		;;
esac

echo ok success !!!

exit 0
