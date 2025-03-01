#!/bin/bash

# Start VM
virt-install --connect qemu:///system --ram 2048 -n guest --osinfo=ubuntu22.04 \
    --disk path=/var/lib/libvirt/images/disk.qcow,size=5 \
    --vcpus=1 --graphics none --import
