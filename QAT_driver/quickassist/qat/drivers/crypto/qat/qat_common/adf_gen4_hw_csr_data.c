// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)
/* Copyright(c) 2019 - 2021 Intel Corporation */

#include <linux/delay.h>

#include "adf_accel_devices.h"
#include "adf_transport_access_macros_gen4.h"
#include "adf_gen4_hw_csr_data.h"

#define ADF_RPRESET_TIMEOUT_MS		5000
#define ADF_RPRESET_POLLING_INTERVAL	20

static u32 build_ring_config(u32 size)
{
	return BUILD_RING_CONFIG(size);
}

static u64 build_resp_ring_config(u32 size,
				  u32 watermark_nf,
				  u32 watermark_ne)
{
	return BUILD_RESP_RING_CONFIG(size, watermark_nf, watermark_ne);
}

static u64 build_ring_base_addr(dma_addr_t addr,
				u32 size)
{
	return BUILD_RING_BASE_ADDR_GEN4(addr, size);
}

static u32 read_csr_ring_head(void __iomem *csr_base_addr,
			      u32 bank,
			      u32 ring)
{
	return READ_CSR_RING_HEAD_GEN4(csr_base_addr, bank, ring);
}

static void write_csr_ring_head(void __iomem *csr_base_addr,
				u32 bank,
				u32 ring,
				u32 value)
{
	WRITE_CSR_RING_HEAD_GEN4(csr_base_addr, bank, ring, value);
}

static u32 read_csr_ring_tail(void __iomem *csr_base_addr,
			      u32 bank,
			      u32 ring)
{
	return READ_CSR_RING_TAIL_GEN4(csr_base_addr, bank, ring);
}

static void write_csr_ring_tail(void __iomem *csr_base_addr,
				u32 bank,
				u32 ring,
				u32 value)
{
	WRITE_CSR_RING_TAIL_GEN4(csr_base_addr, bank, ring, value);
}

static u32 read_csr_stat(void __iomem *csr_base_addr,
			 u32 bank)
{
	return READ_CSR_STAT_GEN4(csr_base_addr, bank);
}

static u32 read_csr_uo_stat(void __iomem *csr_base_addr,
			    u32 bank)
{
	return READ_CSR_UO_STAT_GEN4(csr_base_addr, bank);
}

static u32 read_csr_e_stat(void __iomem *csr_base_addr,
			   u32 bank)
{
	return READ_CSR_E_STAT_GEN4(csr_base_addr, bank);
}

static u32 read_csr_ne_stat(void __iomem *csr_base_addr,
			    u32 bank)
{
	return READ_CSR_NE_STAT_GEN4(csr_base_addr, bank);
}

static u32 read_csr_nf_stat(void __iomem *csr_base_addr,
			    u32 bank)
{
	return READ_CSR_NF_STAT_GEN4(csr_base_addr, bank);
}

static u32 read_csr_f_stat(void __iomem *csr_base_addr,
			   u32 bank)
{
	return READ_CSR_F_STAT_GEN4(csr_base_addr, bank);
}

static u32 read_csr_c_stat(void __iomem *csr_base_addr,
			   u32 bank)
{
	return READ_CSR_C_STAT_GEN4(csr_base_addr, bank);
}

static u32 read_csr_exp_stat(void __iomem *csr_base_addr,
			     u32 bank)
{
	return READ_CSR_EXP_STAT_GEN4(csr_base_addr, bank);
}

static u32 read_csr_exp_int_en(void __iomem *csr_base_addr,
			       u32 bank)
{
	return READ_CSR_EXP_INT_EN_GEN4(csr_base_addr, bank);
}

static void write_csr_exp_int_en(void __iomem *csr_base_addr,
				 u32 bank,
				 u32 value)
{
	WRITE_CSR_EXP_INT_EN_GEN4(csr_base_addr, bank, value);
}

static u32 read_csr_ring_config(void __iomem *csr_base_addr,
				u32 bank,
				u32 ring)
{
	return READ_CSR_RING_CONFIG_GEN4(csr_base_addr, bank, ring);
}

static void write_csr_ring_config(void __iomem *csr_base_addr,
				  u32 bank,
				  u32 ring,
				  u32 value)
{
	WRITE_CSR_RING_CONFIG_GEN4(csr_base_addr, bank, ring, value);
}

static dma_addr_t read_csr_ring_base(void __iomem *csr_base_addr,
				     u32 bank,
				     u32 ring)
{
	return READ_CSR_RING_BASE_GEN4(csr_base_addr, bank, ring);
}

static void write_csr_ring_base(void __iomem *csr_base_addr,
				u32 bank,
				u32 ring,
				dma_addr_t addr)
{
	WRITE_CSR_RING_BASE_GEN4(csr_base_addr, bank, ring, addr);
}

static u32 read_csr_int_en(void __iomem *csr_base_addr,
			   u32 bank)
{
	return READ_CSR_INT_EN_GEN4(csr_base_addr, bank);
}

static void write_csr_int_en(void __iomem *csr_base_addr,
			     u32 bank,
			     u32 value)
{
	WRITE_CSR_INT_EN_GEN4(csr_base_addr, bank, value);
}

static u32 read_csr_int_flag(void __iomem *csr_base_addr,
			     u32 bank)
{
	return READ_CSR_INT_FLAG_GEN4(csr_base_addr, bank);
}

static void write_csr_int_flag(void __iomem *csr_base_addr,
			       u32 bank,
			       u32 value)
{
	WRITE_CSR_INT_FLAG_GEN4(csr_base_addr, bank, value);
}

static u32 read_csr_int_srcsel(void __iomem *csr_base_addr,
			       u32 bank,
			       u32 idx)
{
	return READ_CSR_INT_SRCSEL_GEN4(csr_base_addr, bank, idx);
}

static void write_csr_int_srcsel(void __iomem *csr_base_addr,
				 u32 bank,
				 u32 idx,
				 u32 value)
{
	WRITE_CSR_INT_SRCSEL_GEN4(csr_base_addr, bank, idx, value);
}

static u32 read_csr_int_col_en(void __iomem *csr_base_addr,
			       u32 bank)
{
	return READ_CSR_INT_COL_EN_GEN4(csr_base_addr, bank);
}

static void write_csr_int_col_en(void __iomem *csr_base_addr,
				 u32 bank,
				 u32 value)
{
	WRITE_CSR_INT_COL_EN_GEN4(csr_base_addr, bank, value);
}

static u32 read_csr_int_col_ctl(void __iomem *csr_base_addr,
				u32 bank)
{
	return READ_CSR_INT_COL_CTL_GEN4(csr_base_addr, bank);
}

static void write_csr_int_col_ctl(void __iomem *csr_base_addr,
				  u32 bank,
				  u32 value)
{
	WRITE_CSR_INT_COL_CTL_GEN4(csr_base_addr, bank, value);
}

static u32 read_csr_int_flag_and_col(void __iomem *csr_base_addr,
				     u32 bank)
{
	return READ_CSR_INT_FLAG_AND_COL_GEN4(csr_base_addr, bank);
}

static void write_csr_int_flag_and_col(void __iomem *csr_base_addr,
				       u32 bank,
				       u32 value)
{
	WRITE_CSR_INT_FLAG_AND_COL_GEN4(csr_base_addr, bank, value);
}

static u32 read_csr_ring_srv_arb_en(void __iomem *csr_base_addr, u32 bank)
{
	return READ_CSR_RING_SRV_ARB_EN_GEN4(csr_base_addr, bank);
}

static void write_csr_ring_srv_arb_en(void __iomem *csr_base_addr, u32 bank,
				      u32 value)
{
	WRITE_CSR_RING_SRV_ARB_EN_GEN4(csr_base_addr, bank, value);
}

static u32 get_src_sel_mask(void)
{
	return ADF_BANK_INT_SRC_SEL_MASK_GEN4;
}

static u32 get_int_col_ctl_enable_mask(void)
{
	return ADF_RING_CSR_INT_COL_CTL_ENABLE;
}

static u32 get_bank_irq_mask(u32 irq_mask)
{
	return 0x1;
}

static void bank_pasid_enable(void __iomem *csr_base_addr,
			      u32 bank_number, bool at, bool adi, bool priv,
			      int pasid)
{
	u32 val;

	val = ADF_CSR_RD(csr_base_addr, ADF_WQM_CSR_PASIDCTL(bank_number));
	val &= ~(ADF_PASIDCTL_PASID_MASK | ADF_PASIDCTL_PASID_ENABLE_RING);
	val |= (pasid & ADF_PASIDCTL_PASID_MASK);
	val |= ADF_PASIDCTL_ENABLE_PASID;

	if (adi)
		val |= ADF_PASIDCTL_ENABLE_ADI;

	if (at)
		val |= ADF_PASIDCTL_ENABLE_AT;

	if (priv)
		val |= ADF_PASIDCTL_ENABLE_PRIV;

	ADF_CSR_WR(csr_base_addr,
		   ADF_WQM_CSR_PASIDCTL(bank_number),
		   val);
}

static void bank_pasid_disable(void __iomem *csr_base_addr,
			       u32 bank_number, bool at, bool adi, bool priv)
{
	u32 val;

	val = ADF_CSR_RD(csr_base_addr, ADF_WQM_CSR_PASIDCTL(bank_number));
	val &= ~ADF_PASIDCTL_PASID_MASK;
	val &= ~ADF_PASIDCTL_ENABLE_PASID;

	if (adi)
		val &= ~ADF_PASIDCTL_ENABLE_ADI;

	if (at)
		val &= ~ADF_PASIDCTL_ENABLE_AT;

	if (priv)
		val &= ~ADF_PASIDCTL_ENABLE_PRIV;

	ADF_CSR_WR(csr_base_addr,
		   ADF_WQM_CSR_PASIDCTL(bank_number),
		   val);
}

static void disable_pm_idle_interrupt(void __iomem *csr_base_addr)
{
	u32 val = ADF_CSR_RD(csr_base_addr, ADF_GEN4_PM_INTERRUPT);

	val &= ~ADF_GEN4_PM_IDLE_INT_EN;
	ADF_CSR_WR(csr_base_addr, ADF_GEN4_PM_INTERRUPT, val);
}

static void clear_pm_sts(void __iomem *csr_base_addr)
{
	u32 val = ADF_CSR_RD(csr_base_addr, ADF_GEN4_PM_INTERRUPT);

	val |= ADF_GEN4_PM_INT_STS_MASK;
	ADF_CSR_WR(csr_base_addr, ADF_GEN4_PM_INTERRUPT, val);
}

static void deactive_pm_drive(void __iomem *csr_base_addr)
{
	u32 val = ADF_CSR_RD(csr_base_addr, ADF_GEN4_PM_INTERRUPT);

	val &= ~ADF_GEN4_PM_DRV_ACTIVE;
	ADF_CSR_WR(csr_base_addr, ADF_GEN4_PM_INTERRUPT, val);
}

static void active_pm_drive(void __iomem *csr_base_addr)
{
	u32 val = ADF_CSR_RD(csr_base_addr, ADF_GEN4_PM_INTERRUPT);

	val |= ADF_GEN4_PM_DRV_ACTIVE;
	ADF_CSR_WR(csr_base_addr, ADF_GEN4_PM_INTERRUPT, val);
}

static void enable_misc_interrupts(void __iomem *csr_base_addr)
{
	ADF_CSR_WR(csr_base_addr, ADF_GEN4_SMIAPF_MASK_OFFSET, 0);
}

static void disable_misc_interrupts(void __iomem *csr_base_addr)
{
	ADF_CSR_WR(csr_base_addr, ADF_GEN4_SMIAPF_MASK_OFFSET, 1);
}

static u32 read_vf2pf_isr_sou(void __iomem *csr_base_addr)
{
	u32 val = ADF_CSR_RD(csr_base_addr, ADF_GEN4_VM2PF_SOU);

	return val;
}

static u32 read_vf2pf_isr_mask(void __iomem *csr_base_addr)
{
	u32 val = ADF_CSR_RD(csr_base_addr, ADF_GEN4_VM2PF_MSK);

	return val;
}

static void write_vf2pf_isr_mask(void __iomem *csr_base_addr, u32 val)
{
	ADF_CSR_WR(csr_base_addr, ADF_GEN4_VM2PF_MSK, val);
}

static void clear_pf2vf_msg_register(void __iomem *csr_base_addr, u32 index)
{
	ADF_CSR_WR(csr_base_addr,
		   ADF_GEN4_PF2VM_OFFSET((u64)index), 0x00000000);
}

static void enable_bundle_interrupts(void __iomem *csr_base_addr,
				     struct adf_accel_dev *accel_dev)
{
	ADF_CSR_WR(csr_base_addr, ADF_GEN4_SMIAPF_RP_X0_MASK_OFFSET, 0);
	ADF_CSR_WR(csr_base_addr, ADF_GEN4_SMIAPF_RP_X1_MASK_OFFSET, 0);
}

static void disable_bundle_interrupts(void __iomem *csr_base_addr)
{
	ADF_CSR_WR(csr_base_addr, ADF_GEN4_SMIAPF_RP_X0_MASK_OFFSET, 1);
	ADF_CSR_WR(csr_base_addr, ADF_GEN4_SMIAPF_RP_X1_MASK_OFFSET, 1);
}

static void mask_rp_interrupts(void __iomem *csr_base_addr)
{
	ADF_CSR_WR(csr_base_addr,
		   ADF_GEN4_SMIAPF_RP_X0_MASK_OFFSET, 0xFFFFFFFF);
	ADF_CSR_WR(csr_base_addr,
		   ADF_GEN4_SMIAPF_RP_X1_MASK_OFFSET, 0xFFFFFFFF);
}

static void mask_pfvf_interrupts(void __iomem *csr_base_addr)
{
	ADF_CSR_WR(csr_base_addr, ADF_GEN4_VM2PF_MSK, 0xFF);
}

void gen4_init_hw_csr_info(struct adf_hw_csr_info *csr_info)
{
	struct adf_hw_csr_ops *csr_ops = &csr_info->csr_ops;

	csr_info->csr_addr_offset = ADF_RING_CSR_ADDR_OFFSET_GEN4;
	csr_info->ring_bundle_size = ADF_RING_BUNDLE_SIZE_GEN4;
	csr_info->bank_int_flag_clear_mask = ADF_BANK_INT_FLAG_CLEAR_MASK_GEN4;
	csr_info->num_rings_per_int_srcsel = ADF_RINGS_PER_INT_SRCSEL_GEN4;
	csr_info->arb_enable_mask = 0x1;

	csr_ops->build_ring_config = build_ring_config;
	csr_ops->build_resp_ring_config = build_resp_ring_config;
	csr_ops->build_ring_base_addr = build_ring_base_addr;
	csr_ops->read_csr_ring_head = read_csr_ring_head;
	csr_ops->write_csr_ring_head = write_csr_ring_head;
	csr_ops->read_csr_ring_tail = read_csr_ring_tail;
	csr_ops->write_csr_ring_tail = write_csr_ring_tail;
	csr_ops->read_csr_stat = read_csr_stat;
	csr_ops->read_csr_uo_stat = read_csr_uo_stat;
	csr_ops->read_csr_e_stat = read_csr_e_stat;
	csr_ops->read_csr_ne_stat = read_csr_ne_stat;
	csr_ops->read_csr_nf_stat = read_csr_nf_stat;
	csr_ops->read_csr_f_stat = read_csr_f_stat;
	csr_ops->read_csr_c_stat = read_csr_c_stat;
	csr_ops->read_csr_exp_stat = read_csr_exp_stat;
	csr_ops->read_csr_exp_int_en = read_csr_exp_int_en;
	csr_ops->write_csr_exp_int_en = write_csr_exp_int_en;
	csr_ops->read_csr_ring_config = read_csr_ring_config;
	csr_ops->write_csr_ring_config = write_csr_ring_config;
	csr_ops->read_csr_ring_base = read_csr_ring_base;
	csr_ops->write_csr_ring_base = write_csr_ring_base;
	csr_ops->read_csr_int_en = read_csr_int_en;
	csr_ops->write_csr_int_en = write_csr_int_en;
	csr_ops->read_csr_int_flag = read_csr_int_flag;
	csr_ops->write_csr_int_flag = write_csr_int_flag;
	csr_ops->read_csr_int_srcsel = read_csr_int_srcsel;
	csr_ops->write_csr_int_srcsel = write_csr_int_srcsel;
	csr_ops->read_csr_int_col_en = read_csr_int_col_en;
	csr_ops->write_csr_int_col_en = write_csr_int_col_en;
	csr_ops->read_csr_int_col_ctl = read_csr_int_col_ctl;
	csr_ops->write_csr_int_col_ctl = write_csr_int_col_ctl;
	csr_ops->read_csr_int_flag_and_col = read_csr_int_flag_and_col;
	csr_ops->write_csr_int_flag_and_col = write_csr_int_flag_and_col;
	csr_ops->read_csr_ring_srv_arb_en = read_csr_ring_srv_arb_en;
	csr_ops->write_csr_ring_srv_arb_en = write_csr_ring_srv_arb_en;
	csr_ops->get_src_sel_mask = get_src_sel_mask;
	csr_ops->get_int_col_ctl_enable_mask = get_int_col_ctl_enable_mask;
	csr_ops->get_bank_irq_mask = get_bank_irq_mask;
	csr_ops->bank_pasid_enable = bank_pasid_enable;
	csr_ops->bank_pasid_disable = bank_pasid_disable;
	csr_ops->disable_pm_idle_interrupt = disable_pm_idle_interrupt;
	csr_ops->clear_pm_sts = clear_pm_sts;
	csr_ops->deactive_pm_drive = deactive_pm_drive;
	csr_ops->active_pm_drive = active_pm_drive;
	csr_ops->enable_misc_interrupts = enable_misc_interrupts;
	csr_ops->disable_misc_interrupts = disable_misc_interrupts;
	csr_ops->read_vf2pf_isr_sou = read_vf2pf_isr_sou;
	csr_ops->read_vf2pf_isr_mask = read_vf2pf_isr_mask;
	csr_ops->write_vf2pf_isr_mask = write_vf2pf_isr_mask;
	csr_ops->clear_pf2vf_msg_register = clear_pf2vf_msg_register;
	csr_ops->enable_bundle_interrupts = enable_bundle_interrupts;
	csr_ops->disable_bundle_interrupts = disable_bundle_interrupts;
	csr_ops->mask_rp_interrupts = mask_rp_interrupts;
	csr_ops->mask_pfvf_interrupts = mask_pfvf_interrupts;
}
EXPORT_SYMBOL_GPL(gen4_init_hw_csr_info);

int gen4_ring_pair_reset(void __iomem *csr, u32 bank_number)
{
	int reset_timeout = ADF_RPRESET_TIMEOUT_MS;
	const int timeout_step = ADF_RPRESET_POLLING_INTERVAL;
	u32 val;

	/* Write rpresetctl register bit#0 as 1
	 * As rpresetctl registers have no RW bits, no need to preserve
	 * values for other bits, just write bit#0
	 * NOTE: bit#12-bit#31 are WO, the write operation only takes
	 * effect when bit#1 is written 1 for pasid level reset
	 */
	ADF_CSR_WR(csr, ADF_WQM_CSR_RPRESETCTL(bank_number),
		   BIT(ADF_WQM_CSR_RPRESETCTL_SHIFT));

	/* Read rpresetsts register to wait for rp reset complete */
	while (reset_timeout > 0) {
		val = ADF_CSR_RD(csr,
				 ADF_WQM_CSR_RPRESETSTS(bank_number));
		if (val & ADF_WQM_CSR_RPRESETSTS_MASK)
			break;
		msleep(timeout_step);
		reset_timeout -= timeout_step;
	}
	if (reset_timeout <= 0)
		return -EFAULT;

	/* When rp reset is done, clear rpresetsts bit0 */
	ADF_CSR_WR(csr, ADF_WQM_CSR_RPRESETSTS(bank_number),
		   BIT(ADF_WQM_CSR_RPRESETSTS_SHIFT));
	return 0;
}
EXPORT_SYMBOL_GPL(gen4_ring_pair_reset);

int gen4_ring_pair_drain(void __iomem *csr, u32 bank_number, int timeout_ms)
{
	int drain_timeout = timeout_ms;
	const int timeout_step = ADF_RPRESET_POLLING_INTERVAL;
	u32 val;

	/*
	 * Write rpresetctl register bit#2 as 1 to drain the rp
	 * As rpresetctl registers have no RW bits, no need to preserve
	 * values for other bits, just write bit#2
	 * NOTE: there is no pasid level drain for rp, but that's
	 * not a problem due context of usage.
	 */
	ADF_CSR_WR(csr, ADF_WQM_CSR_RPRESETCTL(bank_number),
		   BIT(ADF_WQM_CSR_RPRESETCTL_DRAIN_SHIFT));

	/* Read rpresetsts register to wait for rp reset completion */
	while (drain_timeout > 0) {
		val = ADF_CSR_RD(csr, ADF_WQM_CSR_RPRESETSTS(bank_number));
		if (val & ADF_WQM_CSR_RPRESETSTS_MASK)
			break;
		msleep(timeout_step);
		drain_timeout -= timeout_step;
	}
	if (drain_timeout <= 0)
		return -EFAULT;

	/* When rp drain is done, clear rpresetsts bit0 */
	ADF_CSR_WR(csr, ADF_WQM_CSR_RPRESETSTS(bank_number),
		   BIT(ADF_WQM_CSR_RPRESETSTS_SHIFT));
	return 0;
}
EXPORT_SYMBOL_GPL(gen4_ring_pair_drain);
#ifdef QAT_ESXI

void gen4_bank_state_save(void __iomem *csr, u32 bank_number,
			  struct bank_state *state, u32 num_rings)
{
	int i = 0;

	state->ringstat0 = read_csr_stat(csr, bank_number);
	state->ringuostat = read_csr_uo_stat(csr, bank_number);
	state->ringestat = read_csr_e_stat(csr, bank_number);
	state->ringnestat = read_csr_ne_stat(csr, bank_number);
	state->ringnfstat = read_csr_nf_stat(csr, bank_number);
	state->ringfstat = read_csr_f_stat(csr, bank_number);
	state->ringcstat0 = read_csr_c_stat(csr, bank_number);
	state->iaintflagen = read_csr_int_en(csr, bank_number);
	state->iaintflagreg = read_csr_int_flag(csr, bank_number);
	state->iaintflagsrcsel0 = read_csr_int_srcsel(csr, bank_number, 0);
	state->iaintcolen = read_csr_int_col_en(csr, bank_number);
	state->iaintcolctl = read_csr_int_col_ctl(csr, bank_number);
	state->iaintflagandcolen = read_csr_int_flag_and_col(csr, bank_number);
	state->ringexpstat = read_csr_exp_stat(csr, bank_number);
	state->ringexpintenable = read_csr_exp_int_en(csr, bank_number);
	state->ringsrvarben = read_csr_ring_srv_arb_en(csr, bank_number);
	state->num_rings = num_rings;

	for (i = 0; i < num_rings; i++) {
		state->rings[i].ringheadoffset =
			read_csr_ring_head(csr, bank_number, i);
		state->rings[i].ringtailoffset =
			read_csr_ring_tail(csr, bank_number, i);
		state->rings[i].ringconfig =
			read_csr_ring_config(csr, bank_number, i);
		state->rings[i].ringbase =
			read_csr_ring_base(csr, bank_number, i);
	}
}

/* F_RISE - Full Going True 0100b */
#define ADF_RP_INT_SRC_SEL_F_RISE_MASK 0x04
/* F_FALL - Full Going False 0111b */
#define ADF_RP_INT_SRC_SEL_F_FALL_MASK 0x07
int gen4_bank_state_restore(void __iomem *csr, u32 bank_number,
			    struct bank_state *state, u32 num_rings,
			    int tx_rx_gap)
{
	u32 val = 0, tmp_val = 0;
	int i;

	/* Only even number of rings is supported. */
	if (num_rings % 2)
		return -EINVAL;

	/* Only 2 rings per bank are supported */
	if (num_rings != 2)
		return -EINVAL;

	/* Disabling arbiter. */
	write_csr_ring_srv_arb_en(csr, bank_number, 0);

	/* Restore ringbase for TX & RX rings. */
	for (i = 0; i < num_rings; i++)
		write_csr_ring_base(csr, bank_number, i,
				    state->rings[i].ringbase);

	/* Restore ringconfig for TX & RX rings. */
	for (i = 0; i < num_rings; i++)
		write_csr_ring_config(csr, bank_number, i,
				      state->rings[i].ringconfig);

	for (i = 0; i < num_rings / 2; i++) {
		int tx = i * (tx_rx_gap + 1);
		int rx = tx + tx_rx_gap;
		u32 tx_idx = tx / ADF_RINGS_PER_INT_SRCSEL_GEN4;
		u32 rx_idx = rx / ADF_RINGS_PER_INT_SRCSEL_GEN4;

		/* Restore TX ring head and tail */
		write_csr_ring_head(csr, bank_number, tx,
				    state->rings[tx].ringheadoffset);

		write_csr_ring_tail(csr, bank_number, tx,
				    state->rings[tx].ringtailoffset);

		/*
		 * Check empty flag of TX ring of RP, if it's set, then
		 * set iaintflagsrcsel.src(2n) (src of TX ring) to F_RISE and
		 * then restore TX ring head
		 */
		if (state->ringestat & (BIT(tx))) {
			val = read_csr_int_srcsel(csr, bank_number, tx_idx);
			val |= (ADF_RP_INT_SRC_SEL_F_RISE_MASK << (8 * tx));
			write_csr_int_srcsel(csr, bank_number, tx_idx, val);
			write_csr_ring_head(csr, bank_number, tx,
					    state->rings[tx].ringheadoffset);
		}

		/* Restore RX ring tail */
		write_csr_ring_tail(csr, bank_number, rx,
				    state->rings[rx].ringtailoffset);

		/* Set iaintflagsrcsel.src(2n+1) (src of RX ring) to F_RISE */
		val = read_csr_int_srcsel(csr, bank_number, rx_idx);
		val |= (ADF_RP_INT_SRC_SEL_F_RISE_MASK << (8 * rx));
		write_csr_int_srcsel(csr, bank_number, rx_idx, val);

		/* Restore RX ring head */
		write_csr_ring_head(csr, bank_number, rx,
				    state->rings[rx].ringheadoffset);

		/* Set iaintflagsrcsel.src(2n+1) (src of RX ring) to F_FALL */
		val = read_csr_int_srcsel(csr, bank_number, rx_idx);
		val |= (ADF_RP_INT_SRC_SEL_F_FALL_MASK << (8 * rx));
		write_csr_int_srcsel(csr, bank_number, rx_idx, val);

		/*
		 * If RX ring of RP is full, that is full flag is set,
		 * copy RX ring tail again to set the full flag correctly
		 */
		if (state->ringfstat & BIT(rx))
			write_csr_ring_tail(csr, bank_number, rx,
					    state->rings[rx].ringtailoffset);
	}

	/* Restore iaintflagen, iaintcolen, iaintflagsrcsel, ringexpintenable */
	write_csr_int_flag_and_col(csr, bank_number, state->iaintflagandcolen);
	write_csr_int_en(csr, bank_number, state->iaintflagen);
	write_csr_int_col_en(csr, bank_number, state->iaintcolen);
	write_csr_int_srcsel(csr, bank_number, 0, state->iaintflagsrcsel0);
	write_csr_exp_int_en(csr, bank_number, state->ringexpintenable);
	write_csr_int_col_ctl(csr, bank_number, state->iaintcolctl);

	/* Check that all ring statuses are restored into a saved state. */
	tmp_val = read_csr_stat(csr, bank_number);
	val = state->ringstat0;
	if (tmp_val != val) {
		pr_err("Fail to restore ringstat register. Expected 0x%x, but actual is 0x%x\n",
		       tmp_val, val);
		return -EFAULT;
	}

	tmp_val = read_csr_e_stat(csr, bank_number);
	val = state->ringestat;
	if (tmp_val != val) {
		pr_err("Fail to restore ringestat register. Expected 0x%x, but actual is 0x%x\n",
		       tmp_val, val);
		return -EFAULT;
	}

	tmp_val = read_csr_ne_stat(csr, bank_number);
	val = state->ringnestat;
	if (tmp_val != val) {
		pr_err("Fail to restore ringnestat register. Expected 0x%x, but actual is 0x%x\n",
		       tmp_val, val);
		return -EFAULT;
	}

	tmp_val = read_csr_nf_stat(csr, bank_number);
	val = state->ringnfstat;
	if (tmp_val != val) {
		pr_err("Fail to restore ringnfstat register. Expected 0x%x, but actual is 0x%x\n",
		       tmp_val, val);
		return -EFAULT;
	}

	tmp_val = read_csr_f_stat(csr, bank_number);
	val = state->ringfstat;
	if (tmp_val != val) {
		pr_err("Fail to restore ringfstat register. Expected 0x%x, but actual is 0x%x\n",
		       tmp_val, val);
		return -EFAULT;
	}

	tmp_val = read_csr_c_stat(csr, bank_number);
	val = state->ringcstat0;
	if (tmp_val != val) {
		pr_err("Fail to restore ringcstat register. Expected 0x%x, but actual is 0x%x\n",
		       tmp_val, val);
		return -EFAULT;
	}

	tmp_val = read_csr_exp_stat(csr, bank_number);
	val = state->ringexpstat;
	/* If ring had exception state during save, we don't care. */
	if (tmp_val && !val) {
		pr_err("Bank was restored with exception: 0x%x\n", val);
		return -EFAULT;
	}

	/* TODO: restore coalescing timer */
	/*
	 * HW arbiter register is not restored there in order to keep HW
	 * quiesced until all SW layers will be ready to kick start HW.
	 */

	return 0;
}
#endif /* QAT_ESXI */
