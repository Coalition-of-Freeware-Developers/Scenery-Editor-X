/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* cmd_line.h
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/
#pragma once

namespace SceneryEditorX
{
	class CommandLineParser
	{
	public:
#ifdef SEDX_PLATFORM_WINDOWS
        CommandLineParser(int argc, const char *argv[]);
#else
        CommandLineParser(int argc, char **argv, bool allow_ms = false);
#endif
        std::vector<std::string_view> GetRawArgs();
        std::string_view GetOpt(const std::string &name);
        bool HaveOpt(const std::string &name);

    private:
        struct Opt
        {
            bool raw;
            bool ms;
            std::string_view name;
            std::string_view param;
        };
        std::vector<Opt> Opts_;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------

