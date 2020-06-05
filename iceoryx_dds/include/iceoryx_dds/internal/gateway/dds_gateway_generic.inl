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

#include <cpptoml.h>

#include <iceoryx_utils/internal/file_reader/file_reader.hpp>
#include "iceoryx_dds/internal/log/logging.hpp"
#include "iceoryx_dds/dds/dds_types.hpp"
#include "iceoryx_dds/gateway/dds_gateway_generic.hpp"


// ================================================== Public ================================================== //

template<typename channel_t, typename gateway_t>
inline iox::dds::DDSGatewayGeneric<channel_t, gateway_t>::~DDSGatewayGeneric() noexcept
{
    shutdown();
}

template<typename channel_t, typename gateway_t>
inline void iox::dds::DDSGatewayGeneric<channel_t, gateway_t>::runMultithreaded() noexcept
{
    m_discoveryThread = std::thread([this] { discoveryLoop(); });
    m_forwardingThread = std::thread([this] { forwardingLoop(); });
    m_isRunning.store(true, std::memory_order_relaxed);
}

template<typename channel_t, typename gateway_t>
inline void iox::dds::DDSGatewayGeneric<channel_t, gateway_t>::shutdown() noexcept
{
    if (m_isRunning.load(std::memory_order_relaxed))
    {
        iox::dds::LogDebug() << "[DDSGatewayGeneric] Shutting down Posh2DDSGateway.";

        m_runDiscoveryLoop.store(false, std::memory_order_relaxed);
        m_runForwardingLoop.store(false, std::memory_order_relaxed);

        m_discoveryThread.join();
        m_forwardingThread.join();

        m_isRunning.store(false, std::memory_order_relaxed);
    }
}

template<typename channel_t, typename gateway_t>
inline uint64_t iox::dds::DDSGatewayGeneric<channel_t, gateway_t>::getNumberOfChannels() const noexcept
{
    auto guardedVector = m_channels.GetScopeGuard();
    return guardedVector->size();
}

// ================================================== Protected ================================================== //

template<typename channel_t, typename gateway_t>
inline iox::dds::DDSGatewayGeneric<channel_t, gateway_t>::DDSGatewayGeneric() noexcept : gateway_t(iox::capro::Interfaces::DDS)
{
     LogDebug() << "[DDSGatewayGeneric] Using default channel factory.";
    m_channelFactory = channel_t::create;
}

template<typename channel_t, typename gateway_t>
inline channel_t iox::dds::DDSGatewayGeneric<channel_t, gateway_t>::addChannel(const iox::capro::ServiceDescription& service) noexcept
{
    auto channel = m_channelFactory(service);
    m_channels->push_back(channel);
    iox::dds::LogDebug() << "[DDSGatewayGeneric] Channel set up for service: "
                    << "/" << service.getInstanceIDString() << "/" << service.getServiceIDString() << "/"
                    << service.getEventIDString();
    return channel;
}

template<typename channel_t, typename gateway_t>
inline iox::cxx::optional<channel_t> iox::dds::DDSGatewayGeneric<channel_t, gateway_t>::findChannel(const iox::capro::ServiceDescription& service) noexcept
{
    auto guardedVector = this->m_channels.GetScopeGuard();
    auto channel = std::find_if(
        guardedVector->begin(), guardedVector->end(), [&service](const channel_t& channel) {
            return channel.getService() == service;
        });
    if(channel == guardedVector->end())
    {
        return iox::cxx::nullopt_t();
    }
    else
    {
        // Needs to be dereferenced since the smart lock returns pointers to elements in concurrent collections.
        return iox::cxx::make_optional<channel_t>(*channel);
    }
}

template<typename channel_t, typename gateway_t>
inline void iox::dds::DDSGatewayGeneric<channel_t, gateway_t>::forEachChannel(const std::function<void(channel_t&)> f) noexcept
{
    auto guardedVector = m_channels.GetScopeGuard();
    for (auto channel = guardedVector->begin(); channel != guardedVector->end(); ++channel)
    {
        f(*channel);
    }
}

template<typename channel_t, typename gateway_t>
inline void iox::dds::DDSGatewayGeneric<channel_t, gateway_t>::discardChannel(const iox::capro::ServiceDescription& service) noexcept
{
    auto guardedVector = this->m_channels.GetScopeGuard();
    auto channel = std::find_if(
        guardedVector->begin(), guardedVector->end(), [&service](const channel_t& channel) {
            return channel.getService() == service;
        });
    if (channel != guardedVector->end())
    {
        guardedVector->erase(channel);
        iox::dds::LogDebug() << "[DDSGatewayGeneric] Channel taken down for service: "
                        << "/" << service.getInstanceIDString() << "/" << service.getServiceIDString() << "/"
                        << service.getEventIDString();
    }
}

// ================================================== Private ================================================== //

template<typename channel_t, typename gateway_t>
inline void iox::dds::DDSGatewayGeneric<channel_t, gateway_t>::discoveryLoop() noexcept
{
    iox::dds::LogDebug() << "[DDSGatewayGeneric] Starting discovery.";
    m_runDiscoveryLoop.store(true, std::memory_order_relaxed);
    while (m_runDiscoveryLoop.load(std::memory_order_relaxed))
    {
        iox::capro::CaproMessage msg;
        while (this->getCaProMessage(msg))
        {
            discover(msg);
        }
        std::this_thread::sleep_until(std::chrono::steady_clock::now()
                                      + std::chrono::milliseconds(DISCOVERY_PERIOD.milliSeconds<int64_t>()));
    }
    iox::dds::LogDebug() << "[DDSGatewayGeneric] Stopped discovery.";
}

template<typename channel_t, typename gateway_t>
inline void iox::dds::DDSGatewayGeneric<channel_t, gateway_t>::forwardingLoop() noexcept
{
    iox::dds::LogDebug() << "[DDSGatewayGeneric] Starting forwarding.";
    m_runForwardingLoop.store(true, std::memory_order_relaxed);
    while (m_runForwardingLoop.load(std::memory_order_relaxed))
    {
        forward();
        std::this_thread::sleep_until(std::chrono::steady_clock::now()
                                      + std::chrono::milliseconds(FORWARDING_PERIOD.milliSeconds<int64_t>()));
    };
    iox::dds::LogDebug() << "[DDSGatewayGeneric] Stopped forwarding.";
}
