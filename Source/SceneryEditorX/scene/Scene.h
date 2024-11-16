#pragma once

namespace SceneryEditorX
{
    enum class SceneViewType
    {
        SCENE_2D,
        SCENE_3D,
        ASSET_EDITING,
        LIBRARY_EDITING
    };

    enum class SceneType
    {
        airport,
        city,
        landscape
    };

    struct SunLight
    {
        glm::vec3 direction = {0.0f, 0.0f, 0.0f};
        glm::vec3 radiance = {0.0f, 0.0f, 0.0f};
        float intensity = 0.0f;
        float shadowPower = 1.0f;
        bool castShadows = true;
    };

    struct SceneProperties
    {
        std::string Name = "Untitled Scene";
        bool initialize = true;
    };

    class Scene
    {
    public:

        Scene(SceneType, SceneViewType);
        ~Scene();

        float GetSceneTime() const {return m_SceneTime;}
        void SetSceneTime(float time) {m_SceneTime = time;}

        float GetSceneWeather() const {return m_SceneWeather;}
        void SetSceneWeather(float weather) {m_SceneWeather = weather;}

        uint32_t GetViewportWidth() const {return ViewportWidth;}
        uint32_t GetViewportHeight() const {return ViewportHeight;}

    public:
        static Ref<Scene> CreateEmpty();

    private:

        SceneViewType m_SceneViewType;
    };
}


