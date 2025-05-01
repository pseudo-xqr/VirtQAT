#!/bin/bash
# Refer to (https://askubuntu.com/questions/1480090/how-to-install-ubuntu-22-04-as-guest-in-kvm)

# Prerequisite: 
# 1. Download the ISO image
wget https://releases.ubuntu.com/focal/ubuntu-20.04.6-live-server-amd64.iso -P /var/lib/libvirt/images/

# 2. Read the ISO image and mount to /mnt/vm
mkdir /mnt/vm
sudo mount -o loop /var/lib/libvirt/images/ubuntu-22.04.6-live-server-amd64.iso /mnt/vm

# 3. Install new VM
sudo virt-install --name guest --ram=4096 --vcpus=1 \
 --hvm  --disk path=/var/lib/libvirt/images/server_image.img,size=15 \
 --cdrom /var/lib/libvirt/images/ubuntu-20.04.6-live-server-amd64.iso \
 --graphics none --console pty,target_type=serial --os-type linux \
 --network network:default --force --debug \
 --boot kernel=/mnt/vm/casper/vmlinuz,initrd=/mnt/vm/casper/initrd,kernel_args="console=ttyS0"

# 4. After the installation, unmount the ISO image
virsh destroy guest
virsh edit guest
# Del the following line:
# <kernel>/home/user/mnt/casper/vmlinuz</kernel>
# <initrd>/home/user/mnt/casper/initrd</initrd>
# <cmdline>console=ttyS0</cmdline>

# 5. Start the VM
virsh start guest
virsh --connect qemu:///system console guest