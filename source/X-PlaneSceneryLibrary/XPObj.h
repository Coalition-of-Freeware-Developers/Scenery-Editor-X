//Module:	XPObj
//Author:	Connor Russell
//Date:		10/11/2024 7:11:50 PM
//Purpose:

#pragma once
#include "XPAsset.h"
#include "XPLayerGroups.h"

namespace XPAsset
{
    /**
     * @brief Represents a draw call (TRIS) in an X-Plane obj8 file
     */
	class ObjDrawCall
	{
	public:
	    size_t idxStart;                           //Zero based start index of the first vertex
	    size_t idxEnd;                             //Zero based end vertex to draw to
	    int intLayerGroup{XPLayerGroups::OBJECTS}; //Layer group
	    bool bDraped{false};                       //Is this draw call in the object draped?
	
	    //There theoretically could be other properties, but are not implemented here.
	};

   /**
    * @brief Represents a vertex in an X-Plane obj8 file
    *
    * @
	*/
	class Vertex
	{
	public:
	    double X;
	    double Y;
	    double Z;
	    double NX;
	    double NY;
	    double NZ;
	    double U;
	    double V;
	};

    /**
     * @brief Represents an X-Plane obj8 file
	 */
	class Obj : public Asset
	{
	public:
	    std::vector<XPAsset::Vertex> Vertices; //Vertices
	    std::vector<size_t> Indices;           //Indices. These are zero based indicies of verticies
	    std::vector<XPAsset::ObjDrawCall>
	        DrawCalls; //These are draw calls that point to the indicies, and contain state data
	
	    //All paths are relative to the obj path
	    std::filesystem::path pDrapedBaseTex;   //The draped base texture
	    std::filesystem::path pDrapedNormalTex; //The draped normal texture. Material is typically in the alpha channel
	    std::filesystem::path
	        pDrapedMaterialTex; //The draped material texture. This is only used if specified to have a dedicated material map - this is generally bundled in with the normal int he b/alpha channels
	    bool bHasDrapedBaseTex{false};
	    bool bHasDrapedNormalTex{false};   //Material is typically in the b/alpha channel
	    bool bHasDrapedMaterialTex{false}; //Only set if the material is in a separate texture
	
	    void *Refcon; //A reference to an object that can be used to store additional data acociated with this object
	
	    /**
	     * @brief Loads the object
		 *
		 * @param InPath = Path to the obj
		 * @returns True on success, false on failure
	     */
	    bool Load(const std::filesystem::path &InPath);
	};

}
