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
#ifndef IOX_DDS_GATEWAY_GATEWAY_CHANNEL_HPP
#define IOX_DDS_GATEWAY_GATEWAY_CHANNEL_HPP

#include <cstdint>

#include <iceoryx_posh/iceoryx_posh_types.hpp>
#include <iceoryx_posh/popo/subscriber.hpp>
#include <iceoryx_utils/internal/objectpool/objectpool.hpp>

#include "ioxdds/dds/data_writer.hpp"
#include "ioxdds/dds/dds_configs.hpp"
#include "ioxdds/dds/dds_types.hpp"

namespace iox
{
namespace dds
{
///
/// @brief This data structure couples the subscriber and data writer components required to form a channel between
/// the POSH and DDS worlds.
/// The structure holds pointers to the related component instances. These instances can either be managed externally
/// or by the class itself.
/// Managed instances are placed in static ObjectPools which are fixed size according to
/// MAX_CHANNEL_NUMBER and are automatically cleaned up when all references to them are discarded.
///
template <typename subscriber_t = iox::popo::Subscriber, typename data_writer_t = iox::dds::data_writer_t>
class Channel
{
  public:
    using SubscriberPtr = std::shared_ptr<subscriber_t>;
    using SubscriberPool = iox::cxx::ObjectPool<subscriber_t, MAX_CHANNEL_NUMBER>;
    using DataWriterPtr = std::shared_ptr<data_writer_t>;
    using DataWriterPool = iox::cxx::ObjectPool<data_writer_t, MAX_CHANNEL_NUMBER>;

    ///
    /// @brief Channel Constructs a channel object with components that are externally managed.
    ///
    /// @param service The service that the channel is connecting.
    /// @param subscriber An externally managed subscriber component.
    /// @param dataWriter An externally managed data writer component.
    ///
    Channel(const iox::capro::ServiceDescription& m_service,
            const SubscriberPtr m_subscriber,
            const DataWriterPtr m_dataWriter) noexcept;

    ///
    /// @brief create Creates a channel whose components are instantiated in static object pools that will be
    /// automatically freed when all references are discarded.
    ///
    /// @param service The service that the channel is connecting.
    /// @return Channel A channel with internally managed endpoints.
    ///
    static Channel create(const iox::capro::ServiceDescription& m_service) noexcept;

    iox::capro::ServiceDescription getService() const noexcept;
    SubscriberPtr getSubscriber() const noexcept;
    DataWriterPtr getDataWriter() const noexcept;

  private:
    // Store in data segment - too large to keep in stack.
    static SubscriberPool s_subscriberPool;
    static DataWriterPool s_dataWriterPool;

    iox::capro::ServiceDescription m_service;
    SubscriberPtr m_subscriber;
    DataWriterPtr m_dataWriter;
};

} // dds
} // iox

#include "ioxdds/internal/gateway/channel.inl"

#endif // IOX_DDS_GATEWAY_GATEWAY_CHANNEL_HPP
