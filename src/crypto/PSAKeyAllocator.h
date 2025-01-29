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

#if CHIP_HAVE_CONFIG_H
#include <crypto/CryptoBuildConfig.h>
#endif // CHIP_HAVE_CONFIG_H

#include <lib/core/DataModelTypes.h>

namespace chip {
namespace Crypto {

static_assert(PSA_KEY_ID_USER_MIN <= CHIP_CONFIG_CRYPTO_PSA_KEY_ID_BASE && CHIP_CONFIG_CRYPTO_PSA_KEY_ID_END <= PSA_KEY_ID_USER_MAX,
              "Matter specific PSA key range doesn't fit within PSA allowed range");

// Each ICD client requires storing two keys- AES and HMAC
static constexpr uint32_t kMaxICDClientKeys = 2 * CHIP_CONFIG_CRYPTO_PSA_ICD_MAX_CLIENTS;

static_assert(kMaxICDClientKeys >= CHIP_CONFIG_ICD_CLIENTS_SUPPORTED_PER_FABRIC * CHIP_CONFIG_MAX_FABRICS,
              "Number of allocated ICD key slots is lower than maximum number of supported ICD clients");

/**
 * @brief Defines subranges of the PSA key identifier space used by Matter.
 */
enum class KeyIdBase : psa_key_id_t
{
    Minimum          = CHIP_CONFIG_CRYPTO_PSA_KEY_ID_BASE,
    Operational      = Minimum, ///< Base of the PSA key ID range for Node Operational Certificate private keys
    DACPrivKey       = Operational + kMaxValidFabricIndex + 1,
    ICDKeyRangeStart = DACPrivKey + 1,
    Maximum          = ICDKeyRangeStart + kMaxICDClientKeys,
};

static_assert(to_underlying(KeyIdBase::Minimum) >= CHIP_CONFIG_CRYPTO_PSA_KEY_ID_BASE &&
                  to_underlying(KeyIdBase::Maximum) <= CHIP_CONFIG_CRYPTO_PSA_KEY_ID_END,
              "PSA key ID base out of allowed range");

/**
 * @brief Finds first free persistent Key slot ID within range.
 *
 * @param[out] keyId Key ID handler to which free ID will be set.
 * @param[in]  start Starting ID in search range.
 * @param[in]  range Search range.
 *
 * @retval CHIP_NO_ERROR               On success.
 * @retval CHIP_ERROR_INTERNAL         On PSA crypto API error.
 * @retval CHIP_ERROR_NOT_FOUND        On no free Key ID within range.
 * @retval CHIP_ERROR_INVALID_ARGUMENT On search arguments out of PSA allowed range.
 */
CHIP_ERROR FindFreeKeySlotInRange(psa_key_id_t & keyId, psa_key_id_t start, uint32_t range);

/**
 * @brief Calculates PSA key ID for Node Operational Certificate private key for the given fabric.
 */
constexpr psa_key_id_t MakeOperationalKeyId(FabricIndex fabricIndex)
{
    return to_underlying(KeyIdBase::Operational) + static_cast<psa_key_id_t>(fabricIndex);
}

class PSAKeyAllocator
{
public:
    virtual ~PSAKeyAllocator() = default;

    virtual psa_key_id_t GetDacKeyId()                             = 0;
    virtual psa_key_id_t GetOpKeyId(FabricIndex fabricIndex)       = 0;
    virtual psa_key_id_t AllocateICDKeyId()                        = 0;
    virtual void UpdateKeyAttributes(psa_key_attributes_t * attrs) = 0;

    static PSAKeyAllocator * Get() { return sInstance; }
    static void Set(PSAKeyAllocator * keyAllocator) { sInstance = keyAllocator; }

private:
    static PSAKeyAllocator * sInstance;
};

class PSADefaultKeyAllocator : public PSAKeyAllocator
{
public:
    psa_key_id_t GetDacKeyId() override { return to_underlying(KeyIdBase::DACPrivKey); }
    psa_key_id_t GetOpKeyId(FabricIndex fabricIndex) override { return MakeOperationalKeyId(fabricIndex); }
    psa_key_id_t AllocateICDKeyId() override
    {
        psa_key_id_t newKeyId = PSA_KEY_ID_NULL;
        if (CHIP_NO_ERROR !=
            Crypto::FindFreeKeySlotInRange(newKeyId, to_underlying(KeyIdBase::ICDKeyRangeStart), kMaxICDClientKeys))
        {
            newKeyId = PSA_KEY_ID_NULL;
        }
        return newKeyId;
    }
    void UpdateKeyAttributes(psa_key_attributes_t * attrs) override
    {
        // Do nothing
    }
};

} // namespace Crypto
} // namespace chip
