#pragma once

#include <concepts>
#include <cstdint>
#include <etl/delegate.h>
#include <type_traits>

namespace details
{
/**
 * @brief 协议支持的消息. 需要持有`ID`编译常量, 且在CAN ID支持范围内
 */
template <typename T>
concept Message = requires {
	{ T::ID } -> std::convertible_to<std::uint16_t>;
} && T::ID > 0 && T::ID < 2048;

/**
 * @brief 协议消息Id的独立性校验. 拦截重复Id.
 */
template <typename... Ts>
concept UniqueIds = []() consteval -> bool {
	constexpr std::uint16_t IDS[] = {Ts::ID...};
	for (std::size_t i = 0; i < sizeof...(Ts); ++i)
	{
		for (std::size_t j = i + 1; j < sizeof...(Ts); ++j)
		{
			if (IDS[i] == IDS[j])
			{
				return false;
			}
		}
	}
	return true;
}();

template <typename T, typename... Ts>
concept MessageOf = (std::same_as<T, Ts> || ...);
template <typename... TMsgs>
concept ValidMessages = (Message<TMsgs> && ...) && UniqueIds<TMsgs...>;
template <typename TAvailable, typename... THandledMsgs>
concept HandledMessagesAreAvailable = (TAvailable::template Contains<THandledMsgs>() && ...);

} // namespace details
