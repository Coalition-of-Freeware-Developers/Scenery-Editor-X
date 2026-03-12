/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* platform.h
* -------------------------------------------------------
* Created: 27/12/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/platform/platform_context.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	enum class ExitCode
	{
		Success = 0,	  // App exited successfully
		Close,			  // App has been requested to close at initialization
		FatalError = 255  // App encountered an unexpected error
	};

	class Platform
	{
	public:
		Platform(const PlatformContext &context);
		virtual ~Platform() = default;

		//virtual ExitCode Init(int argc, char **argv) = 0;
		
		/**
		 * @brief Handles the main update and render loop
		 * @return An exit code representing the outcome of the loop
		 */
		ExitCode MainLoop();

		/**
		 * @brief Handles the update and render of a frame.
		 * Called either from MainLoop(), or from a platform-specific
		 * frame-looping mechanism, typically tied to platform screen refreshes.
		 * @return An exit code representing the outcome of the loop
		 */
		ExitCode MainLoopFrame();

		/**
		 * @brief Runs the application for one frame
		 */
		void Update();

		/**
		 * @brief Terminates the platform and the application
		 * @param code Determines how the platform should exit
		 */
		virtual void Terminate(ExitCode code);

		/**
		 * @brief Requests to close the platform at the next available point
		 */
		virtual void Close();
	};

}

// -------------------------------------------------------
