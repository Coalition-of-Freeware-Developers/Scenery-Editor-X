/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* graph_serializer.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <SceneryEditorX/serialization/asset_serializer.h>
#include <SceneryEditorX/utils/pointers.h>
#include <string_view>
#include <vector>

#include <SceneryEditorX/utils/reflection/type_values.h>

// Forward declarations for missing types
namespace choc::value { class Value; }

/// -------------------------------------------------------

namespace SceneryEditorX
{
    ///< Type definitions that need to be declared before use
    enum class StorageKind
    {
        Value,
        Reference,
        Array
    };

    enum class NodeType
    {
        Simple,
        Complex,
        Input,
        Output
    };

    enum class PinKind
    {
        Input,
        Output
    };

    ///< Forward declarations
    struct Pin;
    struct Link;
    
    ///< Graph Node class (different from scene Node)
    class Node
    {
    public:
        UUID ID;
        std::string Category;
        std::string Name;
        std::string Description;
        std::string State;
        NodeType Type;
        ImColor Color;
        ImVec2 Size;
        std::vector<Pin*> Inputs;
        std::vector<Pin*> Outputs;
        
        Node() = default;
        virtual ~Node() = default;
    };

    ///< Basic Pin structure declaration
    struct Pin
    {
        UUID ID;
        UUID NodeID;
        std::string Name;
        StorageKind Storage;
        PinKind Kind;
        Types::Value Value;

        Pin() = default;
        Pin(const Pin&) = default;
        Pin& operator=(const Pin&) = default;
        
        virtual ~Pin() = default;
        [[nodiscard]] virtual std::string_view GetTypeString() const { return "Pin"; }
    };

    // Basic Link structure declaration  
    struct Link
    {
        UUID ID;
        UUID StartPinID;
        UUID EndPinID;
        ImColor Color;

        Link() = default;
        Link(const UUID startPin, const UUID endPin) : ID(), StartPinID(startPin), EndPinID(endPin) {}
    };

	namespace Utils
    {
		std::string StorageKindToString(StorageKind storageKind);
		StorageKind StorageKindFromString(const std::string_view& storageKindStr);

		std::string NodeTypeToString(NodeType nodeType);
		NodeType NodeTypeFromString(const std::string_view& nodeTypeStr);

	}

	/// -------------------------------------------------------

	/**
	 * Graph serializer base provides static utilities for implementation
	 * to serialize and deserialize graph data.
	 *
	 * Implementation can deserialize nodes on its own, or use handy static
	 * TryLoadNodes() utility, which first deserializes the information about potential
	 * Node and Pins and then passes this info to the implementation factory callbacks
	 * to handle concrete construction and validation.
	 */
	class DefaultGraphSerializer : public AssetSerializer
	{
	public:

		static void SerializeNodes(nlohmann::json& out, const std::vector<Node*>& nodes, std::function<void(nlohmann::json&, const Node*)> nodeCallBack = {});
		static void SerializeLinks(nlohmann::json& out, const std::vector<Link>& links, std::function<void(nlohmann::json&, const Link&)> linkCallBack = {});

		/// -------------------------------------------------------------------------------

		/**
		 * Implementation must provide this factory to deserialize nodes with static
		 * TryLoadNodes() function.
		 */
		struct DeserializationFactory
		{
			///< Deserialized info about a Pin of a Node, may or may not be valid
			struct PinCandidate : Pin
			{
				using Pin::Pin;

				std::string TypeString;	///< implementation specific

                static int GetType() { return -1; }
                [[nodiscard]] virtual std::string_view GetTypeString() const override { return TypeString; }
			};

			///< Deserialized info about a Node, may or may not be valid
			struct NodeCandidate
			{
				UUID ID;
				std::string Category;
				std::string Name;
				std::string Description;
				NodeType Type;
				uint32_t NumInputs;
				uint32_t NumOutputs;
			};

			/**
			 * This factory function should construct a Node with default Input and Output pins
			 * as well as assign default and deserialized values from the candidate if required.
			 */
			std::function<Node*(const NodeCandidate& candidate,
								const std::optional<std::vector<PinCandidate>>& inputs,
								const std::optional<std::vector<PinCandidate>>& outputs)> ConstructNode;

			/**
			 * This factory function should deserialize values from the candidate to the previously
			 * constructed default factory pin as well as do any required validation.
			 */
			std::function<bool(const PinCandidate& candidate, Pin* factoryPin, const NodeCandidate& nodeCandidate)> DeserializePin;

			/**
			 * This factory function should perform any extra post node-construction work
			 * For example, deserializing derived type data members
			 */
			std::function<void(nlohmann::json& node, Node* newNode)> PostConstructNode;
		};

		/**
		 * Try to load graph Nodes from JSON. This function parses JSON into NodeCandidates and PinCandidates,
		 * which are then passed to implementation provided factory to deserialize and validate.
		 *
		 * @note This function throws an exception if deserialization fails, the caller must handle it!
		 */
		static void TryLoadNodes(nlohmann::json& in, std::vector<Node*>& nodes, const DeserializationFactory& factory);
		static void TryLoadLinks(nlohmann::json& in, std::vector<Link>& links, std::function<void(nlohmann::json&, Link&)> linkCallback = {});
	};


}

/// -------------------------------------------------------
