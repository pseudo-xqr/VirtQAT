#!/bin/bash


# parallel-ssh -h host_ip.txt -t 0 "cd ./VirtQAT/QAT_driver;
# ./configure --enable-icp-sriov=guest; 
# echo "123456" | sudo -S make > make.log;
# echo "123456" | sudo -S make install > make_install.log;
# echo "123456" | sudo -S make samples-install > make_samples_install.log;
# echo "123456" | sudo cpa_sample_code runTests=32 > runTests.log;
# cd ../src/;
# bash ./build.sh;
# "

echo "--- Setup done---"
parallel-ssh -h host_ip.txt -t 0 \
"git clone https://github.com/pseudo-xqr/VirtQAT.git qat; 
 mv ~/qat/src_single_vm ~/VirtQAT/src_single_vm;"

parallel-ssh -h host_ip.txt -t 0 \
"cd ./VirtQAT/src/single_vm;
 echo "123456" | sudo -S ./dc_sample > ./dc_sample.log;"
