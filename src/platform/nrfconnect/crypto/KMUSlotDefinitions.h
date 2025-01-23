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

// Check whether the DAC KMU slot doesn not overlap with the KMU slots dedicated for Matter core.
#if defined(CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU) &&                                                                            \
    (CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU_SLOT_ID >= CONFIG_CHIP_CORE_KMU_SLOT_START &&                                         \
     CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU_SLOT_ID <= CONFIG_CHIP_CORE_KMU_SLOT_END)
#error "CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU_SLOT_ID Cannot overlaps with KMU slots dedicated for Matter core"
#endif

// Define how many slots are available for Matter.
#define KMU_MATTER_SLOT_COUNT (CONFIG_CHIP_CORE_KMU_SLOT_END - CONFIG_CHIP_CORE_KMU_SLOT_START)
// For NOC we need 2 slots per fabric (ESDSA)
#define KMU_MATTER_NOC_SLOT_COUNT (CONFIG_CHIP_KMU_MAX_FABRICS * 2)
#ifdef CONFIG_CHIP_ENABLE_ICD_SUPPORT
// For ICD we need 2 slots per ICD entry (AES + HMAC)
#define KMU_MATTER_ICD_SLOT_COUNT (CONFIG_CHIP_KMU_MAX_FABRICS * 2)
#else
#define KMU_MATTER_ICD_SLOT_COUNT 0
#endif

// Define the start of the KMU slots for Matter.
#define KMU_MATTER_NOC_SLOT_START                                                                                                  \
    PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, CONFIG_CHIP_CORE_KMU_SLOT_START)
#define KMU_MATTER_ICD_SLOT_START                                                                                                  \
    PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW,                                                           \
                                        (CONFIG_CHIP_CORE_KMU_SLOT_START + KMU_MATTER_NOC_SLOT_COUNT))

// Check if the maximum of used slots is not greater than the available slots.
// Each fabric requires 2 slots (ESDSA), one ICD entry requires 2 slots (AES + HMAC).
// If migration DAC to KMU is enabled we need to reserve slots for it.
#if (KMU_MATTER_NOC_SLOT_COUNT + KMU_MATTER_ICD_SLOT_COUNT) > KMU_MATTER_SLOT_COUNT
#error "Not enough KMU slots for the requested configuration"
#endif
