/***************************************************************************
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

/****************************************************************************
 * @file  cpa_ec_montedwds_sample_user.c
 *
 * @description
 *     This file contains main function used in EDDSA sample.
 *
 ****************************************************************************/

#include "cpa_sample_utils.h"
#include "icp_sal_user.h"

#if CY_API_VERSION_AT_LEAST(2, 3)

extern CpaStatus ecMontEdwdsDsaSample(void);

int gDebugParam = 1;

int main(int argc, const char **argv)
{
    CpaStatus status = CPA_STATUS_SUCCESS;

    if (argc > 1)
        gDebugParam = atoi(argv[1]);

    PRINT_DBG("Starting EDDSA sample code ...\n");

    status = qaeMemInit();
    if (CPA_STATUS_SUCCESS != status)
    {
        PRINT_ERR("Failed to initialise memory driver\n");
        return (int)status;
    }

    status = icp_sal_userStartMultiProcess("SSL", CPA_FALSE);
    if (CPA_STATUS_SUCCESS != status)
    {
        PRINT_ERR("Failed to start user process SSL\n");
        qaeMemDestroy();
        return (int)status;
    }

    status = ecMontEdwdsDsaSample();
    if (CPA_STATUS_SUCCESS != status)
        PRINT_ERR("EDDSA sample code failed\n");
    else
        PRINT_DBG("EDDSA code finished\n");

    icp_sal_userStop();
    qaeMemDestroy();

    return (int)status;
}
#endif /* CY_API_VERSION_AT_LEAST(2, 3) */
