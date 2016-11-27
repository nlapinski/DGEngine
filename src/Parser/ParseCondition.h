#pragma once

#include "Actions/Action.h"
#include "Game.h"
#include "Json/JsonParser.h"

namespace Parser
{
	std::shared_ptr<Action> getIfCondition(unsigned conditionHash,
		Game& game, const rapidjson::Value& elem);

	std::shared_ptr<Action> getInListCondition(Game& game, const rapidjson::Value& elem);

	std::shared_ptr<Action> getSwitchCondition(Game& game, const rapidjson::Value& elem);
}
