/**
 * @file proto.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief 板间通信协议基本框架(can总线)
 * @note 仅用于can总线通信. 非can总线可以考虑直接使用communicator模块, 此通信设计上用于利用can的id机制进行优先级设定.
 * @version 0.1
 * @date 2026-11-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <etl/delegate.h>
#include <etl/span.h>

namespace app::b2b
{

#include "_proto_details.hpp"

struct TransferInfo
{
	std::uint16_t Id;
	etl::span<const std::byte> Data;
};

/**
 * @brief 表示协议支持的所有消息.纯编译期标记类型.
 *
 * @tparam TMsgs 所有消息类型
 */
template <details::ValidMessages... TMsgs>
    requires(sizeof...(TMsgs) > 0 && details::UniqueIds<TMsgs...>)
struct AvailableMessages
{
	template <typename TMsg>
	static consteval bool Contains()
	{
		return details::MessageOf<TMsg, TMsgs...>;
	}
};

template <typename... TMsgs>
using CallbackTable = std::tuple<etl::delegate<void(const TMsgs&)>...>;

template <typename TAvailable>
class Encoder;

template <typename... TMsgs>
class Encoder<AvailableMessages<TMsgs...>>
{
public:
	template <details::Message TMsg>
	    requires(details::MessageOf<TMsg, TMsgs...>)
	TransferInfo Encode(const TMsg& msg);
};

template <details::Message... THandledMsgs>
    requires(sizeof...(THandledMsgs) > 0 && details::UniqueIds<THandledMsgs...>)
class Decoder
{
public:
	using Callbacks = CallbackTable<THandledMsgs...>;

	template <typename TAvailable>
	    requires(details::HandledMessagesAreAvailable<TAvailable, THandledMsgs...>)
	explicit Decoder(Callbacks callbacks)
	    : _callbacks(callbacks)
	{}

private:
	Callbacks _callbacks;
};

/**
 * @brief 构造 `Decoder`. 充分利用编译器推断.
 * @code {.cpp}
 *
    struct MsgA
    {
        static constexpr std::uint16_t ID = 0x100;
    };
    struct MsgB
    {
        static constexpr std::uint16_t ID = 0x101;
    };

    struct MsgC
    {
        static constexpr std::uint16_t ID = 0x102;
    };

    using Avail = app::b2b::AvailableMessages<MsgA, MsgB>;

    app::b2b::Encoder<Avail> encoder;
    auto cb_a = [](const MsgA&) {};
    auto cb_b = [](const MsgB&) {};
    auto decoder = app::b2b::MkDecoder<Avail>(std::tuple{
        etl::make_delegate(cb_a),
        etl::make_delegate(cb_b),
    });

 * @endcode
 *
 *
 * @tparam TAvailable
 * @tparam THandledMsgs
 */
template <typename TAvailable, details::Message... THandledMsgs>
    requires(details::HandledMessagesAreAvailable<TAvailable, THandledMsgs...>)
Decoder<THandledMsgs...> MkDecoder(CallbackTable<THandledMsgs...> callbacks)
{
	return {callbacks};
}

} // namespace app::b2b
