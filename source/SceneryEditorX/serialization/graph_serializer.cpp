/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* graph_serializer.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <SceneryEditorX/serialization/graph_serializer.h>
#include <SceneryEditorX/serialization/serialization_macros.h>
#include <SceneryEditorX/utils/string_utils.h>

/// -----------------------------------------------------------

namespace SceneryEditorX
{

	/*
	namespace Utils
    {
		std::string StorageKindToString(StorageKind storageKind)
		{
			return std::string(magic_enum::enum_name<StorageKind>(storageKind));
		}

		StorageKind StorageKindFromString(const std::string_view& storageKindStr)
		{
			if (auto optValue = magic_enum::enum_cast<StorageKind>(storageKindStr))
				return *optValue;

			SEDX_CORE_ASSERT(false, "Unknown Storage Kind");
			return StorageKind::Value;
		}

		std::string NodeTypeToString(NodeType nodeType)
		{
			return std::string(magic_enum::enum_name<NodeType>(nodeType));
		}

		NodeType NodeTypeFromString(const std::string_view& nodeTypeStr)
		{
			if (auto optValue = magic_enum::enum_cast<NodeType>(nodeTypeStr))
				return *optValue;

			SEDX_CORE_ASSERT(false, "Unknown Node Type");
			return NodeType::Simple;
		}

	}
	*/

    /// -------------------------------------------------------

	/*
	void DefaultGraphSerializer::SerializeNodes(nlohmann::json& out, const std::vector<GraphNode*>& nodes, std::function<void(nlohmann::json&, const GraphNode*)> nodeCallback)
	{
		nlohmann::json nodesArray = nlohmann::json::array();

		for (const auto node : nodes)
		{
			nlohmann::json nodeObj = nlohmann::json::object();

			const ImVec4& nodeCol = node->Color.Value;
			const ImVec2& nodeSize = node->Size;
			const Vec4 nodeColOut(nodeCol.x, nodeCol.y, nodeCol.z, nodeCol.w);
			const Vec2 nodeSizeOut(nodeSize.x, nodeSize.y);

			SEDX_SERIALIZE_PROPERTY(ID, node->ID, nodeObj);
			SEDX_SERIALIZE_PROPERTY(Category, node->Category, nodeObj);
			SEDX_SERIALIZE_PROPERTY(Name, node->Name, nodeObj);
			if (!node->Description.empty())
			{
				SEDX_SERIALIZE_PROPERTY(Description, node->Description, nodeObj);
			}
			SEDX_SERIALIZE_PROPERTY(Color, nodeColOut, nodeObj);
			SEDX_SERIALIZE_PROPERTY(Type, Utils::NodeTypeToString(node->Type), nodeObj);
			SEDX_SERIALIZE_PROPERTY(Size, nodeSizeOut, nodeObj);
			SEDX_SERIALIZE_PROPERTY(Location, node->State, nodeObj);

			nlohmann::json inputsArray = nlohmann::json::array();
			for (auto in : node->Inputs)
			{
				nlohmann::json inputObj = nlohmann::json::object();

				SEDX_SERIALIZE_PROPERTY(ID, in->ID, inputObj);
				SEDX_SERIALIZE_PROPERTY(Name, in->Name, inputObj);
				SEDX_SERIALIZE_PROPERTY(Type, std::string(in->GetTypeString()), inputObj);
				SEDX_SERIALIZE_PROPERTY(Storage, Utils::StorageKindToString(in->Storage), inputObj);
				SEDX_SERIALIZE_PROPERTY(Value, in->Value, inputObj);

				inputsArray.push_back(inputObj);
			}
			nodeObj["Inputs"] = inputsArray;

			nlohmann::json outputsArray = nlohmann::json::array();
			for (auto output : node->Outputs)
			{
				nlohmann::json outputObj = nlohmann::json::object();

				SEDX_SERIALIZE_PROPERTY(ID, output->ID, outputObj);
				SEDX_SERIALIZE_PROPERTY(Name, output->Name, outputObj);
				SEDX_SERIALIZE_PROPERTY(Type, std::string(output->GetTypeString()), outputObj);
				SEDX_SERIALIZE_PROPERTY(Storage, Utils::StorageKindToString(output->Storage), outputObj);
				SEDX_SERIALIZE_PROPERTY(Value, output->Value, outputObj);

				outputsArray.push_back(outputObj);
			}
			nodeObj["Outputs"] = outputsArray;

			if (nodeCallback)
                nodeCallback(nodeObj, node);

            nodesArray.push_back(nodeObj);
		}
		out["Nodes"] = nodesArray;
	}
	*/

    /// -------------------------------------------------------

	/*
	void DefaultGraphSerializer::SerializeLinks(nlohmann::json& out, const std::vector<Link>& links, std::function<void(nlohmann::json&, const Link&)> linkCallback)
	{
		nlohmann::json linksArray = nlohmann::json::array();

		for (const auto& link : links)
		{
			nlohmann::json linkObj = nlohmann::json::object();

			const auto& col = link.Color.Value;
			const Vec4 colOut(col.x, col.y, col.z, col.w);

			SEDX_SERIALIZE_PROPERTY(ID, link.ID, linkObj);
			SEDX_SERIALIZE_PROPERTY(StartPinID, link.StartPinID, linkObj);
			SEDX_SERIALIZE_PROPERTY(EndPinID, link.EndPinID, linkObj);
			SEDX_SERIALIZE_PROPERTY(Color, colOut, linkObj);

			if (linkCallback)
                linkCallback(linkObj, link);

            linksArray.push_back(linkObj);
		}
		out["Links"] = linksArray;
	}
	*/

    /// -------------------------------------------------------

	/*
	using PinCandidate = DefaultGraphSerializer::DeserializationFactory::PinCandidate;
	using NodeCandidate = DefaultGraphSerializer::DeserializationFactory::NodeCandidate;

	[[nodiscard]] static std::optional<std::vector<PinCandidate>> TryLoadInputs(const nlohmann::json& inputs, const NodeCandidate& node)
	{
		if (!inputs.is_array())
			return {};

		std::vector<PinCandidate> list;

		uint32_t index = 0;
		for (const auto& in : inputs)
		{
			UUID ID;
			std::string pinName;
			std::string valueStr;
            Values::Value value;
			std::string pinType;
			std::string pinStorage;

			SEDX_DESERIALIZE_PROPERTY(ID, ID, in, uint64_t(0))
			SEDX_DESERIALIZE_PROPERTY(Name, pinName, in, std::string())
			SEDX_DESERIALIZE_PROPERTY(Type, pinType, in, std::string())
			SEDX_DESERIALIZE_PROPERTY(Storage, pinStorage, in, std::string())
			SEDX_DESERIALIZE_PROPERTY(Value, valueStr, in, std::string())
            SEDX_DESERIALIZE_PROPERTY(Value, value, in, Values::Value())

			// TODO: load legacy saved valueStr, or manually rewrite JSON files for the new format?

			PinCandidate candidate;
			candidate.ID = ID;
			candidate.Name = pinName;
			candidate.Storage = Utils::StorageKindFromString(pinStorage);
			candidate.Value = value;
			candidate.Kind = PinKind::Input;
			candidate.TypeString = pinType;

			list.push_back(candidate);
		}

		return list;
	}
	*/

    /// -------------------------------------------------------

	/*
	[[nodiscard]] static std::optional<std::vector<PinCandidate>> TryLoadOutputs(const nlohmann::json& outputs, const NodeCandidate& node)
	{
		if (!outputs.is_array())
			return {};

		std::vector<PinCandidate> list;

		uint32_t index = 0;
		for (const auto& out : outputs)
		{
			UUID ID;
			std::string pinName;
			std::string valueStr;
            Values::Value value;
			std::string pinType;
			std::string pinStorage;

			SEDX_DESERIALIZE_PROPERTY(ID, ID, out, uint64_t(0))
			SEDX_DESERIALIZE_PROPERTY(Name, pinName, out, std::string())
			SEDX_DESERIALIZE_PROPERTY(Type, pinType, out, std::string())
			SEDX_DESERIALIZE_PROPERTY(Storage, pinStorage, out, std::string())
			SEDX_DESERIALIZE_PROPERTY(Value, valueStr, out, std::string());
			SEDX_DESERIALIZE_PROPERTY(Value, value, out, Values::Value())

			PinCandidate candidate;
			candidate.ID = ID;
			candidate.Name = pinName;
			candidate.Storage = Utils::StorageKindFromString(pinStorage);
			candidate.Value = value;
			candidate.Kind = PinKind::Output;
			candidate.TypeString = pinType;

			list.push_back(candidate);
		}

		return list;
	}
	*/

    /// -------------------------------------------------------

	/*
	void DefaultGraphSerializer::TryLoadNodes(nlohmann::json& data, std::vector<GraphNode*>& nodes, const DeserializationFactory& factory)
	{
		/*
		 * TODO: Remove exceptions when we implement a dummy "invalid" node/pin types to display to the user?
		 * for now, we just stop loading the whole graph if there's any error in the JSON document and/or node fabrication
		 #1#
		if (!data.contains("Nodes") || !data["Nodes"].is_array())
			return;

		for (auto& node : data["Nodes"])
		{
			UUID nodeID;
			std::string nodeCategory;
			std::string nodeName;
			std::string nodeDesc;
			std::string location;
			std::string nodeTypeStr;
			Vec4 nodeColor;
			Vec2 nodeSize;

			SEDX_DESERIALIZE_PROPERTY(ID, nodeID, node, uint64_t(0));
			SEDX_DESERIALIZE_PROPERTY(Category, nodeCategory, node, std::string());
			SEDX_DESERIALIZE_PROPERTY(Name, nodeName, node, std::string());
			SEDX_DESERIALIZE_PROPERTY(Description, nodeDesc, node, std::string());
			if (node.contains("Colour"))
			{
				SEDX_DESERIALIZE_PROPERTY(Colour, nodeColor, node, Vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
			else
			{
				SEDX_DESERIALIZE_PROPERTY(Color, nodeColor, node, Vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
			SEDX_DESERIALIZE_PROPERTY(Type, nodeTypeStr, node, std::string())	// TODO: JSON serialization for enums to/from string
			SEDX_DESERIALIZE_PROPERTY(Size, nodeSize, node, Vec2())
			SEDX_DESERIALIZE_PROPERTY(Location, location, node, std::string())

			const NodeType nodeType = Utils::NodeTypeFromString(nodeTypeStr);	// TODO: JP. this could also be implementation specific types

			NodeCandidate candidate;
			candidate.ID = nodeID;
			candidate.Category = nodeCategory;
			candidate.Name = nodeName;
			candidate.Description = nodeDesc;
			candidate.Type = nodeType;
			candidate.NumInputs = node.contains("Inputs") && node["Inputs"].is_array() ? static_cast<uint32_t>(node["Inputs"].size()) : 0;
			candidate.NumOutputs = node.contains("Outputs") && node["Outputs"].is_array() ? static_cast<uint32_t>(node["Outputs"].size()) : 0;

			std::optional<std::vector<PinCandidate>> candidateInputs;
			std::optional<std::vector<PinCandidate>> candidateOutputs;

			if (node.contains("Inputs") && node["Inputs"].is_array())
			{
				if (!(candidateInputs = TryLoadInputs(node["Inputs"], candidate)))
                    ///< JSON file contains "Inputs" but we've failed to parse them
                    throw std::runtime_error("Failed to load editor Node inputs '" + candidate.Name + "' inputs.");
                if (candidateInputs->size() != candidate.NumInputs)
                    ///< JSON file contains different number of entries for the Inputs than we've managed to deserialize
                    throw std::runtime_error("Deserialized Node Inputs list doesn't match the number of serialized Node '" + candidate.Name + "' inputs.");
            }

			if (node.contains("Outputs") && node["Outputs"].is_array())
			{
				if (!(candidateOutputs = TryLoadOutputs(node["Outputs"], candidate)))
                    ///< JSON file contains "Outputs" but we've failed to parse them
                    throw std::runtime_error("Failed to load editor Node '" + candidate.Name + "' outputs.");
                if (candidateOutputs->size() != candidate.NumOutputs)
                    ///< JSON file contains different number of entries for the Outputs than we've managed to deserialize
                    throw std::runtime_error("Deserialized Node Outputs list doesn't match the number of serialized Node '" + candidate.Name + "' outputs.");
            }

			///< This is not going to load old Node configurations and enforce old to new Topology compatibility
			///< TODO: Might want to still load old topology as an "invalid" dummy node to display it to the user
			GraphNode* newNode = factory.ConstructNode(candidate, candidateInputs, candidateOutputs);

			if (!newNode)
                throw std::runtime_error("Failed to construct deserialized Node '" + candidate.Name + "'.");

            newNode->ID = candidate.ID;
			newNode->State = location;
			newNode->Color = ImColor(nodeColor.x, nodeColor.y, nodeColor.z, nodeColor.w);
			newNode->Size = ImVec2(nodeSize.x, nodeSize.y);

			if (newNode->Inputs.size() != candidate.NumInputs || newNode->Outputs.size() != candidate.NumOutputs)
			{
				//delete newNode;

				// Factory Node might have changed, and we've deserialized an old version of the Node
				//throw std::runtime_error("Deserialized Node topology doesn't match factory Node '" + candidate.Name + "'.");
				//SEDX_CONSOLE_LOG_WARN("Deserialized Node topology doesn't match factory Node '" + candidate.Name + "'.");
			}

			// Implementation specific construction and/or validation of Node Pins

			if (candidateInputs)
			{
				for(uint32_t i = 0; i < newNode->Inputs.size(); ++i) {
					Pin* factoryPin = newNode->Inputs[i];

					// find candidate with same name as factory pin
					PinCandidate* candidatePin = nullptr;
					for (auto& pin: *candidateInputs)
					{
						if (Utils::String::RemoveWhitespace(pin.Name) == Utils::String::RemoveWhitespace(factoryPin->Name))
						{
							candidatePin = &pin;
							break;
						}
					}

					if (candidatePin)
					{
						if (!factory.DeserializePin(*candidatePin, factoryPin, candidate))
						{
							delete newNode;
							///< This error is pushed by the implementation
							throw std::runtime_error("Failed to deserialize/validate input Pin '" + candidatePin->Name + "' for a Node '" + candidate.Name + "'.");
						}

						factoryPin->ID = candidatePin->ID;
						///< There could be multiple candidates with same name, so remove deserialized candidate from list
						std::erase_if(*candidateInputs, [candidatePin](const PinCandidate& pin) { return pin.ID == candidatePin->ID; });
					}
					else
					{
						factoryPin->ID = UUID();
					}
					factoryPin->NodeID = candidate.ID;
					factoryPin->Kind = PinKind::Input;
				}
			}

			if (candidateOutputs)
			{
				for (uint32_t i = 0; i < newNode->Outputs.size(); ++i)
				{
					Pin* factoryPin = newNode->Outputs[i];

					///< find candidate with same name as factory pin
					PinCandidate* candidatePin = nullptr;
					for (auto& pin : *candidateOutputs)
					{
						if (Utils::String::RemoveWhitespace(pin.Name) == Utils::String::RemoveWhitespace(factoryPin->Name))
						{
							candidatePin = &pin;
							break;
						}
					}

					if (candidatePin)
					{
						if (!factory.DeserializePin(*candidatePin, factoryPin, candidate))
						{
							delete newNode;
							///< This error is pushed by the implementation
							throw std::runtime_error("Failed to deserialize/validate output Pin '" + candidatePin->Name + "' for a Node '" + candidate.Name + "'.");
						}

						factoryPin->ID = candidatePin->ID;
						///< There could be multiple candidates with same name, so remove deserialized candidate from list
						std::erase_if(*candidateOutputs, [candidatePin](const PinCandidate& pin) { return pin.ID == candidatePin->ID; });
					}
					else
					{
						factoryPin->ID = UUID();
					}
					factoryPin->NodeID = candidate.ID;
					factoryPin->Kind = PinKind::Output;
				}
			}

			if (factory.PostConstructNode)
                factory.PostConstructNode(node, newNode);

            nodes.push_back(newNode);
		}
	}
	*/

    /// -------------------------------------------------------

	/*
	void DefaultGraphSerializer::TryLoadLinks(nlohmann::json &data, std::vector<Link>& links, std::function<void(nlohmann::json&, Link&)> linkCallback)
	{
		if (!data.contains("Links") || !data["Links"].is_array())
			return;

		for (auto& link : data["Links"])
		{
			UUID ID;
			UUID StartPinID;
			UUID EndPinID;
			Vec4 color;

			SEDX_DESERIALIZE_PROPERTY(ID, ID, link, uint64_t(0));
			SEDX_DESERIALIZE_PROPERTY(StartPinID, StartPinID, link, uint64_t(0));
			SEDX_DESERIALIZE_PROPERTY(EndPinID, EndPinID, link, uint64_t(0));
			if (link.contains("Colour"))
			{
				SEDX_DESERIALIZE_PROPERTY(Color, color, link, Vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
			else
			{
				SEDX_DESERIALIZE_PROPERTY(Color, color, link, Vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
			links.emplace_back(StartPinID, EndPinID);
			links.back().ID = ID;
			links.back().Color = ImColor(color.x, color.y, color.z, color.w);
			if (linkCallback)
			{
				linkCallback(link, links.back());
			}
		}
	}
	*/

    /// -------------------------------------------------------

}

/// -----------------------------------------------------------
