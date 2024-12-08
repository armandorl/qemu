PRECOMMAND=""
DEBUG=""
if [ "$1" == "" ];then
    IMAGE="/mnt/c/tftpboot/new_ivt_image.bin"
    echo "Using default $IMAGE"
else
    IMAGE=$1
    echo Using $IMAGE
fi
COMMAND="./build/qemu-system-aarch64 -machine s32g_vnp_rdb2 -sd $IMAGE  -serial mon:stdio  -nographic"
ARG1=$2

if [ "$ARG1" == "qgdb" ];
then
   rm -f ~/.gdbinit
   cp /media/armandorl/ubuntu/s32g2/arm-trusted-firmware/gdbinit_file ~/.gdbinit
   PRECOMMAND="gdb --args"
fi

if [ "$ARG1" == "debug" ];
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

