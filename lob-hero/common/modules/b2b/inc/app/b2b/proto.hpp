/**
 * @file proto.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief 板间定长通信协议基本框架
 * @note 变长可以自行拦截传输层得到的数据, 从而支持communicator模块对接
 * @version 0.1
 * @date 2026-11-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "basis.hpp"
#include <etl/delegate.h>
#include <etl/ranges.h>
#include <etl/span.h>
#include <memory>
#include <utility>

namespace app::b2b
{

#include "_proto_details.hpp"

/**
 * @brief 表示定长协议支持的所有消息. 纯编译期标记类型.
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

template <details::Message TMsg>
using Callback = etl::delegate<void(const TMsg&)>;

template <details::ValidMessages... TMsgs>
using CallbackTable = std::tuple<Callback<TMsgs>...>;

template <typename TAvailable>
class Encoder;

template <typename... TMsgs>
class Encoder<AvailableMessages<TMsgs...>>
{
public:
	template <details::Message TMsg>
	    requires(details::MessageOf<TMsg, TMsgs...>)
	TransferInfo Encode(const TMsg& msg)
	{
		constexpr std::uint16_t ID = TMsg::ID;
		return {.Id = ID, .Data = {reinterpret_cast<std::byte*>(&msg), sizeof(TMsg)}};
	}
};

template <details::Message... TMsgs>
    requires(sizeof...(TMsgs) > 0 && details::UniqueIds<TMsgs...>)
class Decoder
{
public:
	using Callbacks = CallbackTable<TMsgs...>;

	template <typename TAvailable>
	    requires(details::HandledMessagesAreAvailable<TAvailable, TMsgs...>)
	explicit Decoder(Callbacks callbacks)
	    : _callbacks(callbacks)
	{}

	/**
	 * @brief 根据处理表遍历查找并处理.
	 * @note 代码相当于手写一个大switch, 只是自动化了
	 *
	 * @param transfer 传输层数据
	 * @return bool 该消息是否支持被处理
	 */
	bool Decode(TransferInfo transfer) { return DecodeImpl(transfer, std::index_sequence_for<TMsgs...>{}); };

private:
	Callbacks _callbacks;

	template <std::size_t... Is>
	bool DecodeImpl(TransferInfo transfer, std::index_sequence<Is...>)
	{
		return (TryDecodeSingle<Is, TMsgs>(transfer) || ...);
	}

	template <std::size_t Index, typename Msg>
	bool TryDecodeSingle(TransferInfo transfer)
	{
		if (transfer.Id == Msg::ID)
		{
			// 仅长度匹配才触发解码
			if (transfer.Data.size_bytes() == sizeof(Msg))
			{
				const auto& msg = *reinterpret_cast<const Msg*>(transfer.Data.data());
				std::get<Index>(_callbacks)(msg);
			}
			return true;
		}
		return false;
	}
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
