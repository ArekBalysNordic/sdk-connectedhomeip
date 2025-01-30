/*
 *    Copyright (c) 2025 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#pragma once
#include <cracen_psa_kmu.h>
#include <crypto/PSAKeyAllocator.h>
// Define the number of slots per NOC and ICD key.
#define KMU_MATTER_SLOTS_PER_NOC_KEY 2
#define KMU_MATTER_SLOTS_PER_ICD_KEY 2
// Define the maximum number of fabrics supported by the KMU.
#if CONFIG_CHIP_ICD_CHECK_IN_SUPPORT
// If ICD is enabled, we need to divide the available slots by the number of slots per NOC and ICD key.
#define CHIP_KMU_MAX_FABRICS                                                                                                       \
    ((CONFIG_CHIP_CORE_KMU_SLOT_END - CONFIG_CHIP_CORE_KMU_SLOT_START) /                                                           \
     (KMU_MATTER_SLOTS_PER_NOC_KEY + KMU_MATTER_SLOTS_PER_ICD_KEY))
#else
// Otherwise we can use all available slots for NOC keys.
#define CHIP_KMU_MAX_FABRICS ((CONFIG_CHIP_CORE_KMU_SLOT_END - CONFIG_CHIP_CORE_KMU_SLOT_START) / KMU_MATTER_SLOTS_PER_NOC_KEY)
#endif // CONFIG_CHIP_ICD_CHECK_IN_SUPPORT
// Define how many slots are available for Matter.
#define KMU_MATTER_SLOT_COUNT (CONFIG_CHIP_CORE_KMU_SLOT_END - CONFIG_CHIP_CORE_KMU_SLOT_START)
// For NOC we need 2 slots per fabric (ESDSA)
#define KMU_MATTER_NOC_SLOT_COUNT (CHIP_KMU_MAX_FABRICS * KMU_MATTER_SLOTS_PER_NOC_KEY)
#ifdef CONFIG_CHIP_ICD_CHECK_IN_SUPPORT
// For ICD we need 2 slots per ICD entry (AES + HMAC)
#define KMU_MATTER_ICD_SLOT_COUNT (CHIP_KMU_MAX_FABRICS * KMU_MATTER_SLOTS_PER_ICD_KEY)
#else
#define KMU_MATTER_ICD_SLOT_COUNT 0
#endif // CONFIG_CHIP_ICD_CHECK_IN_SUPPORT
// Define the start of the KMU slots for Matter.
#define KMU_MATTER_NOC_SLOT_START                                                                                                  \
    PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, CONFIG_CHIP_CORE_KMU_SLOT_START)
#define KMU_MATTER_ICD_SLOT_START                                                                                                  \
    PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW,                                                           \
                                        (CONFIG_CHIP_CORE_KMU_SLOT_START + KMU_MATTER_NOC_SLOT_COUNT))
// Check whether the DAC KMU slot doesn not overlap with the KMU slots dedicated for Matter core.
#if defined(CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU) &&                                                                            \
    (CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU_SLOT_ID >= CONFIG_CHIP_CORE_KMU_SLOT_START &&                                         \
     CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU_SLOT_ID <= CONFIG_CHIP_CORE_KMU_SLOT_END)
#error "CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU_SLOT_ID Cannot overlaps with KMU slots dedicated for Matter core"
#endif
// Check if the maximum of used slots is not greater than the available slots.
// Each fabric requires 2 slots (ESDSA), one ICD entry requires 2 slots (AES + HMAC).
// If migration DAC to KMU is enabled we need to reserve slots for it.
#if (KMU_MATTER_NOC_SLOT_COUNT + KMU_MATTER_ICD_SLOT_COUNT) > KMU_MATTER_SLOT_COUNT
#error "Not enough KMU slots for the requested configuration"
#endif
namespace chip {
namespace DeviceLayer {
class KMUKeyAllocator : public chip::Crypto::PSAKeyAllocator
{
public:
    psa_key_id_t GetDacKeyId() override
    {
        return PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW,
                                                   CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU_SLOT_ID);
    }
    psa_key_id_t GetOpKeyId(FabricIndex fabricIndex) override
    {
        return static_cast<psa_key_id_t>(KMU_MATTER_NOC_SLOT_START + ((fabricIndex - 1) * KMU_MATTER_SLOTS_PER_NOC_KEY));
    }
    psa_key_id_t AllocateICDKeyId() override
    {
        psa_key_id_t newKeyId = PSA_KEY_ID_NULL;
        if (CHIP_NO_ERROR !=
            FindFreeSlot(newKeyId, KMU_MATTER_ICD_SLOT_START, KMU_MATTER_ICD_SLOT_START + KMU_MATTER_ICD_SLOT_COUNT))
        {
            newKeyId = PSA_KEY_ID_NULL;
        }
        return newKeyId;
    }
    void UpdateKeyAttributes(psa_key_attributes_t * attrs) override
    {
        VerifyOrReturn(attrs != nullptr);
        // Set the key lifetime to persistent and the location to CRACEN_KMU if key is in a proper range
        if (psa_get_key_id(attrs) >=
                PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, CONFIG_CHIP_CORE_KMU_SLOT_START) &&
            psa_get_key_id(attrs) <
                PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, CONFIG_CHIP_CORE_KMU_SLOT_END))
        {
            psa_set_key_lifetime(
                attrs, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(PSA_KEY_PERSISTENCE_DEFAULT, PSA_KEY_LOCATION_CRACEN_KMU));
        }
        // TODO: Currently ECDSA keys are supported only if HASH_ANY is provided, so change it.
        // Remove this workaround once KMU supports ECSA SHA256.
        if (psa_get_key_algorithm(attrs) == PSA_ALG_ECDSA(PSA_ALG_SHA_256))
        {
            psa_set_key_algorithm(attrs, PSA_ALG_ECDSA(PSA_ALG_ANY_HASH));
        }

        // TODO: Change PSA_ALG_AEAD_WITH_AT_LEAST_THIS_LENGTH_TAG(PSA_ALG_CCM, 8) to PSA_ALG_CCM
        // To be compatible with the KMU. Remove this workaround once KMU supports PSA_ALG_AEAD_WITH_AT_LEAST_THIS_LENGTH_TAG.
        // Currently we need to use EXPORT flag because the Cracen does not support copying keys from ITS to KMU
        if (psa_get_key_algorithm(attrs) == PSA_ALG_AEAD_WITH_AT_LEAST_THIS_LENGTH_TAG(PSA_ALG_CCM, 8))
        {
            psa_set_key_algorithm(attrs, PSA_ALG_CCM);
            psa_set_key_usage_flags(attrs, psa_get_key_usage_flags(attrs) | PSA_KEY_USAGE_EXPORT);
        }

        // TODO: Currently we need to use EXPORT flag because the Cracen does not support copying keys from ITS to KMU
        if (psa_get_key_type(attrs) == PSA_KEY_TYPE_HMAC)
        {
            psa_set_key_usage_flags(attrs, psa_get_key_usage_flags(attrs) | PSA_KEY_USAGE_EXPORT);
        }
    }

private:
    CHIP_ERROR FindFreeSlot(psa_key_id_t & keyId, psa_key_id_t start, psa_key_id_t end)
    {
        psa_key_attributes_t attributes;
        psa_status_t status = PSA_SUCCESS;
        for (keyId = start; keyId < end; ++keyId)
        {
            status = psa_get_key_attributes(keyId, &attributes);
            if (status == PSA_ERROR_INVALID_HANDLE)
            {
                return CHIP_NO_ERROR;
            }
            else if (status != PSA_SUCCESS)
            {
                return CHIP_ERROR_INTERNAL;
            }
            psa_reset_key_attributes(&attributes);
        }
        return CHIP_ERROR_NOT_FOUND;
    }
};
} // namespace DeviceLayer
} // namespace chip
