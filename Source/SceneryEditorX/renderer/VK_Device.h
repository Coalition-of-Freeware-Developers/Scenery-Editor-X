/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_device.h
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <GLFW/glfw3.h>
#include <SceneryEditorX/renderer/vk_core.h>

// -------------------------------------------------------

class VulkanDevice : public GraphicsEngine
{
public:
	VulkanDevice(GLFWwindow* window);
	~VulkanDevice();

	// -----------------------------------------

	void CreateDevice();
	void DestroyDevice();

};