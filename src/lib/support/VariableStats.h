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
 *          Contains a class handling registration of variable and keep their values.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include <platform/CHIPDeviceConfig.h>
#include <platform/KeyValueStoreManager.h>

// TODO add better error handling than std::bool
// TODO Add better documentation

// TODO parametrize it in config
#define MAXIMUM_VARIABLE_NAME 20
#define MAXIMUM_VARIABLE_COUNT 20

namespace chip {

template <typename T, size_t N>
struct VariablesMap
{
    static constexpr T kInvalidKey{ std::numeric_limits<T>::max() };
    static constexpr size_t kNoSlotsFound{ N + 1 };

    struct Item
    {
        // TODO consider changing a problematic char key names to some IDs (mostly to reduce size)
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

    bool GetOccupiedKeysNames(char * names, size_t bufferSize)
    {
        size_t offset = 0;
        for (auto & it : mMap)
        {
            if (it.value != kInvalidKey)
            {
                if (offset + GetKeyLen(it.key) + 1 <= bufferSize)
                {
                    memcpy(names + offset, it.key, GetKeyLen(it.key));
                    offset += GetKeyLen(it.key);
                    memcpy(names + offset, "\n", 1);
                    offset += 1;
                }
                else
                {
                    return false;
                }
            }
        }
        return true;
    }

private:
    bool validateKey(const char * key)
    {
        if (GetKeyLen(key) >= MAXIMUM_VARIABLE_NAME + 1 || GetKeyLen(key) == 0)
        {
            return false;
        }
        return true;
    }

    size_t GetKeyLen(const char * key) { return strlen(key); }

    Item mMap[N];
    uint16_t mElementsCount{ 0 };
};

// TODO Add possibility to use various types instead of uint32_t
/**
 * @brief A struct to collect variables provided as string and manipulate their values.
 * This struct is based on simple map(string, value) and implements methods to increment and decrement values.
 *
 * The struct can be used to store various statistics data in the Matter Core.
 * To enable full support of this struct set the CHIP_DEVICE_VARIABLE_STATISTICS config to 1.
 * If the config is not set all features of the struct are disabled but you do not need to remove usage of the methods from the
 * code.
 *
 */
struct VariableStats
{
    /**
     * @brief Set the new value for provided key
     *
     * If the key does not exist this method will create a new one if there is available space.
     * If the key exists the method will update its value.
     *
     * @param name A variable name
     * @param newValue A new value for the variable
     * @param persistent true if the value should be saved to persistent storage, false if the value should be kept in RAM.
     * @return true if the new value of the variable has been saved
     * @return false if there is no memory to store the new value
     */
    static bool Set(const char * name, uint32_t newValue, bool persistent = false)
    {
        // TODO add protection against storing too many keys, compare it with size of map
#if CHIP_DEVICE_VARIABLE_STATISTICS
        if (persistent)
        {
            uint32_t value = newValue;
            if (DeviceLayer::PersistedStorage::KeyValueStoreMgrImpl().Put(name, &value, sizeof(value)) != CHIP_NO_ERROR)
            {
                return false;
            }
        }

        return Instance().mVariableMap.Set(name, newValue);
#else
        return false;
#endif
    }

    /**
     * @brief Increment value of given variable name
     *
     * If the variable does not exist this method will create a new one, and increment its value automatically if there is available
     * space.
     *
     * @param name A variable name
     * @param persistent true if the value should be saved to persistent storage, false if the value should be kept in RAM.
     * @return true if the variable value has been incremented and saved.
     * @return false if the variable cannot be incremented.
     */
    static bool Increment(const char * name, bool persistent = false)
    {
#if CHIP_DEVICE_VARIABLE_STATISTICS
        uint32_t variable = {};
        Get(name, variable, persistent);

        if (Set(name, ++variable, persistent))
        {
            return true;
        }
#endif
        return false;
    }

    /**
     * @brief Decrement value of given variable name
     *
     * If the variable does not exist this method will not create a new one.
     *
     * @param name A variable name
     * @param persistent true if the value should be saved to persistent storage, false if the value should be kept in RAM.
     * @return true if the variable value has been decremented and saved.
     * @return false if the variable cannot be decremented or value does not exist.
     */
    static bool Decrement(const char * name, bool persistent = false)
    {
#if CHIP_DEVICE_VARIABLE_STATISTICS
        uint32_t variable = {};
        if (!Get(name, variable, persistent))
        {
            return false;
        }

        if (Set(name, --variable, persistent))
        {
            return true;
        }
#endif
        return false;
    }

    /**
     * @brief Get value of the given variable name
     *
     * @param name A variable name
     * @param value
     * @param persistent true if the value should be read form persistent storage, false if the value should be read from RAM.
     * @return true if the value has been read.
     * @return false if the value does not exist or could not be read.
     */
    static bool Get(const char * name, uint32_t & value, bool persistent = false)
    {
#if CHIP_DEVICE_VARIABLE_STATISTICS
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
#else
        return false;
#endif
    }

    /**
     * @brief Create and get the List of the saved variables names
     *
     * @param keyList output list of the available variables names
     * @param bufferSize buffer size to store the created variable names
     * @param persistent true if the list should base on persistent storage names, false if the list should base on variables stored
     * in RAM.
     * @return true list has been created
     * @return false if the list has not been created due to too small bufferSize
     */
    static bool GetList(char * keyList, size_t bufferSize, bool persistent = false)
    {
#if CHIP_DEVICE_VARIABLE_STATISTICS
        if (persistent)
        {
            // TODO add support for persistent variables list
            return false;
        }
        return Instance().mVariableMap.GetOccupiedKeysNames(keyList, bufferSize);
#endif
        return false;
    }

    static VariableStats & Instance()
    {
        static VariableStats sInstance;
        return sInstance;
    }

private:
    // TODO parametrize size of the variable map
#if CHIP_DEVICE_VARIABLE_STATISTICS
    VariablesMap<uint32_t, 10> mVariableMap;
#endif
};

} // namespace chip
