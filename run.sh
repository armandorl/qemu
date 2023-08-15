PRECOMMAND=""
DEBUG=""
COMMAND="./qemu-system-aarch64 -machine s32g_vnp_rdb2 -sd /media/armandorl/ubuntu/s32g2/arm-trusted-firmware/fsl-image-flash-s32g274ardb2.flashimage -serial mon:stdio  -nographic"
ARG1=$1

if [ "$ARG1" == "qgdb" ];
then
   rm -f ~/.gdbinit
   cp /media/armandorl/ubuntu/qemu/gdbinit_file ~/.gdbinit
   PRECOMMAND="gdb --args"
fi

if [ "$ARG1" == "debug" ];
then
    DEBUG="-S -s"
    rm -f ~/.gdbinit
    cp /media/armandorl/ubuntu/s32g2/arm-trusted-firmware/gdbinit_file ~/.gdbinit
fi

if [ "$DEBUG" == "" && "$ARG1" != "" ];
then
    DEBUG="$1 $*"
fi
${PRECOMMAND} ${COMMAND} ${DEBUG}

