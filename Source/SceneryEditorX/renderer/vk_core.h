/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_core.h
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <SceneryEditorX/core/window.h>

// -------------------------------------------------------


class GraphicsEngine : public Window
{
public:
	GraphicsEngine(GLFWwindow& window) : Window(), m_Window(window) {}
	virtual ~GraphicsEngine() {};

	// -----------------------------------------

	void initEngine();

private:
	void create_instance();
	void create_debug_callback();
	void create_surface();
	void select_physical_device();
	void create_device();
	void create_destruction_handler();
	void create_command_pool();
	void create_sync_objects();
	void create_descriptors();

	// -----------------------------------------

	VkApplicationInfo m_AppInfo;
	VkInstanceCreateInfo m_InstanceInfo;
	VkDebugUtilsMessengerEXT m_DebugMessenger;

	VkInstance m_Instance = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device = VK_NULL_HANDLE;

	uint32_t m_QueueFamily = 0;
	VkSurfaceFormatKHR m_SurfaceFormat = {};

	std::vector<VkImage> Images;
	std::vector<VkImageView> ImageViews;

	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

	VkCommandPool CmdBuffPool_ = VK_NULL_HANDLE;
	VkCommandBuffer CpyCmdBuff = VK_NULL_HANDLE;
	//RenderQueue ImgQueue;

	// -----------------------------------------

protected:
	GLFWwindow& m_Window;
	uint32_t m_CurrentImageIndex = 0;
	uint32_t m_CurrentFrame = 0;
	uint64_t m_FrameCount = 0;
};