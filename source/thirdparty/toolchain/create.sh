#!/bin/bash

export PATH=$PATH:`pwd`/../../../build/crosscompiler/bin

# Download binutils and gcc sources

if [ ! -f binutils-2.17.tar.bz2 ] ; then
    wget http://files.yaosp.org/source/binutils/binutils-2.17.tar.bz2
fi

if [ ! -f gcc-4.1.2.tar.bz2 ] ; then
    wget http://files.yaosp.org/source/gcc/gcc-4.1.2.tar.bz2
fi

# Unpack and patch the sources

echo "Unpacking binutils ..."
tar -jxf binutils-2.17.tar.bz2
echo "Patching binutils ..."
patch -p0 < binutils-2.17.patch

echo "Unpacking gcc ..."
tar -jxf gcc-4.1.2.tar.bz2
echo "Patching gcc ..."
patch -p0 < gcc-4.1.2.patch

# Compile binutils

if [ -f binutils-build ] ; then
    rm -rf binutils-build
fi

mkdir binutils-build

if [ -f binutils-bin ] ; then
    rm -rf binutils-bin
fi

mkdir binutils-bin

cd binutils-build
../binutils-2.17/configure --prefix=`pwd`/../binutils-bin --target=i686-pc-yaosp --host=i686-pc-yaosp --build=`gcc -v 2>&1 | grep Target | awk '{ print $2 }'` --disable-nls --disable-werror
make -j2
make install
cd ..

# Compile gcc

if [ -f gcc-build ] ; then
    rm -rf gcc-build
fi

mkdir gcc-build

if [ -f gcc-bin ] ; then
    rm -rf gcc-bin
fi

mkdir gcc-bin

cd gcc-build

mkdir -p gcc/build
echo "#!/bin/bash" > gcc/build/fix-header
chmod +x gcc/build/fix-header

../gcc-4.1.2/configure --prefix=`pwd`/../gcc-bin --target=i686-pc-yaosp --host=i686-pc-yaosp --build=`gcc -v 2>&1 | grep Target | awk '{ print $2 }'` --disable-nls --enable-languages=c
make -j2
make install
