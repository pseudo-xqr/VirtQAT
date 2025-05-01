#!/bin/bash

### Iteratively installation
# for i in {0..15}; do
#     sudo mkdir /var/lib/libvirt/images/g$i
# done

# for i in {0..15}; do
#     sudo cp /var/lib/libvirt/images/g0/guest_image.img /var/lib/libvirt/images/g$i/guest_image.img
# done

# for i in {0..15}; do
#     sudo virt-install --name g$i --ram=4096 --vcpus=1 \
#     --hvm  --disk path=/var/lib/libvirt/images/g$i/guest_image.img,size=7 \
#     --import --graphics none --console pty,target_type=serial --os-variant=ubuntu20.04 \
#     --network network:default
# done

### Start all VMs
# for i in {0..15}; do
#     virsh start g$i
# done

### Attach vdevs
for i in {0..15}; do
    virsh attach-device g$i /guest_vdev/guest$i.xml --config
done

### Stop all VMs
# for i in {0..15}; do
#     virsh shutdown g$i
# done

