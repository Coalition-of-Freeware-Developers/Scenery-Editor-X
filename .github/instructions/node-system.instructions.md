# Node System Development Instructions for GitHub Copilot

This file provides specific instructions for GitHub Copilot when working with the Node system in Scenery Editor X. Follow these guidelines to ensure consistency with the existing codebase architecture and coding standards.

## Core Principles

### 1. Always Inherit from Node Base Class

**For scene objects, always inherit from the Node base class:**

```cpp
class CustomSceneObject : public Node
{
public:
    CustomSceneObject()
    {
        type = ObjectType::CustomObject; // Set appropriate type
    }
    
    // Override serialization if needed
    void Serialize(Serializer& ser) override
    {
        Node::Serialize(ser); // Always call base class first
        ser("customProperty", m_CustomProperty);
    }
    
private:
    float m_CustomProperty = 1.0f;
};
```

### 2. Use AssetManager for Object Creation

**Never use direct constructors or new. Always use AssetManager:**

```cpp
// CORRECT: Use AssetManager for creation
auto meshNode = assetManager.CreateObject<MeshNode>("MyMesh");
auto lightNode = assetManager.CreateObject<LightNode>("MyLight");

// INCORRECT: Direct instantiation
auto badNode = CreateRef<MeshNode>(); // Don't do this
auto badNode2 = std::make_shared<MeshNode>(); // Don't do this
```

### 3. Smart Pointer Usage with Nodes

**Always use the custom Ref<T> system for Node references:**

```cpp
// Node storage and references
Ref<Node> m_RootNode;
std::vector<Ref<MeshNode>> m_MeshNodes;
WeakRef<Node> m_ParentRef; // Use WeakRef to avoid circular references when needed

// Node creation and assignment
m_RootNode = assetManager.CreateObject<Node>("Root");
auto childNode = assetManager.CreateObject<MeshNode>("Child");

// Set parent-child relationships safely
Node::SetParent(childNode, m_RootNode);
```

### 4. Transformation Best Practices

**Use the transformation API correctly:**

```cpp
// Setting transformations (always use local space)
node->position = Vec3(1.0f, 0.0f, 0.0f);
node->rotation = Vec3(0.0f, 45.0f, 0.0f); // Degrees, not radians
node->scale = Vec3(2.0f, 1.0f, 1.0f);

// Getting transformations
Mat4 localTransform = node->GetLocalTransform();
Mat4 worldTransform = node->GetWorldTransform();
Vec3 worldPosition = node->GetWorldPosition();
Vec3 worldForward = node->GetWorldFront();

// Compose transforms manually when needed
Mat4 composed = Node::ComposeTransform(position, rotation, scale, parentTransform);
```

### 5. Hierarchy Management Patterns

**Use safe hierarchy operations:**

```cpp
// Safe parent-child relationship setup
if (childNode && parentNode)
{
    Node::SetParent(childNode, parentNode);
}

// Update parent references after loading/deserialization
Node::UpdateChildrenParent(rootNode);

// Type-safe node queries
auto lights = scene->GetAll<LightNode>(ObjectType::LightNode);
auto meshes = rootNode->GetAll<MeshNode>(ObjectType::MeshNode);

// Traversing hierarchy safely
void ProcessNodeHierarchy(Ref<Node> node)
{
    if (!node) return;
    
    // Process current node
    ProcessNode(node);
    
    // Process children
    for (auto& child : node->children)
    {
        ProcessNodeHierarchy(child);
    }
}
```

## Specialized Node Types

### MeshNode Implementation

```cpp
class CustomMeshNode : public Node
{
public:
    CustomMeshNode()
    {
        type = ObjectType::MeshNode;
    }
    
    void Serialize(Serializer& ser) override
    {
        Node::Serialize(ser);
        ser.Asset("mesh", mesh);
        ser.Asset("material", material);
        ser("customMeshProperty", m_CustomProperty);
    }
    
    // Custom mesh-specific functionality
    void UpdateMeshData()
    {
        if (mesh && mesh->IsLoaded())
        {
            // Update mesh-specific data
            gpuDirty = true; // Mark for GPU update
        }
    }
    
private:
    Ref<ModelAsset> mesh;
    Ref<MaterialAsset> material;
    float m_CustomProperty = 1.0f;
};
```

### LightNode Implementation

```cpp
class CustomLightNode : public Node
{
public:
    CustomLightNode()
    {
        type = ObjectType::LightNode;
    }
    
    void Serialize(Serializer& ser) override
    {
        Node::Serialize(ser);
        ser("color", color);
        ser("intensity", intensity);
        ser("lightType", lightType);
        ser("customLightProperty", m_CustomProperty);
    }
    
    // Light-specific functionality
    void UpdateLightParameters()
    {
        // Validate light parameters
    intensity = std::max(0.0f, intensity);
    color = Clamp(color, Vec3(0.0f), Vec3(1.0f));
    }
    
private:
    Vec3 color = Vec3(1.0f);
    float intensity = 10.0f;
    LightType lightType = LightType::Point;
    bool m_CustomProperty = false;
};
```

### CameraNode Implementation

```cpp
class CustomCameraNode : public Node
{
public:
    CustomCameraNode()
    {
        type = ObjectType::CameraNode;
    }
    
    void Serialize(Serializer& ser) override
    {
        Node::Serialize(ser);
        ser("cameraType", cameraType);
        ser("horizontalFov", horizontalFov);
        ser("nearDistance", nearDistance);
        ser("farDistance", farDistance);
    }
    
    // Camera matrix calculations
    Mat4 GetViewMatrix()
    {
        Mat4 worldTransform = GetWorldTransform();
    return worldTransform.Inverse();
    }
    
    Mat4 GetProjectionMatrix(float aspectRatio)
    {
        if (cameraType == CameraType::Perspective)
        {
            return Mat4::Perspective(ToRadians(horizontalFov), aspectRatio, nearDistance, farDistance);
        }
        else
        {
            float halfWidth = extent.x * 0.5f;
            float halfHeight = extent.y * 0.5f;
            return Mat4::Orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, orthoNearDistance, orthoFarDistance);
        }
    }
    
private:
    CameraType cameraType = CameraType::Perspective;
    float horizontalFov = 60.0f;
    float nearDistance = 0.01f;
    float farDistance = 1000.0f;
    float orthoNearDistance = -100.0f;
    float orthoFarDistance = 100.0f;
    Vec2 extent = Vec2(10.0f, 10.0f);
};
```

## Module Integration Patterns

### Scene Management Module

```cpp
class SceneModule : public Module
{
public:
    explicit SceneModule(const std::string& name = "SceneModule")
        : Module(name)
    {
    }
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("SCENE", "Initializing {}", GetName());
        
        // Initialize scene management
        m_AssetManager = CreateRef<AssetManager>();
        m_CurrentScene = m_AssetManager->CreateAsset<SceneAsset>("DefaultScene");
        
        // Create default camera
        auto camera = m_AssetManager->CreateObject<CameraNode>("MainCamera");
        camera->position = Vec3(0.0f, 2.0f, 5.0f);
        camera->rotation = Vec3(-20.0f, 0.0f, 0.0f);
        m_CurrentScene->mainCamera = camera;
        m_CurrentScene->Add(camera);
    }
    
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("SCENE", "Cleaning up {}", GetName());
        
        // Cleanup happens automatically with smart pointers
        m_CurrentScene.Reset();
        m_AssetManager.Reset();
    }
    
    void OnUpdate() override
    {
        if (!m_IsEnabled || !m_CurrentScene) return;
        
        SEDX_PROFILE_SCOPE("SceneModule::OnUpdate");
        
        // Update animated nodes
        UpdateAnimatedNodes();
        
        // Update camera if needed
        if (m_CurrentScene->mainCamera)
        {
            UpdateCamera(m_CurrentScene->mainCamera);
        }
    }
    
    void OnUIRender() override
    {
        if (!m_ShowHierarchy) return;
        
        if (ImGui::Begin("Scene Hierarchy", &m_ShowHierarchy))
        {
            if (m_CurrentScene)
            {
                for (auto& node : m_CurrentScene->nodes)
                {
                    RenderNodeInHierarchy(node);
                }
            }
        }
        ImGui::End();
    }
    
private:
    void UpdateAnimatedNodes()
    {
        if (!m_CurrentScene) return;
        
        auto meshNodes = m_CurrentScene->GetAll<MeshNode>(ObjectType::MeshNode);
        for (auto& meshNode : meshNodes)
        {
            // Example: Simple rotation animation
            meshNode->rotation.y += 30.0f * Time::GetDeltaTime();
            if (meshNode->rotation.y > 360.0f)
                meshNode->rotation.y -= 360.0f;
        }
    }
    
    void UpdateCamera(Ref<CameraNode> camera)
    {
        // Camera update logic
        if (camera->mode == CameraMode::Orbit)
        {
            // Orbit camera logic
        }
        else if (camera->mode == CameraMode::Fly)
        {
            // Free-fly camera logic
        }
    }
    
    void RenderNodeInHierarchy(Ref<Node> node)
    {
        if (!node) return;
        
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (node->children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;
            
        bool isOpen = ImGui::TreeNodeEx(node->name.c_str(), flags);
        
        // Context menu for node operations
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
            {
                DeleteNode(node);
            }
            if (ImGui::MenuItem("Duplicate"))
            {
                DuplicateNode(node);
            }
            ImGui::EndPopup();
        }
        
        if (isOpen)
        {
            for (auto& child : node->children)
            {
                RenderNodeInHierarchy(child);
            }
            ImGui::TreePop();
        }
    }
    
    void DeleteNode(Ref<Node> node)
    {
        if (!node || !m_CurrentScene) return;
        
        // Remove from scene
        auto& nodes = m_CurrentScene->nodes;
        nodes.erase(std::remove(nodes.begin(), nodes.end(), node), nodes.end());
        
        // Remove from parent
        if (node->parent)
        {
            auto& siblings = node->parent->children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), node), siblings.end());
        }
    }
    
    Ref<Node> DuplicateNode(Ref<Node> original)
    {
        if (!original) return nullptr;
        
        // Use AssetManager's clone functionality
        auto clone = m_AssetManager->CloneObject(original->type, original);
        
        if (original->parent)
        {
            Node::SetParent(clone.DynamicCast<Node>(), original->parent);
        }
        else
        {
            m_CurrentScene->Add(clone.DynamicCast<Node>());
        }
        
        return clone.DynamicCast<Node>();
    }
    
private:
    Ref<AssetManager> m_AssetManager;
    Ref<SceneAsset> m_CurrentScene;
    bool m_IsEnabled = true;
    bool m_ShowHierarchy = true;
};
```

## Error Handling and Validation

### Node Validation Patterns

```cpp
// Always validate node pointers before use
bool ValidateNode(Ref<Node> node, const std::string& context)
{
    if (!node)
    {
        SEDX_CORE_WARN_TAG("NODE", "Null node in context: {}", context);
        return false;
    }
    
    if (node->uuid == 0)
    {
        SEDX_CORE_WARN_TAG("NODE", "Invalid UUID for node: {}", node->name);
        return false;
    }
    
    return true;
}

// Safe hierarchy operations with validation
bool SafeAddChild(Ref<Node> parent, Ref<Node> child)
{
    if (!ValidateNode(parent, "SafeAddChild::parent") || 
        !ValidateNode(child, "SafeAddChild::child"))
    {
        return false;
    }
    
    // Check for circular references
    Ref<Node> current = parent;
    while (current)
    {
        if (current == child)
        {
            SEDX_CORE_ERROR_TAG("NODE", "Circular reference detected when adding child");
            return false;
        }
        current = current->parent;
    }
    
    try
    {
        Node::SetParent(child, parent);
        return true;
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR_TAG("NODE", "Exception in SafeAddChild: {}", e.what());
        return false;
    }
}
```

### Transform Validation

```cpp
// Validate transformation values
void ValidateTransform(Ref<Node> node)
{
    if (!node) return;
    
    // Check for invalid scale values
    if (Length(node->scale) < 0.001f)
    {
        SEDX_CORE_WARN_TAG("TRANSFORM", "Scale too small for node: {}, resetting to 1.0", node->name);
        node->scale = Vec3(1.0f);
    }
    
    // Check for NaN values
    if (Any(IsNaN(node->position)))
    {
        SEDX_CORE_ERROR_TAG("TRANSFORM", "NaN position detected for node: {}", node->name);
        node->position = Vec3(0.0f);
    }
    
    if (Any(IsNaN(node->rotation)))
    {
        SEDX_CORE_ERROR_TAG("TRANSFORM", "NaN rotation detected for node: {}", node->name);
        node->rotation = Vec3(0.0f);
    }
    
    if (Any(IsNaN(node->scale)))
    {
        SEDX_CORE_ERROR_TAG("TRANSFORM", "NaN scale detected for node: {}", node->name);
        node->scale = Vec3(1.0f);
    }
}
```

## Performance Considerations

### Efficient Node Queries

```cpp
// Cache frequently accessed nodes
class OptimizedSceneModule : public Module
{
private:
    std::unordered_map<ObjectType, std::vector<Ref<Node>>> m_NodeCache;
    bool m_CacheValid = false;
    
public:
    void InvalidateCache()
    {
        m_CacheValid = false;
    }
    
    const std::vector<Ref<Node>>& GetNodesByType(ObjectType type)
    {
        if (!m_CacheValid)
        {
            RebuildCache();
        }
        
        return m_NodeCache[type];
    }
    
private:
    void RebuildCache()
    {
        SEDX_PROFILE_SCOPE("RebuildNodeCache");
        
        m_NodeCache.clear();
        
        if (m_CurrentScene)
        {
            for (auto& node : m_CurrentScene->nodes)
            {
                CacheNodeRecursive(node);
            }
        }
        
        m_CacheValid = true;
    }
    
    void CacheNodeRecursive(Ref<Node> node)
    {
        if (!node) return;
        
        m_NodeCache[node->type].push_back(node);
        
        for (auto& child : node->children)
        {
            CacheNodeRecursive(child);
        }
    }
};
```

### Batch Transform Updates

```cpp
// Batch operations for better performance
void BatchUpdateNodeTransforms(const std::vector<Ref<Node>>& nodes,
                               const std::vector<Mat4>& transforms)
{
    SEDX_PROFILE_SCOPE("BatchUpdateNodeTransforms");
    
    SEDX_CORE_ASSERT(nodes.size() == transforms.size(), 
                      "Node and transform count mismatch");
    
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        if (!nodes[i]) continue;
        
        // Decompose matrix into TRS components
        Vec3 position, rotation, scale;
        DecomposeTransform(transforms[i], position, rotation, scale);
        
        nodes[i]->position = position;
        nodes[i]->rotation = rotation;
        nodes[i]->scale = scale;
    }
}
```

## Serialization Best Practices

### Custom Node Serialization

```cpp
void CustomNode::Serialize(Serializer& ser)
{
    // Always call base class serialization first
    Node::Serialize(ser);
    
    // Serialize custom properties
    ser("customFloat", m_CustomFloat);
    ser("customBool", m_CustomBool);
    ser("customString", m_CustomString);
    
    // Serialize asset references
    ser.Asset("customTexture", m_CustomTexture);
    ser.Asset("customMaterial", m_CustomMaterial);
    
    // Serialize arrays/vectors
    ser.Vector("customData", m_CustomDataVector);
    ser.VectorRef("customNodes", m_CustomNodeReferences);
}
```

### Scene Loading/Saving Patterns

```cpp
class SceneSerializer
{
public:
    static bool SaveScene(Ref<SceneAsset> scene, const std::filesystem::path& path)
    {
        try
        {
            SEDX_CORE_INFO_TAG("SERIALIZE", "Saving scene: {}", path.string());
            
            BinaryStorage storage;
            Json sceneJson;
            
            Serializer serializer(sceneJson, storage, Serializer::SAVE, *AssetManager::Instance());
            serializer.Serialize(scene);
            
            // Write JSON and binary data
            std::ofstream jsonFile(path);
            jsonFile << sceneJson.dump(2);
            
            std::filesystem::path binPath = path;
            binPath.replace_extension(".bin");
            
            std::ofstream binFile(binPath, std::ios::binary);
            binFile.write(reinterpret_cast<const char*>(storage.data.data()), storage.data.size());
            
            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("SERIALIZE", "Failed to save scene: {}", e.what());
            return false;
        }
    }
    
    static Ref<SceneAsset> LoadScene(const std::filesystem::path& path)
    {
        try
        {
            SEDX_CORE_INFO_TAG("SERIALIZE", "Loading scene: {}", path.string());
            
            // Read JSON
            std::ifstream jsonFile(path);
            Json sceneJson;
            jsonFile >> sceneJson;
            
            // Read binary data
            std::filesystem::path binPath = path;
            binPath.replace_extension(".bin");
            
            std::ifstream binFile(binPath, std::ios::binary);
            std::vector<uint8_t> binaryData((std::istreambuf_iterator<char>(binFile)),
                                            std::istreambuf_iterator<char>());
            
            BinaryStorage storage;
            storage.data = std::move(binaryData);
            
            // Deserialize
            Ref<SceneAsset> scene;
            Serializer serializer(sceneJson, storage, Serializer::LOAD, *AssetManager::Instance());
            serializer.Serialize(scene);
            
            // Update parent references
            if (scene)
            {
                for (auto& node : scene->nodes)
                {
                    Node::UpdateChildrenParent(node);
                }
            }
            
            return scene;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("SERIALIZE", "Failed to load scene: {}", e.what());
            return nullptr;
        }
    }
};
```

## Key Reminders for Copilot

1. **Always use AssetManager::CreateObject<T>()** for node creation
2. **Always call Node::Serialize(ser)** first in overridden Serialize methods
3. **Use Node::SetParent()** for safe parent-child relationships
4. **Use type-safe GetAll<T>() methods** for node queries
5. **Always validate node pointers** before use
6. **Use Vec3 for positions, rotations (degrees), and scale**
7. **Use Mat4 for transformation matrices**
8. **Include SEDX_PROFILE_SCOPE()** in performance-critical functions
9. **Use appropriate logging tags** (NODE, SCENE, TRANSFORM, SERIALIZE)
10. **Handle exceptions gracefully** with proper error logging
11. **Mark transforms dirty** when modifying node properties
12. **Use WeakRef<T>** to avoid circular references when needed
13. **Always check for circular references** before setting parent relationships
14. **Update parent references** after deserialization with UpdateChildrenParent()
15. **Use the Module system** for node management components

## Integration with Other Systems

### Renderer Integration

```cpp
// Extract renderable data from scene nodes
void ExtractRenderData(Ref<SceneAsset> scene, RenderData& renderData)
{
    SEDX_PROFILE_SCOPE("ExtractRenderData");
    
    // Extract mesh nodes
    auto meshNodes = scene->GetAll<MeshNode>(ObjectType::MeshNode);
    for (auto& meshNode : meshNodes)
    {
        if (meshNode->mesh && meshNode->material)
        {
            RenderableObject renderableObj;
            renderableObj.mesh = meshNode->mesh;
            renderableObj.material = meshNode->material;
            renderableObj.worldTransform = meshNode->GetWorldTransform();
            renderableObj.nodeUUID = meshNode->uuid;
            
            renderData.renderableObjects.push_back(renderableObj);
        }
    }
    
    // Extract lights
    auto lightNodes = scene->GetAll<LightNode>(ObjectType::LightNode);
    for (auto& lightNode : lightNodes)
    {
        LightData lightData;
        lightData.position = lightNode->GetWorldPosition();
        lightData.color = lightNode->color;
        lightData.intensity = lightNode->intensity;
        lightData.type = lightNode->lightType;
        lightData.worldTransform = lightNode->GetWorldTransform();
        
        renderData.lights.push_back(lightData);
    }
    
    // Set camera data
    if (scene->mainCamera)
    {
        renderData.viewMatrix = scene->mainCamera->GetView();
        renderData.projMatrix = scene->mainCamera->GetProj();
        renderData.cameraPosition = scene->mainCamera->GetWorldPosition();
    }
}
```

This instruction file ensures that GitHub Copilot will generate code that follows the established patterns and best practices for the Node system in Scenery Editor X.
