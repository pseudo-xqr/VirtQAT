#!/bin/bash

cd ../QAT_driver
./configure --enable-icp-sriov=host
make -j`$nprocs`
sudo make install
sudo systemctl enable qat
sudo systemctl start qat

# SR-IOV of QAT will be automatically enabled after these commands