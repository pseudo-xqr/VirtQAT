/******************************************************************************
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
 *****************************************************************************/

/**
 ******************************************************************************
 * @file lac_sym_queue.h
 *
 * @defgroup LacSymQueue Symmetric request queueing functions
 *
 * @ingroup LacSym
 *
 * Function prototypes for sending/queuing symmetric requests
 *****************************************************************************/

#ifndef LAC_SYM_QUEUE_H
#define LAC_SYM_QUEUE_H

#include "cpa.h"
#include "lac_session.h"
#include "lac_sym.h"

/**
*******************************************************************************
* @ingroup LacSymQueue
*      Send a request message to the QAT, or queue it if necessary
*
* @description
*      This function will send a request message to the QAT.  However, if a
*      blocking condition exists on the session (e.g. partial packet in flight,
*      precompute in progress), then the message will instead be pushed on to
*      the request queue for the session and will be sent later to the QAT
*      once the blocking condition is cleared.
*
* @param[in]  instanceHandle       Handle for instance of QAT
* @param[in]  pRequest             Pointer to request cookie
* @param[out] pSessionDesc         Pointer to session descriptor
*
*
* @retval CPA_STATUS_SUCCESS        Success
* @retval CPA_STATUS_FAIL           Function failed.
* @retval CPA_STATUS_RESOURCE       Problem Acquiring system resource
* @retval CPA_STATUS_RETRY          Failed to send message to QAT due to queue
*                                   full condition
*
*****************************************************************************/
CpaStatus LacSymQueue_RequestSend(const CpaInstanceHandle instanceHandle,
                                  lac_sym_bulk_cookie_t *pRequest,
                                  lac_session_desc_t *pSessionDesc);

#endif /* LAC_SYM_QUEUE_H */
