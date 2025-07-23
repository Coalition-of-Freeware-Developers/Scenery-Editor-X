//Module:	XPObj
//Author:	Connor Russell
//Date:		10/11/2024 7:11:58 PM
//Purpose:	Implements XPObj.h
#include "XPObj.h"
#include "TextUtils.h"
#include <fstream>
#include <sstream>

/**
* @brief Loads the object
*
* @Param InPath = Path to the obj
* @return True on success, false on failure
*/
bool XPAsset::Obj::Load(const std::filesystem::path &InPath)
{
    try
    {
        ///<make sure the file exists and ends in .obj
        if (!std::filesystem::exists(InPath) || InPath.extension() == "obj")
            return false;

        ///< Set the real path
        pReal = InPath;

        ///< Open
        std::ifstream ObjFile(InPath);

        ///< Make sure it opened
        if (!ObjFile.is_open())
            return false;

        ///< Buffers
        std::string strLine;
        std::string strCommand;
        bool bInDraped = false;
        int intCurrentDrapedLayerGroup = XPLayerGroups::Resolve("objects", 0);

        ///< Read line by line
        while (ObjFile.good())
        {
            ///< Line buffer
            std::stringstream ssLine;

            ///< Read the line
            strCommand = "";
            getline(ObjFile, strLine);
            ssLine.str(strLine);
            ObjFile.peek(); ///< Set flags

            ///< Get the command
            ssLine >> strCommand;

            ///< Draped commands set the draped flags, which determine whether draw calls are saved
            if (strCommand == "ATTR_draped")
                bInDraped = true;
            else if (strCommand == "ATTR_no_draped")
                bInDraped = false;

            ///< Generic layer group, only applies if we don't have a layer group already
            else if (strCommand == "ATTR_layer_group")
            {
                ///< Format: ATTR_layer_group group offset
                std::string strLayerGroupArgs[2];

                ///< Read the args
                ssLine >> strLayerGroupArgs[0] >> strLayerGroupArgs[1];

                ///< Set the layer group. An object can only be in a single *non-draped* layer group, and said layer group does not effect draped layer groups, so we can just set it directly.
                intLayerGroup = XPLayerGroups::Resolve(strLayerGroupArgs[0], stoi(strLayerGroupArgs[1]));
            }

            ///< Generic layer group, only applies if we don't have a layer group already
            else if (strCommand == "ATTR_layer_group_draped")
            {
                ///< Format: ATTR_layer_group_draped group offset
                std::string strLayerGroupArgs[2];

                ///< Read the args
                ssLine >> strLayerGroupArgs[0] >> strLayerGroupArgs[1];

                ///< Set the layer group
                intCurrentDrapedLayerGroup = XPLayerGroups::Resolve(strLayerGroupArgs[0], stoi(strLayerGroupArgs[1]));
            }

            ///< Vertex, save em all
            else if (strCommand == "VT")
            {
                ///< Format: VT X Y Z Nx Ny Nz U V
                std::string strVertexArgs[8];

                ///< Read the args
                ssLine >> strVertexArgs[0] >> strVertexArgs[1] >> strVertexArgs[2] >> strVertexArgs[3] >> strVertexArgs[4] >> strVertexArgs[5] >> strVertexArgs[6] >> strVertexArgs[7];

                ///< Create a vertex. Y is replaced with the current layer group. These don't have normals cuz they're auto calculated by blender
                XPAsset::Vertex NewVertex;
                NewVertex.X = stof(strVertexArgs[0]);
                NewVertex.Y = intCurrentDrapedLayerGroup * 0.1;
                NewVertex.Z = stof(strVertexArgs[2]);
                NewVertex.NX = stof(strVertexArgs[3]);
                NewVertex.NY = stof(strVertexArgs[4]);
                NewVertex.NZ = stof(strVertexArgs[5]);
                NewVertex.U = stof(strVertexArgs[6]);
                NewVertex.V = stof(strVertexArgs[7]);

                ///< Push it back
                Vertices.push_back(NewVertex);
            }

            ///< IDX10 we add these 10 indices
            else if (strCommand == "IDX10")
            {
                ///< Format: IDX10 i1 i2 i3 i4 i5 i6 i7 i8 i9 i10
                ///< We just push them back into indices vector in order
                std::string strIndicies[10];

                ///< Read the args
                ssLine >> strIndicies[0] >> strIndicies[1] >> strIndicies[2] >> strIndicies[3] >> strIndicies[4] >> strIndicies[5] >> strIndicies[6] >> strIndicies[7] >> strIndicies[8] >> strIndicies[9];

                ///< Push 'em back
                for (const auto &strIndicie : strIndicies)
                {
                    Indices.push_back(stoi(strIndicie));
                }
            }

            ///< IDX we save this one index
            else if (strCommand == "IDX")
            {
                ///< Format: IDX i1
                ///< Push it back into indices vector
                std::string strIndex;

                ///< Read the args
                ssLine >> strIndex;

                ///< Push it back
                Indices.push_back(stoi(strIndex));
            }

            ///< TRIS. This saves a draw call if in draped state
            else if (strCommand == "TRIS")
            {
                ///< Format: TRIS StartIndex EndIndex. Inclusive (ie TRIS 0 6 means indices 0 1 2 3 4 5 and 6).
                ///< Indices here are indices in Indices vector, which are indexes to Vertices. The index's position in the vector does not always match its value!!!
                std::string strDrawCallArgs[2];

                ///< Read the args
                ssLine >> strDrawCallArgs[0] >> strDrawCallArgs[1];

                ///< Create a new draw call
                XPAsset::ObjDrawCall NewDrawCall;
                NewDrawCall.idxStart = stoi(strDrawCallArgs[0]);
                NewDrawCall.idxEnd = stoi(strDrawCallArgs[1]);
                NewDrawCall.bDraped = bInDraped;

                ///< Save the draw call
                DrawCalls.push_back(NewDrawCall);
            }

            ///< TEXTURE_DRAPED
            else if (strCommand == "TEXTURE_DRAPED")
            {
                ///< Format: TEXTURE_DRAPED Tex
                std::string strTexPath;
                ssLine >> strTexPath;

                ///< Set the base texture
                pDrapedBaseTex = strTexPath;
                bHasDrapedBaseTex = true;
            }

            ///< TEXTURE command
            else if (strCommand == "TEXTURE")
            {
                ///< Format: TEXTURE Tex
                std::string strTexPath;
                ssLine >> strTexPath;

                ///< Set the base texture
                pBaseTex = strTexPath;
                bHasBaseTex = true;
            }

            ///< TEXTURE_DRAPED_NORMAL
            else if (strCommand == "TEXTURE_DRAPED_NORMAL")
            {
                ///< Format: TEXTURE_DRAPED_NORMAL TileRatio Tex
                std::string strTexArgs[2];
                ssLine >> strTexArgs[0] >> strTexArgs[1];

                ///< Set the normal texture
                pDrapedNormalTex = strTexArgs[1];
            }
        }

        ///< Close the file
        ObjFile.close();

        ///< Success
        return true;
    }
    catch (...)
    {
        ///< Failure
        return false;
    }
}

