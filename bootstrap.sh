#!/bin/bash
# Ramdisk maker for Samsung YP-GS1

pushd out/target/product/aalto/root/
chmod 0644 *.rc default.prop
chmod 0644 lib/modules/*.ko
find . | cpio -o -H newc | gzip > ../bootstrap/ramdisk-system.cpio.gz
popd

pushd out/target/product/aalto/recovery/root/
chmod 0644 *.rc default.prop
chmod 0644 lib/modules/*.ko
find . | cpio -o -H newc | gzip > ../../bootstrap/ramdisk-recovery.cpio.gz
popd

pushd out/target/product/aalto/bootstrap/
find . | cpio -o -H newc | gzip > ../bootstrap.cpio.gz
popd

exit 0
