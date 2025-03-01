#!/bin/bash
# Restart all devices & load config
# sudo systemctl restart qat.service

# Restart only specified device & load config
service qat_service stop qat_dev0
service qat_service start qat_dev0
# service qat_service stop qat_dev1
# service qat_service start qat_dev1

ll /sys/class/mdev_bus

sudo ../QAT_driver/build/vqat_ctl show
sudo ../QAT_driver/build/vqat_ctl create 0000:6b:00.0 dc # Using `lspci | grep QAT` to see the pci ID of the device, hardcode to 0000:6b:00.0 for now
