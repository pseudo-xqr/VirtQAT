#!/bin/bash

for i in {0..15}; do
    > ./guest_vdev/guest$i.xml
done

index=0
virsh nodedev-list --cap mdev | while read -r line; do
    # Use regex to extract uuid1, uuid2, uuid3
    if [[ $line =~ mdev_([0-9a-fA-F]+)_([0-9a-fA-F]+)_([0-9a-fA-F]+)_([0-9a-fA-F]+)_([0-9a-fA-F]+)_.* ]]; then
        id1="${BASH_REMATCH[1]}"
        id2="${BASH_REMATCH[2]}"
        id3="${BASH_REMATCH[3]}"
        id4="${BASH_REMATCH[4]}"
        id5="${BASH_REMATCH[5]}"
        uuid="${id1}-${id2}-${id3}-${id4}-${id5}"
        echo "UUID is ${uuid}, index is "

        echo "<hostdev mode='subsystem' type='mdev' model='vfio-pci'>" >> ./guest_vdev/guest$index.xml
        echo "  <source>" >> ./guest_vdev/guest$index.xml
        echo "    <address uuid='$uuid'/>" >> ./guest_vdev/guest$index.xml
        echo "  </source>" >> ./guest_vdev/guest$index.xml
        echo "</hostdev>" >> ./guest_vdev/guest$index.xml

        ((index++))
    fi
done

