
/*
 * This is sample code that demonstrates usage of the data compression API,
 * and specifically using this API to statelessly compress an input buffer. It
 * will compress the data using deflate with dynamic huffman trees.
 */

 #include <pthread.h>
 #include <sched.h>
 #include <stdio.h>
 #include <time.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <stdatomic.h>
 
 #include "cpa.h"
 #include "cpa_dc.h"
 #include "cpa_sample_utils.h"
 
 extern int gDebugParam;
 pthread_barrier_t barrier;
 
 // #define SAMPLE_MAX_BUFF 1024
 #define SAMPLE_MAX_SIZE_MB 1.25
 #define SAMPLE_MAX_BUFF SAMPLE_MAX_SIZE_MB * 1024 * 1024
 #define NUM_LINES_PER_FILE 1000
 // #define CHUNK_SIZE_MB 2
 // #define CHUNK_SIZE CHUNK_SIZE_MB * 1024 * 1024  
 // #define SAMPLE_SIZE 512
 #define TIMEOUT_MS 5000 /* 5 seconds */
 #define SINGLE_INTER_BUFFLIST 1
 #define MAX_INSTANCES 4
 #define NUM_COMPL_PER_THREAD 10
 #define TOTAL_INSTANCES MAX_INSTANCES*NUM_COMPL_PER_THREAD
 
 typedef struct {
     CpaInstanceHandle *dcInstHandle;
     Cpa32U index;
     Cpa8U *buf_ptr;
     Cpa32U *interval;
     // CpaStatus *status;
 } qat_arg_t;
 
 typedef struct {
     unsigned long tid;
     struct timespec timestamp;
 } kv_entry_t;
 
 kv_entry_t start_time_table[TOTAL_INSTANCES];
 kv_entry_t end_time_table[TOTAL_INSTANCES];
 
 _Atomic Cpa32U start_time_table_idx = 0;
 _Atomic Cpa32U end_time_table_idx = 0;
 
 /*
 *****************************************************************************
 * Functions to operate timeStamp table 
 *****************************************************************************
 */
 Cpa32U startt_table_add_entry(kv_entry_t *table, unsigned long tid, struct timespec timestamp) {
     Cpa32U cur_idx = atomic_fetch_add(&start_time_table_idx, 1);
     table[cur_idx].tid = tid;
     table[cur_idx].timestamp = timestamp;
     return 0;
 }
 
 Cpa32U endt_table_add_entry(kv_entry_t *table, unsigned long tid, struct timespec timestamp) {
     Cpa32U cur_idx = atomic_fetch_add(&end_time_table_idx, 1);
     table[cur_idx].tid = tid;
     table[cur_idx].timestamp = timestamp;
     
     return 1;
 }
 int compare_tid(const void *thr_tid1, const void *thr_tid2) {
     const kv_entry_t *entry1 = (const kv_entry_t *)thr_tid1;
     const kv_entry_t *entry2 = (const kv_entry_t *)thr_tid2;
     unsigned long tid1 = entry1->tid;
     unsigned long tid2 = entry2->tid;
     if (tid1 < tid2) {
         return -1;
     } else {
         return 1;
     }
     return 0;
 }
 int compare_timestamp(const void *thr_time1, const void *thr_time2) {
     const kv_entry_t *entry1 = (const kv_entry_t *)thr_time1;
     const kv_entry_t *entry2 = (const kv_entry_t *)thr_time2;
     struct timespec time1 = entry1->timestamp;
     struct timespec time2 = entry2->timestamp;
     if (time1.tv_nsec < time2.tv_nsec) {
         return -1;
     } else {
         return 1;
     }
     return 0;
 }
 void print_time_table(kv_entry_t *table, Cpa32U size) {
     for (Cpa32U i = 0; i < size; i++) {
         PRINT_DBG("Thread ID: %lu, Timestamp: %ld.%09ld\n", table[i].tid, table[i].timestamp.tv_sec, table[i].timestamp.tv_nsec);
     }
 } 
 
 /*
 *****************************************************************************
 * Forward declaration
 *****************************************************************************
 */
 CpaStatus dcStatelessSample(void);
 
 /*
 * Callback function
 *
 * This function is "called back" (invoked by the implementation of
 * the API) when the asynchronous operation has completed.  The
 * context in which it is invoked depends on the implementation, but
 * as described in the API it should not sleep (since it may be called
 * in a context which does not permit sleeping, e.g. a Linux bottom
 * half).
 *
 * This function can perform whatever processing is appropriate to the
 * application.  For example, it may free memory, continue processing
 * of a packet, etc.  In this example, the function only sets the
 * complete variable to indicate it has been called.
 */
 //<snippet name="dcCallback">
 static void dcCallback(void *pCallbackTag, CpaStatus status)
 {
     // PRINT_DBG("Callback called with status = %d, tid = %lu\n", status, (unsigned long)pthread_self());
     struct timespec ts;
     clock_gettime(CLOCK_REALTIME, &ts);  // Use CLOCK_MONOTONIC for uptime
     // printf("Timestamp: %ld.%09ld seconds\n", ts.tv_sec, ts.tv_nsec);
     // Assume the response comes back in submission order
     endt_table_add_entry(end_time_table, (unsigned long)pthread_self(), ts);
 
     if (NULL != pCallbackTag)
     {
         /* indicate that the function has been called */
         COMPLETE((struct COMPLETION_STRUCT *)pCallbackTag);
     }
 }
 //</snippet>
 
 /*
 * This function performs a compression and decompress operation.
 */
 static CpaStatus compPerformOp(
     Cpa32U *interval,
     Cpa8U* buf_ptr,
     CpaInstanceHandle dcInstHandle,
     CpaDcSessionHandle sessionHdl,
     CpaDcHuffType huffType
 ){
     CpaStatus status = CPA_STATUS_SUCCESS;
     Cpa8U *pBufferMetaSrc = NULL;
     Cpa8U *pBufferMetaDst = NULL;
     // Cpa8U *pBufferMetaDst2 = NULL;
     Cpa32U bufferMetaSize = 0;
     CpaBufferList *pBufferListSrc = NULL;
     CpaBufferList *pBufferListDst = NULL;
     // CpaBufferList *pBufferListDst2 = NULL;
     CpaFlatBuffer *pFlatBuffer = NULL;
     CpaDcOpData opData = {};
     // Cpa32U bufferSize = sizeof(sampleData);
     Cpa32U bufferSize = SAMPLE_MAX_BUFF;
     Cpa32U dstBufferSize = bufferSize;
     // Cpa32U checksum = 0;
     Cpa32U numBuffers = 1; /* only using 1 buffer in this case */
     /* allocate memory for bufferlist and array of flat buffers in a contiguous
     * area and carve it up to reduce number of memory allocations required. */
     Cpa32U bufferListMemSize =
         sizeof(CpaBufferList) + (numBuffers * sizeof(CpaFlatBuffer));
     Cpa8U *pSrcBuffer = NULL;
     Cpa8U *pDstBuffer = NULL;
     // Cpa8U *pDst2Buffer = NULL;
     /* The following variables are allocated on the stack because we block
     * until the callback comes back. If a non-blocking approach was to be
     * used then these variables should be dynamically allocated */
     CpaDcRqResults dcResults;
     Cpa32U compl_arr_size = NUM_COMPL_PER_THREAD;
     struct COMPLETION_STRUCT complete_array[compl_arr_size];
     INIT_OPDATA(&opData, CPA_DC_FLUSH_FINAL);
 
     /*
     * Different implementations of the API require different
     * amounts of space to store meta-data associated with buffer
     * lists.  We query the API to find out how much space the current
     * implementation needs, and then allocate space for the buffer
     * meta data, the buffer list, and for the buffer itself.
     */
     //<snippet name="memAlloc">
     status = cpaDcBufferListGetMetaSize(dcInstHandle, numBuffers, &bufferMetaSize);
 
     /* Destination buffer size is set as sizeof(sampelData) for a
     * Deflate compression operation with DC_API_VERSION < 2.5.
     * cpaDcDeflateCompressBound API is used to get maximum output buffer size
     * for a Deflate compression operation with DC_API_VERSION >= 2.5 */
 
     status = cpaDcDeflateCompressBound(
         dcInstHandle, huffType, bufferSize, &dstBufferSize);
 
     if (CPA_STATUS_SUCCESS != status)
     {
         PRINT_ERR("cpaDcDeflateCompressBound API failed. (status = %d)\n",
                 status);
         return CPA_STATUS_FAIL;
     }
 
 
     /* Allocate source buffer */
     if (CPA_STATUS_SUCCESS == status)
     {
         status = PHYS_CONTIG_ALLOC(&pBufferMetaSrc, bufferMetaSize);
     }
     if (CPA_STATUS_SUCCESS == status)
     {
         status = OS_MALLOC(&pBufferListSrc, bufferListMemSize);
     }
     if (CPA_STATUS_SUCCESS == status)
     {
         status = PHYS_CONTIG_ALLOC(&pSrcBuffer, bufferSize);
     }
 
     /* Allocate destination buffer the same size as source buffer */
     if (CPA_STATUS_SUCCESS == status)
     {
         status = PHYS_CONTIG_ALLOC(&pBufferMetaDst, bufferMetaSize);
     }
     if (CPA_STATUS_SUCCESS == status)
     {
         status = OS_MALLOC(&pBufferListDst, bufferListMemSize);
     }
     if (CPA_STATUS_SUCCESS == status)
     {
         status = PHYS_CONTIG_ALLOC(&pDstBuffer, dstBufferSize);
     }
     //</snippet>
 
     if (CPA_STATUS_SUCCESS == status)
     {
         /* copy source into buffer */
         // memcpy(pSrcBuffer, sampleData, sizeof(sampleData));
         memcpy(pSrcBuffer, buf_ptr, SAMPLE_MAX_BUFF);
 
         /* Build source bufferList */
         pFlatBuffer = (CpaFlatBuffer *)(pBufferListSrc + 1);
 
         pBufferListSrc->pBuffers = pFlatBuffer;
         pBufferListSrc->numBuffers = 1;
         pBufferListSrc->pPrivateMetaData = pBufferMetaSrc;
 
         pFlatBuffer->dataLenInBytes = bufferSize;
         pFlatBuffer->pData = pSrcBuffer;
 
         /* Build destination bufferList */
         pFlatBuffer = (CpaFlatBuffer *)(pBufferListDst + 1);
 
         pBufferListDst->pBuffers = pFlatBuffer;
         pBufferListDst->numBuffers = 1;
         pBufferListDst->pPrivateMetaData = pBufferMetaDst;
 
         pFlatBuffer->dataLenInBytes = dstBufferSize;
         pFlatBuffer->pData = pDstBuffer;
 
         /*
         * Now, we initialize the completion variable which is used by the
         * callback
         * function to indicate that the operation is complete.  We then perform
         * the
         * operation.
         */
         // PRINT_DBG("cpaDcCompressData2\n");
 
         //<snippet name="perfOp">
 
         for (Cpa32U i = 0; i < compl_arr_size; i++)
         {
             COMPLETION_INIT(&complete_array[i]);
         }
 
         struct timespec start, end;
         long long elapsed_ns;
         pthread_barrier_wait(&barrier); // Put a barrier here to synchronize threads before benchmarking
         struct timespec ts;
         clock_gettime(CLOCK_MONOTONIC, &start);
         
         for (Cpa32U i = 0; i < compl_arr_size; i++)
         {
             clock_gettime(CLOCK_REALTIME, &ts);
             startt_table_add_entry(start_time_table, (unsigned long)pthread_self(), ts);
             status = cpaDcCompressData2(
                 dcInstHandle,
                 sessionHdl,
                 pBufferListSrc,     /* source buffer list */
                 pBufferListDst,     /* destination buffer list */
                 &opData,            /* Operational data */
                 &dcResults,         /* results structure */
                 (void *)&complete_array[i]); /* data sent as is to the callback function*/
             
             if (CPA_STATUS_SUCCESS != status)
             {
                 PRINT_ERR("cpaDcCompressData2 failed. (status = %d)\n", status);
             }
             
             usleep(interval[i]);
         }
 
         /*
         * We now wait until the completion of the operation.  This uses a macro
         * which can be defined differently for different OSes.
         */
         for (Cpa32U i = 0; i < compl_arr_size; i++)
         {
             if (CPA_STATUS_SUCCESS == status)
             {
                 if (!COMPLETION_WAIT(&complete_array[i], TIMEOUT_MS))
                 {
                     PRINT_ERR("timeout or interruption in cpaDcCompressData2\n");
                     status = CPA_STATUS_FAIL;
                 }
             }
         }
         clock_gettime(CLOCK_MONOTONIC, &end);
         elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
         PRINT_DBG("Elapsed time = %lld ns\n",elapsed_ns);
 
 
         /*
         * We now check the results
         */
         if (CPA_STATUS_SUCCESS == status)
         {
             if (dcResults.status != CPA_DC_OK)
             {
                 PRINT_ERR("Results status not as expected (status = %d)\n",
                         dcResults.status);
                 status = CPA_STATUS_FAIL;
             }
             // else
             // {
             //     PRINT_DBG("Data consumed %d\n", dcResults.consumed);
             //     PRINT_DBG("Data produced %d\n", dcResults.produced);
             //     // PRINT_DBG("Adler checksum 0x%x\n", dcResults.checksum);
             // }
 
             /* To compare the checksum with decompressed output */
             // checksum = dcResults.checksum;
         }
     }
 
     /*
     * At this stage, the callback function has returned, so it is
     * sure that the structures won't be needed any more.  Free the
     * memory!
     */
     PHYS_CONTIG_FREE(pSrcBuffer);
     OS_FREE(pBufferListSrc);
     PHYS_CONTIG_FREE(pBufferMetaSrc);
     PHYS_CONTIG_FREE(pDstBuffer);
     OS_FREE(pBufferListDst);
     PHYS_CONTIG_FREE(pBufferMetaDst);
 
     for (Cpa32U i = 0; i < compl_arr_size; i++)
     {
         COMPLETION_DESTROY(&complete_array[i]);
     }
     return status;
 }
 
 // CpaStatus enqueueQATWork(
 //     CpaInstanceHandle* dcInstHandle
 // ) {
 void *enqueueQATWork(void* arg) {
     qat_arg_t* qat_arg = (qat_arg_t*)arg;
 
     CpaStatus status = CPA_STATUS_SUCCESS;
     CpaDcInstanceCapabilities cap = {0};
     CpaBufferList **bufferInterArray = NULL;
     Cpa16U numInterBuffLists = 0;
     Cpa16U bufferNum = 0;
     Cpa32U buffMetaSize = 0;
 
     Cpa32U sess_size = 0;
     Cpa32U ctx_size = 0;
     CpaDcSessionHandle sessionHdl = NULL;
     CpaDcSessionSetupData sd = {0};
     CpaDcStats dcStats = {0};
 
     /* Query Capabilities */
     // PRINT_DBG("cpaDcQueryCapabilities\n");
     // //<snippet name="queryStart">
     status = cpaDcQueryCapabilities(*(qat_arg->dcInstHandle), &cap); // retrieve the capabilities matrix of an instance
     if (status != CPA_STATUS_SUCCESS)
     {
         // return status;
         return NULL;
     }
 
     if (cap.dynamicHuffmanBufferReq)
     { 
         status = cpaDcBufferListGetMetaSize(*(qat_arg->dcInstHandle), 1, &buffMetaSize);
 
         if (CPA_STATUS_SUCCESS == status)
         {
             status = cpaDcGetNumIntermediateBuffers(*(qat_arg->dcInstHandle),
                                                     &numInterBuffLists);
         }
         if (CPA_STATUS_SUCCESS == status && 0 != numInterBuffLists)
         {
             status = PHYS_CONTIG_ALLOC(
                 &bufferInterArray, numInterBuffLists * sizeof(CpaBufferList *));
         }
         for (bufferNum = 0; bufferNum < numInterBuffLists; bufferNum++)
         {
             if (CPA_STATUS_SUCCESS == status)
             {
                 status = PHYS_CONTIG_ALLOC(&bufferInterArray[bufferNum],
                                         sizeof(CpaBufferList));
             }
             if (CPA_STATUS_SUCCESS == status)
             {
                 status = PHYS_CONTIG_ALLOC(
                     &bufferInterArray[bufferNum]->pPrivateMetaData,
                     buffMetaSize);
             }
             if (CPA_STATUS_SUCCESS == status)
             {
                 status =
                     PHYS_CONTIG_ALLOC(&bufferInterArray[bufferNum]->pBuffers,
                                     sizeof(CpaFlatBuffer));
             }
             if (CPA_STATUS_SUCCESS == status)
             {
                 /* Implementation requires an intermediate buffer approximately
                         twice the size of the output buffer */
                 status = PHYS_CONTIG_ALLOC(
                     &bufferInterArray[bufferNum]->pBuffers->pData,
                     2 * SAMPLE_MAX_BUFF);
                 bufferInterArray[bufferNum]->numBuffers = 1;
                 bufferInterArray[bufferNum]->pBuffers->dataLenInBytes =
                     2 * SAMPLE_MAX_BUFF;
             }
 
         } /* End numInterBuffLists */
     }
 
     if (CPA_STATUS_SUCCESS == status)
     {
         /*
         * Set the address translation function for the instance
         */
         status = cpaDcSetAddressTranslation(*(qat_arg->dcInstHandle), sampleVirtToPhys);
     }
 
     if (CPA_STATUS_SUCCESS == status)
     {
         /* Start DataCompression component */
         // PRINT_DBG("cpaDcStartInstance\n");
         status = cpaDcStartInstance(
             *(qat_arg->dcInstHandle), numInterBuffLists, bufferInterArray);
     }
     //</snippet>
 
     if (CPA_STATUS_SUCCESS == status)
     {
         /*
         * If the instance is polled start the polling thread. Note that
         * how the polling is done is implementation-dependent.
         */
         sampleDcStartPolling(*(qat_arg->dcInstHandle));
 
         /*
         * We now populate the fields of the session operational data and create
         * the session.  Note that the size required to store a session is
         * implementation-dependent, so we query the API first to determine how
         * much memory to allocate, and then allocate that memory.
         */
         //<snippet name="initSession">
         sd.compLevel = CPA_DC_L6;
         sd.compType = CPA_DC_DEFLATE;
         sd.huffType = huffmanType_g;
         /* If the implementation supports it, the session will be configured
         * to select static Huffman encoding over dynamic Huffman as
         * the static encoding will provide better compressibility.
         */
         if (cap.autoSelectBestHuffmanTree)
         {
             sd.autoSelectBestHuffmanTree = CPA_DC_ASB_ENABLED;
         }
         else
         {
             sd.autoSelectBestHuffmanTree = CPA_DC_ASB_DISABLED;
         }
         sd.sessDirection = CPA_DC_DIR_COMBINED;
         sd.sessState = CPA_DC_STATELESS;
 
         /* Determine size of session context to allocate */
         // PRINT_DBG("cpaDcGetSessionSize\n");
         status = cpaDcGetSessionSize(*(qat_arg->dcInstHandle), &sd, &sess_size, &ctx_size);
     }
 
     if (CPA_STATUS_SUCCESS == status)
     {
         /* Allocate session memory */
         status = PHYS_CONTIG_ALLOC(&sessionHdl, sess_size);
     }
 
     /* Initialize the Stateless session */
     if (CPA_STATUS_SUCCESS == status)
     {
         // PRINT_DBG("cpaDcInitSession\n");
         status = cpaDcInitSession(
             *(qat_arg->dcInstHandle),
             sessionHdl, /* session memory */
             &sd,        /* session setup data */
             NULL, /* pContexBuffer not required for stateless operations */
             dcCallback); /* callback function */
     }
     //</snippet>
 
     if (CPA_STATUS_SUCCESS == status)
     {
         CpaStatus sessionStatus = CPA_STATUS_SUCCESS;
 
         /* Perform Compression operation */
         status = compPerformOp(qat_arg->interval, qat_arg->buf_ptr, *(qat_arg->dcInstHandle), sessionHdl, sd.huffType);
 
         /*
         * In a typical usage, the session might be used to compression
         * multiple buffers.  In this example however, we can now
         * tear down the session.
         */
         // PRINT_DBG("cpaDcRemoveSession, %d\n", qat_arg->index);
         //<snippet name="removeSession">
         sessionStatus = cpaDcRemoveSession(*(qat_arg->dcInstHandle), sessionHdl);
         //</snippet>
 
         /* Maintain status of remove session only when status of all operations
         * before it are successful. */
         if (CPA_STATUS_SUCCESS == status)
         {
             status = sessionStatus;
         }
     }
 
     if (CPA_STATUS_SUCCESS == status)
     {
         /*
         * We can now query the statistics on the instance.
         *
         * Note that some implementations may also make the stats
         * available through other mechanisms, e.g. in the /proc
         * virtual filesystem.
         */
         status = cpaDcGetStats(*(qat_arg->dcInstHandle), &dcStats);
 
         if (CPA_STATUS_SUCCESS != status)
         {
             PRINT_ERR("cpaDcGetStats failed, status = %d\n", status);
         }
         // else
         // {
         //     PRINT_DBG("Number of compression operations completed: %llu\n",
         //             (unsigned long long)dcStats.numCompCompleted);
         //     PRINT_DBG("Number of decompression operations completed: %llu\n",
         //             (unsigned long long)dcStats.numDecompCompleted);
         // }
     }
 
     /*
     * Free up memory, stop the instance, etc.
     */
 
     /* Stop the polling thread */
     // sampleDcStopPolling();
 
     /* Free session Context */
     PHYS_CONTIG_FREE(sessionHdl);
 
     /* Free intermediate buffers */
     if (bufferInterArray != NULL)
     {
         for (bufferNum = 0; bufferNum < numInterBuffLists; bufferNum++)
         {
             PHYS_CONTIG_FREE(bufferInterArray[bufferNum]->pBuffers->pData);
             PHYS_CONTIG_FREE(bufferInterArray[bufferNum]->pBuffers);
             PHYS_CONTIG_FREE(bufferInterArray[bufferNum]->pPrivateMetaData);
             PHYS_CONTIG_FREE(bufferInterArray[bufferNum]);
         }
         PHYS_CONTIG_FREE(bufferInterArray);
     }
 
     // return status;
     return NULL;
 }
 
 
 /*
 * This is the main entry point for the sample data compression code.
 * demonstrates the sequence of calls to be made to the API in order
 * to create a session, perform one or more stateless compression operations,
 * and then tear down the session.
 */
 CpaStatus dcStatelessSample(void)
 {
     CpaStatus status = CPA_STATUS_SUCCESS;
     Cpa16U numInstances = 0;
     CpaInstanceHandle dcInstHandles[MAX_INSTANCES];
 
     /*
     * In this simplified version of instance discovery, we discover
     * exactly one instance of a data compression service.
     */
     status = cpaDcGetNumInstances(&numInstances);
     if (numInstances >= MAX_INSTANCES) 
     {
         numInstances = MAX_INSTANCES;
     }
 
     for (int i = 0; i < MAX_INSTANCES; i++)
     {
         dcInstHandles[i] = NULL;
     }
 
     status = cpaDcGetInstances(numInstances, dcInstHandles);
     if (status != CPA_STATUS_SUCCESS)
     {
         PRINT_ERR("cpaDcGetNumInstances failed. (status = %d)\n", status);
         return status;
     }
 
     /*
      * Get device info from dcInstHandle
      */
     CpaInstanceInfo2 info[numInstances];
     for (int i = 0; i < numInstances; i++)
     {
         status = cpaDcInstanceGetInfo2(dcInstHandles[i], &info[i]);
         if (status != CPA_STATUS_SUCCESS)
         {
             PRINT_ERR("cpaDcInstanceGetInfo2 failed. (status = %d)\n", status);
             return status;
         }
         PRINT_DBG("Inst: Dev = %u, Accel = %u, EE = %u, BDF = %02X:%02X:%02X\n", 
             info[i].physInstId.packageId, 
             info[i].physInstId.acceleratorId, 
             info[i].physInstId.executionEngineId, 
             (Cpa8U)((info[i].physInstId.busAddress) >> 8), 
             (Cpa8U)((info[i].physInstId.busAddress) & 0xFF) >> 3, 
             (Cpa8U)((info[i].physInstId.busAddress) & 7));
     }
     // PRINT_DBG("cpaDcQueryCapabilities\n");
     //<snippet name="queryStart">
     
     /*--------------------------------------------------------------------*/
     pthread_t threads[numInstances];
     qat_arg_t qat_arg[numInstances];
     char filename[256];
     Cpa32U trace_time[numInstances][NUM_LINES_PER_FILE];
 
     /* Read data file */
     Cpa8U *buffer = malloc(SAMPLE_MAX_BUFF);
     if (!buffer) {
         perror("Malloc failed.");
         return 1;
     }
     FILE *fp = fopen("../benchmark/Silesia_all", "rb");
     if (!fp) {
         perror("File open failed.");
         free(buffer);
         return 1;
     }
     size_t bytesRead = fread(buffer, 1, SAMPLE_MAX_BUFF, fp);
     if (bytesRead != SAMPLE_MAX_BUFF) {
         perror("File read failed.");
         free(buffer);
         fclose(fp);
         return 1;
     }
     fclose(fp);
 
     /* Read trace file */
     for (int i = 1; i <= numInstances; i++) {
         snprintf(filename, sizeof(filename), "../traces/trace_vm%d", i);
         
         FILE *fp_trace = fopen(filename, "r");
         if (fp_trace == NULL) {
             perror("File open failed");
             free(buffer);
             fclose(fp_trace);
             return 1;
         }
         int work_size, interval;
         int line_index = 0;
         while (fscanf(fp_trace, "%d %d", &work_size, &interval) == 2) {
             // printf("Work size: %d, Interval: %d\n", work_size, interval);
             // trace_time[i-1] = malloc(NUM_LINES_PER_FILE * sizeof(int));
             trace_time[i-1][line_index] = interval;
             line_index++;
         }
         fclose(fp_trace);
     }
     
     pthread_barrier_init(&barrier, NULL, numInstances);
     for (int i = 0; i < numInstances; i++)
     {
         qat_arg[i].dcInstHandle = &(dcInstHandles[i]);
         qat_arg[i].index = i;
         qat_arg[i].buf_ptr = buffer;
         qat_arg[i].interval = trace_time[i];
         
         if (pthread_create(&threads[i], NULL, (void *)enqueueQATWork, (void *)&qat_arg[i]))
         {
             perror("pthread_create failed");
             return CPA_STATUS_FAIL;
         }
     }
     for (int i = 0; i < numInstances; i++)
     {
         pthread_join(threads[i], NULL);
     }
     pthread_barrier_destroy(&barrier);
 
     sampleDcStopPolling();
 
     /*--------------------------------------------------------------------*/
     // for (int i = 0; i < numInstances; i++)
     // {
     //     status = enqueueQATWork(&dcInstHandles[i]);
     //     if (status != CPA_STATUS_SUCCESS)
     //     {
     //         PRINT_ERR("enqueueQATWork failed. (status = %d)\n", status);
     //         return status;
     //     }
     // }
     /*--------------------------------------------------------------------*/
 
     for (int i = 0; i < numInstances; i++)
     {
         status = cpaDcStopInstance(dcInstHandles[i]);
         if (status != CPA_STATUS_SUCCESS)
         {
             PRINT_ERR("cpaDcStopInstance failed. (status = %d)\n", status);
             return status;
         }
     }
     /*--------------------------------------------------------------------*/
     /* Sort the timestamp KV store                                        */
     /*--------------------------------------------------------------------*/
     qsort(start_time_table, start_time_table_idx, sizeof(kv_entry_t), compare_tid);
     qsort(end_time_table, end_time_table_idx, sizeof(kv_entry_t), compare_tid);
     // PRINT_DBG("Start time table:\n");
     // print_time_table(start_time_table, start_time_table_idx);
     // PRINT_DBG("End time table:\n");
     // print_time_table(end_time_table, end_time_table_idx);
     
     // Free the allocated buffer
     free(buffer);
 
     if (CPA_STATUS_SUCCESS == status)
     {
         PRINT_DBG("Sample code ran successfully\n");
     }
     else
     {
         PRINT_DBG("Sample code failed with status of %d\n", status);
     }
 
     return status;
 }
 