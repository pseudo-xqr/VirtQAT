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
 *******************************************************************************
 * @file lac_hooks.h
 *
 * @defgroup LacHooks   Hooks
 *
 * @ingroup LacCommon
 *
 * Component Init/Shutdown functions. These are:
 *  - an init function which is called during the intialisation sequence,
 *  - a shutdown function which is called by the overall shutdown function,
 *
 ******************************************************************************/

#ifndef LAC_HOOKS_H
#define LAC_HOOKS_H

/*
********************************************************************************
* Include public/global header files
********************************************************************************
*/

#include "cpa.h"

/*
********************************************************************************
* Include private header files
********************************************************************************
*/

/******************************************************************************/

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function initialises the Large Number (ModExp and ModInv) module
 *
 * @description
 *      This function clears the Large Number statistics
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
CpaStatus LacLn_Init(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function frees statistics array for Large Number module
 *
 * @description
 *      This function frees statistics array for Large Number module
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
void LacLn_StatsFree(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function resets statistics array for Large Number module
 *
 * @description
 *      This function resets statistics array for Large Number module
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
void LacLn_StatsReset(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function initialises the Prime module
 *
 * @description
 *      This function clears the Prime statistics
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
CpaStatus LacPrime_Init(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function frees the Prime module statistics array
 *
 * @description
 *      This function frees the Prime module statistics array
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
void LacPrime_StatsFree(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function resets the Prime module statistics array
 *
 * @description
 *      This function resets the Prime module statistics array
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
void LacPrime_StatsReset(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function initialises the DSA module
 *
 * @param[in] instanceHandle
 *
 * @description
 *      This function clears the DSA statistics
 *
 ******************************************************************************/
CpaStatus LacDsa_Init(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function frees the DSA module statistics array
 *
 * @param[in] instanceHandle
 *
 * @description
 *      This function frees the DSA statistics array
 *
 ******************************************************************************/
void LacDsa_StatsFree(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function resets the DSA module statistics array
 *
 * @param[in] instanceHandle
 *
 * @description
 *      This function resets the DSA statistics array
 *
 ******************************************************************************/
void LacDsa_StatsReset(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function initialises the Diffie Hellmann module
 *
 * @description
 *      This function initialises the Diffie Hellman statistics
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
CpaStatus LacDh_Init(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function frees the Diffie Hellmann module statistics
 *
 * @description
 *      This function frees the Diffie Hellmann module statistics
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
void LacDh_StatsFree(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function resets the Diffie Hellmann module statistics
 *
 * @description
 *      This function resets the Diffie Hellmann module statistics
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
void LacDh_StatsReset(CpaInstanceHandle instanceHandle);

/**
 ******************************************************************************
 * @ingroup LacSymKey
 *      This function registers the callback handlers to SSL/TLS and MGF,
 *      allocates resources that are needed for the component
 *
 * @param[in] instanceHandle
 *
 * @retval CPA_STATUS_SUCCESS   Status Success
 * @retval CPA_STATUS_FAIL      General failure
 * @retval CPA_STATUS_RESOURCE  Resource allocation failure
 *
 *****************************************************************************/
CpaStatus LacSymKey_Init(CpaInstanceHandle instanceHandle);

/**
 ******************************************************************************
 * @ingroup LacSymKey
 *      This function frees up resources obtained by the key gen component
 *
 * @param[in] instanceHandle
 *
 * @retval CPA_STATUS_SUCCESS   Status Success
 *
 *****************************************************************************/
CpaStatus LacSymKey_Shutdown(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function initialises the key gen statistics
 *
 * @description
 *      This function initialises the key gen statistics
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
CpaStatus LacSymKey_StatsInit(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function frees the key gen statistics
 *
 * @description
 *      This function frees the key gen statistics
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
CpaStatus LacSymKey_StatsFree(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function resets the key gen statistics
 *
 * @description
 *      This function resets the key gen statistics
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
CpaStatus LacSymKey_StatsReset(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function initialises the RSA module
 *
 * @description
 *      This function clears the RSA statistics
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
CpaStatus LacRsa_Init(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function frees the RSA module statistics
 *
 * @description
 *      This function frees the RSA module statistics
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
void LacRsa_StatsFree(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function resets the RSA module statistics
 *
 * @description
 *      This function resets the RSA module statistics
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
void LacRsa_StatsReset(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function initialises the EC module
 *
 * @description
 *      This function clears the EC statistics
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
CpaStatus LacEc_Init(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function frees the EC module stats array
 *
 * @description
 *      This function frees the EC module stats array
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
void LacEc_StatsFree(CpaInstanceHandle instanceHandle);

/**
 *******************************************************************************
 * @ingroup LacHooks
 *      This function resets the EC module stats array
 *
 * @description
 *      This function resets the EC module stats array
 *
 * @param[in] instanceHandle
 *
 ******************************************************************************/
void LacEc_StatsReset(CpaInstanceHandle instanceHandle);

#endif /* LAC_HOOKS_H */
