cd /home/qirongx2/QAT_driver_new/quickassist/lookaside/access_layer/src/sample_code/functional/dc/stateless_sample

cc -Wall -O1 -I/home/qirongx2/QAT_driver_new/quickassist/utilities/libusdm_drv// -I/home/qirongx2/QAT_driver_new/quickassist/include/ -I/home/qirongx2/QAT_driver_new/quickassist/include/lac \
 -I/home/qirongx2/QAT_driver_new/quickassist/include/dc -I/home/qirongx2/QAT_driver_new/quickassist/lookaside/access_layer/include \
 -I/home/qirongx2/QAT_driver_new/quickassist/lookaside/access_layer/src/sample_code/functional/include -DUSER_SPACE -DDO_CRYPTO -DSC_ENABLE_DYNAMIC_COMPRESSION \
 ../../common/cpa_sample_utils.c cpa_dc_stateless_sample.c cpa_dc_sample_user.c -L/usr/Lib -L/home/qirongx2/QAT_driver_new/build \
 /home/qirongx2/QAT_driver_new/build/libqat_s.so /home/qirongx2/QAT_driver_new/build/libusdm_drv_s.so -ludev -lpthread -lcrypto -lz -o dc_sample