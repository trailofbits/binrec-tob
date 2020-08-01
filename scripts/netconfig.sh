#!/bin/bash
addr="$(ifconfig | grep -A 1 'eth0' | tail -1 | cut -d ':' -f 2 | cut -d ' ' -f 1)"
gatew=$(echo $addr| cut -d'.' -f 1-3).1
braddr=$(echo $addr| cut -d'.' -f 1-3).2
sudo ip link add br0 type bridge
sudo ip addr flush dev eth0
sudo ip link set eth0 master br0
sudo ip tuntap add dev tap0 mode tap user $(whoami)
sudo ip link set tap0 master br0
sudo ip link set dev br0 up
sudo ip link set dev tap0 up
sudo ifconfig br0 $addr #must be the same as the original interface assigned to the docker vm
sudo ifconfig eth0 0.0.0.0 promisc
#now go to qemu/run-vm.sh and uncomment the qemu args. Dont try parallel lifting with this

sudo route add default gw $gatew
# deactivate filtering on bridge
#these rules are run outside the docker (so on ava), since docker permissions hide them,  
#sudo bash -c 'echo "net.bridge.bridge-nf-call-ip6tables = 0" >> /etc/sysctl.conf'
#sudo bash -c 'echo "net.bridge.bridge-nf-call-iptables = 0" >> /etc/sysctl.conf'
#sudo bash -c 'echo "net.bridge.bridge-nf-call-arptables = 0" >> /etc/sysctl.conf'
#sudo sysctl -p /etc/sysctl.conf

## before next steps do ifconfig in docker to determine the bridge address
#do this in the qemu vm
#sudo bash -c 'echo "auto eth0" >>/etc/network/interfaces'
#sudo bash -c 'echo "nameserver 8.8.8.8" >>/etc/resolv.conf'
#sudo ip link set dev eth0 down
#sudo ip link set dev eth0 up
#sudo /etc/init.d/networking stop
#sudo /etc/init.d/networking start
#sudo ifconfig eth0 172.21.0.3 #new uniqe ip on the same subnet as docker bridge
#sudo route add default gw 172.21.0.1 #gateway ip is *.*.*.1 on the docker bridge subnet

#sudo bash -c 'echo "deb http://security.debian.org/debian-security jessie/updates main" >>/etc/apt/sources.list'

#at this point you might want to save a snapshot with the configuration done 
#ctrl-alt-2 -> savevm coolname



#to run a binary you manually put in the qemu guest, instead of copied via s2eget, edit ~/bin/getrun-cmd 
#comment out s2eget $bin and chmod +x $bin, and in ExportELF.cpp on docker comment out the TODO line
