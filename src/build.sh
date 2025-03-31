cd /home/qirongx2/CS523-Course-Project/src/


cc -Wall -O1 -I/home/qirongx2/QAT_driver_new/quickassist/utilities/libusdm_drv// -I/home/qirongx2/QAT_driver_new/quickassist/include/ -I/home/qirongx2/QAT_driver_new/quickassist/include/lac \
 -I/home/qirongx2/QAT_driver_new/quickassist/include/dc -I/home/qirongx2/QAT_driver_new/quickassist/lookaside/access_layer/include \
 -I/home/qirongx2/QAT_driver_new/quickassist/lookaside/access_layer/src/sample_code/functional/include -DUSER_SPACE -DDO_CRYPTO -DSC_ENABLE_DYNAMIC_COMPRESSION \
 /home/qirongx2/QAT_driver_new/quickassist/lookaside/access_layer/src/sample_code/functional/common/cpa_sample_utils.c dc_qat_funcs.c dc_qat_main.c -L/usr/Lib -L/home/qirongx2/QAT_driver_new/build \
 /home/qirongx2/QAT_driver_new/build/libqat_s.so /home/qirongx2/QAT_driver_new/build/libusdm_drv_s.so -ludev -lpthread -lcrypto -lz -o dc_sample