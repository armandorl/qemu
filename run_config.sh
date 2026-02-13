mkdir -p build
pushd build
../configure --target-list=aarch64-softmmu,arm-softmmu --enable-slirp
make -j 16
popd
