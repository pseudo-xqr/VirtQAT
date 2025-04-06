# How to Run

### Build the QAT driver
 - __Required Linux version__: 6.8.0-31-generic
 - __Required grub cmdline__: intel_iommu=on,sm_on
 - __Required BIOS setting__: Enable SR-IOV
```
cd ~/CS523-Course-Project/QAT_driver_new
./configure --enable-icp-sriov=host
sudo make 
sudo make install
sudo make samples-install
```

### Build & run the program

```
### Build
cd ~/CS523-Course-Project/src
bash ./build.sh <your path to QAT driver> 
# e.g. bash ./build.sh ~/CS523-Course-Project/QAT_driver_new

### Run
sudo ./dc_sample
```


### Core code for enqueuing tasks concurrently to virtual devices
 - `dc_qat_funcs.c`: Line 611-628, for creating multiple threads for each instance (in total 32 instances, mapping to 32 virtual devices).
 - `dc_qat_funcs.c`: Line 254-261, for enqueuing the task.
