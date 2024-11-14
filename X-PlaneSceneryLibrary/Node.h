//Module:	Node
//Author:	Connor Russell
//Date:		9/5/2023 11:48:29 AM
//Purpose:	Encapsulates a Node data structure

//Compile once
#pragma once

//Include necessary headers
#include <vector>
#include <string>
#include <map>

namespace XSLGeoutils
{
	/// <summary>
	/// Class to represent a node in space. This is a 3D point with optional UV coordinates and properties, and no support for bezeirs. 
	/// </summary>
	struct Node
	{
		double X{ 0 };
		double Y{ 0 };
		double Z{ 0 };
		double U{ 0 };
		double V{ 0 };
		std::map<std::string, std::string> Properties;
		bool CurveStart{ false };
		bool CurveEnd{ false };

		//Default Constructor
		inline Node() {}
		inline Node(double InX, double InY) : X(InX), Y(InY) {}
		inline Node(Node& InNode) : X(InNode.X), Y(InNode.Y), Z(InNode.Z), U(InNode.U), V(InNode.V) { Properties = InNode.Properties; }

		inline Node operator=(const Node& InNode)
		{
			X = InNode.X;
			Y = InNode.Y;
			Z = InNode.Z;
			U = InNode.U;
			V = InNode.V;
			Properties = InNode.Properties;
			return *this;
		}

		inline bool SameAs(const Node& other) const
		{
			return (X == other.X && Y == other.Y && Z == other.Z && U == other.U && V == other.V);
		}

		inline bool Colocated(const Node& other) const
		{
			return (X == other.X && Y == other.Y && Z == other.Z);
		}
	};

	/// <summary>
	/// Class to represent a node in space. This is a 3D point with optional UV coordinates and properties, and support for bezeirs. This is generally what you should use.
	/// <summary>
	struct BezeirNode
	{
		bool HasEntryControlPoint{ false };
		bool HasExitControlPoint{ false };
		double X{ 0 };
		double Y{ 0 };
		double Z{ 0 };
		double XCtl1{ 0 };
		double YCtl1{ 0 };
		double XCtl2{ 0 };
		double YCtl2{ 0 };
		double U{ 0 };
		double V{ 0 };
		std::map<std::string, std::string> Properties;

		inline BezeirNode() : HasEntryControlPoint(false), HasExitControlPoint(false) {}
		inline BezeirNode(Node InPoint) : HasEntryControlPoint(false), HasExitControlPoint(false), X(InPoint.X), Y(InPoint.Y), Z(InPoint.Z), U(InPoint.U), V(InPoint.V) { Properties = InPoint.Properties; }
		inline BezeirNode(Node InPoint, Node InControl1, Node InControl2) : HasEntryControlPoint(true), HasExitControlPoint(true), X(InPoint.X), Y(InPoint.Y), Z(InPoint.Z), XCtl1(InControl1.X), YCtl1(InControl1.Y), XCtl2(InControl2.X), YCtl2(InControl2.Y), U(InPoint.U), V(InPoint.V) { Properties = InPoint.Properties; }
		inline BezeirNode(BezeirNode& InNode) : HasEntryControlPoint(InNode.HasEntryControlPoint), HasExitControlPoint(InNode.HasExitControlPoint), X(InNode.X), Y(InNode.Y), Z(InNode.Z), XCtl1(InNode.XCtl1), YCtl1(InNode.YCtl1), XCtl2(InNode.XCtl2), YCtl2(InNode.YCtl2), U(InNode.U), V(InNode.V) { Properties = InNode.Properties; }

		BezeirNode operator=(const BezeirNode& InNode)
		{
			HasEntryControlPoint = InNode.HasEntryControlPoint;
			HasExitControlPoint = InNode.HasExitControlPoint;
			X = InNode.X;
			Y = InNode.Y;
			Z = InNode.Z;
			XCtl1 = InNode.XCtl1;
			YCtl1 = InNode.YCtl1;
			XCtl2 = InNode.XCtl2;
			YCtl2 = InNode.YCtl2;
			U = InNode.U;
			V = InNode.V;
			Properties = InNode.Properties;
			
			return *this;
		}

		//Default Constructor
		inline BezeirNode() {}

		//Colocated
		inline bool Colocated(const BezeirNode& other) const
		{
			return X == other.X && Y == other.Y && Z == other.Z && XCtl1 == other.XCtl1 && YCtl1 == other.YCtl1 && XCtl2 == other.XCtl2 && YCtl2 == other.YCtl2;
		}
	};

	/// <summary>
	/// Class to represent a series of BezeirNodes in space, with functions for converting between the X-Plane format, the natural bezeir format, and a subdivided straight-line format.
	/// </summary>
	class Winding
	{
	public:
		std::vector<BezeirNode> Nodes;
		bool Closed;

		/// <summary>
		/// Loads the XP nodes (in the X-Plane bezeir curve format) into the winding
		/// </summary>
		/// <param name="InNodes">The nodes</param>
		/// <param name="InClosed">Whether the winding is closed</param>
		void LoadFromXPNodes(std::vector<Node>& InNodes, bool InClosed);

		/// <summary>
		/// Loads the nodes directly, without any accounting for curves.
		/// </summary>
		/// <param name="InNodes">Nodes to load</param>
		/// <param name="InClosed">Whether the winding is closed</param>
		void LoadFromStraightNodes(std::vector<Node>& InNodes, bool InClosed);

		/// <summary>
		/// Gets the nodes in the X-Plane format (bezeir curve format)
		/// </summary>
		/// <returns>The nodes in the X-Plane format (bezeir curve format)</returns>
		std::vector<Node> GetXPNodes();

		/// <summary>
		/// Gets the nodes resolved into straight lines
		/// </summary>
		/// <param name="CurveSubdivisions">Number of subdivisions for each curve</param>
		/// <returns>The nodes resolved into straight lines</returns>
		/// <remarks>CurveSubdivisions is the number of subdivisions for each curve. 10 is a good value for most cases.</remarks>
		std::vector<Node> GetRealNodes(int CurveSubdivisions = 10);

		/// <summary>
		/// Determines if a face is clockwise or counter-clockwise
		/// </summary>
		/// <param name="vertices">Vector of Nodes</param>
		/// <returns>True if clockwise, otherwise false</returns>
		inline bool IsClockwise()
		{
			auto vertices = GetRealNodes(3);
			double sum = 0.0;
			for (size_t i = 0; i < vertices.size(); i++)
			{
				Node v1 = vertices[i];
				Node v2 = vertices[(i + 1) % vertices.size()]; // next Node, or first if we're at the end
				sum += (v2.X - v1.X) * (v2.Y + v1.Y);
			}
			return sum > 0;
		}
	};
}