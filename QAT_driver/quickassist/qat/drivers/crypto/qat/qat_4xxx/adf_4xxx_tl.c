// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)
/* Copyright(c) 2021 Intel Corporation */

#include "adf_accel_devices.h"
#include "icp_qat_fw_init_admin.h"
#include "adf_common_drv.h"
#include "adf_4xxx_tl.h"
#include <linux/sysfs.h>

#ifdef QAT_ESXI
static int get_tl_num_desc(struct adf_accel_dev *accel_dev)
{
	struct adf_tl_slice_cnt slice_cnt = accel_dev->telemetry->slice_cnt;

	return DEV_TL_DESC_ID_SLICE_UTIL_ATH + slice_cnt.tl_ath_slice_cnt +
	       slice_cnt.tl_cph_slice_cnt + slice_cnt.tl_cpr_slice_cnt +
	       slice_cnt.tl_xlt_slice_cnt + slice_cnt.tl_dcpr_slice_cnt +
	       slice_cnt.tl_pke_slice_cnt + slice_cnt.tl_ucs_slice_cnt +
	       slice_cnt.tl_wat_slice_cnt + slice_cnt.tl_wcp_slice_cnt +
	       HW_MAX_TL_RP_NUM * RP_TL_DESC_ID_MAX;
}

static void create_cntr_desc(struct adf_tl_cntr_desc *desc, char *name, u64 val)
{
	strscpy(desc->name, name, sizeof(desc->name));
	desc->val = val;
}

static void create_slice_cntrs_desc(struct adf_tl_cntr_desc *desc, char *name,
				    size_t num_slices, u32 *val)
{
	size_t i = 0;

	for (i = 0; i < num_slices; i++) {
		scnprintf(desc[i].name, sizeof(desc[i].name), "%s%u", name, i);
		desc[i].val = *(val + i);
	}
}

static void create_rp_cntrs_desc(struct adf_tl_cntr_desc *desc, size_t rp_num,
				 struct adf_tl_ring_pair_data tl_rp_data)
{
	int num_desc = 0;

	scnprintf(desc[num_desc].name, sizeof(desc[num_desc].name),
		  "rp_%u_%s", rp_num, PCI_TRANS_CNT_NAME);
	desc[num_desc++].val = tl_rp_data.tl_pci_trans_cnt;
	scnprintf(desc[num_desc].name, sizeof(desc[num_desc].name),
		  "rp_%u_%s", rp_num, LAT_ACC_NAME);
	desc[num_desc++].val = tl_rp_data.tl_lat_acc;
	scnprintf(desc[num_desc].name, sizeof(desc[num_desc].name),
		  "rp_%u_%s", rp_num, BW_IN_NAME);
	desc[num_desc++].val = tl_rp_data.tl_bw_in;
	scnprintf(desc[num_desc].name, sizeof(desc[num_desc].name),
		  "rp_%u_%s", rp_num, BW_OUT_NAME);
	desc[num_desc++].val = tl_rp_data.tl_bw_out;
	scnprintf(desc[num_desc].name, sizeof(desc[num_desc].name),
		  "rp_%u_%s", rp_num, AT_GLOB_DTLB_HIT_NAME);
	desc[num_desc++].val = tl_rp_data.tl_at_glob_devtlb_hit;
	scnprintf(desc[num_desc].name, sizeof(desc[num_desc].name),
		  "rp_%u_%s", rp_num, AT_GLOB_DTLB_MISS_NAME);
	desc[num_desc++].val = tl_rp_data.tl_at_glob_devtlb_miss;
	scnprintf(desc[num_desc].name, sizeof(desc[num_desc].name),
		  "rp_%u_%s", rp_num, AT_PAYLD_DTLB_HIT_NAME);
	desc[num_desc++].val = tl_rp_data.tl_at_payld_devtlb_hit;
	scnprintf(desc[num_desc].name, sizeof(desc[num_desc].name),
		  "rp_%u_%s", rp_num, AT_PAYLD_DTLB_MISS_NAME);
	desc[num_desc++].val = tl_rp_data.tl_at_payld_devtlb_miss;
}

static void adf_create_tl_desc(struct adf_accel_dev *accel_dev,
			       struct adf_tl_desc *tl_data)
{
	struct adf_telemetry *telemetry = accel_dev->telemetry;
	struct adf_tl_device_data tl_device_data =
		telemetry->tl_data.tl_device_data;
	struct adf_tl_ring_pair_data *tl_rp_data =
		&telemetry->tl_data.tl_ring_pairs_data[0];
	struct adf_tl_slice_cnt slice_cnt = telemetry->slice_cnt;
	struct adf_tl_cntr_desc *desc = tl_data->desc;
	int num_desc = 0;
	size_t rp_num = 0;

	create_cntr_desc(&desc[num_desc++], SNAPSHOT_CNT_MSG,
			 telemetry->tl_data.tl_msg_cnt);
	create_cntr_desc(&desc[num_desc++], PCI_TRANS_CNT_NAME,
			 tl_device_data.tl_pci_trans_cnt);
	create_cntr_desc(&desc[num_desc++], MAX_RD_LAT_NAME,
			 tl_device_data.tl_max_rd_lat);
	create_cntr_desc(&desc[num_desc++], RD_LAT_ACC_NAME,
			 tl_device_data.tl_rd_lat_acc);
	create_cntr_desc(&desc[num_desc++], MAX_LAT_NAME,
			 tl_device_data.tl_max_lat);
	create_cntr_desc(&desc[num_desc++], LAT_ACC_NAME,
			 tl_device_data.tl_lat_acc);
	create_cntr_desc(&desc[num_desc++], BW_IN_NAME,
			 tl_device_data.tl_bw_in);
	create_cntr_desc(&desc[num_desc++], BW_OUT_NAME,
			 tl_device_data.tl_bw_out);
	create_cntr_desc(&desc[num_desc++], PAGE_REQ_LAT_NAME,
			 tl_device_data.tl_at_page_req_lat_acc);
	create_cntr_desc(&desc[num_desc++], AT_TRANS_LAT_NAME,
			 tl_device_data.tl_at_trans_lat_acc);
	create_cntr_desc(&desc[num_desc++], AT_MAX_UTLB_USED_NAME,
			 tl_device_data.tl_at_max_tlb_used);
	create_slice_cntrs_desc(&desc[num_desc], ATH_SLICE_UTIL_NAME,
				slice_cnt.tl_ath_slice_cnt,
				&tl_device_data.tl_sliceutil_ath[0]);
	num_desc += slice_cnt.tl_ath_slice_cnt;
	create_slice_cntrs_desc(&desc[num_desc], CPH_SLICE_UTIL_NAME,
				slice_cnt.tl_cph_slice_cnt,
				&tl_device_data.tl_sliceutil_cph[0]);
	num_desc += slice_cnt.tl_cph_slice_cnt;
	create_slice_cntrs_desc(&desc[num_desc], CPR_SLICE_UTIL_NAME,
				slice_cnt.tl_cpr_slice_cnt,
				&tl_device_data.tl_sliceutil_cpr[0]);
	num_desc += slice_cnt.tl_cpr_slice_cnt;
	create_slice_cntrs_desc(&desc[num_desc], XLT_SLICE_UTIL_NAME,
				slice_cnt.tl_xlt_slice_cnt,
				&tl_device_data.tl_sliceutil_xlt[0]);
	num_desc += slice_cnt.tl_xlt_slice_cnt;
	create_slice_cntrs_desc(&desc[num_desc], DCPR_SLICE_UTIL_NAME,
				slice_cnt.tl_dcpr_slice_cnt,
				&tl_device_data.tl_sliceutil_dcpr[0]);
	num_desc += slice_cnt.tl_dcpr_slice_cnt;
	create_slice_cntrs_desc(&desc[num_desc], PKE_SLICE_UTIL_NAME,
				slice_cnt.tl_pke_slice_cnt,
				&tl_device_data.tl_sliceutil_pke[0]);
	num_desc += slice_cnt.tl_pke_slice_cnt;
	create_slice_cntrs_desc(&desc[num_desc], UCS_SLICE_UTIL_NAME,
				slice_cnt.tl_ucs_slice_cnt,
				&tl_device_data.tl_sliceutil_ucs[0]);
	num_desc += slice_cnt.tl_ucs_slice_cnt;
	create_slice_cntrs_desc(&desc[num_desc], WAT_SLICE_UTIL_NAME,
				slice_cnt.tl_wat_slice_cnt,
				&tl_device_data.tl_sliceutil_wat[0]);
	num_desc += slice_cnt.tl_wat_slice_cnt;
	create_slice_cntrs_desc(&desc[num_desc], WCP_SLICE_UTIL_NAME,
				slice_cnt.tl_wcp_slice_cnt,
				&tl_device_data.tl_sliceutil_wcp[0]);
	num_desc += slice_cnt.tl_wcp_slice_cnt;
	for (rp_num = 0; rp_num < HW_MAX_TL_RP_NUM; rp_num++) {
		create_rp_cntrs_desc(&desc[num_desc], rp_num,
				     tl_rp_data[rp_num]);
		num_desc += RP_TL_DESC_ID_MAX;
	}
}
#endif /* QAT_ESXI */

static int adf_4xxx_alloc_mem_tl(struct adf_accel_dev *accel_dev)
{
	struct adf_telemetry *telemetry = NULL;
	u32 tl_data_size = sizeof(struct adf_tl_data_regs);
	struct device *dev = &GET_DEV(accel_dev);

	telemetry = kzalloc(sizeof(*telemetry), GFP_KERNEL);
	if (!telemetry)
		goto free_tl_mem;

	telemetry->virt_addr =
		dma_alloc_coherent(dev,
				   PAGE_ALIGN(tl_data_size),
				   &telemetry->phy_addr,
				   GFP_KERNEL);
	if (!telemetry->virt_addr)
		goto free_tl_mem;
	memset(telemetry->virt_addr, 0, PAGE_ALIGN(tl_data_size));

	telemetry->rp_num_indexes[TL_RP_0_DATA_INDEX] = TL_RP_0_DEFAULT_NUM;
	telemetry->rp_num_indexes[TL_RP_1_DATA_INDEX] = TL_RP_1_DEFAULT_NUM;
	telemetry->rp_num_indexes[TL_RP_2_DATA_INDEX] = TL_RP_2_DEFAULT_NUM;
	telemetry->rp_num_indexes[TL_RP_3_DATA_INDEX] = TL_RP_3_DEFAULT_NUM;

	accel_dev->telemetry = telemetry;

	return 0;

free_tl_mem:
	dev_err(dev, "Failed to allocate memory for telemetry.\n");
	kfree(telemetry);

	return -ENOMEM;
}

static void adf_4xxx_free_mem_tl(struct adf_accel_dev *accel_dev)
{
	u32 tl_data_size = sizeof(struct adf_tl_data_regs);
	struct device *dev = &GET_DEV(accel_dev);

	if (accel_dev->telemetry) {
		dma_free_coherent(dev, PAGE_ALIGN(tl_data_size),
				  accel_dev->telemetry->virt_addr,
				  accel_dev->telemetry->phy_addr);

		kfree(accel_dev->telemetry);
		accel_dev->telemetry = NULL;
	}
}

static int adf_4xxx_validate_slice_cnt(u8 *slice_cnt_data, u32 slice_type_cnt)
{
	u32 counter = 0;
	int ret = 0;

	for (counter = 0; counter < slice_type_cnt; counter++) {
		if (slice_cnt_data[counter] > HW_MAX_NUM_OF_SLICES) {
			ret = -EFAULT;
			break;
		}
	}

	return ret;
}

static int adf_4xxx_stop_tl(struct adf_accel_dev *accel_dev)
{
	struct icp_qat_fw_init_admin_req req = { 0 };
	struct icp_qat_fw_init_admin_resp rsp = { 0 };
	u32 ae_mask = accel_dev->hw_device->admin_ae_mask;

	if (!accel_dev->telemetry)
		return -ENOMEM;

	accel_dev->telemetry->state = TL_OFF;

	if (!accel_dev->admin)
		return -EFAULT;

	req.cmd_id = ICP_QAT_FW_TL_STOP;
	if (adf_send_admin(accel_dev, &req, &rsp, ae_mask))
		return -EFAULT;

	if (accel_dev->hw_device->switch_drv_active)
		accel_dev->hw_device->switch_drv_active(accel_dev);

	return 0;
}

static int adf_4xxx_start_tl(struct adf_accel_dev *accel_dev)
{
	struct icp_qat_fw_init_admin_req req = { 0 };
	struct icp_qat_fw_init_admin_resp rsp = { 0 };
	u32 ae_mask = accel_dev->hw_device->admin_ae_mask;
	int ret = 0;

	if (!accel_dev->admin)
		return -EFAULT;

	if (!accel_dev->telemetry)
		return -ENOMEM;

	req.cmd_id = ICP_QAT_FW_TL_START;
	req.init_cfg_ptr = (u64)accel_dev->telemetry->phy_addr;
	req.rp_num_index_0 = accel_dev->telemetry->rp_num_indexes[TL_RP_0_DATA_INDEX];
	req.rp_num_index_1 = accel_dev->telemetry->rp_num_indexes[TL_RP_1_DATA_INDEX];
	req.rp_num_index_2 = accel_dev->telemetry->rp_num_indexes[TL_RP_2_DATA_INDEX];
	req.rp_num_index_3 = accel_dev->telemetry->rp_num_indexes[TL_RP_3_DATA_INDEX];
	if (adf_send_admin(accel_dev, &req, &rsp, ae_mask))
		return -EFAULT;

	memcpy(&accel_dev->telemetry->slice_cnt, &rsp.tl_slice_count,
	       sizeof(struct adf_tl_slice_cnt));
	ret = adf_4xxx_validate_slice_cnt((u8 *)(&accel_dev->telemetry->slice_cnt),
					  sizeof(struct adf_tl_slice_cnt));
	if (ret) {
		adf_4xxx_stop_tl(accel_dev);
		return -EFAULT;
	}

	if (accel_dev->hw_device->switch_drv_active)
		accel_dev->hw_device->switch_drv_active(accel_dev);

	accel_dev->telemetry->state = TL_ON;

	return 0;
}

#ifndef QAT_ESXI
static void
adf_4xxx_create_key_val_tl_stat(char *buf, size_t *offset,
				const char *stat_name, u64 stat_value)
{
	*offset += sysfs_emit_at(buf, *offset, "%s %llu\n", stat_name, stat_value);
}

static void
adf_4xxx_get_slice_tl_data(char *buf, size_t *offset, u32 *slice_data,
			   u32 slice_num, const char *slice_name)
{
	char text[MAX_STAT_NAME_BUF_SIZE];
	u32 counter = 0;

	for (counter = 0; counter < slice_num; counter++) {
		scnprintf(text, sizeof(text), "%s%u", slice_name, counter);
		adf_4xxx_create_key_val_tl_stat(buf, offset, text, slice_data[counter]);
	}
}

static ssize_t
adf_4xxx_print_dev_tl_data(struct device *dev, char *buf)
{
	struct adf_accel_dev *accel_dev = pci_get_drvdata(to_pci_dev(dev));
	struct adf_telemetry *telemetry = accel_dev->telemetry;
	struct adf_tl_device_data tl_device_data;
	struct adf_tl_slice_cnt slice_cnt;
	size_t offset = 0;

	if (!telemetry)
		return -ENOMEM;

	tl_device_data = telemetry->tl_data.tl_device_data;
	slice_cnt = telemetry->slice_cnt;

	if (telemetry->state == TL_ON) {
		adf_4xxx_create_key_val_tl_stat(buf, &offset, SNAPSHOT_CNT_MSG,
						telemetry->tl_data.tl_msg_cnt);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, PCI_TRANS_CNT_NAME,
						tl_device_data.tl_pci_trans_cnt);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, MAX_RD_LAT_NAME,
						tl_device_data.tl_max_rd_lat);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, RD_LAT_ACC_NAME,
						tl_device_data.tl_rd_lat_acc);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, MAX_LAT_NAME,
						tl_device_data.tl_max_lat);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, LAT_ACC_NAME,
						tl_device_data.tl_lat_acc);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, BW_IN_NAME,
						tl_device_data.tl_bw_in);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, BW_OUT_NAME,
						tl_device_data.tl_bw_out);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, PAGE_REQ_LAT_NAME,
						tl_device_data.tl_at_page_req_lat_acc);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, AT_TRANS_LAT_NAME,
						tl_device_data.tl_at_trans_lat_acc);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, AT_MAX_UTLB_USED_NAME,
						tl_device_data.tl_at_max_tlb_used);
		adf_4xxx_get_slice_tl_data(buf, &offset, tl_device_data.tl_sliceutil_cpr,
					   slice_cnt.tl_cpr_slice_cnt, CPR_SLICE_UTIL_NAME);
		adf_4xxx_get_slice_tl_data(buf, &offset, tl_device_data.tl_sliceutil_dcpr,
					   slice_cnt.tl_dcpr_slice_cnt, DCPR_SLICE_UTIL_NAME);
		adf_4xxx_get_slice_tl_data(buf, &offset, tl_device_data.tl_sliceutil_xlt,
					   slice_cnt.tl_xlt_slice_cnt, XLT_SLICE_UTIL_NAME);
		adf_4xxx_get_slice_tl_data(buf, &offset, tl_device_data.tl_sliceutil_cph,
					   slice_cnt.tl_cph_slice_cnt, CPH_SLICE_UTIL_NAME);
		adf_4xxx_get_slice_tl_data(buf, &offset, tl_device_data.tl_sliceutil_ath,
					   slice_cnt.tl_ath_slice_cnt, ATH_SLICE_UTIL_NAME);
		adf_4xxx_get_slice_tl_data(buf, &offset, tl_device_data.tl_sliceutil_ucs,
					   slice_cnt.tl_ucs_slice_cnt, UCS_SLICE_UTIL_NAME);
		adf_4xxx_get_slice_tl_data(buf, &offset, tl_device_data.tl_sliceutil_pke,
					   slice_cnt.tl_pke_slice_cnt, PKE_SLICE_UTIL_NAME);
		adf_4xxx_get_slice_tl_data(buf, &offset, tl_device_data.tl_sliceutil_wat,
					   slice_cnt.tl_wat_slice_cnt, WAT_SLICE_UTIL_NAME);
		adf_4xxx_get_slice_tl_data(buf, &offset, tl_device_data.tl_sliceutil_wcp,
					   slice_cnt.tl_wcp_slice_cnt, WCP_SLICE_UTIL_NAME);
	} else {
		offset = sysfs_emit(buf, "%s\n", TL_NOT_TURNED_ON_MSG);
	}

	return offset;
}

static ssize_t
device_data_show(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
	return adf_4xxx_print_dev_tl_data(dev, buf);
}

DEVICE_ATTR_RO(device_data);

static ssize_t
adf_4xxx_print_rp_stat(struct device *dev, char *buf, u8 rp_num_index)
{
	struct adf_accel_dev *accel_dev = pci_get_drvdata(to_pci_dev(dev));
	struct adf_telemetry *telemetry = accel_dev->telemetry;
	struct adf_tl_ring_pair_data tl_ring_pair_data;
	size_t offset = 0;

	if (!telemetry)
		return -ENOMEM;

	tl_ring_pair_data = telemetry->tl_data.tl_ring_pairs_data[rp_num_index];

	if (telemetry->state == TL_ON) {
		adf_4xxx_create_key_val_tl_stat(buf, &offset, SNAPSHOT_CNT_MSG,
						telemetry->tl_data.tl_msg_cnt);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, RP_NUM_INDEX,
						telemetry->rp_num_indexes[rp_num_index]);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, PCI_TRANS_CNT_NAME,
						tl_ring_pair_data.tl_pci_trans_cnt);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, LAT_ACC_NAME,
						tl_ring_pair_data.tl_lat_acc);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, BW_IN_NAME,
						tl_ring_pair_data.tl_bw_in);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, BW_OUT_NAME,
						tl_ring_pair_data.tl_bw_out);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, AT_GLOB_DTLB_HIT_NAME,
						tl_ring_pair_data.tl_at_glob_devtlb_hit);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, AT_GLOB_DTLB_MISS_NAME,
						tl_ring_pair_data.tl_at_glob_devtlb_miss);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, AT_PAYLD_DTLB_HIT_NAME,
						tl_ring_pair_data.tl_at_payld_devtlb_hit);
		adf_4xxx_create_key_val_tl_stat(buf, &offset, AT_PAYLD_DTLB_MISS_NAME,
						tl_ring_pair_data.tl_at_payld_devtlb_miss);
	} else {
		offset = sysfs_emit(buf, "%s\n", TL_NOT_TURNED_ON_MSG);
	}

	return offset;
}

static ssize_t
adf_4xxx_change_rp_index(struct device *dev, const char *buf, size_t count, u8 rp_num_index)
{
	struct adf_accel_dev *accel_dev = pci_get_drvdata(to_pci_dev(dev));
	struct adf_telemetry *telemetry = accel_dev->telemetry;
	u32 new_rp_num = 0;
	u32 counter = 0;

	if (!telemetry)
		return -ENOMEM;

	if (kstrtouint(buf, AUTOMATIC_BASE_DETECT, &new_rp_num))
		return -EINVAL;

	if (new_rp_num > TL_RP_MAX_NUM) {
		dev_warn(dev, TL_MAX_RP_INDEX_WARN);
		return -EINVAL;
	}

	for (counter = 0; counter < HW_MAX_TL_RP_NUM; counter++) {
		if (telemetry->rp_num_indexes[counter] == new_rp_num) {
			dev_warn(dev, "%s in file rp_%c_data",
				 TL_RP_DUP_INDEX_WARN,
				 TL_PR_ALPHA_INDEX(counter));
			return -EINVAL;
		}
	}

	adf_4xxx_stop_tl(accel_dev);
	telemetry->rp_num_indexes[rp_num_index] = new_rp_num;
	adf_4xxx_start_tl(accel_dev);

	return count;
}

static ssize_t rp_A_data_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return adf_4xxx_print_rp_stat(dev, buf, TL_RP_0_DATA_INDEX);
}

static ssize_t rp_A_data_store(struct device *dev,
			       struct device_attribute *dev_attr,
			       const char *buf, size_t count)
{
	return adf_4xxx_change_rp_index(dev, buf, count, TL_RP_0_DATA_INDEX);
}
static DEVICE_ATTR_RW(rp_A_data);

static ssize_t rp_B_data_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return adf_4xxx_print_rp_stat(dev, buf, TL_RP_1_DATA_INDEX);
}

static ssize_t rp_B_data_store(struct device *dev,
			       struct device_attribute *dev_attr,
			       const char *buf, size_t count)
{
	return adf_4xxx_change_rp_index(dev, buf, count, TL_RP_1_DATA_INDEX);
}
static DEVICE_ATTR_RW(rp_B_data);

static ssize_t rp_C_data_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return adf_4xxx_print_rp_stat(dev, buf, TL_RP_2_DATA_INDEX);
}

static ssize_t rp_C_data_store(struct device *dev,
			       struct device_attribute *dev_attr,
			       const char *buf, size_t count)
{
	return adf_4xxx_change_rp_index(dev, buf, count, TL_RP_2_DATA_INDEX);
}
static DEVICE_ATTR_RW(rp_C_data);

static ssize_t rp_D_data_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return adf_4xxx_print_rp_stat(dev, buf, TL_RP_3_DATA_INDEX);
}

static ssize_t rp_D_data_store(struct device *dev,
			       struct device_attribute *dev_attr,
			       const char *buf, size_t count)
{
	return adf_4xxx_change_rp_index(dev, buf, count, TL_RP_3_DATA_INDEX);
}
static DEVICE_ATTR_RW(rp_D_data);

static ssize_t
control_store(struct device *dev, struct device_attribute *dev_attr,
	      const char *buf, size_t count)
{
	struct adf_accel_dev *accel_dev = pci_get_drvdata(to_pci_dev(dev));
	struct adf_telemetry *telemetry = accel_dev->telemetry;
	u32 new_state;

	if (!telemetry)
		return -ENOMEM;

	if (kstrtouint(buf, AUTOMATIC_BASE_DETECT, &new_state))
		return -EINVAL;

	if (new_state == TL_OFF) {
		if (telemetry->state == TL_ON) {
			if (adf_4xxx_stop_tl(accel_dev))
				return -EFAULT;
			telemetry->state = new_state;
		} else {
			dev_warn(dev, TL_STOPPED_WARN);
		}
	} else if (new_state == TL_ON) {
		if (telemetry->state == TL_OFF) {
			if (adf_4xxx_start_tl(accel_dev))
				return -EFAULT;
			telemetry->state = new_state;
		} else {
			dev_warn(dev, TL_STARTED_WARN);
		}
	} else {
		return -EINVAL;
	}

	return count;
}

static ssize_t
control_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct adf_accel_dev *accel_dev = pci_get_drvdata(to_pci_dev(dev));
	size_t offset = 0;

	if (!accel_dev->telemetry)
		return -ENOMEM;

	if (accel_dev->telemetry->state == TL_ON)
		offset = sysfs_emit(buf, "%s\n", TL_ON_MSG);
	else
		offset = sysfs_emit(buf, "%s\n", TL_OFF_MSG);

	return offset;
}
DEVICE_ATTR_RW(control);

static struct attribute *accel_tl_attrs[] = {
	&dev_attr_device_data.attr,
	&dev_attr_rp_A_data.attr,
	&dev_attr_rp_B_data.attr,
	&dev_attr_rp_C_data.attr,
	&dev_attr_rp_D_data.attr,
	&dev_attr_control.attr,
	NULL,
};

static const struct attribute_group tl_sysfs_group = {
	.name = "telemetry",
	.attrs = accel_tl_attrs,
};
#endif /* QAT_ESXI */

static int adf_4xxx_sysfs_create(struct device *dev)
{
#ifndef QAT_ESXI
	return devm_device_add_group(dev, &tl_sysfs_group);
#else
	return 0;
#endif /* QAT_ESXI */
}

static void adf_4xxx_sysfs_delete(struct device *dev)
{
#ifndef QAT_ESXI
	devm_device_remove_group(dev, &tl_sysfs_group);
#endif /* QAT_ESXI */
}

static u32 adf_4xxx_calc_bw(u64 raw_bw)
{
	/* Changing raw data from HW to bytes per second  */
	raw_bw = raw_bw * TL_BW_HW_UNIT_TO_BYTES;
	/* Changing bytes per second to bits per second */
	raw_bw = raw_bw * BITS_IN_BYTE;
	/* Changing bits per second to Mbps */
	return div_u64(raw_bw, BITS_IN_MBITS);
}

static u64 adf_4xxx_calc_avg(u64 dividend, u32 divisor)
{
	if (divisor == 0)
		return 0;
	else
		return div_u64(dividend, divisor);
}

static u64 adf_4xxx_conv_cycles_to_ns(u64 cycles_num)
{
	return cycles_num * CYCLES_TO_NS_MULTIPLIER;
}

static u64 adf_4xxx_calc_avg_ns(u64 dividend, u32 divisor)
{
	u64 quotient = adf_4xxx_calc_avg(dividend, divisor);

	return adf_4xxx_conv_cycles_to_ns(quotient);
}

static void
adf_4xxx_calc_slices_tl_data(u32 *sliceutils, u32 slice_num,
			     struct adf_tl_slice_data_regs *slice_data_regs)
{
	u32 counter;

	for (counter = 0; counter < slice_num; counter++)
		sliceutils[counter] = slice_data_regs[counter].reg_tm_sliceutil;
}

static int adf_4xxx_calc_rp_tl_data(struct adf_telemetry *telemetry)
{
	struct adf_tl_data_regs *tl_data_regs = NULL;
	struct adf_tl_ring_pair_data_regs *tl_rp_regs = NULL;
	struct adf_tl_ring_pair_data *tl_rp = NULL;
	u32 counter = 0;

	if (!telemetry)
		return -EFAULT;

	tl_data_regs = (struct adf_tl_data_regs *)telemetry->virt_addr;

	for (counter = 0; counter < HW_MAX_TL_RP_NUM; counter++) {
		tl_rp_regs = &tl_data_regs->tl_ring_pairs_data_regs[counter];
		tl_rp = &telemetry->tl_data.tl_ring_pairs_data[counter];

		tl_rp->tl_pci_trans_cnt = tl_rp_regs->reg_tl_pci_trans_cnt;
		tl_rp->tl_lat_acc = adf_4xxx_calc_avg_ns(tl_rp_regs->reg_tl_lat_acc,
							 tl_rp_regs->reg_tl_ae_put_cnt);
		tl_rp->tl_bw_in = adf_4xxx_calc_bw(tl_rp_regs->reg_tl_bw_in);
		tl_rp->tl_bw_out = adf_4xxx_calc_bw(tl_rp_regs->reg_tl_bw_out);
		tl_rp->tl_at_glob_devtlb_hit = tl_rp_regs->reg_tl_at_glob_devtlb_hit;
		tl_rp->tl_at_glob_devtlb_miss = tl_rp_regs->reg_tl_at_glob_devtlb_miss;
		tl_rp->tl_at_payld_devtlb_hit = tl_rp_regs->reg_tl_at_payld_devtlb_hit;
		tl_rp->tl_at_payld_devtlb_miss = tl_rp_regs->reg_tl_at_payld_devtlb_miss;
	}

	return 0;
}

static int adf_4xxx_calc_dev_tl_data(struct adf_telemetry *telemetry)
{
	struct adf_tl_data_regs *tl_data_regs = NULL;
	struct adf_tl_device_data_regs *tl_dev_regs = NULL;
	struct adf_tl_device_data *tl_dev = NULL;
	struct adf_tl_slice_cnt slice_cnt;

	if (!telemetry)
		return -EFAULT;

	tl_data_regs = (struct adf_tl_data_regs *)telemetry->virt_addr;
	tl_dev_regs = &tl_data_regs->tl_device_data_regs;
	tl_dev = &telemetry->tl_data.tl_device_data;
	slice_cnt = telemetry->slice_cnt;

	tl_dev->tl_pci_trans_cnt = tl_dev_regs->reg_tl_pci_trans_cnt;
	tl_dev->tl_max_rd_lat = adf_4xxx_conv_cycles_to_ns(tl_dev_regs->reg_tl_max_rd_lat);
	tl_dev->tl_rd_lat_acc = adf_4xxx_calc_avg_ns(tl_dev_regs->reg_tl_rd_lat_acc,
						     tl_dev_regs->reg_tl_rd_cmpl_cnt);
	tl_dev->tl_max_lat = adf_4xxx_conv_cycles_to_ns(tl_dev_regs->reg_tl_max_lat);
	tl_dev->tl_lat_acc = adf_4xxx_calc_avg_ns(tl_dev_regs->reg_tl_lat_acc,
						  tl_dev_regs->reg_tl_ae_put_cnt);
	tl_dev->tl_bw_in = adf_4xxx_calc_bw(tl_dev_regs->reg_tl_bw_in);
	tl_dev->tl_bw_out = adf_4xxx_calc_bw(tl_dev_regs->reg_tl_bw_out);
	tl_dev->tl_at_page_req_lat_acc =
		adf_4xxx_calc_avg_ns(tl_dev_regs->reg_tl_at_page_req_lat_acc,
				     tl_dev_regs->reg_tl_at_page_req_cnt);
	tl_dev->tl_at_trans_lat_acc = adf_4xxx_calc_avg_ns(tl_dev_regs->reg_tl_at_trans_lat_acc,
							   tl_dev_regs->reg_tl_at_trans_lat_cnt);
	tl_dev->tl_at_max_tlb_used = tl_dev_regs->reg_tl_at_max_tlb_used;

	adf_4xxx_calc_slices_tl_data(tl_dev->tl_sliceutil_ath, slice_cnt.tl_ath_slice_cnt,
				     tl_dev_regs->ath_slices);
	adf_4xxx_calc_slices_tl_data(tl_dev->tl_sliceutil_cph, slice_cnt.tl_cph_slice_cnt,
				     tl_dev_regs->cph_slices);
	adf_4xxx_calc_slices_tl_data(tl_dev->tl_sliceutil_cpr, slice_cnt.tl_cpr_slice_cnt,
				     tl_dev_regs->cpr_slices);
	adf_4xxx_calc_slices_tl_data(tl_dev->tl_sliceutil_xlt, slice_cnt.tl_xlt_slice_cnt,
				     tl_dev_regs->xlt_slices);
	adf_4xxx_calc_slices_tl_data(tl_dev->tl_sliceutil_dcpr, slice_cnt.tl_dcpr_slice_cnt,
				     tl_dev_regs->dcpr_slices);
	adf_4xxx_calc_slices_tl_data(tl_dev->tl_sliceutil_pke, slice_cnt.tl_pke_slice_cnt,
				     tl_dev_regs->pke_slices);
	adf_4xxx_calc_slices_tl_data(tl_dev->tl_sliceutil_ucs, slice_cnt.tl_ucs_slice_cnt,
				     tl_dev_regs->ucs_slices);
	adf_4xxx_calc_slices_tl_data(tl_dev->tl_sliceutil_wat, slice_cnt.tl_wat_slice_cnt,
				     tl_dev_regs->wat_slices);
	adf_4xxx_calc_slices_tl_data(tl_dev->tl_sliceutil_wcp, slice_cnt.tl_wcp_slice_cnt,
				     tl_dev_regs->wcp_slices);

	return 0;
}

static u64 adf_4xxx_get_next_tl_timeout(u32 timeout_val)
{
	unsigned long timeout = msecs_to_jiffies(timeout_val);

	return rounddown(jiffies + timeout, timeout);
}

static void timer_handler(struct timer_list *tl)
{
	struct adf_telemetry *telemetry = from_timer(telemetry, tl, timer);
	struct adf_tl_data_regs *tl_data_regs = NULL;
	unsigned long timeout = adf_4xxx_get_next_tl_timeout(TL_TIMER_INT_MS);

	mod_timer(tl, timeout);

	if (telemetry->state == TL_ON) {
		tl_data_regs = (struct adf_tl_data_regs *)telemetry->virt_addr;

		if (telemetry->tl_data.tl_msg_cnt != tl_data_regs->reg_tl_msg_cnt) {
			adf_4xxx_calc_dev_tl_data(telemetry);
			adf_4xxx_calc_rp_tl_data(telemetry);
			telemetry->tl_data.tl_msg_cnt = tl_data_regs->reg_tl_msg_cnt;
		}
	}
}

#ifdef QAT_ESXI
static int adf_4xxx_update_rp_indexes(struct adf_accel_dev *accel_dev,
				      struct adf_tl_rp_indices *rp_indexes)
{
	struct adf_telemetry *telemetry = accel_dev->telemetry;
	u32 tl_state = accel_dev->telemetry->state;
	int index = 0;

	/* No change of indices */
	if (rp_indexes->num_rps > HW_MAX_TL_RP_NUM) {
		rp_indexes->num_rps = HW_MAX_TL_RP_NUM;
		if (rp_indexes->rp_num_indexes)
			for (index = 0; index < rp_indexes->num_rps; index++)
				rp_indexes->rp_num_indexes[index] =
					telemetry->rp_num_indexes[index];
		return -EINVAL;
	}
	if (!rp_indexes->num_rps)
		goto done;

	if (tl_state == TL_ON && adf_4xxx_stop_tl(accel_dev))
		return -EFAULT;
	for (index = 0; index < rp_indexes->num_rps; index++)
		telemetry->rp_num_indexes[index] =
			rp_indexes->rp_num_indexes[index];
	if (tl_state == TL_ON && adf_4xxx_start_tl(accel_dev))
		return -EFAULT;

done:
	return 0;
}

int adf_4xxx_control_tl(struct adf_accel_dev *accel_dev,
			struct adf_tl_rp_indices *rp_indexes, bool enable)
{
	u32 new_state = enable ? TL_ON : TL_OFF;
	int ret = 0;

	if (!accel_dev->telemetry)
		return -EPERM;
	ret = adf_4xxx_update_rp_indexes(accel_dev, rp_indexes);
	if (ret)
		return ret;
	if (new_state != accel_dev->telemetry->state) {
		if (new_state == TL_ON)
			return adf_4xxx_start_tl(accel_dev);
		if (new_state == TL_OFF)
			return adf_4xxx_stop_tl(accel_dev);
	}
	return 0;
}

int adf_4xxx_get_tl(struct adf_accel_dev *accel_dev,
		    struct adf_tl_desc *tl_data)
{
	if (!accel_dev->telemetry || accel_dev->telemetry->state != TL_ON)
		return -EPERM;
	if (!tl_data)
		return -EINVAL;
	if (!tl_data->desc || tl_data->num_desc != get_tl_num_desc(accel_dev)) {
		tl_data->num_desc = get_tl_num_desc(accel_dev);
		return -ENOMEM;
	}
	adf_create_tl_desc(accel_dev, tl_data);
	return 0;
}
#endif /* QAT_ESXI */

int adf_4xxx_init_tl(struct adf_accel_dev *accel_dev)
{
	struct device *dev = &GET_DEV(accel_dev);
	int ret = 0;
	unsigned long timeout = 0;

	if (adf_4xxx_alloc_mem_tl(accel_dev)) {
		ret = -ENOMEM;
		goto mem_clean_up;
	}

	ret = adf_4xxx_sysfs_create(dev);
	if (ret)
		goto tl_sysfs_create_fail;
	timeout = adf_4xxx_get_next_tl_timeout(TL_TIMER_INT_MS);
	timer_setup(&accel_dev->telemetry->timer, timer_handler, 0);
	mod_timer(&accel_dev->telemetry->timer, timeout);

	return 0;

tl_sysfs_create_fail:
	adf_4xxx_free_mem_tl(accel_dev);

mem_clean_up:
	dev_err(dev, "Failed to init telemetry!\n");
	return ret;
}

int adf_4xxx_exit_tl(struct adf_accel_dev *accel_dev)
{
	struct device *dev = &GET_DEV(accel_dev);
	int ret = 0;

	if (!accel_dev->telemetry)
		return -ENOMEM;

	if (accel_dev->telemetry->state == TL_ON) {
		if (adf_4xxx_stop_tl(accel_dev)) {
			dev_err(dev, "Failed to send telemetry init stop\n");
			ret = -EFAULT;
		}
	}

	del_timer_sync(&accel_dev->telemetry->timer);
	adf_4xxx_sysfs_delete(dev);
	adf_4xxx_free_mem_tl(accel_dev);

	return ret;
}
