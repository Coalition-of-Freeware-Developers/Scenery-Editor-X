/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_pass.cpp
* -------------------------------------------------------
* Created: 18/8/2025
* -------------------------------------------------------
*/
#include "render_pass.h"
#include "SceneryEditorX/core/application/application.h"
#include "swapchain.h"
#include "vulkan_utils.h"
#include "SceneryEditorX/logging/logging.hpp"

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief Main frame production pipeline coordinating all render passes
	 * 
	 * This is the central orchestrator that executes all render passes in proper order
	 * to produce a complete frame. It coordinates both graphics and compute command lists.
	 * 
	 * @param cmd_list_graphics_present Command manager for graphics/presentation queue
	 * @param cmd_list_compute Command manager for compute operations (can be null if no compute)
	 */
	void RenderPass::ProduceFrame(CommandManager *cmd_list_graphics_present, CommandManager *cmd_list_compute)
	{
		// For now, just clear the screen without using CommandManager
		// TODO: Properly integrate CommandManager when time-block methods are implemented
		
		// Clear the screen to a default color
		Pass_Clear(cmd_list_graphics_present);
		
		// TODO: Add additional render passes here as they are implemented
		// Pass_Depth_Prepass(cmd_list_graphics_present);
		// Pass_GBuffer(cmd_list_graphics_present, false);
		// Pass_Light(cmd_list_graphics_present, false);
		// Pass_PostProcess(cmd_list_graphics_present);
	}
	
	/**
	 * @brief Clear screen render pass
	 * 
	 * Clears the current framebuffer to a solid color. This is the most basic
	 * render pass and ensures the screen starts from a known state each frame.
	 * 
	 * @param cmd_list Command manager to record clear commands into
	 */
	void RenderPass::Pass_Clear(CommandManager* cmd_list)
	{
		// TODO: Implement proper SwapChain access through CommandManager or Renderer
		// The Window class does not have a GetSwapChain() method - need to refactor
		// this to use the proper rendering architecture once it's fully implemented
		
		SEDX_CORE_WARN_TAG("RenderPass", "Pass_Clear not yet implemented - SwapChain access needs refactoring");
		
		/* Commented out until SwapChain access is properly implemented
		// Get the active swapchain image to clear
		SwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();
		VkCommandBuffer commandBuffer = swapChain.GetActiveDrawCommandBuffer();
		
		// Setup clear values (cornflower blue: RGB(100, 149, 237) normalized)
		VkClearValue clearColor = {};
		clearColor.color = {{0.392f, 0.584f, 0.929f, 1.0f}}; // Cornflower blue
		
		// Begin render pass with clear operation
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = swapChain.GetRenderPass();
		renderPassInfo.framebuffer = swapChain.GetActiveFramebuffer();
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapChain.GetSwapExtent();
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		
		// Begin the render pass (this performs the clear due to VK_ATTACHMENT_LOAD_OP_CLEAR)
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		// TODO: Add actual rendering commands here in future passes
		
		// End the render pass
		vkCmdEndRenderPass(commandBuffer);
		*/
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
