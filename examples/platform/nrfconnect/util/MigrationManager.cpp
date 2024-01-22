/*
 *    Copyright (c) 2024 Project CHIP Authors
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

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "MigrationManager.h"

namespace Migration {
#ifdef CONFIG_CHIP_CRYPTO_PSA
CHIP_ERROR MoveOperationalKeysFromKvsToIts(chip::PersistentStorageDelegate * storage, chip::Crypto::OperationalKeystore * keystore)
{
    VerifyOrReturnValue(keystore && storage, CHIP_ERROR_INVALID_ARGUMENT);
    /* Initialize the obsolete Operational Keystore*/
    chip::PersistentStorageOperationalKeystore obsoleteKeystore;
    VerifyOrReturn(obsoleteKeystore.Init(storage));
    /* Migrate all obsolete Operational Keys to PSA ITS */
    for (const chip::FabricInfo & fabric : chip::Server::GetInstance().GetFabricTable())
    {
        CHIP_ERROR err = keystore->MigrateOpKeypairForFabric(fabric.GetFabricIndex(), obsoleteKeystore);

        if (CHIP_NO_ERROR != err)
        {
#ifdef CONFIG_NCS_SAMPLE_MATTER_FACTORY_RESET_ON_KEY_MIGRATION_FAILURE
            chip::Server::GetInstance().ScheduleFactoryReset();
#else
            return err;
#endif
        }
    }
    return CHIP_NO_ERROR;
}
#endif
} // namespace Migration