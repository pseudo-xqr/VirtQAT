/**
 ******************************************************************************
 * @file  dc_sample_main.c
 *
 *****************************************************************************/

 #include "cpa_sample_utils.h"
 #include "icp_sal_user.h"
 
 extern CpaStatus dcStatelessSample(void);
 
 int gDebugParam = 1;
 
 int main(int argc, const char **argv)
 {
     CpaStatus stat = CPA_STATUS_SUCCESS;
     const char* procName = argv[1];
 
     if (argc > 2)
     {
         gDebugParam = atoi(argv[2]);
     }
 
     PRINT_DBG("Starting Stateless Compression Sample Code App ...\n");
 
     stat = qaeMemInit();
     if (CPA_STATUS_SUCCESS != stat)
     {
         PRINT_ERR("Failed to initialize memory driver\n");
         return (int)stat;
     }
 
    //  stat = icp_sal_userStartMultiProcess("SSL", CPA_FALSE);
    stat = icp_sal_userStart(procName);
     if (CPA_STATUS_SUCCESS != stat)
     {
         PRINT_ERR("Failed to start user process SSL\n");
         qaeMemDestroy();
         return (int)stat;
     }
 
     stat = dcStatelessSample();
     if (CPA_STATUS_SUCCESS != stat)
     {
         PRINT_ERR("\nStateless Compression Sample Code App failed\n");
     }
     else
     {
         PRINT_DBG("\nStateless Compression Sample Code App finished\n");
     }
 
     icp_sal_userStop();
 
     qaeMemDestroy();
 
     return (int)stat;
 }
 