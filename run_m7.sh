#!/bin/bash
set -x
PRECOMMAND=""
DEBUG=""
if [[ "$1" == "" ]];then
    IMAGE="/root/zephyrproject/zephyr/build/s32g274ardb_ivt_image.qspi"
    echo "Using default $IMAGE"
else
    IMAGE=$1
    echo Using $IMAGE
fi
COMMAND="./build/qemu-system-arm -machine s32g_vnp_rdb2 -sd $IMAGE -serial mon:stdio  -nographic"
#"  -netdev tap,id=net0,ifname=tap0,script=no,downscript=no -device e1000,netdev=net0"

ARG1=$2

if [[ "$ARG1" == "qgdb" ]];
then
   rm -f ~/.gdbinit
   cp /media/armandorl/ubuntu/s32g2/arm-trusted-firmware/gdbinit_file ~/.gdbinit
   PRECOMMAND="gdb --args"
fi

if [[ "$ARG1" == "debug" ]];
then
    DEBUG="-S -s"
    rm -f ~/.gdbinit
    cp /media/armandorl/ubuntu/s32g2/arm-trusted-firmware/gdbinit_file ~/.gdbinit
fi

if [[ "$DEBUG" == "" && "$ARG1" != "" ]];
then
    DEBUG="$1 $*"
fi
${PRECOMMAND} ${COMMAND} ${DEBUG}

