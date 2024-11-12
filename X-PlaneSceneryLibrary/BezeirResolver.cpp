//Module:	BezeirResolver
//Author:	Connor Russell
//Date:		9/29/2023 9:58:20 PM
//Purpose:	Implements BezeirResolver.h

//Compile once
#pragma once

//Include necessary headers
#include "BezeirResolver.h"
#include <cmath>
#include "GeoUtils.h"

namespace XSLGeoutils
{
    /// <summary>
    /// Two of BezeirNodes into a vector of verticies
    /// </summary>
    /// <param name="startBP">Start BezeirNode</param>
    /// <param name="endBP">End BezeirNode</param>
    /// <param name="numSegments">Number of segments to subdivide the curve into</param>
    /// <returns>Vector of verticies</returns>
    std::vector<Node> SubdivideBezierCurve(BezeirNode startBP, BezeirNode endBP, int numSegments) {

        //If neither control point is present, return a straight line
        if (!startBP.HasExitControlPoint && !endBP.HasEntryControlPoint)
        {
            std::vector<Node> vertices;
            vertices.push_back(startBP.Point);
            vertices.push_back(endBP.Point);
            return vertices;
        }

        //If the exit control of startBP is missing, replace it with the entry control of endBP
        if (!startBP.HasExitControlPoint)
        {
            startBP.Control2 = startBP.Point;
            startBP.HasExitControlPoint = true;
        }

        //If the entry control of endBP is missing, replace it with the exit control of startBP
        if (!endBP.HasEntryControlPoint)
        {
            endBP.Control1 = endBP.Point;
            endBP.HasEntryControlPoint = true;
        }

        std::vector<Node> vertices;
        vertices.push_back(startBP.Point);

        for (int i = 1; i <= numSegments; ++i) {
            double t = i / static_cast<double>(numSegments);
            double u = 1.0 - t;

            Node NodeOnCurve;
            NodeOnCurve.X = std::pow(u, 3) * startBP.Point.X + 3 * std::pow(u, 2) * t * startBP.Control2.X + 3 * u * std::pow(t, 2) * endBP.Control1.X + std::pow(t, 3) * endBP.Point.X;
            NodeOnCurve.Y = std::pow(u, 3) * startBP.Point.Y + 3 * std::pow(u, 2) * t * startBP.Control2.Y + 3 * u * std::pow(t, 2) * endBP.Control1.Y + std::pow(t, 3) * endBP.Point.Y;
            NodeOnCurve.Z = std::pow(u, 3) * startBP.Point.Z + 3 * std::pow(u, 2) * t * startBP.Control2.Z + 3 * u * std::pow(t, 2) * endBP.Control1.Z + std::pow(t, 3) * endBP.Point.Z;
            NodeOnCurve.Properties = startBP.Point.Properties;

            vertices.push_back(NodeOnCurve);
        }

        //Set the curve end point flag on the first and last node, used by certain items that need curve metadata (such as the AptDat parser which has higher density lights on curves)
        vertices[0].CurveStart = true;
        vertices[vertices.size() - 1].CurveEnd = true;

        return vertices;
    }

    /// <summary>
    /// Converts a vector of verticies in the X-Plane curve format to a vector of BezeirNodes with control points
    /// </summary>
    /// <param name="InVerts">Vector of verticies</param>
    /// <param name="InClosed">True if the line is closed, otherwise false</param>
    /// <returns>Vector of BezeirNodes</returns>
    std::vector<BezeirNode> VertsToBezeirNodes(std::vector<Node>& InVerts, bool InClosed)
    {
        if (InVerts.size() == 0) return std::vector<BezeirNode>();

        std::vector<BezeirNode> vctBps;

        //If this is a closed line and the first and last points are colocated, the last node is the entry control to the first, and needs to be moved to the front
        if (InClosed && InVerts[0].Colocated(InVerts[InVerts.size() - 1]))              //If the first and last points are colocated, we are a closed line
        {
            //Move the last point to the front
            InVerts.insert(InVerts.begin(), InVerts.back());
            InVerts.pop_back();
        }

        //The first step is to convert the InVerts into a vector of bezeir points, where each point has it's own controls, vs the complex system of colocated single handle points
        for (size_t i = 0; i < InVerts.size(); i++)
        {
            //Get refs to all the InVerts
            Node v = InVerts[i];
            Node vn = InVerts[(i + 1) % InVerts.size()];
            Node vnn = InVerts[(i + 2) % InVerts.size()];

            //Define the bezeir point
            BezeirNode bp;
            bp.Point = v;

            //Check if the next 3 points are colocated. This is the case where there are two different handles
            if (v.Colocated(vn) && v.Colocated(vnn))
            {
                //v is entry, vn is main, vnn is next
                bp.HasEntryControlPoint = true;
                bp.HasExitControlPoint = true;
                v.X = v.U;
                v.Y = v.V;
                vnn.X = vnn.U;
                vnn.Y = vnn.V;
                RotatePoint(v.X, v.Y, vn.X, vn.Y, 180, &v.X, &v.Y);
                bp.Control1 = v;
                bp.Control2 = vnn;
                bp.Point = vn;

                //Skip the next two points, as they've been used
                i += 2;
            }

            //Check if just the next two are colocated. This is the case where there is only one handle
            else if (v.Colocated(vn))
            {
                //Check if the current has the same control as location, or the next has the same control as location.
                //The one with a control the same as it's location is the main point, the other is the control.
                //If the other comes after the main, it is the exit, otherwise it is the entry

                //The current is the main point, the next is the exit control
                if (v.U == v.X && v.V == v.Y)
                {
                    vn.X = vn.U;
                    vn.Y = vn.V;
                    bp.HasExitControlPoint = true;
                    bp.Control2 = vn;
                    bp.Point = v;
                }

                //The next is the main, the current is the entry control, which are rotated 180 around the center
                else
                {
                    RotatePoint(v.U, v.V, v.X, v.Y, 180, &v.X, &v.Y);
                    bp.HasEntryControlPoint = true;
                    bp.Control1 = v;
                    bp.Point = vn;
                }

                //Skip the next point, as it's been used
                i += 1;
            }

            //Check if the handle is differnt from the location. This is the case where there are two semetrical handles, and only the exit is provided
            else if (v.X != v.U || v.Y != v.V)
            {
                //The control points are in the U/V. These are the exit points. Entry points are the same but rotated 180 around the x/y
                Node vExit;
                vExit.X = v.U;
                vExit.Y = v.V;
                Node vEntry;
                RotatePoint(v.U, v.V, v.X, v.Y, 180, &vEntry.X, &vEntry.Y);
                bp.Control1 = vEntry;
                bp.Control2 = vExit;
                bp.HasEntryControlPoint = true;
                bp.HasExitControlPoint = true;
            }

            //Otherwise there are no handles.
            else
            {
                bp.Point = v;
            }

            vctBps.push_back(bp);
        }

        //Now we need to make sure the properties are in sync
        for (auto& b : vctBps)
        {
            //The main point may be missing properties. We know this if it doesn't match those of a control (rmemeber these where all generated from the same point), and said control does have properties.
            if (b.Point.Properties != b.Control1.Properties && b.Control1.Properties.size() > 0)
            {
                b.Point.Properties = b.Control1.Properties;
            }

            else if (b.Point.Properties != b.Control2.Properties && b.Control2.Properties.size() > 0)
            {
                b.Point.Properties = b.Control2.Properties;
            }
        }

        return vctBps;
    }

    /// <summary>
    /// Converts a vector of BezeirNodes to a vector of verticies with control points
    /// </summary>
    /// <param name="InVerts">Vector of BezeirNodes</param>
    /// <returns>Vector of verticies</returns>
    std::vector<Node> BezeirNodesToXPVerts(std::vector<BezeirNode>& InVerts, bool InClosed)
    {
        std::vector<Node> verts;

        for (size_t i = 0; i < InVerts.size(); i++)
        {
            auto& v = InVerts[i];

            //Case where there are both control points
            if (v.HasEntryControlPoint && v.HasExitControlPoint)
            {
                //In this case the points are:
                // first: x/y colocated with main, u/v are the entry control point rotated 180 around the main point
                // second: x/y the main point, u/v is colocated with main
                // third: x/y colocated with main, u/v are the exit control point
                Node v1 = v.Point;
                Node v2 = v.Point;
                Node v3 = v.Point;

                //Set v1s control
                v1.U = v.Control1.X;
                v1.V = v.Control1.Y;
                RotatePoint(v1.U, v1.V, v1.X, v1.Y, 180, &v1.U, &v1.V);

                //Set v3s control
                v3.U = v.Control2.X;
                v3.V = v.Control2.Y;

                //Add the verticies
                verts.push_back(v1);
                verts.push_back(v2);
                verts.push_back(v3);
            }

            //Case where there is a start control point
            else if (v.HasEntryControlPoint)
            {
                //In this case the points are:
                // first: x/y are the main, u/v are the entry control point rotated 180 around the main point
                // second: x/y are the main, u/v are colocated with main
                Node v1 = v.Point;
                Node v2 = v.Point;

                //Set v1s control
                v1.U = v.Control1.X;
                v1.V = v.Control1.Y;
                RotatePoint(v1.U, v1.V, v1.X, v1.Y, 180, &v1.U, &v1.V);

                //Set v2s control
                v2.U = v2.X;
                v2.V = v2.Y;

                //Add the verticies
                verts.push_back(v1);
                verts.push_back(v2);
            }

            //Case where there is an end control point
            else if (v.HasExitControlPoint)
            {
                //In this case the points are:
                // first: x/y are the main, u/v are colocated with main
                // second: x/y are colocated with the main, u/v are exit control point
                Node v1 = v.Point;
                Node v2 = v.Point;

                //Set v1s control
                v1.U = v1.X;
                v1.V = v1.Y;

                //Set v2s control
                v2.U = v.Control2.X;
                v2.V = v.Control2.Y;

                //Add the verticies
                verts.push_back(v1);
                verts.push_back(v2);
            }

            //Case where there are no control points
            else
            {
                //Simple. But remember we need to colocate the controls (u/v) with the main, just because of teh XP format
                Node v1 = v.Point;
                v1.U = v1.X;
                v1.V = v1.Y;

                //Add the verticies
                verts.push_back(v1);
            }
        }

        //Now we handle the weird case: When there is an entry control point on the first point and the line is closed, we move that to the end.
        if (InClosed)
        {
            //If the first point has an entry control point, we need to move it to the end
            if (InVerts[0].HasEntryControlPoint)
            {
                //Get the first point
                Node ecp = verts[0];

                //Remove the first point
                verts.erase(verts.begin());

                //Add the point to the end
                verts.push_back(ecp);
            }
        }

        return verts;
    }

    /// <summary>
    /// Resolves a vector of verticies that have control points in te X-Plane DSF format to a vector of verticies where curves have been subdivided into straight segments
    /// </summary>
    /// <param name="verts">Vector of verticies to resolve</param>
    /// <returns>Vector of verticies with curves resolved</returns>
    std::vector<Node> BezeirNodesToRealVerts(std::vector<BezeirNode>& InVerts, bool InClosed, int Resolution)
    {
        /*
        *
        * If there are colocated POLYGON_POINTs adjacent in the file:
        *
        * The first colocated point, where the point and its control points are the same, is the entry control point.
        * The second colocated point, where the point and its control points are the same, is the point itself.
        * The third colocated point, where the point and its control points are different, has the exit control point in its control coordinates.
        * If there is a single POLYGON_POINT at a given location, but its control points are different from its location:
        *
        * Its control point coordinates are the exit control point.
        * The entrance control point is the same but rotated 180 degrees around the point itself.
        * If there is a single POLYGON_POINT, and its control points are the same as it, it has no handles.
        *
        * Unique case:
        * One closed lines, the handle for the first point is the last point. Seriously what is LR thinking!?
        *
        * Lines notes: If it is clockwise, it needs to be reversed. We also need to check our logic on being closed as that seems to be broken.
        *
        */

        //Copy
        std::vector<BezeirNode> vctBps = InVerts;

        //Define an output Node vector
        std::vector<Node> vctOutVerts;

        //If we're closed, we go to the end, which loops us around to the start, connecting us. Otherwise we only go to the end
        size_t idxEndPos = InClosed ? (vctBps.size()) : (vctBps.size() - 1);

        //Now we loop through every bezeir point and subdivide the curve into straight segments
        for (size_t i = 0; i < idxEndPos; i++)
        {
            BezeirNode& bp = vctBps[i];
            BezeirNode& bpNext = vctBps[(i + 1) % vctBps.size()];
            std::vector<Node> vctVerts = SubdivideBezierCurve(bp, bpNext, Resolution);
            vctOutVerts.insert(vctOutVerts.end(), vctVerts.begin(), vctVerts.end());
        }

        //Now loop through all the output verts and remove duplicates
        for (size_t i = 0; i < vctOutVerts.size(); i++)
        {
            //Get refs to all the verts
            Node& vp = vctOutVerts[(i - 1 + vctOutVerts.size()) % vctOutVerts.size()];
            Node& v = vctOutVerts[i];

            //If the current vert is colocated with the prior, remove it
            if (v.Colocated(vp))
            {
                //If this is the start of a curve, and vp was the end, remove the end flag from vp so the curve metadata just continues
                if (v.CurveStart && vp.CurveEnd)
                {
                    v.CurveEnd = false;
                    v.CurveStart = false;
                }
                else if (vp.CurveEnd)
                {
                    v.CurveEnd = true;
                }
                else if (vp.CurveStart)
                {
                    v.CurveStart = true;
                }

                vctOutVerts.erase(vctOutVerts.begin() + ((i - 1 + vctOutVerts.size()) % vctOutVerts.size()));
                i--;
            }
        }

        //Return the out verticies
        return vctOutVerts;
    }

    /// <summary>
    /// Merges nearly colocated points in the polygon. This is to fix heading issue due to extremely short segments typically caused by bezier curves
    /// </summary>
    void MergeByDistance(std::vector<Node>* InVerts, double MergeDistance)
    {
        std::vector<Node>& w = *InVerts;

        for (size_t i = 0; i < w.size() - 1; i++)
        {
            //Get the dif between this point and the next
            double dblDiffX = std::abs(w[i + 1].X - w[i].X);
            double dblDiffY = std::abs(w[i + 1].Y - w[i].Y);
            double dblDiffDist = std::sqrt(std::pow(dblDiffX, 2) + std::pow(dblDiffY, 2));

            //If the distance is less than the merge distance, merge the points
            if (dblDiffDist < MergeDistance)
            {
                //Set w[i] to the average of the two points
                w[i].X = (w[i].X + w[i + 1].X) / 2;
                w[i].Y = (w[i].Y + w[i + 1].Y) / 2;

                //Remove w[i + 1]
                w.erase(w.begin() + i + 1);
                i--;
            }

        }
    }

}