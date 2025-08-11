/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* object.h
* -------------------------------------------------------
* Created: 9/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/identifiers/uuid.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
	/**
	 * @brief Enumeration of all supported object types in the Scenery Editor X engine
	 * 
	 * ObjectType provides a comprehensive classification system for all identifiable entities
	 * within the Scenery Editor X engine. This enumeration serves as the foundation for
	 * runtime type identification, polymorphic operations, and asset management workflows.
	 * 
	 * The enum is designed to support the engine's asset pipeline, serialization system,
	 * and runtime type checking without requiring expensive RTTI operations. Each value
	 * represents a distinct category of object that can be created, managed, and processed
	 * by various engine systems.
	 * 
	 * Key architectural benefits:
	 * - Enables fast runtime type checking through simple integer comparison
	 * - Supports asset system categorization and filtering operations
	 * - Facilitates serialization system type dispatch and validation
	 * - Provides editor UI with object type information for specialized panels
	 * - Enables reflection system to perform type-specific operations
	 * - Supports dependency resolution in asset loading and management
	 * 
	 * Usage in engine systems:
	 * - Asset Management: Used to categorize and filter assets by type
	 * - Serialization: Enables type-specific serializer selection and validation
	 * - Editor UI: Drives specialized property panels and object browsers
	 * - Rendering: Allows render system to dispatch objects to appropriate passes
	 * - Scene Graph: Enables type-based traversal and processing optimizations
	 * - Memory Management: Supports type-specific allocation and cleanup strategies
	 * 
	 * Performance characteristics:
	 * - Underlying type is uint16_t for memory efficiency and fast comparison
	 * - Enum values start at 0 and increment sequentially for optimal switch optimization
	 * - Compatible with bitfield operations for type filtering and masking
	 * - Supports efficient hash table lookups for type-based registries
	 * 
	 * @note The enum uses uint16_t as underlying type to balance range (65,536 possible types)
	 *       with memory efficiency in Object instances
	 * @note NOLINT comment suppresses performance warnings about enum size optimization
	 * @note Values are intentionally sequential starting from 0 for switch statement optimization
	 * @note None=0 provides a safe default/invalid state for uninitialized objects
	 * 
	 * @warning Adding new enum values requires updating ObjectTypeName() and ObjectTypeToString()
	 *          functions to maintain consistency in string conversion operations
	 * @warning Changing existing enum values will break serialized data compatibility
	 * @warning The maximum value is limited by uint16_t range (0-65535)
	 * 
	 * @thread_safety Enum values are compile-time constants and inherently thread-safe
	 * 
	 * @code
	 * // Basic type checking
	 * if (object->type == ObjectType::Mesh)
	 * {
	 *     auto mesh = object.As<MeshObject>();
	 *     ProcessMeshObject(mesh);
	 * }
	 * 
	 * // Asset filtering by type
	 * std::vector<ObjectType> renderableTypes = {
	 *     ObjectType::Mesh,
	 *     ObjectType::StaticMesh,
	 *     ObjectType::Light
	 * };
	 * 
	 * for (ObjectType type : renderableTypes)
	 * {
	 *     auto assets = AssetManager::GetAssetsByType(type);
	 *     ProcessRenderableAssets(assets);
	 * }
	 * 
	 * // Serializer dispatch
	 * switch (object->type)
	 * {
	 *     case ObjectType::Material:
	 *         return GetSerializer<MaterialSerializer>();
	 *     case ObjectType::Texture:
	 *         return GetSerializer<TextureSerializer>();
	 *     case ObjectType::Mesh:
	 *         return GetSerializer<MeshSerializer>();
	 *     default:
	 *         SEDX_CORE_WARN("No serializer for type: {}", ObjectTypeToString(object->type));
	 *         return nullptr;
	 * }
	 * 
	 * // Editor UI type-specific handling
	 * void ShowObjectProperties(const Ref<Object>& object)
	 * {
	 *     ImGui::Text("Object Type: %s", ObjectTypeToString(object->type));
	 *     
	 *     switch (object->type)
	 *     {
	 *         case ObjectType::Light:
	 *             ShowLightProperties(object.As<LightObject>());
	 *             break;
	 *         case ObjectType::Material:
	 *             ShowMaterialProperties(object.As<MaterialObject>());
	 *             break;
	 *         case ObjectType::Camera:
	 *             ShowCameraProperties(object.As<CameraObject>());
	 *             break;
	 *         // ... handle other types
	 *     }
	 * }
	 * @endcode
	 * 
	 * @see ObjectTypeName() for string-to-enum conversion
	 * @see ObjectTypeToString() for enum-to-string conversion
	 * @see Object::type for usage in base object class
	 * @see AssetManager for type-based asset filtering and management
	 */
	enum class ObjectType : uint16_t  // NOLINT(performance-enum-size)
	{
		None = 0,
		Scene,
        Node,
		Prefab,
		Mesh,
		StaticMesh,
		Light,
		MeshSource,
		Material,
		Texture,
		EnvMap,
		Font,
		Script,
		ScriptFile,
		MeshCollider,
		Animation,
		AnimationGraph
	};

	/**
	 * @brief Converts a string representation to its corresponding ObjectType enumeration value
	 * 
	 * This function provides a standardized way to convert human-readable string representations
	 * into ObjectType enum values. It serves as the inverse operation to ObjectTypeToString()
	 * and is primarily used for:
	 * - Deserialization systems that need to convert stored string-based type names back to enums
	 * - Configuration file parsing where object types are specified as readable strings
	 * - Asset management systems that load type information from external files
	 * - Editor UI components that need to map user-selected type names to internal enum values
	 * - Reflection and introspection systems for runtime type resolution from string identifiers
	 * - Command-line interfaces and scripting systems that accept type names as parameters
	 * 
	 * The function uses a series of string comparisons to match the input string against all
	 * known ObjectType enum value names. String matching is case-sensitive and requires exact
	 * matches with the canonical enum value names returned by ObjectTypeToString().
	 * 
	 * Performance characteristics:
	 * - O(n) linear time complexity where n is the number of ObjectType enum values
	 * - Uses early termination - returns immediately upon finding a match
	 * - String comparison operations are optimized by the standard library
	 * - Suitable for infrequent use such as loading/parsing operations
	 * - Not recommended for hot code paths due to multiple string comparisons
	 * 
	 * @param objectType A string_view containing the object type name to convert.
	 *                   Must exactly match one of the canonical ObjectType names:
	 *                   "None", "Scene", "Node", "Prefab", "Mesh", "StaticMesh", 
	 *                   "Light", "MeshSource", "Material", "Texture", "EnvMap", 
	 *                   "Font", "Script", "ScriptFile", "MeshCollider", 
	 *                   "Animation", "AnimationGraph"
	 * 
	 * @return ObjectType The corresponding enumeration value for the given string.
	 *         Returns ObjectType::None if the input string does not match any known type name.
	 *         This provides a safe fallback for invalid or unrecognized type strings.
	 * 
	 * @note String comparison is case-sensitive - "mesh" will not match "Mesh"
	 * @note Uses std::string_view for efficient string handling without unnecessary copies
	 * @note The function does not trim whitespace - " Mesh " will not match "Mesh"
	 * @note ObjectType::None is returned both for explicit "None" input and as fallback for unknown strings
	 * 
	 * @thread_safety Thread-safe. No shared state is accessed or modified.
	 * 
	 * @code
	 * // Basic usage for deserialization
	 * std::string typeString = "Mesh";
	 * ObjectType type = ObjectTypeName(typeString);
	 * if (type != ObjectType::None)
	 * {
	 *     SEDX_CORE_INFO("Successfully parsed object type: {}", ObjectTypeToString(type));
	 * }
	 * 
	 * // Usage in asset loading
	 * void LoadAssetFromConfig(const nlohmann::json& config)
	 * {
	 *     std::string typeStr = config["objectType"].get<std::string>();
	 *     ObjectType type = ObjectTypeName(typeStr);
	 *     
	 *     switch (type)
	 *     {
	 *         case ObjectType::Texture:
	 *             LoadTextureAsset(config);
	 *             break;
	 *         case ObjectType::Material:
	 *             LoadMaterialAsset(config);
	 *             break;
	 *         case ObjectType::None:
	 *             SEDX_CORE_WARN("Unknown object type in config: {}", typeStr);
	 *             break;
	 *         default:
	 *             ProcessGenericAsset(type, config);
	 *             break;
	 *     }
	 * }
	 * 
	 * // Usage in serialization systems
	 * ObjectType DeserializeObjectType(Deserializer& des)
	 * {
	 *     std::string typeName;
	 *     des.ReadString("objectType", typeName);
	 *     
	 *     ObjectType type = ObjectTypeName(typeName);
	 *     if (type == ObjectType::None && typeName != "None")
	 *     {
	 *         SEDX_CORE_ERROR("Invalid object type in serialized data: {}", typeName);
	 *         // Could throw exception or use default type based on requirements
	 *     }
	 *     
	 *     return type;
	 * }
	 * 
	 * // Usage in configuration parsing
	 * void ParseAssetFilter(const std::string& filterString)
	 * {
	 *     std::vector<std::string> typeNames = SplitString(filterString, ',');
	 *     std::vector<ObjectType> allowedTypes;
	 *     
	 *     for (const std::string& typeName : typeNames)
	 *     {
	 *         ObjectType type = ObjectTypeName(TrimWhitespace(typeName));
	 *         if (type != ObjectType::None)
	 *         {
	 *             allowedTypes.push_back(type);
	 *         }
	 *         else
	 *         {
	 *             SEDX_CORE_WARN("Ignoring unknown asset type in filter: {}", typeName);
	 *         }
	 *     }
	 *     
	 *     SetAssetTypeFilter(allowedTypes);
	 * }
	 * @endcode
	 * 
	 * @see ObjectTypeToString() for the inverse operation (ObjectType to string conversion)
	 * @see ObjectType enumeration for all supported object type values
	 * @see std::string_view for efficient string parameter handling
	 */
	inline ObjectType ObjectTypeName(std::string_view objectType)
    {
	        if (objectType == "None")                return ObjectType::None;
			if (objectType == "Scene")               return ObjectType::Scene;
			if (objectType == "Node")                return ObjectType::Node;
			if (objectType == "Prefab")              return ObjectType::Prefab;
			if (objectType == "Mesh")                return ObjectType::Mesh;
			if (objectType == "StaticMesh")          return ObjectType::StaticMesh;
			if (objectType == "MeshSource")          return ObjectType::MeshSource;
			if (objectType == "Material")            return ObjectType::Material;
			if (objectType == "Texture")             return ObjectType::Texture;
			if (objectType == "EnvMap")              return ObjectType::EnvMap;
			if (objectType == "Font")                return ObjectType::Font;
			if (objectType == "Script")              return ObjectType::Script;
			if (objectType == "ScriptFile")          return ObjectType::ScriptFile;
			if (objectType == "MeshCollider")        return ObjectType::MeshCollider;
			if (objectType == "Animation")           return ObjectType::Animation;
			if (objectType == "AnimationGraph")      return ObjectType::AnimationGraph;

			return ObjectType::None;
	}

    /**
     * @brief Converts an ObjectType enumeration value to its corresponding string representation
     * 
     * This function provides a standardized way to convert ObjectType enum values into
     * human-readable string representations. It is primarily used for:
     * - Debug output and logging to display readable object type names
     * - Serialization systems that need string-based type identification
     * - Editor UI components that display object type information to users
     * - Asset management systems for type-based filtering and categorization
     * - Reflection and introspection systems for runtime type identification
     * 
     * The function uses a comprehensive switch statement to handle all defined ObjectType
     * values, ensuring that every valid enum value has a corresponding string representation.
     * This design maintains type safety and compile-time validation of enum coverage.
     * 
     * Performance characteristics:
     * - O(1) constant time lookup using switch statement optimization
     * - Returns compile-time string literals (no dynamic allocation)
     * - Suitable for frequent use in hot code paths like rendering loops
     * - String literals are stored in read-only memory section
     * 
     * @param type The ObjectType enumeration value to convert to string
     * 
     * @return const char* A null-terminated string literal representing the object type.
     *         Returns "None" for ObjectType::None or as fallback for unknown types.
     * 
     * @warning This function will trigger a SEDX_CORE_ASSERT assertion failure in debug
     *          builds if an unknown/invalid ObjectType value is passed. In release builds,
     *          the function will return "None" as a safe fallback.
     * 
     * @note The returned pointer points to a string literal with static storage duration,
     *       so the pointer remains valid for the entire program execution. No memory
     *       management is required by the caller.
     * 
     * @note This function is marked inline for performance optimization in header-only
     *       usage scenarios, allowing the compiler to potentially eliminate function
     *       call overhead when used frequently.
     * 
     * @thread_safety Thread-safe. No shared state is accessed or modified.
     * 
     * @code
     * // Basic usage for logging
     * ObjectType meshType = ObjectType::Mesh;
     * SEDX_CORE_INFO("Processing object of type: {}", ObjectTypeToString(meshType));
     * // Output: "Processing object of type: Mesh"
     * 
     * // Usage in asset management
     * void ProcessAsset(const Ref<Object>& object)
     * {
     *     const char* typeName = ObjectTypeToString(object->type);
     *     SEDX_CORE_DEBUG_TAG("ASSET", "Loading {} asset: {}", typeName, object->name);
     *     
     *     // Type-specific processing
     *     if (object->type == ObjectType::Texture)
     *     {
     *         auto texture = object.As<Texture2D>();
     *         ProcessTexture(texture);
     *     }
     * }
     * 
     * // Usage in serialization
     * void SerializeObject(const Object& obj, Serializer& ser)
     * {
     *     const char* typeString = ObjectTypeToString(obj.type);
     *     ser.WriteString("objectType", typeString);
     * }
     * 
     * // Editor UI usage
     * void RenderObjectInspector(const Ref<Object>& object)
     * {
     *     ImGui::Text("Object Type: %s", ObjectTypeToString(object->type));
     *     ImGui::Text("Object Name: %s", object->name.c_str());
     * }
     * @endcode
     * 
     * @see ObjectTypeName() for the inverse operation (string to ObjectType conversion)
     * @see ObjectType enumeration for all supported object type values
     * @see SEDX_CORE_ASSERT for assertion behavior in debug builds
     */
    inline const char* ObjectTypeToString(ObjectType type)
	{
		switch (type)
		{
			case ObjectType::None:           return "None";
			case ObjectType::Scene:          return "Scene";
			case ObjectType::Node:           return "Node";
			case ObjectType::Prefab:         return "Prefab";
			case ObjectType::Mesh:           return "Mesh";
			case ObjectType::StaticMesh:     return "StaticMesh";
			case ObjectType::Light:          return "Light";
			case ObjectType::MeshSource:     return "MeshSource";
			case ObjectType::Material:       return "Material";
			case ObjectType::Texture:        return "Texture";
			case ObjectType::EnvMap:         return "EnvMap";
			case ObjectType::Font:           return "Font";
			case ObjectType::Script:         return "Script";
			case ObjectType::ScriptFile:     return "ScriptFile";
			case ObjectType::MeshCollider:   return "MeshCollider";
			case ObjectType::Animation:      return "Animation";
			case ObjectType::AnimationGraph: return "AnimationGraph";
		}

        SEDX_CORE_ASSERT(false, "Unknown Asset Type");
        return "None";
    }

	/// -------------------------------------------------------

	/**
	 * @brief Base object class for all identifiable entities in the Scenery Editor X engine
	 * 
	 * The Object struct serves as the fundamental base class for all entities that require
	 * unique identification, type information, and basic metadata management within the
	 * engine. This includes assets, scene nodes, components, and other core engine objects
	 * that need to be tracked, serialized, or referenced by other systems.
	 * 
	 * Key responsibilities:
	 * - Provides unique UUID-based identification for persistent object references
	 * - Maintains human-readable names for debugging and editor display
	 * - Tracks object type information for runtime type checking and polymorphic operations
	 * - Manages GPU synchronization state through the dirty flag system
	 * - Integrates with the reference counting system for automatic memory management
	 * - Supports serialization and deserialization through the reflection system
	 * 
	 * The Object class uses UUID-based identification instead of raw pointers to enable:
	 * - Persistent references that survive application restarts
	 * - Safe serialization of object relationships without pointer corruption
	 * - Hot-reloading and asset streaming without breaking existing references
	 * - Cross-module object sharing with type safety
	 * 
	 * GPU Dirty Flag System:
	 * The gpuDirty flag indicates when an object's data has been modified and needs
	 * to be synchronized with GPU resources. This is particularly important for:
	 * - Vertex buffer updates when mesh data changes
	 * - Texture uploads when image data is modified
	 * - Uniform buffer updates when material properties change
	 * - Descriptor set rebuilding when resource bindings change
	 * 
	 * @note All objects start with gpuDirty=true to ensure initial GPU upload
	 * @note The assignment operator automatically sets gpuDirty=true to handle data changes
	 * @note UUID assignment is intentionally excluded from copy operations to preserve identity
	 * 
	 * @code
	 * // Creating and using objects
	 * class CustomAsset : public Object
	 * {
	 * public:
	 *     CustomAsset(const std::string& assetName)
	 *     {
	 *         name = assetName;
	 *         type = ObjectType::Custom;
	 *         uuid = UUID::Generate(); // Create unique identifier
	 *         // gpuDirty is already true by default
	 *     }
	 * };
	 * 
	 * // Object identification and lookup
	 * auto asset = CreateRef<CustomAsset>("MyTexture");
	 * UUID assetId = asset->uuid;
	 * 
	 * // Later retrieval by UUID
	 * auto foundAsset = ObjectRegistry::FindByUUID(assetId);
	 * if (foundAsset && foundAsset->type == ObjectType::Custom)
	 * {
	 *     // Safe to use - object identity verified
	 *     ProcessAsset(foundAsset);
	 * }
	 * 
	 * // GPU synchronization workflow
	 * void UpdateAssetData(Ref<CustomAsset> asset)
	 * {
	 *     // Modify asset data
	 *     asset->ModifyGeometry();
	 *     
	 *     // Check if GPU update needed
	 *     if (asset->gpuDirty)
	 *     {
	 *         UploadToGPU(asset);
	 *         asset->gpuDirty = false; // Mark as synchronized
	 *     }
	 * }
	 * @endcode
	 */
	struct Object : RefCounted
	{
	    /**
	     * @brief Human-readable name for debugging and editor display
	     * 
	     * This name is primarily used for:
	     * - Editor UI display and object browser listings
	     * - Debug output and logging for easier identification
	     * - Serialization to maintain readable project files
	     * - Search and filtering operations in the editor
	     * 
	     * The name does not need to be unique and can be changed at runtime
	     * without affecting object identity or references. It should be
	     * descriptive enough to help developers and users identify the object's
	     * purpose or content.
	     * 
	     * @note Default value "Uninitialized" helps identify objects that haven't been properly configured
	     * @note Names are not used for object lookup - use UUID for identification
	     */
	    std::string name = "Uninitialized";
	    
        /**
         * @brief Globally unique identifier for persistent object reference
         * 
         * The UUID provides a stable, unique identity for this object that persists
         * across application sessions and enables safe serialization of object
         * relationships. This identifier is used by:
         * 
         * - Asset management system for tracking loaded resources
         * - Serialization system for maintaining object references in saved files
         * - Event system for targeted object notifications and callbacks
         * - Cross-module communication where objects need to be referenced safely
         * - Undo/redo system for tracking object state changes
         * 
         * The UUID is automatically generated when objects are created and should
         * remain constant throughout the object's lifetime. It is intentionally
         * excluded from assignment operations to preserve object identity.
         * 
         * @warning Never manually modify this UUID after object creation
         * @note UUID() default constructor creates a null/invalid UUID
         * @note Use UUID::Generate() to create valid unique identifiers
         */
        UUID uuid = UUID();
        
	    /**
	     * @brief Runtime type identifier for polymorphic operations and type checking
	     * 
	     * This enum value identifies the specific type of this object at runtime,
	     * enabling safe type casting and polymorphic behavior without requiring
	     * expensive RTTI operations. The type information is used by:
	     * 
	     * - Asset system for loading the correct asset serializer and processor
	     * - Rendering system for dispatching objects to appropriate render passes
	     * - Editor UI for showing type-specific property panels and tools
	     * - Serialization system for selecting the correct deserialization path
	     * - Reflection system for accessing type-specific metadata and methods
	     * 
	     * The type should be set during object construction and generally should not
	     * change during the object's lifetime, as many systems cache type information
	     * for performance optimization.
	     * 
	     * @note ObjectType::None indicates an unspecified or invalid object type
	     * @note Derived classes should override this with their specific type
	     */
	    ObjectType type = ObjectType::None;

	    /**
	     * @brief GPU synchronization flag indicating when object data needs GPU upload
	     * 
	     * This flag tracks whether the object's data has been modified since the last
	     * GPU synchronization, helping optimize rendering performance by avoiding
	     * unnecessary GPU uploads. The flag is used by:
	     * 
	     * - Vertex buffer systems to update mesh geometry when modified
	     * - Texture systems to re-upload image data after changes
	     * - Material systems to update uniform buffers when properties change
	     * - Descriptor management to rebuild resource bindings when needed
	     * - Culling systems to update spatial data structures
	     * 
	     * Performance considerations:
	     * - Objects start with gpuDirty=true to ensure initial GPU upload
	     * - Assignment operations automatically set gpuDirty=true for safety
	     * - Rendering systems should check this flag before expensive GPU operations
	     * - Flag should be cleared after successful GPU synchronization
	     * 
	     * @todo The current design may need refinement for fine-grained GPU state tracking
	     * @note This is a simple boolean flag - consider bitflags for specific GPU state types
	     * @warning Always set to true when modifying object data that affects GPU resources
	     */
	    bool gpuDirty = true;

        /**
         * @brief Assignment operator with automatic GPU dirty flag management
         * 
         * Copies object data from another Object instance while preserving this object's
         * unique identity (UUID). The assignment automatically sets the GPU dirty flag
         * to ensure that any GPU resources associated with this object are updated
         * to reflect the new data.
         * 
         * Behavior details:
         * - Copies name and type from the source object
         * - Preserves the target object's UUID to maintain identity
         * - Automatically sets gpuDirty=true to trigger GPU synchronization
         * - Does not affect reference counting (handled by RefCounted base class)
         * 
         * This design ensures that object identity is preserved during assignment
         * operations while still allowing data updates. The automatic GPU dirty
         * flag prevents rendering artifacts that could occur if GPU resources
         * were not updated after data changes.
         * 
         * @param rhs The source Object to copy data from
         * @return Reference to this object for method chaining
         * 
         * @note UUID is intentionally not copied to preserve object identity
         * @note GPU dirty flag is always set regardless of whether data actually changed
         * @warning This is a shallow copy - derived classes should override if deep copying is needed
         * 
         * @code
         * auto object1 = CreateRef<CustomObject>("Object1");
         * auto object2 = CreateRef<CustomObject>("Object2");
         * 
         * UUID originalId = object1->uuid;
         * 
         * *object1 = *object2; // Copy data but preserve identity
         * 
         * SEDX_CORE_ASSERT(object1->uuid == originalId, "UUID should be preserved");
         * SEDX_CORE_ASSERT(object1->name == object2->name, "Data should be copied");
         * SEDX_CORE_ASSERT(object1->gpuDirty == true, "GPU sync should be required");
         * @endcode
         */
        Object &operator=(const Object &rhs)
	    {
	        name = rhs.name;
	        type = rhs.type;
	        gpuDirty = true;
	        return *this;
	    }

	    /**
	     * @brief Virtual destructor ensures proper cleanup of derived object types
	     * 
	     * Provides a virtual destructor to enable proper polymorphic destruction
	     * when Object-derived instances are deleted through base class pointers.
	     * This is essential for the reference counting system and ensures that
	     * derived class destructors are called correctly.
	     * 
	     * The destructor is part of the RefCounted interface and integrates with
	     * the smart pointer system to provide automatic memory management.
	     * When the reference count reaches zero, this destructor ensures that
	     * all derived class resources are properly released.
	     * 
	     * @note Override keyword indicates this overrides RefCounted::~RefCounted()
	     * @note Derived classes should implement their own destructors for resource cleanup
	     * @note The destructor is automatically called by the reference counting system
	     */
	    virtual ~Object() override;
	    
	    /**
	     * @brief Serialization interface for persistent object storage (commented out)
	     * 
	     * This method would provide a standardized interface for serializing object
	     * data to persistent storage formats. When implemented, it would enable:
	     * - Scene file saving and loading with full object state preservation
	     * - Asset pack creation for efficient distribution
	     * - Network synchronization of object state between clients
	     * - Undo/redo system state capture and restoration
	     * 
	     * @param ser Reference to the serializer that handles the actual data writing
	     * 
	     * @note Currently commented out - serialization is handled by derived classes
	     * @todo Consider implementing a base serialization interface for common Object data
	     */
	    //virtual void Serialize(SerializeWriter &ser) = 0;
	};

}

/// -------------------------------------------------------
