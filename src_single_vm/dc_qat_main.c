/**
 ******************************************************************************
 * @file  dc_sample_main.c
 *
 *****************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "cpa_sample_utils.h"
#include "icp_sal_user.h"

extern CpaStatus dcStatelessSample(Cpa32U vm_idx);

int gDebugParam = 1;

int main(int argc, const char **argv)
{
    CpaStatus stat = CPA_STATUS_SUCCESS;
    
    // const char* procName = argv[2];
    const char* vm_idx_str = argv[1];
    Cpa32U vm_idx = atoi(vm_idx_str);

    if (argc > 2)
    {
        gDebugParam = atoi(argv[2]);
    }

    // PRINT_DBG("Starting Stateless Compression Sample Code App ...\n");

    stat = qaeMemInit();
    if (CPA_STATUS_SUCCESS != stat)
    {
        PRINT_ERR("Failed to initialize memory driver\n");
        return (int)stat;
    }

    stat = icp_sal_userStartMultiProcess("SSL", CPA_TRUE);
    // stat = icp_sal_userStart("SSL");
    if (CPA_STATUS_SUCCESS != stat)
    {
        PRINT_ERR("Failed to start user process SSL\n");
        qaeMemDestroy();
        return (int)stat;
    }

    stat = dcStatelessSample(vm_idx);
    if (CPA_STATUS_SUCCESS != stat)
    {
        PRINT_ERR("Stateless Compression Sample Code App failed\n");
    }
    else
    {
        PRINT_DBG("Stateless Compression Sample Code App finished\n");
    }

    icp_sal_userStop();

    qaeMemDestroy();

    return (int)stat;
}
