########################################################################
#  SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)
#  Copyright(c) 2014 - 2021 Intel Corporation
########################################################################
ifeq ($(shell test $(VERSION) -ge 5 -a $(PATCHLEVEL) -ge 16; echo $$?), 0)
	CONFIG_QAT_MDEV=0
else ifeq ($(shell test $(VERSION) -ge 5 -a $(PATCHLEVEL) -ge 14; echo $$?), 0)
  ifdef CONFIG_VFIO_MDEV
	CONFIG_QAT_MDEV=1
  endif
else ifeq ($(shell test $(VERSION) -ge 4 -a $(PATCHLEVEL) -ge 10; echo $$?), 0)
  ifdef CONFIG_VFIO_MDEV_DEVICE
	CONFIG_QAT_MDEV=1
  endif
endif
ifeq ($(CONFIG_QAT_MDEV), 1)
ifdef CONFIG_IRQ_BYPASS_MANAGER
  export CONFIG_CRYPTO_DEV_QAT_VDCM=m
endif
endif

obj-$(CONFIG_CRYPTO_DEV_QAT) += qat_common/
obj-$(CONFIG_CRYPTO_DEV_QAT_DH895xCC) += qat_dh895xcc/
obj-$(CONFIG_CRYPTO_DEV_QAT_C3XXX) += qat_c3xxx/
obj-$(CONFIG_CRYPTO_DEV_QAT_C62X) += qat_c62x/
obj-$(CONFIG_CRYPTO_DEV_QAT_C4XXX) += qat_c4xxx/
obj-$(CONFIG_CRYPTO_DEV_QAT_DH895xCCVF) += qat_dh895xccvf/
obj-$(CONFIG_CRYPTO_DEV_QAT_C3XXXVF) += qat_c3xxxvf/
obj-$(CONFIG_CRYPTO_DEV_QAT_C62XVF) += qat_c62xvf/
obj-$(CONFIG_CRYPTO_DEV_QAT_C4XXXVF) += qat_c4xxxvf/
obj-$(CONFIG_CRYPTO_DEV_QAT_D15XX) += qat_d15xx/
obj-$(CONFIG_CRYPTO_DEV_QAT_D15XXVF) += qat_d15xxvf/
obj-$(CONFIG_CRYPTO_DEV_QAT_4XXX) += qat_4xxx/
obj-$(CONFIG_CRYPTO_DEV_QAT_4XXXVF) += qat_4xxxvf/
obj-$(CONFIG_CRYPTO_DEV_QAT_VQAT) += qat_vqat/
ifdef CONFIG_CRYPTO_DEV_QAT_VDCM
subdir-ccflags-y += -DCONFIG_CRYPTO_DEV_QAT_VDCM=y
endif
#ifdef INTEL_NEXT
ifdef INTEL_NEXT
subdir-ccflags-y += -DINTEL_NEXT=y
endif
#endif

subdir-ccflags-$(QAT_UIO) += -DQAT_UIO=y

ifdef KPT
subdir-ccflags-$(QAT_UIO) += -DQAT_KPT=y
endif

