#pragma once

#include<future>

template<typename TCommand, typename TResponse>
class CommandHandler
{
public:
	virtual ~CommandHandler() = default;

	virtual std::future<TResponse> ExecuteAsync(const TCommand& command) = 0;
};
