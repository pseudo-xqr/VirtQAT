// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)
/* Copyright(c) 2018 - 2022 Intel Corporation */
#include <linux/atomic.h>
#include <adf_accel_devices.h>
#include <adf_common_drv.h>
#include <adf_pf2vf_msg.h>
#include <adf_dev_err.h>
#include <adf_cfg.h>
#include <adf_transport_access_macros_gen4.h>
#include <adf_gen4_hw_csr_data.h>
#include <qat_crypto.h>
#include "adf_gen4_hw_data.h"
#include "adf_gen4_ras.h"
#include "adf_heartbeat.h"
#include "adf_4xxx_hw_data.h"
#include "adf_4xxx_ras.h"
#include "adf_4xxx_reset.h"
#include "adf_4xxx_pm.h"
#include "adf_4xxx_kpt.h"
#include "adf_4xxx_tl.h"
#include "icp_qat_fw_init_admin.h"
#include "icp_qat_hw.h"
#include "adf_adi.h"
#include "adf_gen4_adi_hal.h"
#include "adf_gen4_timer.h"
#include "adf_pasid.h"
#include "adf_transport_internal.h"
#include "adf_gen4_rl.h"

#define MAX_CLUSTER 4
#define ADF_CONST_TABLE_SIZE 1024

/* Accel unit information */
static const struct adf_accel_unit adf_4xxx_au_a_ae[] = {
	{0x1,  0x1,   0xF,       0x1B,       4, ADF_ACCEL_SERVICE_NULL},
	{0x2,  0x1,   0xF0,      0x6C0,      4, ADF_ACCEL_SERVICE_NULL},
	{0x4,  0x1,   0x100,     0xF000,     1, ADF_ACCEL_ADMIN},
};

/* Masks representing ME thread-service mappings.
 * Thread 7 carries out Admin work and is thus
 * left out.
 */
static u8 default_active_thd_mask = 0x7F;
static u8 dc_me_active_thd_mask = 0x03;

static u32 thrd_to_arb_map_gen[ADF_4XXX_MAX_ACCELENGINES] = {0};

#define ADF_4XXX_DEFAULT_RING_TO_SRV_MAP \
	(ASYM | SYM << ADF_CFG_SERV_RING_PAIR_1_SHIFT | \
	ASYM << ADF_CFG_SERV_RING_PAIR_2_SHIFT | \
	SYM << ADF_CFG_SERV_RING_PAIR_3_SHIFT)

#define ADF_4XXX_ASYM_SYM ADF_4XXX_DEFAULT_RING_TO_SRV_MAP

#define ADF_4XXX_DC \
	(COMP | COMP << ADF_CFG_SERV_RING_PAIR_1_SHIFT | \
	COMP << ADF_CFG_SERV_RING_PAIR_2_SHIFT | \
	COMP << ADF_CFG_SERV_RING_PAIR_3_SHIFT)

#define ADF_4XXX_SYM \
	(SYM | SYM << ADF_CFG_SERV_RING_PAIR_1_SHIFT | \
	SYM << ADF_CFG_SERV_RING_PAIR_2_SHIFT | \
	SYM << ADF_CFG_SERV_RING_PAIR_3_SHIFT)

#define ADF_4XXX_ASYM \
	(ASYM | ASYM << ADF_CFG_SERV_RING_PAIR_1_SHIFT | \
	ASYM << ADF_CFG_SERV_RING_PAIR_2_SHIFT | \
	ASYM << ADF_CFG_SERV_RING_PAIR_3_SHIFT)

#define ADF_4XXX_ASYM_DC \
	(ASYM | ASYM << ADF_CFG_SERV_RING_PAIR_1_SHIFT | \
	COMP << ADF_CFG_SERV_RING_PAIR_2_SHIFT | \
	COMP << ADF_CFG_SERV_RING_PAIR_3_SHIFT)

#define ADF_4XXX_SYM_DC \
	(SYM | SYM << ADF_CFG_SERV_RING_PAIR_1_SHIFT | \
	COMP << ADF_CFG_SERV_RING_PAIR_2_SHIFT | \
	COMP << ADF_CFG_SERV_RING_PAIR_3_SHIFT)

#define ADF_4XXX_NA \
	(NA | NA << ADF_CFG_SERV_RING_PAIR_1_SHIFT | \
	NA << ADF_CFG_SERV_RING_PAIR_2_SHIFT | \
	NA << ADF_CFG_SERV_RING_PAIR_3_SHIFT)

struct adf_enabled_services {
	const char svcs_enabled[ADF_CFG_MAX_VAL_LEN_IN_BYTES];
	u16 rng_to_svc_msk;
};

static struct adf_enabled_services adf_4xxx_svcs[] = {
	{"dc", ADF_4XXX_DC},
	{"sym", ADF_4XXX_SYM},
	{"asym", ADF_4XXX_ASYM},
	{"dc;asym", ADF_4XXX_ASYM_DC},
	{"asym;dc", ADF_4XXX_ASYM_DC},
	{"sym;dc", ADF_4XXX_SYM_DC},
	{"dc;sym", ADF_4XXX_SYM_DC},
	{"asym;sym", ADF_4XXX_ASYM_SYM},
	{"sym;asym", ADF_4XXX_ASYM_SYM},
};

static struct adf_hw_device_class adf_4xxx_class = {
	.name = ADF_4XXX_DEVICE_NAME,
	.type = DEV_4XXX,
	.instances = 0
};

struct reg_info {
	size_t	offs;
	char	*name;
};

static struct reg_info adf_err_regs[] = {
	{ADF_4XXX_AT_GLOBAL0_PAR_STS, "AT_GLOBAL0_PAR_STS"},
	{ADF_4XXX_AT_GLOBAL1_PAR_STS, "AT_GLOBAL1_PAR_STS"},
	{ADF_4XXX_AT_PAR_STS, "AT_PAR_STS"},
	{ADF_4XXX_AT_PAYLOAD_PAR_STS, "AT_PAYLOAD_PAR_STS"},
	{ADF_4XXX_AT_SC_PAR_STS, "AT_SC_PAR_STS"},
	{ADF_4XXX_AT_SRT_PAR_STS, "AT_SRT_PAR_STS"},
	{ADF_4XXX_AT_EXCEP_STS, "AT_EXCEP_STS"},
	{ADF_4XXX_BFMGRERROR, "BFMGRERROR"},
	{ADF_4XXX_CERRSSMSH(0), "CERRSSMSH"},
	{ADF_4XXX_CPP_CFC_ERR_STATUS, "CPP_CFC_ERR_STATUS"},
	{ADF_4XXX_CPP_CFC_ERR_PPID_LO, "CPP_CFC_ERR_PPID_LO"},
	{ADF_4XXX_CPP_CFC_ERR_PPID_HI, "CPP_CFC_ERR_PPID_HI"},
	{ADF_4XXX_CPPMEMTGTERR, "CPPMEMTGTERR"},
	{ADF_4XXX_ERRSOU0, "ERRSOU0"},
	{ADF_4XXX_ERRSOU1, "ERRSOU1"},
	{ADF_4XXX_ERRSOU2, "ERRSOU2"},
	{ADF_4XXX_ERRSOU3, "ERRSOU3"},
	{ADF_4XXX_FCU_STATUS, "FCU_STATUS"},
	{ADF_4XXX_FW_ERR_STATUS, "FW_ERR_STATUS"},
	{ADF_4XXX_HICPPAGENTCMDPARERRLOG, "HICPPAGENTCMDPARERRLOG"},
	{ADF_4XXX_HIAECORERRLOG_CPP0, "HIAECORERRLOG_CPP0"},
	{ADF_4XXX_HIAEUNCERRLOG_CPP0, "HIAEUNCERRLOG_CPP0"},
	{ADF_INTSTATSSM(0), "INTSTATSSM"},
	{ADF_IAINTSTATSSM(0), "IAINTSTATSSM"},
	{ADF_PPERR(0), "PPERR"},
	{ADF_PPERRID(0), "PPERRID"},
	{ADF_4XXX_QM_PAR_STS, "QM_PAR_STS"},
	{ADF_4XXX_RICPPINTSTS, "RICPPINTSTS"},
	{ADF_4XXX_RIERRPULLID, "RIERRPULLID"},
	{ADF_4XXX_RIERRPUSHID, "RIERRPUSHID"},
	{ADF_4XXX_RI_MEM_PAR_ERR_FERR, "RI_MEM_PAR_ERR_FERR"},
	{ADF_4XXX_RIMEM_PARERR_STS, "RIMEM_PARERR_STS"},
	{ADF_4XXX_RL_PAR_STS, "RL_PAR_STS"},
	{ADF_4XXX_RSRCMGRERROR, "RSRCMGRERROR"},
	{ADF_4XXX_SER_ERR_SSMSH, "SER_ERR_SSMSH"},
	{ADF_4XXX_SINTPF, "SINTPF"},
	{ADF_4XXX_SLICEHANGSTATUS_ATH_CPH, "SLICEHANGSTATUS_ATH_CPH"},
	{ADF_4XXX_SLICEHANGSTATUS_CPR_XLT, "SLICEHANGSTATUS_CPR_XLT"},
	{ADF_4XXX_SLICEHANGSTATUS_DCPR_UCS, "SLICEHANGSTATUS_DCPR_UCS"},
	{ADF_4XXX_SLICEHANGSTATUS_PKE, "SLICEHANGSTATUS_PKE"},
	{ADF_4XXX_SLICEHANGSTATUS_WAT_WCP, "SLICEHANGSTATUS_WAT_WCP"},
	{ADF_4XXX_SLPAGEFWDERR, "SLPAGEFWDERR"},

	{ADF_4XXX_SPPPULLCMDPARERR_ATH_CPH, "SPPPULLCMDPARERR_ATH_CPH"},
	{ADF_4XXX_SPPPULLCMDPARERR_CPR_XLT, "SPPPULLCMDPARERR_CPR_XLT"},
	{ADF_4XXX_SPPPULLCMDPARERR_DCPR_UCS, "SPPPULLCMDPARERR_DCPR_UCS"},
	{ADF_4XXX_SPPPULLCMDPARERR_PKE, "SPPPULLCMDPARERR_PKE"},
	{ADF_4XXX_SPPPULLCMDPARERR_WAT_WCP, "SPPPULLCMDPARERR_WAT_WCP"},

	{ADF_4XXX_SPPPULLDATAPARERR_ATH_CPH, "SPPPULLDATAPARERR_ATH_CPH"},
	{ADF_4XXX_SPPPULLDATAPARERR_CPR_XLT, "SPPPULLDATAPARERR_CPR_XLT"},
	{ADF_4XXX_SPPPULLDATAPARERR_DCPR_UCS, "SPPPULLDATAPARERR_DCPR_UCS"},
	{ADF_4XXX_SPPPULLDATAPARERR_PKE, "SPPPULLDATAPARERR_PKE"},
	{ADF_4XXX_SPPPULLDATAPARERR_WAT_WCP, "SPPPULLDATAPARERR_WAT_WCP"},

	{ADF_4XXX_SPPPUSHCMDPARERR_ATH_CPH, "SPPPUSHCMDPARERR_ATH_CPH"},
	{ADF_4XXX_SPPPUSHCMDPARERR_CPR_XLT, "SPPPUSHCMDPARERR_CPR_XLT"},
	{ADF_4XXX_SPPPUSHCMDPARERR_DCPR_UCS, "SPPPUSHCMDPARERR_DCPR_UCS"},
	{ADF_4XXX_SPPPUSHCMDPARERR_PKE, "SPPPUSHCMDPARERR_PKE"},
	{ADF_4XXX_SPPPUSHCMDPARERR_WAT_WCP, "SPPPUSHCMDPARERR_WAT_WCP"},

	{ADF_4XXX_SPPPUSHDATAPARERR_ATH_CPH, "SPPPUSHDATAPARERR_ATH_CPH"},
	{ADF_4XXX_SPPPUSHDATAPARERR_CPR_XLT, "SPPPUSHDATAPARERR_CPR_XLT"},
	{ADF_4XXX_SPPPUSHDATAPARERR_DCPR_UCS, "SPPPUSHDATAPARERR_DCPR_UCS"},
	{ADF_4XXX_SPPPUSHDATAPARERR_PKE, "SPPPUSHDATAPARERR_PKE"},
	{ADF_4XXX_SPPPUSHDATAPARERR_WAT_WCP, "SPPPUSHDATAPARERR_WAT_WCP"},

	{ADF_4XXX_SSMCPPERR(0), "SSMCPPERR"},
	{ADF_4XXX_SSMSOFTERRORPARITY_SRC, "SSMSOFTERRORPARITY_SRC"},
	{ADF_4XXX_SSMSOFTERRORPARITY_ATH_CPH, "SSMSOFTERRORPARITY_ATH_CPH"},
	{ADF_4XXX_SSMSOFTERRORPARITY_CPR_XLT, "SSMSOFTERRORPARITY_CPR_XLT"},
	{ADF_4XXX_SSMSOFTERRORPARITY_DCPR_UCS, "SSMSOFTERRORPARITY_DCPR_UCS"},
	{ADF_4XXX_SSMSOFTERRORPARITY_PKE, "SSMSOFTERRORPARITY_PKE"},
	{ADF_4XXX_SSMSOFTERRORPARITY_WAT_WCP, "SSMSOFTERRORPARITY_WAT_WCP"},
	{ADF_4XXX_TI_CD_PAR_STS, "TI_CD_PAR_STS"},
	{ADF_4XXX_TI_CI_PAR_STS, "TI_CI_PAR_STS"},
	{ADF_4XXX_TICPPINTSTS, "TICPPINTSTS"},
	{ADF_4XXX_TIERRPPID, "TIERRPPID"},
	{ADF_4XXX_TIERRPULLID, "TIERRPULLID"},
	{ADF_4XXX_TIERRPUSHID, "TIERRPUSHID"},
	{ADF_4XXX_TI_MEM_PAR_ERR_FIRST_ERROR, "TI_MEM_PAR_ERR_FIRST_ERROR"},
	{ADF_4XXX_TIMISCSTS, "TIMISCSTS"},
	{ADF_4XXX_TI_PULL0FUB_PAR_STS, "TI_PULL0FUB_PAR_STS"},
	{ADF_4XXX_TI_PUSHFUB_PAR_STS, "TI_PUSHFUB_PAR_STS"},
	{ADF_4XXX_TI_TRNSB_PAR_STS, "TI_TRNSB_PAR_STS"},
	{ADF_4XXX_UERRSSMSH(0), "UERRSSMSH"},
	{ADF_4XXX_RLT_ERRLOG, "RLT_ERRLOG"},
	{ADF_UERRSSMSHAD(0), "UERRSSMSHAD"}
};

static int qat_asym_inst_config(struct adf_accel_dev *accel_dev, int inst,
				int bank)
{
	char key[ADF_CFG_MAX_KEY_LEN_IN_BYTES];
	unsigned long val = bank;
	struct adf_hw_device_data *hw_device = accel_dev->hw_device;
	unsigned long tx_rx_offset = hw_device->tx_rx_gap;

	snprintf(key, sizeof(key), ADF_CY "%d" ADF_RING_BANK_NUM_ASYM, inst);
	if (adf_cfg_add_key_value_param(accel_dev, ADF_KERNEL_SEC,
					key, &val, ADF_DEC))
		goto err;

	snprintf(key, sizeof(key),
		 ADF_CY "%d" ADF_ETRMGR_CORE_AFFINITY, inst);
	if (adf_cfg_add_key_value_param(accel_dev, ADF_KERNEL_SEC,
					key, &val, ADF_DEC))
		goto err;

	val = ADF_DEFAULT_ASYM_RING_SIZE;
	snprintf(key, sizeof(key), ADF_CY "%d" ADF_RING_ASYM_SIZE, inst);
	if (adf_cfg_add_key_value_param(accel_dev, ADF_KERNEL_SEC, key,
					&val, ADF_DEC))
		goto err;

	val = 0;
	snprintf(key, sizeof(key), ADF_CY "%d" ADF_RING_ASYM_TX, inst);
	if (adf_cfg_add_key_value_param(accel_dev, ADF_KERNEL_SEC, key,
					&val, ADF_DEC))
		goto err;

	val = tx_rx_offset;
	snprintf(key, sizeof(key), ADF_CY "%d" ADF_RING_ASYM_RX, inst);
	if (adf_cfg_add_key_value_param(accel_dev, ADF_KERNEL_SEC, key,
					&val, ADF_DEC))
		goto err;

	val = ADF_COALESCING_DEF_TIME;
	snprintf(key, sizeof(key), ADF_ETRMGR_COALESCE_TIMER_FORMAT, bank);
	if (adf_cfg_add_key_value_param(accel_dev, "Accelerator0", key,
					&val, ADF_DEC))
		goto err;

	return 0;
err:
	return -EINVAL;
}

static int qat_sym_inst_config(struct adf_accel_dev *accel_dev, int inst,
			       int bank)
{
	char key[ADF_CFG_MAX_KEY_LEN_IN_BYTES];
	unsigned long val = bank;
	struct adf_hw_device_data *hw_device = accel_dev->hw_device;
	unsigned long tx_rx_offset = hw_device->tx_rx_gap;

	snprintf(key, sizeof(key), ADF_CY "%d" ADF_RING_BANK_NUM_SYM, inst);
	if (adf_cfg_add_key_value_param(accel_dev, ADF_KERNEL_SEC,
					key, &val, ADF_DEC))
		goto err;

	snprintf(key, sizeof(key),
		 ADF_CY "%d" ADF_ETRMGR_CORE_AFFINITY, inst);
	if (adf_cfg_add_key_value_param(accel_dev, ADF_KERNEL_SEC,
					key, &val, ADF_DEC))
		goto err;

	val = ADF_DEFAULT_SYM_RING_SIZE;
	snprintf(key, sizeof(key), ADF_CY "%d" ADF_RING_SYM_SIZE, inst);
	if (adf_cfg_add_key_value_param(accel_dev, ADF_KERNEL_SEC, key,
					&val, ADF_DEC))
		goto err;

	val = 0;
	snprintf(key, sizeof(key), ADF_CY "%d" ADF_RING_SYM_TX, inst);
	if (adf_cfg_add_key_value_param(accel_dev, ADF_KERNEL_SEC, key,
					&val, ADF_DEC))
		goto err;

	val = tx_rx_offset;
	snprintf(key, sizeof(key), ADF_CY "%d" ADF_RING_SYM_RX, inst);
	if (adf_cfg_add_key_value_param(accel_dev, ADF_KERNEL_SEC, key,
					&val, ADF_DEC))
		goto err;

	val = ADF_COALESCING_DEF_TIME;
	snprintf(key, sizeof(key), ADF_ETRMGR_COALESCE_TIMER_FORMAT, bank);
	if (adf_cfg_add_key_value_param(accel_dev, "Accelerator0", key,
					&val, ADF_DEC))
		goto err;

	return 0;
err:
	return -EINVAL;
}

int adf_4xxx_qat_crypto_dev_config(struct adf_accel_dev *accel_dev)
{
	int banks = GET_MAX_BANKS(accel_dev);
	u16 serv_type = 0;
	int bank = 0;
	unsigned long num_asym = 0;
	unsigned long num_sym = 0;
	unsigned long num_cy = 0;
	struct adf_hw_device_data *hw_device = accel_dev->hw_device;

	if (adf_cfg_section_add(accel_dev, ADF_KERNEL_SEC))
		goto err;
	if (adf_cfg_section_add(accel_dev, "Accelerator0"))
		goto err;

	while (bank < banks) {
		serv_type = GET_SRV_TYPE(hw_device->ring_to_svc_map,
					 bank % ADF_CFG_NUM_SERVICES);
		switch (serv_type) {
		case ASYM:
			if (qat_asym_inst_config(accel_dev, num_asym, bank))
				goto err;
			num_asym++;
			break;
		case SYM:
			if (qat_sym_inst_config(accel_dev, num_sym, bank))
				goto err;
			num_sym++;
			break;
		default:
			break;
		}
		bank++;
	}

	num_cy = max(num_asym, num_sym);
	if (adf_cfg_add_key_value_param(accel_dev, ADF_KERNEL_SEC,
					ADF_NUM_CY, &num_cy, ADF_DEC))
		goto err;

	set_bit(ADF_STATUS_CONFIGURED, &accel_dev->status);
	return 0;
err:
	dev_err(&GET_DEV(accel_dev), "Failed to start QAT accel dev\n");
	return -EINVAL;
}

static u32 get_accel_mask(struct adf_accel_dev *accel_dev)
{
	return ADF_4XXX_ACCELERATORS_MASK;
}

static u32 get_ae_mask(struct adf_accel_dev *accel_dev)
{
	struct pci_dev *pdev = accel_dev->accel_pci_dev.pci_dev;
	u32 me_disable;

	pci_read_config_dword(pdev, ADF_4XXX_FUSECTL4_OFFSET,
			      &me_disable);
	return (~me_disable) & ADF_4XXX_ACCELENGINES_MASK;
}

static u32 get_num_accels(struct adf_hw_device_data *self)
{
	return ADF_4XXX_MAX_ACCELERATORS;
}

static u32 get_num_aes(struct adf_hw_device_data *self)
{
	u32 i, ctr = 0;

	if (!self || !self->ae_mask)
		return 0;

	for (i = 0; i < ADF_4XXX_MAX_ACCELENGINES; i++) {
		if (self->ae_mask & (1 << i))
			ctr++;
	}
	return ctr;
}

static u32 get_misc_bar_id(struct adf_hw_device_data *self)
{
	return ADF_4XXX_PMISC_BAR;
}

static u32 get_etr_bar_id(struct adf_hw_device_data *self)
{
	return ADF_4XXX_ETR_BAR;
}

static u32 get_sram_bar_id(struct adf_hw_device_data *self)
{
	return ADF_4XXX_SRAM_BAR;
}

/**
 * adf_4xxx_set_ssm_wdtimer() - Initialize the slice hang watchdog timer.
 * @accel_dev: Structure holding accelerator data
 *
 * Return: 0 on success, error code otherwise
 */
static int adf_4xxx_set_ssm_wdtimer(struct adf_accel_dev *accel_dev)
{

	struct adf_hw_device_data *hw_device = accel_dev->hw_device;
	struct adf_bar *misc_bar =
		&GET_BARS(accel_dev)[hw_device->get_misc_bar_id(hw_device)];
	void __iomem *csr = misc_bar->virt_addr;
	u32 wdt_val_h = (u32)(ADF_4XXX_SSM_WDT_64BIT_DEFAULT_VALUE >> 32);
	u32 wdt_val_l = (u32)ADF_4XXX_SSM_WDT_64BIT_DEFAULT_VALUE;
	u32 accel;

	/* Convert timer values from milliseconds to CPP clock cycles. */

	for (accel = 0; accel < hw_device->get_num_accels(hw_device); accel++) {
		/* Configures Slice Hang watchdogs */
		ADF_CSR_WR(csr, ADF_4XXX_SSMWDTL_OFFSET(accel), wdt_val_l);
		ADF_CSR_WR(csr, ADF_4XXX_SSMWDTH_OFFSET(accel), wdt_val_h);
		ADF_CSR_WR(csr, ADF_4XXX_SSMWDTPKEL_OFFSET(accel), wdt_val_l);
		ADF_CSR_WR(csr, ADF_4XXX_SSMWDTPKEH_OFFSET(accel), wdt_val_h);
	}
	return 0;
}

/*
 * The vector routing table is be used to select the MSI-X entry to use for
 * each interrupt source.
 * The first ADF_4XXX_ETR_MAX_BANKS entries correspond to ring interrupts.
 * The final entry corresponds to VM2PF or error interrupts.
 * This vector table could be used to configure one MSI-X entry to be shared
 * between multiple interrupt sources.
 *
 * The default routing is set to have a one to one correspondence between the
 * interrupt source and the MSI-X entry used.
 */
static void adf_4xxx_set_msix_default_rttable(struct adf_accel_dev *accel_dev)
{
	void __iomem *csr;
	int i;

	csr = (&GET_BARS(accel_dev)[ADF_4XXX_PMISC_BAR])->virt_addr;
	for (i = 0; i <= ADF_4XXX_ETR_MAX_BANKS; i++)
		ADF_CSR_WR(csr, ADF_4XXX_MSIX_RTTABLE_OFFSET(i), i);
}

/**
 * adf_4xxx_handle_slice_hang_error() - Check slice hang status
 * @accel_dev: Structure holding accelerator data
 * @accel_num: maximum number of acceleration unit
 * @csr: pointer to a HW register
 */
void adf_4xxx_handle_slice_hang_error(struct adf_accel_dev *accel_dev,
				      u32 accel_num,
				      void __iomem *csr)
{
	adf_gen4_handle_slice_hang_error(accel_dev, accel_num, csr);
}

static u32 get_clock_speed(struct adf_hw_device_data *self)
{
	/* CPP clock is half high-speed clock */
	return self->clock_frequency / 2;

}

static enum dev_sku_info get_sku(struct adf_hw_device_data *self)
{
	return DEV_SKU_1;
}

#if defined(CONFIG_PCI_IOV)
static void mask_misc_irq(struct adf_accel_dev *accel_dev, const bool mask_irq)
{
	void __iomem *addr;

	addr = (&GET_BARS(accel_dev)[ADF_4XXX_PMISC_BAR])->virt_addr;

	ADF_CSR_WR(addr, ADF_4XXX_SMIAPF_MASK_OFFSET, mask_irq);
}

static void process_and_get_vf2pf_int(void __iomem *pmiscbar_addr,
				      u32 vf_mask_sets[ADF_MAX_VF2PF_SET])
{
	int i;
	u32 sou, mask;

	sou = ADF_CSR_RD(pmiscbar_addr, ADF_4XXX_VM2PF_SOU);
	mask = ADF_CSR_RD(pmiscbar_addr, ADF_4XXX_VM2PF_MSK);
	vf_mask_sets[0] = sou & ~mask;

	for (i = 1; i < ADF_MAX_VF2PF_SET; i++)
		vf_mask_sets[i] = 0;

	/*
	 * Due to HW limitations, when disabling the interrupts, we can't
	 * just disable the requested sources, as this would lead to missed
	 * interrupts if sources change just before writing to ERRMSK3.
	 * To resolve this, disable all interrupts and re-enable only the
	 * sources that are not currently being serviced and the sources that
	 * were not already disabled. Re-enabling will trigger a new interrupt
	 * for the sources that have changed in the meantime, if any.
	 */
	ADF_CSR_WR(pmiscbar_addr, ADF_4XXX_VM2PF_MSK, ADF_VF2PF_INT_MASK);
	ADF_CSR_WR(pmiscbar_addr, ADF_4XXX_VM2PF_MSK, mask | sou);
}

static void enable_vm2pf_interrupts(void __iomem *pmiscbar_addr,
				    u32 vf_mask, u8 i)
{
	if (i)
		return;

	adf_csr_fetch_and_and(pmiscbar_addr,
			      ADF_4XXX_VM2PF_MSK,
			      ~vf_mask);
}

static void disable_vm2pf_interrupts(void __iomem *pmiscbar_addr,
				     u32 vf_mask, u8 i)
{
	if (i)
		return;

	adf_csr_fetch_and_or(pmiscbar_addr,
			     ADF_4XXX_VM2PF_MSK,
			     vf_mask);
}

static int check_arbitrary_numvfs(struct adf_accel_dev *accel_dev,
				  const int numvfs)
{
	int totalvfs = pci_sriov_get_totalvfs(accel_to_pci_dev(accel_dev));

	return numvfs > totalvfs ? totalvfs : numvfs;
}
#endif

static struct adf_accel_unit *get_au_by_ae(struct adf_accel_dev *accel_dev,
					   int ae_num)
{
	int i = 0;
	struct adf_accel_unit *accel_unit =  accel_dev->au_info->au;

	if (!accel_unit)
		return NULL;

	for (i = 0; i < ADF_4XXX_MAX_ACCELUNITS; i++)
		if (accel_unit[i].ae_mask & BIT(ae_num))
			return &accel_unit[i];

	return NULL;
}

static bool check_accel_unit_service(enum adf_accel_unit_services au_srv,
				     enum adf_cfg_service_type ring_srv)
{
	if ((au_srv & ADF_ACCEL_SERVICE_NULL) && ring_srv == NA)
		return true;
	if ((au_srv & ADF_ACCEL_COMPRESSION) && ring_srv == COMP)
		return true;
	if ((au_srv & ADF_ACCEL_ASYM) && ring_srv == ASYM)
		return true;
	if ((au_srv & ADF_ACCEL_CRYPTO) && ring_srv == SYM)
		return true;

	return false;
}

static void adf_4xxx_cfg_gen_dispatch_arbiter(struct adf_accel_dev *accel_dev,
					   u32 *thrd_to_arb_map_gen)
{
	struct adf_accel_unit *au = NULL;
	int engine = 0;
	int thread = 0;
	int service;
	u16 service_type;
	u32 service_mask;
	unsigned long thd_srv_mask = default_active_thd_mask;
	u16 ena_srv_mask = accel_dev->hw_device->ring_to_svc_map;
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;

	for (engine = 0;
		engine < ADF_4XXX_MAX_ACCELENGINES - 1;
		engine++) {
		thrd_to_arb_map_gen[engine] = 0;
		service_mask = 0;
		au = get_au_by_ae(accel_dev, engine);
		if (!au)
			continue;

		for (service = 0; service < ADF_CFG_MAX_SERVICES;
		     service++) {
			service_type =
				GET_SRV_TYPE(ena_srv_mask, service);
			if (check_accel_unit_service(au->services,
						     service_type))
				service_mask |= BIT(service);
		}

		if (au->services == ADF_ACCEL_COMPRESSION)
			thd_srv_mask = dc_me_active_thd_mask;
		else if (au->services == ADF_ACCEL_ASYM)
			thd_srv_mask = hw_data->asym_ae_active_thd_mask;
		else
			thd_srv_mask = default_active_thd_mask;

		for_each_set_bit(thread, &thd_srv_mask, 8) {
			thrd_to_arb_map_gen[engine] |=
				(service_mask << (ADF_CFG_MAX_SERVICES * thread));
		}

	}
}

static void adf_get_arbiter_mapping(struct adf_accel_dev *accel_dev,
				    u32 const **arb_map_config)
{
	adf_4xxx_cfg_gen_dispatch_arbiter(accel_dev, thrd_to_arb_map_gen);
	*arb_map_config = thrd_to_arb_map_gen;
}

static u32 get_pf2vm_offset(u32 i)
{
	return ADF_4XXX_PF2VM_OFFSET(i);
}

static u32 get_vm2pf_offset(u32 i)
{
	return ADF_4XXX_VM2PF_OFFSET(i);
}

static void get_arb_info(struct arb_info *arb_csrs_info)
{
	arb_csrs_info->arbiter_offset = ADF_4XXX_ARB_OFFSET;
	arb_csrs_info->wrk_thd_2_srv_arb_map =
					ADF_4XXX_ARB_WRK_2_SER_MAP_OFFSET;
}

static void get_admin_info(struct admin_info *admin_csrs_info)
{
	admin_csrs_info->mailbox_offset = ADF_4XXX_MAILBOX_BASE_OFFSET;
	admin_csrs_info->admin_msg_ur = ADF_4XXX_ADMINMSGUR_OFFSET;
	admin_csrs_info->admin_msg_lr = ADF_4XXX_ADMINMSGLR_OFFSET;
}

static void adf_enable_error_correction(struct adf_accel_dev *accel_dev)
{
	struct adf_bar *misc_bar = &GET_BARS(accel_dev)[ADF_4XXX_PMISC_BAR];
	void __iomem *csr = misc_bar->virt_addr;
	struct pci_dev *pdev = accel_dev->accel_pci_dev.pci_dev;
	u32 pfcgciosfprir;

	/* Mask VFLRNOTIFY interrupt */
	adf_csr_fetch_and_or(csr, ADF_4XXX_ERRMSK3,
			     ADF_4XXX_ERRMSK3_VFLRNOTIFY);

	/* Parity Check Enable */
	pci_cfg_access_lock(pdev);
	pci_read_config_dword(pdev, ADF_4XXX_PFCGC_IOSF_PRIR,
			      &pfcgciosfprir);
	pfcgciosfprir |= ADF_4XXX_PFCGC_IOSF_PRIR_MASK;
	pci_write_config_dword(pdev, ADF_4XXX_PFCGC_IOSF_PRIR,
			       pfcgciosfprir);
	pci_cfg_access_unlock(pdev);
}

static void adf_enable_ints(struct adf_accel_dev *accel_dev)
{
	void __iomem *addr;

	addr = (&GET_BARS(accel_dev)[ADF_4XXX_PMISC_BAR])->virt_addr;

	/* Enable bundle interrupts */
	ADF_CSR_WR(addr, ADF_4XXX_SMIAPF_RP_X0_MASK_OFFSET, 0);
	ADF_CSR_WR(addr, ADF_4XXX_SMIAPF_RP_X1_MASK_OFFSET, 0);

	/*Enable misc interrupts*/
	ADF_CSR_WR(addr, ADF_4XXX_SMIAPF_MASK_OFFSET, 0);
}

static u32 adf_4xxx_get_hw_cap(struct adf_accel_dev *accel_dev)
{
	struct pci_dev *pdev = accel_dev->accel_pci_dev.pci_dev;
	u32 fusectl1;
	u32 capabilities;

	/* Read accelerator capabilities mask */
	pci_read_config_dword(pdev, ADF_4XXX_FUSECTL1_OFFSET,
			      &fusectl1);
	dev_info(&GET_DEV(accel_dev), "Fuses %x\n", fusectl1);
	capabilities =
		ICP_ACCEL_CAPABILITIES_CRYPTO_SYMMETRIC |
		ICP_ACCEL_CAPABILITIES_CRYPTO_ASYMMETRIC |
		ICP_ACCEL_CAPABILITIES_CIPHER |
		ICP_ACCEL_CAPABILITIES_AUTHENTICATION |
		ICP_ACCEL_CAPABILITIES_COMPRESSION |
		ICP_ACCEL_CAPABILITIES_LZ4_COMPRESSION |
		ICP_ACCEL_CAPABILITIES_LZ4S_COMPRESSION |
		ICP_ACCEL_CAPABILITIES_SHA3_EXT |
		ICP_ACCEL_CAPABILITIES_SM2 |
		ICP_ACCEL_CAPABILITIES_SM3 |
		ICP_ACCEL_CAPABILITIES_SM4 |
		ICP_ACCEL_CAPABILITIES_CHACHA_POLY |
		ICP_ACCEL_CAPABILITIES_AESGCM_SPC |
		ICP_ACCEL_CAPABILITIES_CNV_INTEGRITY64 |
		ICP_ACCEL_CAPABILITIES_AES_V2 |
		ICP_ACCEL_CAPABILITIES_RL;

	if (fusectl1 & ICP_ACCEL_4XXX_MASK_CIPHER_SLICE) {
		capabilities &= ~ICP_ACCEL_CAPABILITIES_CRYPTO_SYMMETRIC;
		capabilities &= ~ICP_ACCEL_CAPABILITIES_CIPHER;
	}
	if (fusectl1 & ICP_ACCEL_4XXX_MASK_AUTH_SLICE)
		capabilities &= ~ICP_ACCEL_CAPABILITIES_AUTHENTICATION;
	if (fusectl1 & ICP_ACCEL_4XXX_MASK_PKE_SLICE)
		capabilities &= ~ICP_ACCEL_CAPABILITIES_CRYPTO_ASYMMETRIC;
	if (fusectl1 & ICP_ACCEL_4XXX_MASK_COMPRESS_SLICE) {
		capabilities &= ~ICP_ACCEL_CAPABILITIES_COMPRESSION;
		capabilities &= ~ICP_ACCEL_CAPABILITIES_CNV_INTEGRITY64;
	}
	if (fusectl1 & ICP_ACCEL_4XXX_MASK_SMX_SLICE) {
		capabilities &= ~ICP_ACCEL_CAPABILITIES_SM3;
		capabilities &= ~ICP_ACCEL_CAPABILITIES_SM4;
	}
	return capabilities;
}

static u32 get_hb_clock(struct adf_hw_device_data *self)
{
	/*
	 * 4XXX uses KPT counter for HB
	 */
	return ADF_4XXX_KPT_COUNTER_FREQ;
}

static u32 get_ae_clock(struct adf_hw_device_data *self)
{
	/*
	 * Clock update interval is <16> ticks for qat_4xxx.
	 */
	return self->clock_frequency / 16;
}

static int measure_clock(struct adf_accel_dev *accel_dev)
{
	u32 frequency;
	int ret = 0;

	ret = adf_dev_measure_clock(accel_dev, &frequency,
				    ADF_4XXX_MIN_AE_FREQ,
				    ADF_4XXX_MAX_AE_FREQ);
	if (ret)
		return ret;

	accel_dev->hw_device->clock_frequency = frequency;
	return 0;
}

static int adf_4xxx_configure_accel_units(struct adf_accel_dev *accel_dev)
{
	char key[ADF_CFG_MAX_KEY_LEN_IN_BYTES] = {0};
	char val_str[ADF_CFG_MAX_VAL_LEN_IN_BYTES] = {0};

	if (adf_cfg_section_add(accel_dev, ADF_GENERAL_SEC))
		goto err;

	snprintf(key, sizeof(key), ADF_SERVICES_ENABLED);
	snprintf(val_str, sizeof(val_str), ADF_CFG_ASYM
		    ADF_SERVICES_SEPARATOR ADF_CFG_SYM);

	if (adf_cfg_add_key_value_param(accel_dev, ADF_GENERAL_SEC,
					key, (void *)val_str, ADF_STR))
		goto err;

	return 0;
err:
	dev_err(&GET_DEV(accel_dev), "Failed to configure accel units\n");
	return -EINVAL;
}

#ifdef QAT_UIO
static void adf_set_asym_rings_mask(struct adf_accel_dev *accel_dev)
{
	accel_dev->hw_device->asym_rings_mask = ADF_4XXX_DEF_ASYM_MASK;
}
#endif

static int get_ring_to_svc_map(struct adf_accel_dev *accel_dev,
			       u16 *ring_to_svc_map)
{
	char key[ADF_CFG_MAX_KEY_LEN_IN_BYTES];
	char val[ADF_CFG_MAX_KEY_LEN_IN_BYTES];
	u32 i = 0;

	*ring_to_svc_map = 0;
	/* Get the services enabled by user */
	snprintf(key, sizeof(key), ADF_SERVICES_ENABLED);
	if (adf_cfg_get_param_value(accel_dev, ADF_GENERAL_SEC, key, val))
		return -EFAULT;

	for (i = 0; i < ARRAY_SIZE(adf_4xxx_svcs); i++) {
		if (!strncmp(val, adf_4xxx_svcs[i].svcs_enabled,
				ADF_CFG_MAX_KEY_LEN_IN_BYTES)) {
			*ring_to_svc_map = adf_4xxx_svcs[i].rng_to_svc_msk;
			return 0;
		}
	}

	dev_err(&GET_DEV(accel_dev), "Invalid services enabled: %s\n", val);
	return -EFAULT;
}

static u32 get_num_accel_units(struct adf_hw_device_data *self)
{
	return ADF_4XXX_MAX_ACCELUNITS;
}

static void get_accel_unit(struct adf_hw_device_data *self,
			   struct adf_accel_unit **accel_unit)
{
	memcpy(*accel_unit, adf_4xxx_au_a_ae, sizeof(adf_4xxx_au_a_ae));
}

static void adf_exit_accel_unit_services(struct adf_accel_dev *accel_dev)
{
	if (accel_dev->au_info) {
		kfree(accel_dev->au_info->au);
		accel_dev->au_info->au = NULL;
		kfree(accel_dev->au_info);
		accel_dev->au_info = NULL;
	}
}

static int get_accel_unit_config(struct adf_accel_dev *accel_dev,
				 u8 *num_sym_au,
				 u8 *num_dc_au,
				 u8 *num_asym_au)
{
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	char key[ADF_CFG_MAX_KEY_LEN_IN_BYTES];
	char val[ADF_CFG_MAX_VAL_LEN_IN_BYTES];
	u32 num_au = hw_data->get_num_accel_units(hw_data);
	/* One AU will be allocated by default if a service enabled */
	u32 alloc_au = 1;
	/* There's always one AU that is used for Admin AE */
	u32 service_mask = ADF_ACCEL_ADMIN;
	char *token, *cur_str;
	u32 disabled_caps = 0;

	/* Get the services enabled by user */
	snprintf(key, sizeof(key), ADF_SERVICES_ENABLED);
	if (adf_cfg_get_param_value(accel_dev, ADF_GENERAL_SEC, key, val))
		return -EFAULT;
	cur_str = val;
	token = strsep(&cur_str, ADF_SERVICES_SEPARATOR);
	while (token) {
		if (!strncmp(token, ADF_CFG_SYM, strlen(ADF_CFG_SYM)))
			service_mask |= ADF_ACCEL_CRYPTO;
		if (!strncmp(token, ADF_CFG_ASYM, strlen(ADF_CFG_ASYM)))
			service_mask |= ADF_ACCEL_ASYM;

		/* cy means both asym & crypto should be enabled
		 * Hardware resources allocation check will be done later
		 */
		if (!strncmp(token, ADF_CFG_CY, strlen(ADF_CFG_CY)))
			service_mask |= ADF_ACCEL_ASYM | ADF_ACCEL_CRYPTO;
		if (!strncmp(token, ADF_SERVICE_DC, strlen(ADF_SERVICE_DC)))
			service_mask |= ADF_ACCEL_COMPRESSION;

		token = strsep(&cur_str, ADF_SERVICES_SEPARATOR);
	}

	/* Ensure the user won't enable more services than it can support */
	if (hweight32(service_mask) > num_au) {
		dev_err(&GET_DEV(accel_dev), "Can't enable more services than ");
		dev_err(&GET_DEV(accel_dev), "%d!\n", num_au);
		return -EFAULT;
	} else if (hweight32(service_mask) == 2) {
		/* Due to limitation, besides AU for Admin AE
		 * only 2 more AUs can be allocated
		 */
		alloc_au = 2;
	}

	if (service_mask & ADF_ACCEL_CRYPTO)
		*num_sym_au = alloc_au;
	if (service_mask & ADF_ACCEL_ASYM)
		*num_asym_au = alloc_au;
	if (service_mask & ADF_ACCEL_COMPRESSION)
		*num_dc_au = alloc_au;

	/*update capability*/
	if (!*num_sym_au || !(service_mask & ADF_ACCEL_CRYPTO)) {
		disabled_caps = ICP_ACCEL_CAPABILITIES_CRYPTO_SYMMETRIC
			| ICP_ACCEL_CAPABILITIES_CIPHER
			| ICP_ACCEL_CAPABILITIES_SHA3_EXT
			| ICP_ACCEL_CAPABILITIES_SM3
			| ICP_ACCEL_CAPABILITIES_SM4
			| ICP_ACCEL_CAPABILITIES_CHACHA_POLY
			| ICP_ACCEL_CAPABILITIES_AESGCM_SPC
			| ICP_ACCEL_CAPABILITIES_AES_V2;
	}
	if (!*num_asym_au || !(service_mask & ADF_ACCEL_ASYM)) {
		disabled_caps |= ICP_ACCEL_CAPABILITIES_CRYPTO_ASYMMETRIC
			| ICP_ACCEL_CAPABILITIES_AUTHENTICATION;
	}
	if (!*num_dc_au || !(service_mask & ADF_ACCEL_COMPRESSION)) {
		disabled_caps |= ICP_ACCEL_CAPABILITIES_COMPRESSION
		| ICP_ACCEL_CAPABILITIES_LZ4_COMPRESSION
		| ICP_ACCEL_CAPABILITIES_LZ4S_COMPRESSION
		| ICP_ACCEL_CAPABILITIES_CNV_INTEGRITY64;
		accel_dev->hw_device->extended_dc_capabilities = 0;
	}
	accel_dev->hw_device->accel_capabilities_mask =
		adf_4xxx_get_hw_cap(accel_dev) & ~disabled_caps;

	hw_data->service_mask = service_mask;
	hw_data->service_to_load_mask = service_mask;

	return 0;
}

static int adf_init_accel_unit_services(struct adf_accel_dev *accel_dev)
{
	u8 num_sym_au = 0, num_dc_au = 0, num_asym_au = 0;
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	u32 num_au = hw_data->get_num_accel_units(hw_data);
	u32 au_size = num_au * sizeof(struct adf_accel_unit);
	u8 i;

	if (get_accel_unit_config(accel_dev, &num_sym_au, &num_dc_au,
				    &num_asym_au))
		return -EFAULT;

	accel_dev->au_info = kzalloc(sizeof(*accel_dev->au_info), GFP_KERNEL);
	if (!accel_dev->au_info)
		return -ENOMEM;

	accel_dev->au_info->au = kzalloc(au_size, GFP_KERNEL);
	if (!accel_dev->au_info->au) {
		kfree(accel_dev->au_info);
		accel_dev->au_info = NULL;
		return -ENOMEM;
	}

	accel_dev->au_info->num_cy_au = num_sym_au;
	accel_dev->au_info->num_dc_au = num_dc_au;
	accel_dev->au_info->num_asym_au = num_asym_au;

	get_accel_unit(hw_data, &accel_dev->au_info->au);

	/* Enable ASYM accel units */
	for (i = 0; i < num_au && num_asym_au > 0; i++) {
		if (accel_dev->au_info->au[i].services ==
				ADF_ACCEL_SERVICE_NULL) {
			accel_dev->au_info->au[i].services = ADF_ACCEL_ASYM;
			num_asym_au--;
		}
	}
	/* Enable SYM accel units */
	for (i = 0; i < num_au && num_sym_au > 0; i++) {
		if (accel_dev->au_info->au[i].services ==
				ADF_ACCEL_SERVICE_NULL) {
			accel_dev->au_info->au[i].services = ADF_ACCEL_CRYPTO;
			num_sym_au--;
		}
	}
	/* Enable compression accel units */
	for (i = 0; i < num_au && num_dc_au > 0; i++) {
		if (accel_dev->au_info->au[i].services ==
				ADF_ACCEL_SERVICE_NULL) {
			accel_dev->au_info->au[i].services =
				ADF_ACCEL_COMPRESSION;
			num_dc_au--;
		}
	}
	accel_dev->au_info->dc_ae_msk |=
		hw_data->get_obj_cfg_ae_mask(accel_dev, ADF_ACCEL_COMPRESSION);

	return 0;
}

static int adf_init_accel_units(struct adf_accel_dev *accel_dev)
{
	return adf_init_accel_unit_services(accel_dev);
}

static void adf_exit_accel_units(struct adf_accel_dev *accel_dev)
{
	/* reset the AU service */
	adf_exit_accel_unit_services(accel_dev);
}

static const char *get_obj_name_4xxx(struct adf_accel_dev *accel_dev,
				     enum adf_accel_unit_services service)
{
	switch (service) {
	case ADF_ACCEL_ASYM:
		return ADF_4XXX_ASYM_OBJ;
	case ADF_ACCEL_CRYPTO:
		return ADF_4XXX_SYM_OBJ;
	case ADF_ACCEL_COMPRESSION:
		return ADF_4XXX_DC_OBJ;
	case ADF_ACCEL_ADMIN:
		return ADF_4XXX_ADMIN_OBJ;
	default:
		return NULL;
	}
}


static uint32_t get_objs_num(struct adf_accel_dev *accel_dev)
{
	return ADF_4XXX_MAX_OBJ;
}

static uint32_t get_obj_cfg_ae_mask(struct adf_accel_dev *accel_dev,
				    enum adf_accel_unit_services service)
{
	u32 ae_mask = 0;
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	u32 num_au = hw_data->get_num_accel_units(hw_data);
	struct adf_accel_unit *accel_unit = accel_dev->au_info->au;
	u32 i = 0;

	if (service == ADF_ACCEL_SERVICE_NULL)
		return 0;

	for (i = 0; i < num_au; i++) {
		if (accel_unit[i].services == service)
			ae_mask |= accel_unit[i].ae_mask;
	}

	return ae_mask;
}

static enum adf_accel_unit_services adf_4xxx_get_service_type
			(struct adf_accel_dev *accel_dev, s32 obj_num)
{
	struct adf_accel_unit *accel_unit;
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	u8 num_au = hw_data->get_num_accel_units(hw_data);
	int i;

	if (!hw_data->service_to_load_mask)
		return ADF_ACCEL_SERVICE_NULL;

	if (accel_dev->au_info && accel_dev->au_info->au)
		accel_unit = accel_dev->au_info->au;
	else
		return ADF_ACCEL_SERVICE_NULL;

	for (i = num_au - 2; i >= 0; i--) {
		if (hw_data->service_to_load_mask &
			accel_unit[i].services) {
			hw_data->service_to_load_mask &=
				~accel_unit[i].services;
			return accel_unit[i].services;
		}
	}


	/* admin AE should be loaded last */
	if (hw_data->service_to_load_mask & accel_unit[num_au - 1].services) {
		hw_data->service_to_load_mask &=
			~accel_unit[num_au - 1].services;
		return accel_unit[num_au - 1].services;
	}

	return ADF_ACCEL_SERVICE_NULL;
}

static int
check_ae_exist(unsigned char ae, unsigned int relmask, unsigned char cppmask)
{
	unsigned char ae_per_cluster[MAX_CLUSTER] = {9, 9, 0, 0};
	unsigned int cluster;

	if (cppmask != 0) {
		for (cluster = 0; cluster < MAX_CLUSTER; cluster++) {
			if (ae < ae_per_cluster[cluster])
				break;
			ae -= ae_per_cluster[cluster];
		}
		if ((cppmask & (1 << cluster)) && (relmask & (1 << ae)))
			return 1;
		else
			return 0;
	} else {
		return relmask & (1 << ae);
	}
}

static void get_ring_svc_map_data(int ring_pair_index, u16 ring_to_svc_map,
				  u8 *serv_type, int *ring_index,
				  int *num_rings_per_srv, int bundle_num)
{
	*serv_type = GET_SRV_TYPE(ring_to_svc_map, bundle_num %
			ADF_CFG_NUM_SERVICES);
	*ring_index = 0;
	*num_rings_per_srv = ADF_4XXX_NUM_RINGS_PER_BANK / 2;
}

static int adf_get_fw_status(struct adf_accel_dev *accel_dev,
			     u8 *major, u8 *minor, u8 *patch)
{
	struct icp_qat_fw_init_admin_req req;
	struct icp_qat_fw_init_admin_resp resp;
	u32 ae_mask = 1;

	memset(&req, 0, sizeof(req));
	memset(&resp, 0, sizeof(resp));
	req.cmd_id = ICP_QAT_FW_STATUS_GET;

	if (adf_send_admin(accel_dev, &req, &resp, ae_mask))
		return -EFAULT;

	*major = resp.version_major_num;
	*minor = resp.version_minor_num;
	*patch = resp.version_patch_num;

	return 0;
}

static int adf_get_dc_extcapabilities(struct adf_accel_dev *accel_dev,
				      u32 *capabilities)
{
	struct icp_qat_fw_init_admin_req req;
	struct icp_qat_fw_init_admin_resp resp;
	u8 i;
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	u8 num_au = hw_data->get_num_accel_units(hw_data);
	u32 first_dc_ae = 0;

	for (i = 0; i < num_au; i++) {
		if (accel_dev->au_info->au[i].services &
		    ADF_ACCEL_COMPRESSION) {
			first_dc_ae = accel_dev->au_info->au[i].ae_mask;
			first_dc_ae &= ~(first_dc_ae - 1);
		}
	}

	memset(&req, 0, sizeof(req));
	memset(&resp, 0, sizeof(resp));
	req.cmd_id = ICP_QAT_FW_COMP_CAPABILITY_GET;

	if (likely(first_dc_ae)) {
		if (adf_send_admin(accel_dev, &req, &resp, first_dc_ae) ||
		    resp.status) {
			*capabilities = 0;
			return -EFAULT;
		}

		*capabilities = resp.extended_features;
	}

	return 0;
}

static void adf_4xxx_print_err_registers(struct adf_accel_dev *accel_dev)
{
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	struct adf_bar *misc_bar =
		&GET_BARS(accel_dev)[hw_data->get_misc_bar_id(hw_data)];
	void __iomem *csr = misc_bar->virt_addr;
	size_t i;
	u32 val;

	for (i = 0; i < ARRAY_SIZE(adf_err_regs); ++i) {
		val = ADF_CSR_RD(csr, adf_err_regs[i].offs);

		adf_print_reg(accel_dev, adf_err_regs[i].name, 0, val);
	}

	adf_print_flush(accel_dev);
}

static int adf_4xxx_send_admin_init(struct adf_accel_dev *accel_dev)
{
	int ret = 0;
	struct icp_qat_fw_init_admin_req req;
	struct icp_qat_fw_init_admin_resp resp;
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	u32 ae_mask = hw_data->ae_mask;
	u32 admin_ae_mask = hw_data->admin_ae_mask;
	u8 num_au = hw_data->get_num_accel_units(hw_data);
	u8 i;
	u32 dc_capabilities = 0;

	for (i = 0; i < num_au; i++) {
		if (accel_dev->au_info->au[i].services ==
			ADF_ACCEL_SERVICE_NULL)
			ae_mask &= ~accel_dev->au_info->au[i].ae_mask;

		if (accel_dev->au_info->au[i].services !=
			ADF_ACCEL_ADMIN)
			admin_ae_mask &= ~accel_dev->au_info->au[i].ae_mask;
	}

	if (!accel_dev->admin) {
		dev_err(&GET_DEV(accel_dev), "adf_admin not available\n");
		return -EFAULT;
	}

	memset(&req, 0, sizeof(req));
	memset(&resp, 0, sizeof(resp));

	req.cmd_id = ICP_QAT_FW_CONSTANTS_CFG;
	req.init_cfg_sz = ADF_CONST_TABLE_SIZE;
	req.init_cfg_ptr = accel_dev->admin->const_tbl_addr;
	if (adf_send_admin(accel_dev, &req, &resp, admin_ae_mask)) {
		dev_err(&GET_DEV(accel_dev),
			"Error sending constants config message\n");
		return -EFAULT;
	}

	memset(&req, 0, sizeof(req));
	memset(&resp, 0, sizeof(resp));
	req.cmd_id = ICP_QAT_FW_INIT_ME;
	if (accel_dev->at_enabled)
		req.fw_flags |= ICP_QAT_FW_INIT_AE_AT_ENABLE_FLAG;

	if (adf_send_admin(accel_dev, &req, &resp, ae_mask)) {
		dev_err(&GET_DEV(accel_dev),
			"Error sending init message\n");
		return -EFAULT;
	}

	memset(&req, 0, sizeof(req));
	memset(&resp, 0, sizeof(resp));
	req.cmd_id = ICP_QAT_FW_HEARTBEAT_TIMER_SET;
	req.init_cfg_ptr = accel_dev->admin->phy_hb_addr;
	if (adf_get_hb_timer(accel_dev, &req.heartbeat_ticks))
		return -EINVAL;

	if (adf_send_admin(accel_dev, &req, &resp, ae_mask))
		dev_err(&GET_DEV(accel_dev), "Heartbeat is not supported\n");

	ret = adf_get_dc_extcapabilities(accel_dev, &dc_capabilities);
	if (unlikely(ret)) {
		dev_err(&GET_DEV(accel_dev),
			"Could not get FW ext. capabilities\n");
	}

	accel_dev->hw_device->extended_dc_capabilities =
		dc_capabilities;

	adf_get_fw_status(accel_dev,
			  &accel_dev->fw_versions.fw_version_major,
			  &accel_dev->fw_versions.fw_version_minor,
			  &accel_dev->fw_versions.fw_version_patch);

	dev_info(&GET_DEV(accel_dev), "FW version: %d.%d.%d\n",
		 accel_dev->fw_versions.fw_version_major,
		 accel_dev->fw_versions.fw_version_minor,
		 accel_dev->fw_versions.fw_version_patch);

	return ret;
}

static int adf_4xxx_ring_pair_reset(struct adf_accel_dev *accel_dev,
				    u32 bank_number)
{
	struct adf_hw_device_data *hw_data;
	struct adf_bar *etr_bar;
	void __iomem *csr;

	hw_data = accel_dev->hw_device;
	if (bank_number >= hw_data->num_banks)
		return -EINVAL;

	etr_bar = &GET_BARS(accel_dev)[hw_data->get_etr_bar_id(hw_data)];
	csr = etr_bar->virt_addr;

	dev_dbg(&GET_DEV(accel_dev), "ring pair reset for bank:%d\n",
		bank_number);
	if (gen4_ring_pair_reset(csr, bank_number)) {
		dev_err(&GET_DEV(accel_dev),
			"ring pair reset failure (timeout)\n");
		return -EFAULT;
	}

	dev_dbg(&GET_DEV(accel_dev), "ring pair reset successfully\n");
	return 0;
}

static int adf_4xxx_ring_pair_drain(struct adf_accel_dev *accel_dev,
				    u32 bank_number, int timeout_ms)
{
	struct adf_hw_device_data *hw_data;
	struct adf_bar *etr_bar;
	void __iomem *csr;

	hw_data = accel_dev->hw_device;
	if (bank_number >= hw_data->num_banks)
		return -EINVAL;

	etr_bar = &GET_BARS(accel_dev)[hw_data->get_etr_bar_id(hw_data)];
	csr = etr_bar->virt_addr;

	dev_dbg(&GET_DEV(accel_dev), "ring pair drain for bank:%d\n",
		bank_number);
	if (gen4_ring_pair_drain(csr, bank_number, timeout_ms)) {
		dev_err(&GET_DEV(accel_dev),
			"ring pair drain failure (timeout)\n");
		return -EFAULT;
	}

	dev_dbg(&GET_DEV(accel_dev), "ring pair drain successfully\n");
	return 0;
}

#ifdef QAT_ESXI
static int adf_4xxx_bank_state_save(struct adf_accel_dev *accel_dev,
				    u32 bank_number, struct bank_state *state)
{
	struct adf_hw_device_data *hw_data;
	struct adf_bar *etr_bar;
	void __iomem *csr;

	hw_data = accel_dev->hw_device;
	if (bank_number >= hw_data->num_banks || !state)
		return -EINVAL;

	etr_bar = &GET_BARS(accel_dev)[hw_data->get_etr_bar_id(hw_data)];
	csr = etr_bar->virt_addr;

	dev_dbg(&GET_DEV(accel_dev), "Saving state of bank: %d\n", bank_number);

	gen4_bank_state_save(csr, bank_number, state,
			     hw_data->num_rings_per_bank);

	dev_dbg(&GET_DEV(accel_dev), "Bank %d state saved successfully\n",
		bank_number);
	return 0;
}

static int adf_4xxx_bank_state_restore(struct adf_accel_dev *accel_dev,
				       u32 bank_number,
				       struct bank_state *state)
{
	struct adf_hw_device_data *hw_data;
	struct adf_bar *etr_bar;
	void __iomem *csr;

	hw_data = accel_dev->hw_device;
	if (bank_number >= hw_data->num_banks)
		return -EINVAL;

	etr_bar = &GET_BARS(accel_dev)[hw_data->get_etr_bar_id(hw_data)];
	csr = etr_bar->virt_addr;

	dev_dbg(&GET_DEV(accel_dev), "Restoring state of bank: %d\n",
		bank_number);

	if (gen4_bank_state_restore(csr, bank_number, state,
				    hw_data->num_rings_per_bank,
				    hw_data->tx_rx_gap)) {
		dev_err(&GET_DEV(accel_dev),
			"Unable to restore state of bank %d\n", bank_number);
		return -EFAULT;
	}

	dev_dbg(&GET_DEV(accel_dev), "Bank %d state restored successfully\n",
		bank_number);
	return 0;
}

#endif /* QAT_ESXI */
static void adf_4xxx_config_ring_irq(struct adf_accel_dev *accel_dev,
				     u32 bank_number, u16 ring_mask)
{
	struct adf_hw_device_data *hw_data;
	struct adf_etr_data *etr_data;
	struct adf_etr_bank_data *bank;
	struct adf_hw_csr_ops *csr_ops;
	u32 enable_int_col_mask;

	hw_data = accel_dev->hw_device;
	etr_data = accel_dev->transport;
	bank = &etr_data->banks[bank_number];
	csr_ops = &hw_data->csr_info.csr_ops;
	enable_int_col_mask = csr_ops->get_int_col_ctl_enable_mask();
	if (!(ring_mask & hw_data->tx_rings_mask)) {
		csr_ops->write_csr_int_srcsel(bank->csr_addr, bank->bank_number,
					      0, csr_ops->get_src_sel_mask());
		csr_ops->write_csr_int_col_ctl(bank->csr_addr,
					       bank->bank_number,
					       bank->irq_coalesc_timer |
					       enable_int_col_mask);
	}
}

void adf_init_hw_data_4xxx(struct adf_hw_device_data *hw_data, u32 id)
{
	hw_data->dev_class = &adf_4xxx_class;
	hw_data->instance_id = adf_4xxx_class.instances++;
	hw_data->num_banks = ADF_4XXX_ETR_MAX_BANKS;
	hw_data->num_rings_per_bank = ADF_4XXX_NUM_RINGS_PER_BANK;
	hw_data->num_banks_per_vf = ADF_4XXX_NUM_BANKS_PER_VF;
	hw_data->num_accel = ADF_4XXX_MAX_ACCELERATORS;
	hw_data->num_engines = ADF_4XXX_MAX_ACCELENGINES;
	hw_data->check_ae_exist = &check_ae_exist;
	hw_data->num_logical_accel = 1;
	hw_data->tx_rx_gap = ADF_4XXX_RX_RINGS_OFFSET;
	hw_data->tx_rings_mask = ADF_4XXX_TX_RINGS_MASK;
	hw_data->alloc_irq = adf_isr_resource_alloc;
	hw_data->free_irq = adf_isr_resource_free;
	hw_data->enable_error_correction = adf_enable_error_correction;
	hw_data->print_err_registers = adf_4xxx_print_err_registers;
	hw_data->init_ras = adf_4xxx_init_ras;
	hw_data->exit_ras = adf_4xxx_exit_ras;
	hw_data->ras_interrupts = adf_4xxx_ras_interrupts;
	hw_data->get_accel_mask = get_accel_mask;
	hw_data->get_ae_mask = get_ae_mask;
	hw_data->get_num_accels = get_num_accels;
	hw_data->get_num_aes = get_num_aes;
	hw_data->get_sram_bar_id = get_sram_bar_id;
	hw_data->get_etr_bar_id = get_etr_bar_id;
	hw_data->get_misc_bar_id = get_misc_bar_id;
	hw_data->get_pf2vf_offset = get_pf2vm_offset;
	hw_data->get_vf2pf_offset = get_vm2pf_offset;
	hw_data->pfvf_type_shift = ADF_PFVF_2X_MSGTYPE_SHIFT;
	hw_data->pfvf_type_mask = ADF_PFVF_2X_MSGTYPE_MASK;
	hw_data->pfvf_data_shift = ADF_PFVF_2X_MSGDATA_SHIFT;
	hw_data->pfvf_data_mask = ADF_PFVF_2X_MSGDATA_MASK;
	hw_data->get_arb_info = get_arb_info;
	hw_data->get_admin_info = get_admin_info;
	hw_data->notify_and_wait_ethernet = NULL;
	hw_data->get_eth_doorbell_msg = NULL;
	hw_data->get_clock_speed = get_clock_speed;
	hw_data->clock_frequency = ADF_4XXX_AE_FREQ;
	hw_data->get_sku = get_sku;
	hw_data->heartbeat_ctr_num = ADF_NUM_HB_CNT_PER_AE;
#if defined(CONFIG_PCI_IOV)
	hw_data->mask_misc_irq = mask_misc_irq;
	hw_data->process_and_get_vf2pf_int = process_and_get_vf2pf_int;
	hw_data->enable_vf2pf_interrupts = enable_vm2pf_interrupts;
	hw_data->disable_vf2pf_interrupts = disable_vm2pf_interrupts;
	hw_data->check_arbitrary_numvfs = check_arbitrary_numvfs;
#endif
	switch (id) {
	case ADF_401XX_PCI_DEVICE_ID:
		hw_data->fw_name = ADF_4XXX_FW;
		hw_data->fw_mmp_name = ADF_4XXX_MMP;
		hw_data->asym_ae_active_thd_mask = DEFAULT_401XX_ASYM_AE_MASK;
		break;

	default:
		hw_data->fw_name = ADF_4XXX_FW;
		hw_data->fw_mmp_name = ADF_4XXX_MMP;
		hw_data->asym_ae_active_thd_mask = DEFAULT_4XXX_ASYM_AE_MASK;
	}
	hw_data->init_admin_comms = adf_init_admin_comms;
	hw_data->exit_admin_comms = adf_exit_admin_comms;
	hw_data->disable_iov = adf_disable_sriov;
	hw_data->send_admin_init = adf_4xxx_send_admin_init;
	hw_data->init_arb = adf_init_arb;
	hw_data->exit_arb = adf_exit_arb;
	hw_data->get_arb_mapping = adf_get_arbiter_mapping;
	hw_data->enable_ints = adf_enable_ints;
	hw_data->set_ssm_wdtimer = adf_4xxx_set_ssm_wdtimer;
	hw_data->set_msix_rttable = adf_4xxx_set_msix_default_rttable;
	hw_data->enable_vf2pf_comms = adf_pf_enable_vf2pf_comms;
	hw_data->disable_vf2pf_comms = adf_pf_disable_vf2pf_comms;
	hw_data->reset_device = adf_reset_flr;
	hw_data->restore_device = adf_dev_restore;
	hw_data->min_iov_compat_ver = ADF_PFVF_COMPATIBILITY_VERSION;
	hw_data->ring_to_svc_map = ADF_4XXX_DEFAULT_RING_TO_SRV_MAP;
	hw_data->get_ring_svc_map_data = get_ring_svc_map_data;
	hw_data->admin_ae_mask = ADF_4XXX_ADMIN_AE_MASK;
	hw_data->init_accel_units = adf_init_accel_units;
	hw_data->exit_accel_units = adf_exit_accel_units;
	hw_data->get_num_accel_units = get_num_accel_units;
	hw_data->get_objs_num = get_objs_num;
	switch (id) {
	default:
		hw_data->get_obj_name = get_obj_name_4xxx;
	}
	hw_data->get_obj_cfg_ae_mask = get_obj_cfg_ae_mask;
	hw_data->get_service_type = adf_4xxx_get_service_type;
	hw_data->init_pm = adf_4xxx_init_pm;
	hw_data->switch_drv_active = adf_4xxx_switch_drv_active;
	hw_data->exit_pm = adf_4xxx_exit_pm;
	hw_data->update_qat_pm_state = adf_4xxx_set_pm_drv_active;
	hw_data->check_pm_interrupts = adf_4xxx_pm_check_interrupts;
	hw_data->configure_accel_units = adf_4xxx_configure_accel_units;
	hw_data->get_ring_to_svc_map = get_ring_to_svc_map;
	hw_data->query_storage_cap = 0;
	hw_data->init_adis = adf_init_adis;
	hw_data->exit_adis = adf_exit_adis;
	hw_data->init_kpt = adf_4xxx_init_kpt;
	hw_data->config_kpt = adf_4xxx_config_kpt;
#ifdef QAT_UIO
	hw_data->config_device = adf_config_device;
	hw_data->set_asym_rings_mask = adf_set_asym_rings_mask;
#endif
#ifdef QAT_ESXI
	hw_data->bank_state_save = adf_4xxx_bank_state_save;
	hw_data->bank_state_restore = adf_4xxx_bank_state_restore;
#endif
	hw_data->config_bank_pasid = adf_pasid_config_bank;
	hw_data->init_rl_v2 = adf_rl_v2_init;
	hw_data->exit_rl_v2 = adf_rl_v2_exit;
	hw_data->telemetry_init = adf_4xxx_init_tl;
#ifdef QAT_ESXI
	hw_data->telemetry_control = adf_4xxx_control_tl;
	hw_data->telemetry_get = adf_4xxx_get_tl;
#endif /* QAT_ESXI */
	hw_data->telemetry_exit = adf_4xxx_exit_tl;
	hw_data->ring_pair_reset = adf_4xxx_ring_pair_reset;
	hw_data->ring_pair_drain = adf_4xxx_ring_pair_drain;
	hw_data->config_ring_irq = adf_4xxx_config_ring_irq;
	hw_data->extended_dc_capabilities = 0;
	hw_data->get_hb_clock = get_hb_clock;
	hw_data->get_ae_clock = get_ae_clock;
	hw_data->measure_clock = measure_clock;
	hw_data->get_accel_cap = adf_4xxx_get_hw_cap;
	hw_data->get_heartbeat_status = adf_get_heartbeat_status;
	hw_data->int_timer_init = adf_int_timer_init;
	hw_data->int_timer_exit = adf_int_timer_exit;
	hw_data->pre_reset = adf_dev_pre_reset;
	hw_data->post_reset = adf_dev_post_reset;
	hw_data->disable_arb = adf_disable_arb;
#ifdef QAT_HB_FAIL_SIM
	hw_data->adf_disable_ae_wrk_thds = adf_disable_arbiter;
	hw_data->adf_set_max_hb_timer = adf_gen4_set_max_hb_timer;
#endif
	/* mmp fw will be loaded  by default */
	hw_data->load_mmp_always = true;
	gen4_init_hw_csr_info(&hw_data->csr_info);
	gen4_init_adi_ops(&hw_data->adi_ops);
	/* KAT test enabled by default for qat_4xxx */
	hw_data->fw_integr_selftest = true;
}

void adf_clean_hw_data_4xxx(struct adf_hw_device_data *hw_data)
{
	hw_data->dev_class->instances--;
}
