
PREFIX=10.0.0.100
ROUTE=0.0.0.0
ip link add br0 type bridge
ip tuntap add dev tap0 mode tap
ip link set dev tap0 master br0   # set br0 as the target bridge for tap0
ip link set dev eth0 master br0   # set br0 as the target bridge for eth0
ip link set dev br0 up

ip address delete $PREFIX dev eth0
ip address add $PREFIX dev br0
ip route add default via $ROUTE dev br0
