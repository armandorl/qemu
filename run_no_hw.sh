./build/qemu-system-aarch64 -nographic -machine virt -cpu cortex-a53 -kernel /media/armandorl/ubuntu/s32g2/fsl-auto-yocto-bsp-40/build_s32g274ardb2/tmp/deploy/images/s32g274azeus/Image-s32g274azeus.bin  -initrd /media/armandorl/ubuntu/s32g2/fsl-auto-yocto-bsp-40/build_s32g274ardb2/tmp/deploy/images/s32g274azeus/fsl-image-base-s32g274azeus.cpio.gz -serial mon:stdio -append "root=/dev/ram rw earlycon maxcpus=1" -m 2G -smp 4 -netdev tap,id=net0,ifname=tap0,script=no,downscript=no -device e1000,netdev=net0


#-netdev user,id=mynet0,net=192.168.76.0/24,dhcpstart=192.168.76.9 -device e1000,netdev=mynet0


#-netdev user,id=mynet0,net=192.168.76.0/24,dhcpstart=192.168.76.9 

#-device pci-bridge,id=bridge0,chassis_nr=1 \
#-device virtio-scsi-pci,id=scsi0,bus=bridge0,addr=0x3 \
#-device pci-bridge,id=bridge1,chassis_nr=2 \
#-device virtio-scsi-pci,id=scsi1,bus=bridge1,addr=0x3 \
#-device virtio-scsi-pci,id=scsi2,bus=bridge1,addr=0x4


