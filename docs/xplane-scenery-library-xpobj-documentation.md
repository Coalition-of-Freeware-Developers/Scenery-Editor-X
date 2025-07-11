# X-Plane Scenery Library - XPObj System Documentation

## Overview

The XPObj system provides comprehensive support for loading, parsing, and managing X-Plane OBJ8 format files within Scenery Editor X. This system handles the complex vertex data, material properties, layer groups, and rendering states that define X-Plane 3D objects.

## Architecture

### Core Components

- **`XPAsset::Obj`** - Main OBJ8 file representation, inherits from `XPAsset::Asset`
- **`XPAsset::Vertex`** - Individual vertex data structure
- **`XPAsset::ObjDrawCall`** - Rendering draw call representation
- **Layer Group Integration** - Seamless integration with XPLayerGroups system

## Class Reference

### XPAsset::Obj

The primary class for representing X-Plane OBJ8 files, extending the base Asset class with geometry-specific functionality.

#### Key Properties

```cpp
class Obj : public Asset
{
public:
    // Geometry data
    std::vector<XPAsset::Vertex> Vertices;      // All vertex data
    std::vector<size_t> Indices;                // Zero-based vertex indices
    std::vector<XPAsset::ObjDrawCall> DrawCalls; // Rendering draw calls
    
    // Draped-specific textures (for ground-projected geometry)
    std::filesystem::path pDrapedBaseTex;
    std::filesystem::path pDrapedNormalTex;
    std::filesystem::path pDrapedMaterialTex;
    
    // Draped texture availability flags
    bool bHasDrapedBaseTex{false};
    bool bHasDrapedNormalTex{false};
    bool bHasDrapedMaterialTex{false};
    
    // Extension data
    void* Refcon;  // Reference pointer for additional data association
    
    // Core functionality
    bool Load(const std::filesystem::path& InPath);
};
```

### XPAsset::Vertex

Represents a single vertex in the OBJ8 format with position, normal, and UV data.

```cpp
class Vertex
{
public:
    double X, Y, Z;       // World position coordinates
    double NX, NY, NZ;    // Normal vector components
    double U, V;          // Texture coordinates
};
```

### XPAsset::ObjDrawCall

Encapsulates rendering state and geometry ranges for efficient draw call batching.

```cpp
class ObjDrawCall
{
public:
    size_t idxStart;                           // Start index for geometry
    size_t idxEnd;                             // End index for geometry
    int intLayerGroup{XPLayerGroups::OBJECTS}; // Rendering layer assignment
    bool bDraped{false};                       // Ground projection flag
};
```

## Usage Examples

### Basic OBJ8 Loading

```cpp
class ObjLoaderModule : public Module
{
public:
    explicit ObjLoaderModule() : Module("ObjLoaderModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("OBJ", "Initializing OBJ8 loader system");
        
        // Load objects from scenery directory
        LoadSceneryObjects("assets/scenery/objects/");
    }
    
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("OBJ", "Cleaning up OBJ8 objects");
        m_LoadedObjects.clear(); // Smart pointers handle cleanup
    }
    
private:
    std::vector<Ref<XPAsset::Obj>> m_LoadedObjects;
    
    void LoadSceneryObjects(const std::filesystem::path& sceneryDir)
    {
        try
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(sceneryDir))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".obj")
                {
                    LoadSingleObject(entry.path());
                }
            }
            
            SEDX_CORE_INFO_TAG("OBJ", "Loaded {} OBJ8 files", m_LoadedObjects.size());
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("OBJ", "Failed to load scenery objects: {}", e.what());
        }
    }
    
    void LoadSingleObject(const std::filesystem::path& objPath)
    {
        auto obj = CreateRef<XPAsset::Obj>();
        
        if (obj->Load(objPath))
        {
            ValidateAndOptimizeObject(obj);
            m_LoadedObjects.push_back(obj);
            
            SEDX_CORE_INFO_TAG("OBJ", "Successfully loaded: {} ({} vertices, {} draw calls)", 
                objPath.filename().string(), obj->Vertices.size(), obj->DrawCalls.size());
        }
        else
        {
            SEDX_CORE_ERROR_TAG("OBJ", "Failed to load OBJ8 file: {}", objPath.string());
        }
    }
    
    void ValidateAndOptimizeObject(Ref<XPAsset::Obj>& obj)
    {
        // Validate geometry integrity
        SEDX_CORE_ASSERT(!obj->Vertices.empty(), "Object has no vertices");
        SEDX_CORE_ASSERT(!obj->Indices.empty(), "Object has no indices");
        
        // Validate index bounds
        for (size_t idx : obj->Indices)
        {
            SEDX_CORE_ASSERT(idx < obj->Vertices.size(), "Index out of bounds");
        }
        
        // Log geometry statistics
        SEDX_CORE_INFO_TAG("OBJ", "Object stats - Vertices: {}, Indices: {}, Draw calls: {}", 
            obj->Vertices.size(), obj->Indices.size(), obj->DrawCalls.size());
        
        // Check for draped geometry
        bool hasDrapedCalls = std::any_of(obj->DrawCalls.begin(), obj->DrawCalls.end(),
            [](const XPAsset::ObjDrawCall& call) { return call.bDraped; });
        
        if (hasDrapedCalls)
        {
            SEDX_CORE_INFO_TAG("OBJ", "Object contains draped geometry");
            ValidateDrapedTextures(obj);
        }
    }
    
    void ValidateDrapedTextures(Ref<XPAsset::Obj>& obj)
    {
        // Ensure draped textures exist if claimed
        if (obj->bHasDrapedBaseTex && !std::filesystem::exists(obj->pDrapedBaseTex))
        {
            SEDX_CORE_WARN_TAG("OBJ", "Draped base texture missing: {}", obj->pDrapedBaseTex.string());
            obj->bHasDrapedBaseTex = false;
        }
        
        if (obj->bHasDrapedNormalTex && !std::filesystem::exists(obj->pDrapedNormalTex))
        {
            SEDX_CORE_WARN_TAG("OBJ", "Draped normal texture missing: {}", obj->pDrapedNormalTex.string());
            obj->bHasDrapedNormalTex = false;
        }
    }
};
```

### Advanced OBJ8 Processing

```cpp
class AdvancedObjProcessor : public Module
{
public:
    void ProcessObjectForRendering(Ref<XPAsset::Obj>& obj)
    {
        SEDX_PROFILE_SCOPE("AdvancedObjProcessor::ProcessObjectForRendering");
        
        // Separate draped and non-draped geometry
        auto [drapedCalls, regularCalls] = SeparateDrawCalls(obj);
        
        // Process each type differently
        ProcessRegularGeometry(obj, regularCalls);
        ProcessDrapedGeometry(obj, drapedCalls);
        
        // Generate optimized render batches
        OptimizeDrawCalls(obj);
    }
    
private:
    std::pair<std::vector<XPAsset::ObjDrawCall>, std::vector<XPAsset::ObjDrawCall>>
    SeparateDrawCalls(const Ref<XPAsset::Obj>& obj)
    {
        std::vector<XPAsset::ObjDrawCall> drapedCalls;
        std::vector<XPAsset::ObjDrawCall> regularCalls;
        
        for (const auto& call : obj->DrawCalls)
        {
            if (call.bDraped)
            {
                drapedCalls.push_back(call);
            }
            else
            {
                regularCalls.push_back(call);
            }
        }
        
        SEDX_CORE_INFO_TAG("OBJ", "Separated draw calls: {} draped, {} regular", 
            drapedCalls.size(), regularCalls.size());
        
        return {drapedCalls, regularCalls};
    }
    
    void ProcessRegularGeometry(Ref<XPAsset::Obj>& obj, 
                               const std::vector<XPAsset::ObjDrawCall>& calls)
    {
        for (const auto& call : calls)
        {
            // Configure rendering state for regular geometry
            ConfigureRegularRenderState(call);
            
            // Process vertices in this draw call range
            ProcessVertexRange(obj, call.idxStart, call.idxEnd);
        }
    }
    
    void ProcessDrapedGeometry(Ref<XPAsset::Obj>& obj, 
                              const std::vector<XPAsset::ObjDrawCall>& calls)
    {
        for (const auto& call : calls)
        {
            // Special handling for draped geometry
            ConfigureDrapedRenderState(call);
            
            // Adjust Y coordinates for layer group offset
            AdjustDrapedVertices(obj, call);
        }
    }
    
    void ConfigureRegularRenderState(const XPAsset::ObjDrawCall& call)
    {
        // Set up rendering for 3D objects
        SetDepthTesting(true);
        SetCulling(true);
        SetLayerGroup(call.intLayerGroup);
        
        SEDX_CORE_INFO_TAG("OBJ", "Configured regular render state for layer group: {}", 
            call.intLayerGroup);
    }
    
    void ConfigureDrapedRenderState(const XPAsset::ObjDrawCall& call)
    {
        // Set up rendering for ground-projected geometry
        SetDepthTesting(false);  // Draped geometry uses different depth handling
        SetCulling(false);       // Often need both sides visible
        SetLayerGroup(call.intLayerGroup);
        EnableDrapedProjection();
        
        SEDX_CORE_INFO_TAG("OBJ", "Configured draped render state for layer group: {}", 
            call.intLayerGroup);
    }
    
    void AdjustDrapedVertices(Ref<XPAsset::Obj>& obj, const XPAsset::ObjDrawCall& call)
    {
        // Draped vertices have Y coordinate adjusted for layer group
        const double layerOffset = call.intLayerGroup * 0.1;
        
        for (size_t i = call.idxStart; i < call.idxEnd && i < obj->Indices.size(); ++i)
        {
            size_t vertexIndex = obj->Indices[i];
            if (vertexIndex < obj->Vertices.size())
            {
                obj->Vertices[vertexIndex].Y = layerOffset;
            }
        }
    }
    
    void OptimizeDrawCalls(Ref<XPAsset::Obj>& obj)
    {
        // Sort draw calls by layer group for optimal rendering
        std::sort(obj->DrawCalls.begin(), obj->DrawCalls.end(),
            [](const XPAsset::ObjDrawCall& a, const XPAsset::ObjDrawCall& b)
            {
                // Sort by layer group first, then by draped status
                if (a.intLayerGroup != b.intLayerGroup)
                    return a.intLayerGroup < b.intLayerGroup;
                return a.bDraped < b.bDraped;  // Regular before draped within same layer
            });
        
        SEDX_CORE_INFO_TAG("OBJ", "Optimized draw call order for efficient rendering");
    }
    
    void ProcessVertexRange(Ref<XPAsset::Obj>& obj, size_t start, size_t end)
    {
        // Process vertices in the specified index range
        for (size_t i = start; i < end && i < obj->Indices.size(); ++i)
        {
            size_t vertexIndex = obj->Indices[i];
            if (vertexIndex < obj->Vertices.size())
            {
                auto& vertex = obj->Vertices[vertexIndex];
                
                // Validate and potentially fix vertex data
                ValidateVertex(vertex, vertexIndex);
            }
        }
    }
    
    void ValidateVertex(XPAsset::Vertex& vertex, size_t index)
    {
        // Check for invalid values
        if (!std::isfinite(vertex.X) || !std::isfinite(vertex.Y) || !std::isfinite(vertex.Z))
        {
            SEDX_CORE_WARN_TAG("OBJ", "Invalid position in vertex {}", index);
            // Reset to origin as fallback
            vertex.X = vertex.Y = vertex.Z = 0.0;
        }
        
        // Validate normal vector
        double normalLength = std::sqrt(vertex.NX * vertex.NX + vertex.NY * vertex.NY + vertex.NZ * vertex.NZ);
        if (normalLength < 0.001) // Nearly zero normal
        {
            SEDX_CORE_WARN_TAG("OBJ", "Invalid normal in vertex {}", index);
            // Set default upward normal
            vertex.NX = 0.0; vertex.NY = 1.0; vertex.NZ = 0.0;
        }
        else if (std::abs(normalLength - 1.0) > 0.001) // Not normalized
        {
            // Normalize the normal vector
            vertex.NX /= normalLength;
            vertex.NY /= normalLength;
            vertex.NZ /= normalLength;
        }
    }
};
```

### Integration with Rendering Pipeline

```cpp
class ObjRenderingModule : public Module
{
public:
    void OnUIRender() override
    {
        if (m_ShowDebugWindow)
        {
            RenderDebugInterface();
        }
    }
    
    void RenderObject(const Ref<XPAsset::Obj>& obj)
    {
        SEDX_PROFILE_SCOPE("ObjRenderingModule::RenderObject");
        
        // Set up object-wide rendering state
        SetupObjectRendering(obj);
        
        // Render each draw call
        for (const auto& drawCall : obj->DrawCalls)
        {
            RenderDrawCall(obj, drawCall);
        }
        
        // Cleanup object-wide state
        CleanupObjectRendering();
    }
    
private:
    bool m_ShowDebugWindow = false;
    struct RenderStats
    {
        size_t objectsRendered = 0;
        size_t drawCallsRendered = 0;
        size_t verticesProcessed = 0;
    } m_Stats;
    
    void SetupObjectRendering(const Ref<XPAsset::Obj>& obj)
    {
        // Bind textures for the entire object
        BindObjectTextures(obj);
        
        // Set up vertex buffer
        UploadVertexData(obj->Vertices);
        UploadIndexData(obj->Indices);
        
        m_Stats.objectsRendered++;
    }
    
    void RenderDrawCall(const Ref<XPAsset::Obj>& obj, const XPAsset::ObjDrawCall& drawCall)
    {
        // Configure render state for this draw call
        if (drawCall.bDraped)
        {
            SetupDrapedRendering();
            BindDrapedTextures(obj);
        }
        else
        {
            SetupRegularRendering();
        }
        
        // Set layer-specific depth offset
        float depthOffset = drawCall.intLayerGroup * 0.001f;
        SetDepthOffset(depthOffset);
        
        // Submit draw call
        size_t indexCount = drawCall.idxEnd - drawCall.idxStart;
        DrawIndexed(drawCall.idxStart, indexCount);
        
        // Update statistics
        m_Stats.drawCallsRendered++;
        m_Stats.verticesProcessed += indexCount;
    }
    
    void BindObjectTextures(const Ref<XPAsset::Obj>& obj)
    {
        // Bind regular textures
        if (obj->bHasBaseTex)
        {
            auto texture = LoadTexture(obj->pBaseTex);
            BindTexture(0, texture);
        }
        
        if (obj->bHasNormalTex)
        {
            auto normalTexture = LoadTexture(obj->pNormalTex);
            BindTexture(1, normalTexture);
            
            // Configure normal mapping intensity
            SetNormalScale(static_cast<float>(obj->dblNormalScale));
        }
        
        if (obj->bHasMaterialTex)
        {
            auto materialTexture = LoadTexture(obj->pMaterialTex);
            BindTexture(2, materialTexture);
        }
    }
    
    void BindDrapedTextures(const Ref<XPAsset::Obj>& obj)
    {
        // Override with draped-specific textures if available
        if (obj->bHasDrapedBaseTex)
        {
            auto drapedTexture = LoadTexture(obj->pDrapedBaseTex);
            BindTexture(0, drapedTexture);
        }
        
        if (obj->bHasDrapedNormalTex)
        {
            auto drapedNormal = LoadTexture(obj->pDrapedNormalTex);
            BindTexture(1, drapedNormal);
        }
    }
    
    void SetupDrapedRendering()
    {
        // Configure for ground projection
        EnableDrapedShader();
        SetDepthWrite(false);     // Don't write depth for draped
        SetBlendMode(BlendMode::Alpha);
    }
    
    void SetupRegularRendering()
    {
        // Configure for normal 3D rendering
        EnableStandardShader();
        SetDepthWrite(true);
        SetBlendMode(BlendMode::Opaque);
    }
    
    void RenderDebugInterface()
    {
        ImGui::Begin("OBJ8 Renderer Debug");
        
        ImGui::Text("Render Statistics:");
        ImGui::Text("Objects Rendered: %zu", m_Stats.objectsRendered);
        ImGui::Text("Draw Calls: %zu", m_Stats.drawCallsRendered);
        ImGui::Text("Vertices Processed: %zu", m_Stats.verticesProcessed);
        
        ImGui::Separator();
        
        if (ImGui::Button("Reset Statistics"))
        {
            m_Stats = {};
        }
        
        ImGui::End();
    }
};
```

## Best Practices

### 1. Efficient Loading and Validation

```cpp
class OptimizedObjLoader
{
public:
    static std::optional<Ref<XPAsset::Obj>> LoadWithValidation(const std::filesystem::path& path)
    {
        // Pre-validate file
        if (!ValidateObjFile(path))
        {
            return std::nullopt;
        }
        
        auto obj = CreateRef<XPAsset::Obj>();
        
        if (!obj->Load(path))
        {
            SEDX_CORE_ERROR_TAG("OBJ", "Failed to load OBJ8 file: {}", path.string());
            return std::nullopt;
        }
        
        // Post-load validation and optimization
        if (!PostProcessObject(obj))
        {
            SEDX_CORE_ERROR_TAG("OBJ", "Post-processing failed for: {}", path.string());
            return std::nullopt;
        }
        
        return obj;
    }
    
private:
    static bool ValidateObjFile(const std::filesystem::path& path)
    {
        // Quick file validation before attempting to load
        if (!std::filesystem::exists(path))
        {
            SEDX_CORE_ERROR_TAG("OBJ", "File does not exist: {}", path.string());
            return false;
        }
        
        if (path.extension() != ".obj")
        {
            SEDX_CORE_ERROR_TAG("OBJ", "Invalid file extension: {}", path.extension().string());
            return false;
        }
        
        // Check file size (basic sanity check)
        auto fileSize = std::filesystem::file_size(path);
        if (fileSize == 0)
        {
            SEDX_CORE_ERROR_TAG("OBJ", "Empty file: {}", path.string());
            return false;
        }
        
        return true;
    }
    
    static bool PostProcessObject(Ref<XPAsset::Obj>& obj)
    {
        // Validate geometry integrity
        if (obj->Vertices.empty())
        {
            SEDX_CORE_ERROR_TAG("OBJ", "Object has no vertices");
            return false;
        }
        
        if (obj->Indices.empty())
        {
            SEDX_CORE_ERROR_TAG("OBJ", "Object has no indices");
            return false;
        }
        
        // Validate all indices are within bounds
        size_t maxIndex = obj->Vertices.size() - 1;
        for (size_t idx : obj->Indices)
        {
            if (idx > maxIndex)
            {
                SEDX_CORE_ERROR_TAG("OBJ", "Index {} out of bounds (max: {})", idx, maxIndex);
                return false;
            }
        }
        
        // Optional: Remove degenerate triangles
        RemoveDegenerateTriangles(obj);
        
        return true;
    }
    
    static void RemoveDegenerateTriangles(Ref<XPAsset::Obj>& obj)
    {
        // Remove triangles with duplicate vertices
        std::vector<size_t> validIndices;
        
        for (size_t i = 0; i < obj->Indices.size(); i += 3)
        {
            if (i + 2 < obj->Indices.size())
            {
                size_t idx1 = obj->Indices[i];
                size_t idx2 = obj->Indices[i + 1];
                size_t idx3 = obj->Indices[i + 2];
                
                // Skip degenerate triangles
                if (idx1 != idx2 && idx2 != idx3 && idx1 != idx3)
                {
                    validIndices.push_back(idx1);
                    validIndices.push_back(idx2);
                    validIndices.push_back(idx3);
                }
            }
        }
        
        if (validIndices.size() != obj->Indices.size())
        {
            SEDX_CORE_INFO_TAG("OBJ", "Removed {} degenerate triangles", 
                (obj->Indices.size() - validIndices.size()) / 3);
            obj->Indices = std::move(validIndices);
        }
    }
};
```

### 2. Memory and Performance Optimization

```cpp
class OptimizedObjManager : public Module
{
private:
    // Use object pooling for frequently loaded/unloaded objects
    std::queue<Ref<XPAsset::Obj>> m_ObjectPool;
    std::unordered_map<std::string, WeakRef<XPAsset::Obj>> m_LoadedObjects;
    
public:
    Ref<XPAsset::Obj> GetOrCreateObject(const std::filesystem::path& path)
    {
        const std::string pathKey = path.string();
        
        // Check if already loaded
        if (auto existing = m_LoadedObjects[pathKey].Lock())
        {
            return existing;
        }
        
        // Get from pool or create new
        Ref<XPAsset::Obj> obj;
        if (!m_ObjectPool.empty())
        {
            obj = m_ObjectPool.front();
            m_ObjectPool.pop();
            
            // Reset the pooled object
            ResetObject(obj);
        }
        else
        {
            obj = CreateRef<XPAsset::Obj>();
        }
        
        // Load the object
        if (obj->Load(path))
        {
            m_LoadedObjects[pathKey] = obj;
            return obj;
        }
        
        // Return to pool if load failed
        ReturnToPool(obj);
        return nullptr;
    }
    
private:
    void ResetObject(Ref<XPAsset::Obj>& obj)
    {
        // Clear all data for reuse
        obj->Vertices.clear();
        obj->Indices.clear();
        obj->DrawCalls.clear();
        
        // Reset paths
        obj->pReal.clear();
        obj->pBaseTex.clear();
        obj->pNormalTex.clear();
        obj->pMaterialTex.clear();
        obj->pDrapedBaseTex.clear();
        obj->pDrapedNormalTex.clear();
        obj->pDrapedMaterialTex.clear();
        
        // Reset flags
        obj->bHasBaseTex = false;
        obj->bHasNormalTex = false;
        obj->bHasMaterialTex = false;
        obj->bHasDrapedBaseTex = false;
        obj->bHasDrapedNormalTex = false;
        obj->bHasDrapedMaterialTex = false;
        obj->bSuperRoughness = false;
        
        // Reset other properties
        obj->dblNormalScale = 1.0;
        obj->intLayerGroup = XPLayerGroups::TERRAIN;
        obj->Refcon = nullptr;
    }
    
    void ReturnToPool(Ref<XPAsset::Obj>& obj)
    {
        if (m_ObjectPool.size() < 10) // Limit pool size
        {
            ResetObject(obj);
            m_ObjectPool.push(obj);
        }
        // Otherwise let it be destroyed normally
    }
};
```

This comprehensive documentation covers the XPObj system's architecture, usage patterns, and best practices for integration with the Scenery Editor X framework.
