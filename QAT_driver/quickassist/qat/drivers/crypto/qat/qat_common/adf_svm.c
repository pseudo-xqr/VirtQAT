// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)
/* Copyright(c) 2018 - 2021 Intel Corporation */

#include <linux/pci.h>
#include <linux/version.h>
#include "adf_svm.h"
#include "adf_pasid.h"
#ifdef QAT_UIO
#include "adf_cfg.h"
#endif
#if (KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE)
#include <linux/intel-svm.h>
#endif
#if (KERNEL_VERSION(5, 7, 0) <= LINUX_VERSION_CODE)
#include <linux/iommu.h>
#endif

#ifndef at_enabled
#define at_enabled(pdev) ((pdev)->ats_enabled)
#endif

void adf_init_svm(void)
{
	adf_pasid_init();
}

void adf_exit_svm(void)
{
	adf_pasid_destroy();
}

#if (KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE)
static int adf_svm_enable_pri(struct pci_dev *pdev)
{
	int pri_offset;
	u16 prs_status;
	u32 max_pri_requests;

	if (!pdev || pdev->pri_enabled)
		return -EFAULT;

	/* This function is for PF only */
	pri_offset = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_PRI);
	if (!pri_offset)
		return -EFAULT;

	/* Cannot enable PRI when there are inflight page requests */
	pci_read_config_word(pdev, pri_offset + PCI_PRI_STATUS, &prs_status);
	if (!(prs_status & PCI_PRI_STATUS_STOPPED))
		return -EBUSY;

	/* Use maximum quota */
	pci_read_config_dword(pdev, pri_offset + PCI_PRI_MAX_REQ,
			      &max_pri_requests);

	pdev->pri_reqs_alloc = max_pri_requests;

	pci_write_config_dword(pdev, pri_offset + PCI_PRI_ALLOC_REQ,
			       max_pri_requests);

	/* Enable PRI */
	pci_write_config_word(pdev, pri_offset + PCI_PRI_CTRL,
			      PCI_PRI_CTRL_ENABLE);

	pdev->pri_enabled = 1;
	return 0;
}
#endif

int adf_svm_device_init(struct adf_accel_dev *accel_dev)
{
	struct adf_accel_pci *accel_pci_dev = &accel_dev->accel_pci_dev;
	struct pci_dev *pdev = accel_pci_dev->pci_dev;
#ifdef QAT_UIO
	u64 cfg_val = 0;
	u32 cfg_svm_enable;
	u32 cfg_at_enable;
	char val[ADF_CFG_MAX_VAL_LEN_IN_BYTES];
#endif
	accel_dev->at_enabled = false;
	accel_dev->svm_enabled = false;

#ifdef QAT_UIO
	if (!adf_cfg_get_param_value(accel_dev, ADF_GENERAL_SEC,
				     ADF_SVM_ENABLE, val)) {
		if (kstrtouint(val, 0, &cfg_svm_enable)) {
			dev_err(&GET_DEV(accel_dev),
				"Invalid %s configuration\n", ADF_SVM_ENABLE);
			return -EFAULT;
		}

		if (!cfg_svm_enable) {
			dev_err(&GET_DEV(accel_dev),
				"SVM disabled by device config\n");
			return 0;
		} else {
			accel_dev->svm_enabled =
				adf_svm_get_dev_capability(accel_dev);
		}
	}
#endif

#if (KERNEL_VERSION(5, 7, 0) <= LINUX_VERSION_CODE)
	if (accel_dev->svm_enabled &&
	    iommu_dev_enable_feature(&pdev->dev, IOMMU_DEV_FEAT_SVA)) {
		dev_err(&GET_DEV(accel_dev),
			"Failed to enable SVM!\n");

		return -EFAULT;
	}
#endif

	if (accel_dev->svm_enabled) {
		dev_dbg(&GET_DEV(accel_dev),
			"Intel SVM is supported on device\n");

#ifdef QAT_UIO
		/* ATS can be disabled by config when SVM is enabled */
		if (!adf_cfg_get_param_value(accel_dev, ADF_GENERAL_SEC,
					     ADF_AT_ENABLE, val)) {
			if (kstrtouint(val, 0, &cfg_at_enable)) {
				dev_err(&GET_DEV(accel_dev),
					"Invalid %s configuration\n",
					ADF_AT_ENABLE);

				return -EFAULT;
			}

			if (!cfg_at_enable) {
				dev_err(&GET_DEV(accel_dev),
					"ATS disabled by device config\n");
			} else {
				accel_dev->at_enabled = at_enabled(pdev);
			}
		}
#endif
	}

#if (KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE)
	/*
	 * In case PRI is not enabled by the system when ATS is enabled,
	 * enable PRI on PF device.
	 */
	if (accel_dev->at_enabled && !accel_dev->is_vf &&
	    !pdev->pri_enabled && adf_svm_enable_pri(pdev)) {
		dev_err(&GET_DEV(accel_dev),
			"Failed to enable Page Request Interface!\n");

		return -EFAULT;
	}
#endif
#ifdef QAT_UIO
	cfg_val = accel_dev->svm_enabled;
	if (adf_cfg_add_key_value_param(accel_dev,
					ADF_GENERAL_SEC,
					ADF_SVM_ENABLE,
					&cfg_val,
					ADF_DEC))
		return -EFAULT;

	cfg_val = accel_dev->at_enabled;
	if (adf_cfg_add_key_value_param(accel_dev,
					ADF_GENERAL_SEC,
					ADF_AT_ENABLE,
					&cfg_val,
					ADF_DEC)) {
		adf_cfg_remove_key_param(accel_dev,
					 ADF_GENERAL_SEC,
					 ADF_SVM_ENABLE);

		return -EFAULT;
	}
#endif
	return 0;
}

void adf_svm_device_disable(struct adf_accel_dev *accel_dev)
{
	adf_svm_device_exit(accel_dev);
}

bool adf_svm_get_dev_capability(struct adf_accel_dev *accel_dev)
{
#if (KERNEL_VERSION(5, 12, 0) <= LINUX_VERSION_CODE)
	const struct iommu_ops *ops = GET_DEV(accel_dev).bus->iommu_ops;

	if (!ops || !ops->dev_has_feat)
		return false;

	return ops->dev_has_feat(&GET_DEV(accel_dev), IOMMU_DEV_FEAT_SVA);
#elif (KERNEL_VERSION(5, 7, 0) <= LINUX_VERSION_CODE)
	return iommu_dev_has_feature(&GET_DEV(accel_dev), IOMMU_DEV_FEAT_SVA);
#elif (KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE)
	return intel_svm_available(&GET_DEV(accel_dev));
#else
	return false;
#endif
}

void adf_svm_device_exit(struct adf_accel_dev *accel_dev)
{
#if (KERNEL_VERSION(5, 7, 0) <= LINUX_VERSION_CODE)
	struct adf_accel_pci *accel_pci_dev = &accel_dev->accel_pci_dev;
	struct pci_dev *pdev = accel_pci_dev->pci_dev;

	if (accel_dev->svm_enabled)
		iommu_dev_disable_feature(&pdev->dev, IOMMU_DEV_FEAT_SVA);
#endif
	accel_dev->at_enabled = false;
	accel_dev->svm_enabled = false;

#ifdef QAT_UIO
	adf_cfg_remove_key_param(accel_dev,
				 ADF_GENERAL_SEC,
				 ADF_AT_ENABLE);
	adf_cfg_remove_key_param(accel_dev,
				 ADF_GENERAL_SEC,
				 ADF_SVM_ENABLE);
#endif
}

int adf_svm_bind_bank_with_pid(struct adf_accel_dev *accel_dev,
			       u32 bank_nr, int pid,
			       cleanup_svm_orphan_fn cleanup_orphan,
			       void *cleanup_priv)
{
	return adf_pasid_bind_bank_with_pid(accel_dev, bank_nr, pid,
					    cleanup_orphan, cleanup_priv);
}

int adf_svm_unbind_bank_with_pid(struct adf_accel_dev *accel_dev,
				 int pid,
				 u32 bank_nr)
{
	return adf_pasid_unbind_bank_with_pid(accel_dev, pid, bank_nr);
}
