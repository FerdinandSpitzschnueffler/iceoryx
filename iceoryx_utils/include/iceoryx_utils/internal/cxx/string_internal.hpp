// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef IOX_UTILS_CXX_STRING_INTERNAL_HPP
#define IOX_UTILS_CXX_STRING_INTERNAL_HPP

#include <cstdint>
#include <cstring>

namespace iox
{
namespace cxx
{
template <uint64_t>
class string;

namespace internal
{
template <uint64_t N>
using charTemp = char[N];

/// @brief struct to get capacity of fixed string/string literal
template <typename T>
struct GetCapa;

template <uint64_t N>
struct GetCapa<string<N>>
{
    static constexpr uint64_t capa = N;
};

template <uint64_t N>
struct GetCapa<char[N]>
{
    static constexpr uint64_t capa = N - 1;
};

/// @brief struct to get size of fixed string/string literal
template <typename T>
struct GetSize;

template <uint64_t N>
struct GetSize<string<N>>
{
    static uint64_t call(const string<N>& data)
    {
        return data.size();
    }
};

template <uint64_t N>
struct GetSize<char[N]>
{
    static uint64_t call(const charTemp<N>&)
    {
        return N - 1u;
    }
};

/// @brief struct to get a pointer to the char array of the fixed string/string literal
template <typename T>
struct GetData;

template <uint64_t N>
struct GetData<string<N>>
{
    static const char* call(const string<N>& data)
    {
        return data.c_str();
    }
};

template <uint64_t N>
struct GetData<char[N]>
{
    static const char* call(const charTemp<N>& data)
    {
        return &data[0];
    }
};

/// @brief struct to get the sum of the capacities of fixed strings/string literals
template <typename... Targs>
struct SumCapa;

template <>
struct SumCapa<>
{
    static constexpr uint64_t value = 0;
};

template <typename T, typename... Targs>
struct SumCapa<T, Targs...>
{
    static constexpr uint64_t value = GetCapa<T>::capa + SumCapa<Targs...>::value;
};
} // namespace internal
} // namespace cxx
} // namespace iox
#endif // IOX_UTILS_CXX_STRING_INTERNAL_HPP