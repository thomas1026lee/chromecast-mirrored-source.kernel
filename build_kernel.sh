#!/bin/bash
# Build kernel for use with Eureka.

set -o errtrace
trap 'echo Fatal error: script $0 aborting at line $LINENO, command \"$BASH_COMMAND\" returned $?; exit 1' ERR

# List of files to be copied to Eureka build
declare -a \
    COPY_FILE_LIST=(arch/arm/boot/uImage-8787:kernel/uImage
                    arch/arm/boot/uImage-8801:kernel/uImage-8801
                    arch/arm/boot/zImage-dtb.berlin2cd-dongle-8787:kernel/zImage
                    arch/arm/boot/zImage-dtb.berlin2cd-dongle-8801:kernel/zImage-8801
                    COPYING:kernel
                    tools/perf/perf:sdk/bin
                    vmlinux-8787:kernel/vmlinux
                    vmlinux-8801:kernel/vmlinux-8801
                   )


# Kernel configuration for Eureka
kernel_config=''
arch=arm
if [[ -e ../prebuilt/toolchain/armv7a/bin/armv7a-cros-linux-gnueabi-gcc ]]; then
    cross_compile=armv7a-cros-linux-gnueabi-
else
    cross_compile=arm-unknown-linux-gnueabi-
fi
cpu_num=$(grep -c processor /proc/cpuinfo)

function usage(){
    echo "Usage: $0 <soc: berlin or anchovy>"
}

function run_kernel_make(){
    echo "***** building $5 *****"
    CROSS_COMPILE=$1 make -j$2 ARCH=$3 $4
    echo "***** completed building $5 *****"
}

function run_perf_make(){
    echo "***** building perf *****"
    LDFLAGS='-static-libgcc -static-libstdc++' \
    CROSS_COMPILE=$1 ARCH=$2 NO_DWARF=1 make -j$3
    echo "***** completed building perf *****"
}

function build_kernel(){
    local kernel_dir=$(readlink -f $1)

    cd $kernel_dir

    # Clean kernel
    run_kernel_make $cross_compile $cpu_num $arch clean
    # Build kernel config
    run_kernel_make $cross_compile $cpu_num $arch $kernel_config
    # Verify kernel config
    diff .config arch/arm/configs/$kernel_config

    # Build (default) kernel with sd8787
    run_kernel_make $cross_compile $cpu_num $arch uImage
    run_kernel_make $cross_compile $cpu_num $arch zImage-dtb.berlin2cd-dongle
    mv arch/arm/boot/uImage arch/arm/boot/uImage-8787
    mv arch/arm/boot/zImage-dtb.berlin2cd-dongle arch/arm/boot/zImage-dtb.berlin2cd-dongle-8787
    mv vmlinux vmlinux-8787

    # Build kernel with sd8801
    # Modify .config
    sed -e 's/# CONFIG_BERLIN_SDIO_WLAN_8801 is not set/CONFIG_BERLIN_SDIO_WLAN_8801=y/'\
        -e 's/CONFIG_BERLIN_SDIO_WLAN_8787=y/# CONFIG_BERLIN_SDIO_WLAN_8787 is not set/'\
        .config > .config.8801
    cp .config.8801 .config

    run_kernel_make $cross_compile $cpu_num $arch uImage
    run_kernel_make $cross_compile $cpu_num $arch zImage-dtb.berlin2cd-dongle
    mv arch/arm/boot/uImage arch/arm/boot/uImage-8801
    mv arch/arm/boot/zImage-dtb.berlin2cd-dongle arch/arm/boot/zImage-dtb.berlin2cd-dongle-8801
    mv vmlinux vmlinux-8801

    cd -
}

function build_perf(){
    local kernel_dir=$(readlink -f $1)
    cd ${kernel_dir}/tools/perf
    run_perf_make $cross_compile $arch $cpu_num
    cd -
}

function create_kernel_pkg(){
    local kernel_dir=$(readlink -f $1)
    local pkg_dir=$(mktemp -d)
    local wd=$(pwd)

    mkdir -p $pkg_dir/kernel
    mkdir -p $pkg_dir/sdk/bin

    for f in ${COPY_FILE_LIST[@]}
    do
      s=${f%%:*}
      d=${f##*:}
      cp $kernel_dir/$s $pkg_dir/$d
    done

    (cd $pkg_dir; tar zcvf $wd/kernel.tgz kernel sdk)
    rm -fr $pkg_dir
}

if (( $# < 1 ))
then
    usage
    exit 2
fi

soc=$1

# Choose kernel config based on SoC
# being targeted.
if [[ $soc == 'berlin' ]]
then
  kernel_config=eureka_mv88de31xx_defconfig
elif [[ $soc == 'anchovy' ]]
then
  kernel_config=eureka_mv88de30xx_defconfig
else
  echo "ERROR: unknown SoC: $soc"
  exit 1
fi

kernel_dir=linux-3.8
# Build kernel
build_kernel $kernel_dir

# Build the perf tool
build_perf $kernel_dir

# Create a kernel package
create_kernel_pkg $kernel_dir
