/* SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only) */
/* Copyright(c) 2022 Intel Corporation */
#ifndef ADF_GEN4_RAS_H_
#define ADF_GEN4_RAS_H_

#define ADF_GEN4_MAX_ACCELERATORS 1
#define ADF_GEN4_IAINTSTATSSM(i)   ((i) * 0x4000 + 0x28)

/* Slice Hang handling related registers */
#define ADF_GEN4_SLICEHANGSTATUS_ATH_CPH (0x84)
#define ADF_GEN4_SLICEHANGSTATUS_CPR_XLT (0x88)
#define ADF_GEN4_SLICEHANGSTATUS_DCPR_UCS (0x90)
#define ADF_GEN4_SLICEHANGSTATUS_PKE (0x94)

#define ADF_GEN4_SLICEHANGSTATUS_ATH_CPH_OFFSET(accel) \
	(ADF_GEN4_SLICEHANGSTATUS_ATH_CPH + ((accel) * 0x4000))
#define ADF_GEN4_SLICEHANGSTATUS_CPR_XLT_OFFSET(accel) \
	(ADF_GEN4_SLICEHANGSTATUS_CPR_XLT + ((accel) * 0x4000))
#define ADF_GEN4_SLICEHANGSTATUS_DCPR_UCS_OFFSET(accel) \
	(ADF_GEN4_SLICEHANGSTATUS_DCPR_UCS + ((accel) * 0x4000))
#define ADF_GEN4_SLICEHANGSTATUS_PKE_OFFSET(accel) \
	(ADF_GEN4_SLICEHANGSTATUS_PKE + ((accel) * 0x4000))

#define ADF_GEN4_SLICE_HANG_ERROR_MASK BIT(3)

void adf_gen4_handle_slice_hang_error(struct adf_accel_dev *accel_dev,
				      u32 accel_num, void __iomem *csr);

#endif
