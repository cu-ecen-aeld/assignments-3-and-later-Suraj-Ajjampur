#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.


set -e # Causes script to exit immediately if any command it runs exits with non-zero exit status
set -u # Causes script to treat uninitialized variables as an error and exit immediately

# Variables Initialization
OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    # “deep clean” the kernel build tree - removing the .config file with any existing configurations
    make ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE} mrproper
    #Configure for our “virt” arm dev board we will simulate in QEMU
    make ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE} defconfig
    # Build a kernel image for booting with QEMU
    make -j4 ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE} all
    # Build any kernel modules
    make ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE} modules
    # Build the devicetree
    make ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE} dtbs

fi

echo "Adding the Image in outdir"
cp -r ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}
echo "Creating the staging directory for the root filesystem"

cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log
mkdir -p home/finder-app

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean # Clean the build environment from compiled object files, binary executables and configuration files
    make defconfig # Creates a default config file for building the software
else
    cd busybox
fi

# TODO: Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs/ ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
echo "Library dependencies"

# Added needed shared libraries from toolchain sysroot
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
ROOT=$(${CROSS_COMPILE}gcc --print-sysroot)
sudo cp ${ROOT}/lib/ld-linux-aarch64.so.* ${OUTDIR}/rootfs/lib
sudo cp ${ROOT}/lib64/libc.so.* ${OUTDIR}/rootfs/lib64
sudo cp ${ROOT}/lib64/libm.so.* ${OUTDIR}/rootfs/lib64
sudo cp ${ROOT}/lib64/libresolv.so.* ${OUTDIR}/rootfs/lib64
# TODO: Make device nodes
# Created character device nodes modified permissions to 666 and major and minur numbers
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1
# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp writer ${OUTDIR}/rootfs/home
cp finder.sh ${OUTDIR}/rootfs/home
cp finder-test.sh ${OUTDIR}/rootfs/home
cp -r conf/ ${OUTDIR}/rootfs/home
cp autorun-qemu.sh ${OUTDIR}/rootfs/home

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs/
# Make the contents owned by root
sudo chown -R root:root *
# TODO: Create initramfs.cpio.gz
# cpio -H newc -ov --owner root:root: The cpio utility reads the file list from standard input and creates an archive in the "newc" format. 
# The -o option is for creating the archive, -v is for verbose mode, 
# and --owner root:root sets the owner of all the files in the archive to root.
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ..
gzip -f initramfs.cpio
