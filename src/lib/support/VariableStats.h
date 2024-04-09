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

#include "IntrusiveList.h"

#define MAXIMUM_VARIABLE_NAME 10
#define MAXIMUM_VARIABLE_COUNT 10

namespace chip {

template <typename T, size_t N>
struct VariablesMap
{
    static constexpr size_t kNoSlotsFound{ N + 1 };

    struct Item
    {
        char * key[MAXIMUM_VARIABLE_NAME];
        size_t keySize = 0;
        T value;
    };

    bool Set(const char * key, size_t keySize, T value)
    {
        const auto & it = std::find_if(std::begin(mMap), std::end(mMap),
                                       [key](const Item & item) { return memcmp(item.key, key, item.keySize) == 0; });

        if (it != std::end(mMap))
        {
            it->value = std::move(value);
            return true;
        }
        else if (mElementsCount < N)
        {
            size_t slot = GetFirstFreeSlot();
            if (slot == kNoSlotsFound)
            {
                return false;
            }
            memcpy(mMap[slot].key, key, keySize);
            mMap[slot].value = std::move(value);
            mElementsCount++;
            return true;
        }

        return false;
    }

    bool Erase(const char * key, size_t keySize)
    {
        const auto & it = std::find_if(std::begin(mMap), std::end(mMap),
                                       [key](const Item & item) { return memcmp(item.key, key, item.keySize) == 0; });

        if (it != std::end(mMap))
        {
            it->value   = T{};
            it->keySize = 0;
            mElementsCount--;
            return true;
        }
        return false;
    }

    bool Get(const char * key, size_t keySize, T & value)
    {
        for (auto & it : mMap)
        {
            if (memcmp(it.key, key, it.keySize) == 0)
            {
                value = it.value;
                return true;
            }
        }
        return false;
    }

    bool Contains(const char * key, size_t keySize)
    {
        for (auto & it : mMap)
        {
            if (strcmp(key, it.key) == 0 && keySize == it.keySize)
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
            if (it.keySize != 0)
                return foundIndex;
            foundIndex++;
        }
        return kNoSlotsFound;
    }

    Item mMap[N];
    uint16_t mElementsCount{ 0 };
};

template <typename ValueType>
class VariableStats
{
private:
    struct Dictionary
    {
        char * name[MAXIMUM_VARIABLE_NAME + 1];
        size_t nameSize = 0;
        ValueType value;
    };

public:
    static bool Set(char * name, size_t nameSize, ValueType newValue, bool persistent = false)
    {
        return Instance().mVariableMap.Set(name, nameSize, newValue);
    }

    static bool Increment(char * name) { return false; }

    static bool Get(char * name, size_t nameSize, ValueType & value) { return Instance().mVariableMap.Get(name, nameSize, value); }

    static VariableStats & Instance()
    {
        static VariableStats sInstance;
        return sInstance;
    }

private:
    VariableStats() {}

    VariablesMap<ValueType, 10> mVariableMap;
};

} // namespace chip