/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* console_message.h
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/
#pragma once

#include <ctime>
#include <SceneryEditorX/core/base.hpp>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX
{
	enum class ConsoleMessageFlags : int16_t
	{
		None = -1,
		Info = BIT(0),
		Warning = BIT(1),
		Error = BIT(2),

		All = Info | Warning | Error
	};

	struct ConsoleMessage
	{
		std::string ShortMessage;
		std::string LongMessage;
		int16_t Flags;
		time_t Time;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------
