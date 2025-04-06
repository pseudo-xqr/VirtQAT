#!/bin/bash
QAT_DRIVER_PATH="$1"

cc -Wall -O1 \
 -I"$QAT_DRIVER_PATH/quickassist/utilities/libusdm_drv/" \
 -I"$QAT_DRIVER_PATH/quickassist/include/" \
 -I"$QAT_DRIVER_PATH/quickassist/include/lac" \
 -I"$QAT_DRIVER_PATH/quickassist/include/dc" \
 -I"$QAT_DRIVER_PATH/quickassist/lookaside/access_layer/include" \
 -I"$QAT_DRIVER_PATH/quickassist/lookaside/access_layer/src/sample_code/functional/include" \
 -DUSER_SPACE -DDO_CRYPTO -DSC_ENABLE_DYNAMIC_COMPRESSION \
 "$QAT_DRIVER_PATH/quickassist/lookaside/access_layer/src/sample_code/functional/common/cpa_sample_utils.c" \
 dc_qat_funcs.c dc_qat_main.c \
 -L/usr/Lib -L"$QAT_DRIVER_PATH/build" \
 "$QAT_DRIVER_PATH/build/libqat_s.so" "$QAT_DRIVER_PATH/build/libusdm_drv_s.so" \
 -ludev -lpthread -lcrypto -lz -o dc_sample
