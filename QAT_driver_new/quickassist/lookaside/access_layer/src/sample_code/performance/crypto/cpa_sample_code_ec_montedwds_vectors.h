/**
 *****************************************************************************
 *
 *
 *   BSD LICENSE
 * 
 *   Copyright(c) 2007-2023 Intel Corporation. All rights reserved.
 *   All rights reserved.
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 *  version: QAT20.L.1.2.30-00078
 *
 *
 ***************************************************************************/

/**
 *****************************************************************************
 * @file cpa_sample_code_eced_vectors.h
 *
 * @defgroup ecMontEdwdsThreads
 *
 * @ingroup ecMontEdwdsThreads
 *
 * @description
 *      This file contains structure declaration for test vectors
 *      and getEcedTestVectors function definition used in ECED test
 *
 *****************************************************************************/
#ifndef CPA_SAMPLE_CODE_ECMONTEDWDS_VECTORS_H
#define CPA_SAMPLE_CODE_ECMONTEDWDS_VECTORS_H

#include "cpa.h"
#include "cpa_types.h"
#include "cpa_sample_code_crypto_utils.h"

#if CY_API_VERSION_AT_LEAST(2, 3)
/**
 *****************************************************************************
 * @ingroup ecMontEdwdsThreads
 *
 * @description
 *      This structure contains data relating to test vectors used in ECED
 *      test.
 *
 ****************************************************************************/
typedef struct sample_ec_montedwds_vectors_s
{
    /* pointer to x vector */
    Cpa8U *x;
    /* x vector size */
    Cpa32U xSize;
    /* pointer to y vector */
    Cpa8U *y;
    /* y vector size */
    Cpa32U ySize;
    /* pointer to k vector */
    Cpa8U *k;
    /* k vector size */
    Cpa32U kSize;
    /* pointer to u vector - generated x */
    Cpa8U *u;
    /* u vector size */
    Cpa32U uSize;
    /* pointer to v vector - generated y */
    Cpa8U *v;
    /* v vector size */
    Cpa32U vSize;
    /* number of vectors in selected curve type */
    Cpa32U vectorsNum;
} sample_ec_montedwds_vectors_t;

/**
 *****************************************************************************
 * @ingroup ecMontEdwdsThreads
 *
 * @description
 *      This functions selects vectors used in ECED test
 *
 ****************************************************************************/
CpaStatus getEcMontEdwdsTestVectors(CpaBoolean generator,
                                    CpaCyEcMontEdwdsCurveType curveType,
                                    Cpa32U vector,
                                    sample_ec_montedwds_vectors_t *testVectors);

#endif /* CY_API_VERSION_AT_LEAST(2, 3) */
#endif
