/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
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

/**
 *    @file
 *          Contains a class handling registration of variable and keep their occurrences.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include <platform/KeyValueStoreManager.h>

#include "IntrusiveList.h"

#define MAXIMUM_VARIABLE_NAME 10
#define MAXIMUM_VARIABLE_COUNT 10

namespace chip {

template <typename T, size_t N>
struct VariablesMap
{
    static constexpr T kInvalidKey{ std::numeric_limits<T>::max() };
    static constexpr size_t kNoSlotsFound{ N + 1 };

    struct Item
    {
        char key[MAXIMUM_VARIABLE_NAME + 1] = {};
        T value                             = kInvalidKey;
    };

    bool Set(const char * key, T value)
    {
        if (!key || !validateKey(key))
        {
            return false;
        }

        for (auto & it : mMap)
        {
            if (memcmp(it.key, key, GetKeyLen(key)) == 0 && GetKeyLen(it.key) == GetKeyLen(key))
            {
                it.value = std::move(value);
                return true;
            }
        }

        if (mElementsCount < N)
        {
            size_t slot = GetFirstFreeSlot();
            if (slot == kNoSlotsFound)
            {
                return false;
            }

            memcpy(mMap[slot].key, key, GetKeyLen(key));
            mMap[slot].value = std::move(value);
            mElementsCount++;
            return true;
        }

        return false;
    }

    bool Get(const char * key, T & value)
    {
        if (!key || !validateKey(key))
        {
            return false;
        }

        for (auto & it : mMap)
        {
            if (memcmp(it.key, key, GetKeyLen(key)) == 0 && GetKeyLen(it.key) == GetKeyLen(key))
            {
                value = it.value;
                return true;
            }
        }
        return false;
    }

    bool Contains(const char * key)
    {
        if (!key || !validateKey(key))
        {
            return false;
        }

        for (auto & it : mMap)
        {
            if (memcmp(it.key, key, GetKeyLen(key)) == 0)
            {
                return true;
            }
        }
        return false;
    }

    size_t FreeSlots() { return N - mElementsCount; }

    size_t GetFirstFreeSlot()
    {
        size_t foundIndex = 0;
        for (auto & it : mMap)
        {
            if (it.value == kInvalidKey)
                return foundIndex;
            foundIndex++;
        }
        return kNoSlotsFound;
    }

private:
    bool validateKey(const char * key)
    {
        if (GetKeyLen(key) == MAXIMUM_VARIABLE_NAME + 1 || GetKeyLen(key) == 0)
        {
            return false;
        }
        return true;
    }

    size_t GetKeyLen(const char * key) { return strnlen(key, MAXIMUM_VARIABLE_NAME + 1); }

    Item mMap[N];
    uint16_t mElementsCount{ 0 };
};

struct VariableStats
{
    static bool Set(const char * name, uint32_t newValue, bool persistent = false)
    {
        if (persistent)
        {
            uint32_t value = newValue;
            if (DeviceLayer::PersistedStorage::KeyValueStoreMgrImpl().Put(name, &value, sizeof(value)) != CHIP_NO_ERROR)
            {
                return false;
            }
        }

        return Instance().mVariableMap.Set(name, newValue);
    }

    static bool Increment(const char * name, bool persistent = false)
    {
        uint32_t variable = {};
        Get(name, variable, persistent);

        if (Set(name, ++variable, persistent))
        {
            return true;
        }

        return false;
    }

    static bool Decrement(const char * name, bool persistent = false)
    {
        uint32_t variable = {};
        Get(name, variable, persistent);

        if (Set(name, --variable, persistent))
        {
            return true;
        }

        return false;
    }

    static bool Get(const char * name, uint32_t & value, bool persistent = false)
    {
        if (persistent)
        {
            uint32_t persistentValue = 0;
            if (DeviceLayer::PersistedStorage::KeyValueStoreMgrImpl().Get(name, &persistentValue, sizeof(persistentValue)) ==
                CHIP_NO_ERROR)
            {
                value = persistentValue;
                return true;
            }
            return false;
        }
        return Instance().mVariableMap.Get(name, value);
    }

    static VariableStats & Instance()
    {
        static VariableStats sInstance;
        return sInstance;
    }

private:
    VariablesMap<uint32_t, 10> mVariableMap;
};

} // namespace chip