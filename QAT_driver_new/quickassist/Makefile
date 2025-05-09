################################################################
#   BSD LICENSE
# 
#   Copyright(c) 2007-2023 Intel Corporation. All rights reserved.
#   All rights reserved.
# 
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
# 
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#     * Neither the name of Intel Corporation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
# 
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
#  version: QAT20.L.1.2.30-00078
################################################################

# Ensure The ICP_ROOT environmental var is defined.
ifndef ICP_ROOT
$(error ICP_ROOT is undefined. Please set the path \
        "-> setenv ICP_ROOT <path to root of Source Code>")
endif
KERNEL_HEADER_FILE_LIST=$(sort $(shell cat $(ICP_BUILD_OUTPUT)/kernel_header_file_list.txt))
KERNEL_HEADER_FILE_PATH=$(strip $(KERNEL_SOURCE_ROOT))/include/linux

DATE=$(shell date '+%m_%d_%y')

BASENAME=qat

ifeq ($(ADF_PLATFORM), ACCELDEVVF)
	DRIVER=$(BASENAME)_vf.ko
else
	DRIVER=$(BASENAME).ko
endif

#Paths to Top-Level Makefiles for each team####

OSAL_PATH=$(ICP_ROOT)/quickassist/utilities/osal/
CMN_MEM_PATH=$(ICP_ROOT)/quickassist/utilities/libusdm_drv

LAC_PATH=$(ICP_ROOT)/quickassist/lookaside/access_layer/
QAT_DIRECT_ROOT=$(ICP_ROOT)/quickassist/lookaside/access_layer/src/qat_direct/
PERF_PATH=$(ICP_ROOT)/quickassist/lookaside/access_layer/src/sample_code/performance
ICP_TOOLS_TARGET?=accelcomp
ICP_BUILD_OUTPUT?=build_$(DATE)
ICP_TOP_ENV=$(ICP_BUILDSYSTEM_PATH)/build_files/env_files/
export ICP_ACCEL_INC=YES
export QAT_LEGACY_ALGORITHMS ?= n
LAC_LIB_DIR=$(LAC_PATH)/build/libs

#Release Package build steps
ALL_TARGETS =  clean lac_lib_dir qat_direct libosal_user lac_user


ifdef ICP_SRIOV
ALL_TARGETS += libosalvf adfvf lacvf libosal_uservf lac_uservf
endif

all: $(ALL_TARGETS)
	@echo 'Build Done';


user: lac_lib_dir libosal_user lac_user

install_scripts:
	@echo ; echo 'Copying Install Scripts';
	@cp $(ICP_BUILDSYSTEM_PATH)/build_files/qat_service $(ICP_BUILD_OUTPUT)/;
	@chmod 750 $(ICP_BUILD_OUTPUT)/qat_service;

#
# Common memory driver
#
#userspace common memory library
cmn_user: clean output_dir lac_lib_dir
	@echo ; echo 'Building common mem driver for user space';
	@cd $(CMN_MEM_PATH) && $(MAKE) ARCH=$(ICP_ARCH_USER) ICP_ENV_DIR=$(ICP_TOP_ENV) OS=linux ICP_OS?=linux_2.6 ICP_OS_LEVEL=user_space CPM_UPSTREAM=1 cm_user;\
	echo ; echo 'Copying Common mem library';
	cp $(CMN_MEM_PATH)/libusdm_drv_s.so $(CMN_MEM_PATH)/libusdm_drv.a $(ICP_BUILD_OUTPUT)/;

#common mem driver ko
cmn_ko: clean output_dir
	@echo ; echo 'Building usdm_drv.ko';
	@cd $(CMN_MEM_PATH) && $(MAKE) ICP_ENV_DIR=$(ICP_TOP_ENV) OS=linux ICP_OS?=linux_2.6 ICP_OS_LEVEL=kernel_space ICP_QDM_IOMMU=1 CPM_UPSTREAM=1 cm_kernel
	@cp $(CMN_MEM_PATH)/usdm_drv.ko $(ICP_BUILD_OUTPUT)


#userspace osal library
libosal_user: clean output_dir lac_lib_dir
	@echo ; echo 'Building OSAL for user space';
	@cd $(OSAL_PATH)/src && $(MAKE) ARCH=$(ICP_ARCH_USER) ICP_ENV_DIR=$(ICP_TOP_ENV) OS_LEVEL=linux ICP_OS_LEVEL=user_space lib;\
	echo ; echo 'Copying OSAL library';
	cp $(OSAL_PATH)/src/build/linux_2.6/user_space/libosal.a $(ICP_BUILD_OUTPUT)/;


#build linux qat_direct layer
qat_direct: clean output_dir lac_lib_dir libosal_user cmn_user
	@echo ; echo 'Building qat_direct for user space';
	@cd $(QAT_DIRECT_ROOT)/src && $(MAKE) ARCH=$(ICP_ARCH_USER) ICP_OS_LEVEL=user_space ;\
	echo ; echo 'Copying qat_direct library';
	cp $(QAT_DIRECT_ROOT)/src/build/linux_2.6/user_space/libadf.a $(ICP_BUILD_OUTPUT)/;


lac_user: clean output_dir qat_direct libosal_user cmn_user cmn_ko
	@echo ; echo 'Building LAC in user space';
	@cd $(LAC_PATH) && export ICP_ENV_DIR=$(ICP_TOP_ENV) && cd src && $(MAKE) ARCH=$(ICP_ARCH_USER) lib_static ICP_OS_LEVEL=user_space;
	cp $(LAC_PATH)/src/build/linux_2.6/user_space/lib$(BASENAME).a $(ICP_BUILD_OUTPUT)/;
	@cd $(LAC_PATH) && export ICP_ENV_DIR=$(ICP_TOP_ENV) && cd src && $(MAKE) ARCH=$(ICP_ARCH_USER) lib_shared ICP_OS_LEVEL=user_space;
	cp $(LAC_PATH)/src/build/linux_2.6/user_space/lib$(BASENAME)_s.so $(ICP_BUILD_OUTPUT)/;


clean:
ifeq ($(ICP_NO_CLEAN),)
	@echo ; echo 'Cleaning qat_direct';
	@cd $(QAT_DIRECT_ROOT)/src && $(MAKE) clean;
	@echo ; echo 'Cleaning common memory driver';
	@cd $(CMN_MEM_PATH) && $(MAKE) ICP_ENV_DIR=$(ICP_TOP_ENV) OS=linux clean;
	@echo ; echo 'Cleaning OSAL';
	@cd $(OSAL_PATH)/src && $(MAKE) ICP_ENV_DIR=$(ICP_TOP_ENV) ICP_OS_LEVEL=user_space OS_LEVEL=linux osal_clean;
	@echo ; echo 'Cleaning LAC';
	@cd $(LAC_PATH) && export ICP_ENV_DIR=$(ICP_TOP_ENV) && cd src && $(MAKE) lac_clean ICP_OS_LEVEL=user_space;
	@echo ; echo 'Cleaning Binaries in Output Dir';
	@rm -rf $(ICP_BUILD_OUTPUT);
	@rm -rf $(LAC_LIB_DIR);
	@echo 'Cleaning done';
endif


output_dir: clean
	test -d $(ICP_BUILD_OUTPUT) || mkdir -p $(ICP_BUILD_OUTPUT);

lac_lib_dir: clean
	test -d $(LAC_LIB_DIR) || mkdir -p $(LAC_LIB_DIR);

osal_depend: clean
	@cd $(OSAL_PATH)/src; grep "include <linux*" `find . -iname "*.c" -o -iname "*.h"` | cut -d '<' -f2 | cut -d '/' -f2 | cut -d '>' -f1 | sort | uniq > ${ICP_BUILD_OUTPUT}/temp_list.txt;

cmn_depend: clean
	@cd $(CMN_MEM_PATH); grep "include <linux*" `find . -iname "*.c" -o -iname "*.h"` | cut -d '<' -f2 | cut -d '/' -f2 | cut -d '>' -f1 | sort | uniq  >> ${ICP_BUILD_OUTPUT}/temp_list.txt;

lac_depend: clean
	@cd $(LAC_PATH)/src; grep "include <linux*" `find . -iname "*.c" -o -iname "*.h"` | cut -d '<' -f2 | cut -d '/' -f2 | cut -d '>' -f1 | sort | uniq >> ${ICP_BUILD_OUTPUT}/temp_list.txt;
	@sort ${ICP_BUILD_OUTPUT}/temp_list.txt | uniq > ${ICP_BUILD_OUTPUT}/kernel_header_file_list.txt;rm ${ICP_BUILD_OUTPUT}/temp_list.txt;

depend_linux: clean output_dir osal_depend cmn_depend lac_depend
	@echo -e "Building kernel source header file dependency list; Refer ${ICP_BUILD_OUTPUT}/kernel_header_file_list.txt for header files which are used to build other kernel space objects and binaries\n";
	@for file in  $(KERNEL_HEADER_FILE_LIST); do \
		if( ! test -e "$(strip $(KERNEL_HEADER_FILE_PATH))/$$file"); then \
			echo "Kernel header file-> $$file is not found"; \
		fi; \
	done;
	@echo -e "\tQuerying system package files:"

ifeq ("$(wildcard /lib64/libgcc_s.so.1 /lib/libgcc_s.so.1 /usr/libgcc_s.so.1 /lib64/*/libgcc_s.so.1 /lib/*/libgcc_s.so.1 /usr/*/libgcc_s.so.1)","")
	@echo "gcc package is not present"
else
	@echo "gcc package is present"
endif
ifeq ("$(wildcard /lib64/libc.so.6 /lib/libc.so.6 /usr/libc.so.6 /lib64/*/libc.so.6 /lib/*/libc.so.6 /usr/*/libc.so.6)","")
	@echo "glibc package is not present"
else
	@echo "glibc package is present"
endif
ifeq ("$(wildcard /lib64/libcrypto.so* /lib/libcrypto.so* /usr/libcrypto.so* /lib64/*/libcrypto.so* /lib/*/libcrypto.so* /usr/*/libcrypto.so*)","")
	@echo "openssl package is not present"
else
	@echo "openssl package is present"
endif
ifeq ("$(wildcard /lib64/libz.so.1 /lib/libz.so.1 /usr/libz.so.1 /lib64/*/libz.so.1 /lib/*/libz.so.1 /usr/*/libz.so.1)","")
	@echo "zlib package is not present"
else
	@echo "zlib package is present"
endif
ifeq ("$(wildcard /lib64/libudev.so* /lib/libudev.so* /usr/libudev.so* /lib64/*/libudev.so* /lib/*/libudev.so* /usr/*/libudev.so*)","")
	@echo "udev package(used by SAL) is not present"
else
	@echo "udev package(used by SAL) is present"
endif
ifeq ("$(wildcard /lib64/libboost_regex.so* /lib/libboost_regex.so* /usr/libboost_regex.so* /lib64/*/libboost_regex.so* /lib/*/libboost_regex.so* /usr/*/libboost_regex.so*)","")
	@echo "boost package(used by adf_ctl) is not present"
else
	@echo "boost package(used by adf_ctl) is present"
endif

