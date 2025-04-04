/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* cmd_line.cpp
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/cmd_line.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	CommandLineParser::CommandLineParser(int argc, const char* argv[]) {
		Opts_.reserve(argc - 1);
		bool lastWasNamed = false;

        for(int i = 1; i < argc; ++i) {
			Opt newOpt;
			bool longopt = false;
			const char* opt = argv[i];
			const char* colon;

			if(opt[0] == '-') {
			    ++opt;
			    opt += (longopt = (opt[0] == '-'));
			    if(longopt && (colon = strrchr(opt, '='))) {
			        *const_cast<char*>(colon) = 0;
			        newOpt.param = colon + 1;
			    }
			    else lastWasNamed = true;
			}
			//else if(allow_ms && opt[0] == '/') {
			//	newOpt.ms = true;
			//	opt += (longopt = true);
			//	if((colon = strrchr(opt, ':'))) {
			//		*colon = 0;
			//		newOpt.param = colon + 1;
			//	}
			//}
			else {
				if(lastWasNamed) {
					Opts_.back().param = opt;
					lastWasNamed = false;
				}
				else newOpt.raw = true;
			}

			newOpt.name = opt;

			Opts_.emplace_back(newOpt);
		}
	}

	std::vector<std::string_view> CommandLineParser::GetRawArgs() {
		std::vector<std::string_view> result;

		for(auto& opt : Opts_) {
			if(opt.raw) result.emplace_back(opt.name);
		}

		return std::move(result);
	}

	std::string_view CommandLineParser::GetOpt(const std::string& name) {
		for(auto& opt : Opts_) {
			if(opt.name == name && !opt.raw) return opt.param;
		}

		return "";
	}

	bool CommandLineParser::HaveOpt(const std::string& name) {
		return !GetOpt(name).empty();
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
