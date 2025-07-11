# Scenery Editor X - Node System Complete Documentation

---

## Overview

The Node system in Scenery Editor X provides a hierarchical scene graph architecture for managing 3D objects, transformations, and scene composition. The `Node` class serves as the base class for all scene objects and implements a tree-like structure with parent-child relationships, spatial transformations, and type-safe object management.

## Architecture

### Class Hierarchy

```
RefCounted
└── Object
    └── Node
        ├── MeshNode
        ├── LightNode
        └── CameraNode
```

### Core Components

1. **Node Base Class**: Provides fundamental hierarchical structure and transformation capabilities
2. **Specialized Node Types**: Extend base functionality for specific scene object types
3. **Smart Pointer Integration**: Uses the custom `Ref<T>` system for memory management
4. **Serialization Support**: Integrates with the serialization framework for persistence

---

## Node Base Class

### Declaration

```cpp
class Node : public Object
{
public:
    Node();
  
    // Hierarchy management
    Ref<Node> parent;
    std::vector<Ref<Node>> children;
  
    // Spatial transformation properties
    Vec3 position = Vec3(0.0f);
    Vec3 rotation = Vec3(0.0f);  // Euler angles in degrees
    Vec3 scale = Vec3(1.0f);
  
    // Transformation methods
    glm::mat4 GetLocalTransform();
    glm::mat4 GetWorldTransform();
    glm::mat4 GetParentTransform();
    Vec3 GetWorldPosition();
    Vec3 GetWorldFront();
  
    // Static utility methods
    static glm::mat4 ComposeTransform(const Vec3& pos, const Vec3& rot, 
                                      const Vec3& scl, const glm::mat4& parent = glm::mat4(1));
    static void SetParent(const Ref<Node>& child, const Ref<Node>& parent);
    static void UpdateChildrenParent(const Ref<Node>& node);
    static Ref<Node> Clone(Ref<Node>& node);
  
    // Template methods for type-safe operations
    template<typename T>
    void GetAll(ObjectType type, std::vector<Ref<T>>& all);
  
    template<typename T>
    std::vector<Ref<T>> GetAll(ObjectType type);
};
```

### Key Properties

#### Hierarchical Structure

- **`parent`**: Reference to the parent node (nullptr for root nodes)
- **`children`**: Vector of child node references

#### Spatial Transformation

- **`position`**: Local position relative to parent (Vec3)
- **`rotation`**: Local rotation in Euler angles (Vec3, degrees)
- **`scale`**: Local scale factors (Vec3)

### Transformation System

The Node system implements a hierarchical transformation model where each node maintains local transformation properties that are combined with parent transformations to compute world-space values.

#### Local vs World Space

**Local Space**: Transformations relative to the parent node
**World Space**: Final transformations in global coordinate system

#### Transformation Methods

```cpp
// Get the local transformation matrix (position, rotation, scale)
Mat4 localTransform = node->GetLocalTransform();

// Get the world transformation matrix (local * parent world)
Mat4 worldTransform = node->GetWorldTransform();

// Get the parent's world transformation matrix
Mat4 parentTransform = node->GetParentTransform();

// Get world-space position
Vec3 worldPos = node->GetWorldPosition();

// Get world-space forward direction
Vec3 worldFront = node->GetWorldFront();
```

#### Implementation Details

```cpp
Mat4 Node::ComposeTransform(const Vec3 &pos, const Vec3 &rot, const Vec3 &scl, const Mat4 &parent)
{
    Mat4 rotationMat = glm::toMat4(glm::quat(glm::radians(rot)));
    Mat4 translationMat = glm::translate(Mat4(1.0f), pos);
    Mat4 scaleMat = glm::scale(scl);
    return parent * (translationMat * rotationMat * scaleMat);
}

Mat4 Node::GetLocalTransform()
{
    return ComposeTransform(position, rotation, scale);
}

Mat4 Node::GetWorldTransform()
{
    return GetParentTransform() * GetLocalTransform();
}

Mat4 Node::GetParentTransform()
{
    return parent ? parent->GetWorldTransform() : Mat4(1);
}
```

### Hierarchy Management

#### Setting Parent-Child Relationships

```cpp
// Safely set parent-child relationship
Node::SetParent(childNode, parentNode);

// Update all children's parent references after loading
Node::UpdateChildrenParent(rootNode);
```

#### Implementation

```cpp
static void SetParent(const Ref<Node>& child, const Ref<Node>& parent)
{
    // Remove from old parent if exists
    if (child->parent)
    {
        const Ref<Node> oldParent = child->parent;
        const auto it = std::ranges::find_if(oldParent->children, [&](auto& n) {
            return child->uuid == n->uuid;
        });
        SEDX_ASSERT(it != oldParent->children.end(), "Child not found in children vector");
        oldParent->children.erase(it);
    }
  
    // Set new parent-child relationship
    child->parent = parent;
    parent->children.push_back(child);
}
```

### Type-Safe Querying

The Node system provides template-based methods for type-safe retrieval of child nodes by type.

```cpp
// Get all nodes of a specific type (with output parameter)
std::vector<Ref<LightNode>> lights;
rootNode->GetAll<LightNode>(ObjectType::LightNode, lights);

// Get all nodes of a specific type (return value)
auto meshNodes = rootNode->GetAll<MeshNode>(ObjectType::MeshNode);
```



---

## Specialized Node Types

### MeshNode

Represents renderable geometry in the scene with material properties.

```cpp
struct MeshNode : Node
{
    uint32_t parent = 0xffffffff;          // Parent UUID for serialization
    Ref<ModelAsset> mesh;                   // Geometry data
    Ref<MaterialAsset> material;            // Material properties
    std::vector<uint32_t> children;         // Child UUIDs for serialization
    std::vector<uint32_t> submeshes;        // Submesh references
    std::string name;                       // Node name
    glm::mat4 localTransform;              // Cached local transform
  
    bool IsRoot() const { return parent == 0xffffffff; }
    MeshNode();
};
```

#### Usage Example

```cpp
// Create a mesh node
auto meshNode = assetManager.CreateObject<MeshNode>("MyMesh");

// Set transformation
meshNode->position = Vec3(1.0f, 0.0f, 0.0f);
meshNode->rotation = Vec3(0.0f, 45.0f, 0.0f);
meshNode->scale = Vec3(2.0f, 1.0f, 1.0f);

// Assign mesh and material assets
meshNode->mesh = assetManager.Get<ModelAsset>(meshUUID);
meshNode->material = assetManager.Get<MaterialAsset>(materialUUID);

// Add to scene
scene->Add(meshNode);
```

### LightNode

Represents light sources in the scene with lighting parameters.

```cpp
class LightNode : public Node
{
public:
    // Light properties
    Vec3 color = Vec3(1);
    float intensity = 10.0f;
    LightType lightType = Point;
    float radius = 2.0f;
    float innerAngle = 60.f;
    float outerAngle = 50.f;
  
    // Shadow mapping
    float shadowMapRange = 3.0f;
    float shadowMapFar = 2000.0f;
  
    // Volumetric lighting
    VolumetricType volumetricType = ScreenSpace;
    VolumetricScreenSpaceParams volumetricScreenSpaceParams;
    VolumetricShadowMapParams volumetricShadowMapParams;
};
```

#### Light Types

```cpp
enum LightType : uint8_t
{
    Point = 0,        // Omnidirectional point light
    Spot = 1,         // Cone-shaped spotlight
    Directional = 2,  // Parallel directional light
};
```

#### Usage Example

```cpp
// Create a point light
auto pointLight = assetManager.CreateObject<LightNode>("MainLight");
pointLight->lightType = LightType::Point;
pointLight->color = Vec3(1.0f, 0.9f, 0.8f);
pointLight->intensity = 15.0f;
pointLight->position = Vec3(0.0f, 5.0f, 0.0f);

// Create a spotlight
auto spotLight = assetManager.CreateObject<LightNode>("SpotLight");
spotLight->lightType = LightType::Spot;
spotLight->innerAngle = 30.0f;
spotLight->outerAngle = 45.0f;
spotLight->rotation = Vec3(-45.0f, 0.0f, 0.0f);
```

### CameraNode

Represents camera viewpoints for rendering.

```cpp
class CameraNode : public Node
{
public:
    // Camera type and mode
    CameraType cameraType = CameraType::Perspective;
    CameraMode mode = Orbit;
  
    // View parameters
    Vec3 eye = Vec3(0);
    Vec3 center = Vec3(0);
    Vec3 rotation = Vec3(0);
  
    // Projection parameters
    float farDistance = 1000.0f;
    float nearDistance = 0.01f;
    float horizontalFov = 60.0f;
  
    // Orthographic parameters
    float orthoFarDistance = 10.0f;
    float orthoNearDistance = -100.0f;
    Vec2 extent = Vec2(1.0f);
  
    // Anti-aliasing
    bool useJitter = true;
    float zoom = 10.0f;
  
    // Methods
    glm::mat4 GetView();
    glm::mat4 GetProj();
    glm::mat4 GetProjJittered();
    glm::mat4 GetProj(float zNear, float zFar);
};
```

#### Camera Types and Modes

```cpp
enum class CameraType : uint8_t
{
    Perspective,
    Orthographic
};

enum CameraMode : uint8_t
{
    Orbit,  // Orbit around center point
    Fly     // Free-flying camera
};
```

#### Usage Example

```cpp
// Create a perspective camera
auto camera = assetManager.CreateObject<CameraNode>("MainCamera");
camera->cameraType = CameraType::Perspective;
camera->horizontalFov = 75.0f;
camera->position = Vec3(0.0f, 2.0f, 5.0f);
camera->rotation = Vec3(-20.0f, 0.0f, 0.0f);

// Set as scene's main camera
scene->mainCamera = camera;
```



---

## Integration with Asset Management

### Object Creation

```cpp
// Create objects through AssetManager for proper UUID assignment
template<typename T>
Ref<T> AssetManager::CreateObject(const std::string& name, uint32_t uuid = 0)
{
    if (uuid == 0)
        uuid = NewUUID();
      
    Ref<T> object = CreateRef<T>();
    object->name = name;
    object->uuid = uuid;
    return object;
}
```

### Scene Integration

```cpp
// Add nodes to scenes
template<typename T>
Ref<T> SceneAsset::Add()
{
    Ref<T> node = std::make_shared<T>();
    nodes.push_back(node);
    return node;
}

void SceneAsset::Add(const Ref<Node>& node)
{
    nodes.push_back(node);
}
```



---

## Serialization

The Node system integrates with the serialization framework for persistence and loading.

### Base Node Serialization

```cpp
void Node::Serialize(Serializer& ser)
{
    ser.VectorRef("children", children);
    ser("position", position);
    ser("rotation", rotation);
    ser("scale", scale);
}
```

### Specialized Node Serialization

```cpp
void MeshNode::Serialize(Serializer& ser)
{
    Node::Serialize(ser);  // Call base class serialization
    ser.Asset("mesh", mesh);
    ser.Asset("material", material);
}

void LightNode::Serialize(Serializer& ser)
{
    Node::Serialize(ser);
    ser("color", color);
    ser("intensity", intensity);
    ser("lightType", lightType);
    // ... additional light properties
}
```



---

## Usage Patterns and Best Practices

### 1. Scene Graph Construction

```cpp
// Create scene hierarchy
auto scene = assetManager.CreateAsset<SceneAsset>("MainScene");

// Create root node
auto rootNode = assetManager.CreateObject<Node>("Root");
scene->Add(rootNode);

// Create child nodes
auto meshNode = assetManager.CreateObject<MeshNode>("Cube");
auto lightNode = assetManager.CreateObject<LightNode>("Light");

// Build hierarchy
Node::SetParent(meshNode, rootNode);
Node::SetParent(lightNode, rootNode);
```

### 2. Transformation Manipulation

```cpp
// Animate node position
void UpdateAnimation(Ref<Node> node, float deltaTime)
{
    // Local space animation
    node->rotation.y += 90.0f * deltaTime;  // Rotate 90 degrees per second
  
    // World space queries
    Vec3 worldPos = node->GetWorldPosition();
    Mat4 worldTransform = node->GetWorldTransform();
  
    // Apply constraints or physics
    if (worldPos.y < 0.0f)
        node->position.y = 0.0f;
}
```

### 3. Type-Safe Node Queries

```cpp
// Find all lights in scene
void ConfigureLighting(Ref<SceneAsset> scene)
{
    auto lights = scene->GetAll<LightNode>(ObjectType::LightNode);
  
    for (auto& light : lights)
    {
        // Configure shadow mapping
        if (light->lightType == LightType::Directional)
        {
            light->shadowMapRange = 100.0f;
            light->shadowMapFar = 500.0f;
        }
    }
}
```

### 4. Hierarchical Operations

```cpp
// Apply transformation to entire hierarchy
void ScaleHierarchy(Ref<Node> root, float scaleFactor)
{
    root->scale *= scaleFactor;
  
    for (auto& child : root->children)
    {
        ScaleHierarchy(child, scaleFactor);
    }
}

// Find node by name in hierarchy
Ref<Node> FindNodeByName(Ref<Node> root, const std::string& name)
{
    if (root->name == name)
        return root;
      
    for (auto& child : root->children)
    {
        if (auto found = FindNodeByName(child, name))
            return found;
    }
  
    return nullptr;
}
```

### 5. Module Integration

```cpp
class SceneModule : public Module
{
private:
    Ref<SceneAsset> m_CurrentScene;
  
public:
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("SceneModule::OnUpdate");
      
        if (!m_CurrentScene)
            return;
          
        // Update all animated nodes
        auto animatedNodes = m_CurrentScene->GetAll<Node>(ObjectType::Node);
        for (auto& node : animatedNodes)
        {
            UpdateNodeAnimation(node);
        }
      
        // Update camera
        if (m_CurrentScene->mainCamera)
        {
            UpdateCamera(m_CurrentScene->mainCamera);
        }
    }
  
    void OnUIRender() override
    {
        if (ImGui::Begin("Scene Hierarchy"))
        {
            if (m_CurrentScene)
            {
                for (auto& node : m_CurrentScene->nodes)
                {
                    RenderNodeHierarchy(node);
                }
            }
        }
        ImGui::End();
    }
  
private:
    void RenderNodeHierarchy(Ref<Node> node)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (node->children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;
          
        bool open = ImGui::TreeNodeEx(node->name.c_str(), flags);
      
        if (ImGui::IsItemClicked())
        {
            // Select node for editing
        }
      
        if (open)
        {
            for (auto& child : node->children)
            {
                RenderNodeHierarchy(child);
            }
            ImGui::TreePop();
        }
    }
};
```



---

## Error Handling and Validation

### Assertions and Validation

```cpp
// Use assertions for development-time validation
void ValidateNodeHierarchy(Ref<Node> node)
{
    SEDX_CORE_ASSERT(node != nullptr, "Node cannot be null");
  
    // Check for circular references
    std::set<uint32_t> visited;
    ValidateNodeHierarchyRecursive(node, visited);
  
    // Validate transformations
    SEDX_CORE_ASSERT(glm::length(node->scale) > 0.001f, "Scale too small");
}

void ValidateNodeHierarchyRecursive(Ref<Node> node, std::set<uint32_t>& visited)
{
    SEDX_CORE_ASSERT(visited.find(node->uuid) == visited.end(), 
                      "Circular reference detected in node hierarchy");
                    
    visited.insert(node->uuid);
  
    for (auto& child : node->children)
    {
        SEDX_CORE_ASSERT(child->parent.get() == node.get(), 
                          "Child's parent reference doesn't match");
        ValidateNodeHierarchyRecursive(child, visited);
    }
}
```

### Error Recovery

```cpp
// Safe node operations with error handling
bool SafeSetParent(Ref<Node> child, Ref<Node> parent)
{
    try
    {
        if (!child || !parent)
        {
            SEDX_CORE_WARN("Null node passed to SafeSetParent");
            return false;
        }
      
        // Check for circular reference
        Ref<Node> current = parent;
        while (current)
        {
            if (current == child)
            {
                SEDX_CORE_WARN("Attempted to create circular reference");
                return false;
            }
            current = current->parent;
        }
      
        Node::SetParent(child, parent);
        return true;
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR("Exception in SafeSetParent: {}", e.what());
        return false;
    }
}
```



---

## Performance Considerations

### 1. Transform Caching

For frequently accessed transformations, consider caching world transforms:

```cpp
class OptimizedNode : public Node
{
private:
    mutable Mat4 m_CachedWorldTransform;
    mutable bool m_WorldTransformDirty = true;
  
public:
    Mat4 GetWorldTransform() const override
    {
        if (m_WorldTransformDirty)
        {
            m_CachedWorldTransform = GetParentTransform() * GetLocalTransform();
            m_WorldTransformDirty = false;
        }
        return m_CachedWorldTransform;
    }
  
    void MarkTransformDirty()
    {
        m_WorldTransformDirty = true;
      
        // Mark children dirty recursively
        for (auto& child : children)
        {
            if (auto optimizedChild = child.DynamicCast<OptimizedNode>())
            {
                optimizedChild->MarkTransformDirty();
            }
        }
    }
};
```

### 2. Batch Operations

```cpp
// Batch transformation updates
void BatchUpdateTransforms(const std::vector<Ref<Node>>& nodes, 
                          const std::vector<Mat4>& transforms)
{
    SEDX_PROFILE_SCOPE("BatchUpdateTransforms");
  
    SEDX_CORE_ASSERT(nodes.size() == transforms.size(), 
                      "Node and transform counts must match");
  
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        // Decompose transform into position, rotation, scale
        Vec3 position, rotation, scale;
        DecomposeTransform(transforms[i], position, rotation, scale);
      
        nodes[i]->position = position;
        nodes[i]->rotation = rotation;
        nodes[i]->scale = scale;
    }
}
```

### 3. Memory Management

```cpp
// Use object pooling for frequently created/destroyed nodes
class NodePool
{
private:
    std::vector<Ref<Node>> m_Pool;
    size_t m_NextIndex = 0;
  
public:
    Ref<Node> Acquire()
    {
        if (m_NextIndex < m_Pool.size())
        {
            return m_Pool[m_NextIndex++];
        }
      
        auto node = CreateRef<Node>();
        m_Pool.push_back(node);
        m_NextIndex++;
        return node;
    }
  
    void Release(Ref<Node> node)
    {
        // Reset node state
        node->position = Vec3(0.0f);
        node->rotation = Vec3(0.0f);
        node->scale = Vec3(1.0f);
        node->children.clear();
        node->parent.Reset();
      
        // Return to pool
        if (m_NextIndex > 0)
        {
            m_NextIndex--;
            m_Pool[m_NextIndex] = node;
        }
    }
};
```



---

## Thread Safety

The Node system is not inherently thread-safe. For multi-threaded access:

```cpp
class ThreadSafeNode : public Node
{
private:
    mutable std::shared_mutex m_Mutex;
  
public:
    Mat4 GetWorldTransform() const override
    {
        std::shared_lock lock(m_Mutex);
        return Node::GetWorldTransform();
    }
  
    void SetPosition(const Vec3& pos)
    {
        std::unique_lock lock(m_Mutex);
        position = pos;
    }
  
    void AddChild(Ref<Node> child)
    {
        std::unique_lock lock(m_Mutex);
        Node::SetParent(child, shared_from_this());
    }
};
```



---

## Debugging and Visualization

### Debug Visualization

```cpp
void RenderNodeDebugInfo(Ref<Node> node)
{
    if (!node)
        return;
      
    // Render coordinate system at node position
    Vec3 worldPos = node->GetWorldPosition();
    Mat4 worldTransform = node->GetWorldTransform();
  
    // Extract axes from world transform
    Vec3 right = Vec3(worldTransform[0]);
    Vec3 up = Vec3(worldTransform[1]);
    Vec3 forward = Vec3(worldTransform[2]);
  
    // Render debug lines
    DebugRenderer::DrawLine(worldPos, worldPos + right, Vec3(1, 0, 0));    // X-axis: Red
    DebugRenderer::DrawLine(worldPos, worldPos + up, Vec3(0, 1, 0));       // Y-axis: Green
    DebugRenderer::DrawLine(worldPos, worldPos + forward, Vec3(0, 0, 1));  // Z-axis: Blue
  
    // Render bounding box
    if (auto meshNode = node.DynamicCast<MeshNode>())
    {
        if (meshNode->mesh)
        {
            BoundingBox bounds = meshNode->mesh->GetBounds();
            bounds.Transform(worldTransform);
            DebugRenderer::DrawWireBox(bounds, Vec3(1, 1, 0));
        }
    }
  
    // Recurse to children
    for (auto& child : node->children)
    {
        RenderNodeDebugInfo(child);
    }
}
```

### Logging and Profiling

```cpp
void LogNodeHierarchy(Ref<Node> node, int depth = 0)
{
    std::string indent(depth * 2, ' ');
  
    SEDX_CORE_INFO_TAG("SCENE", "{}Node: {} (UUID: {}, Type: {})", 
                        indent, node->name, node->uuid, 
                        ObjectTypeName[static_cast<int>(node->type)]);
                      
    SEDX_CORE_INFO_TAG("SCENE", "{}  Position: ({:.2f}, {:.2f}, {:.2f})", 
                        indent, node->position.x, node->position.y, node->position.z);
                      
    SEDX_CORE_INFO_TAG("SCENE", "{}  Children: {}", indent, node->children.size());
  
    for (auto& child : node->children)
    {
        LogNodeHierarchy(child, depth + 1);
    }
}
```
