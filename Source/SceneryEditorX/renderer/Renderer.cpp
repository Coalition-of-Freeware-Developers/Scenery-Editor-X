#include "Renderer.h"

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::Init(Scene* scene)
{
    SetUniforms();
    SetShaders();
}

void Renderer::SetUniforms()
{
    // common
    RendererBasic::GetUniforms().insert(std::make_pair("model", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("view", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("projection", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("nearPlane", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("farPlane", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("dirLightTransform", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("normalMap", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("lightPosition", 0));

    // main
    RendererBasic::GetUniforms().insert(std::make_pair("eyePosition", 0));

    // water
    //RendererBasic::GetUniforms().insert(std::make_pair("reflectionTexture", 0));
    //RendererBasic::GetUniforms().insert(std::make_pair("refractionTexture", 0));
    //RendererBasic::GetUniforms().insert(std::make_pair("dudvMap", 0));
    //RendererBasic::GetUniforms().insert(std::make_pair("depthMap", 0));
    //RendererBasic::GetUniforms().insert(std::make_pair("moveFactor", 0));
    //RendererBasic::GetUniforms().insert(std::make_pair("cameraPosition", 0));
    //RendererBasic::GetUniforms().insert(std::make_pair("lightColor", 0));

    // PBR - physically based rendering
    RendererBasic::GetUniforms().insert(std::make_pair("albedo", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("metallic", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("roughness", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("ao", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("albedoMap", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("normalMap", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("metallicMap", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("roughnessMap", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("aoMap", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("camPos", 0));
    RendererBasic::GetUniforms().insert(std::make_pair("ambientIntensity", 0));

    // cubemap shader
    RendererBasic::GetUniforms().insert(std::make_pair("equirectangularMap", 0));

    // skybox shader
    RendererBasic::GetUniforms().insert(std::make_pair("environmentMap", 0));

}

void Renderer::SetShaders()
{

}

void Renderer::RenderPassShadow(Window *mainWindow, Scene *scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableShadows) return;
}


void Renderer::BeginFrame()
{
}

void Renderer::WaitAndRender(float deltaTime,
                             Window *mainWindow,
                             Scene *scene,
                             glm::mat4 projectionMatrix)
{
    RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

    RenderPassShadow(mainWindow, scene, projectionMatrix);
    RenderOmniShadows(mainWindow, scene, projectionMatrix);
    RenderWaterEffects(deltaTime, mainWindow, scene, projectionMatrix);
    RenderPassMain(mainWindow, scene, projectionMatrix);
}
