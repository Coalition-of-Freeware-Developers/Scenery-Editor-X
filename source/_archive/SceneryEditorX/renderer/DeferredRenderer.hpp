#pragma once

#include <core/Base.hpp>

namespace DeferredShading
{

struct OpaqueConstants
{
    int sceneBufferIndex;
    int modelBufferIndex;
    int modelID;
};

struct LightConstants
{
    int sceneBufferIndex;
    int frameID;
    int albedoRID;
    int normalRID;
    int materialRID;
    int emissionRID;
    int depthRID;
};

void CreateShaders();
void CreateImages(uint32_t width, uint32_t height);
void Destroy();

void RenderMesh(RID meshId);
void LightPass(LightConstants constants);
void ComposePass();
void BeginOpaquePass();
void EndPass();
void BeginPresentPass();
void EndPresentPass();
void OnImgui(int numFrame);

void ViewportOnImGui();

} // namespace DeferredShading
