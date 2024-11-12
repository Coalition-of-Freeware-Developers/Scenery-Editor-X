//Module:		Geoutils
//Purpose:		Provide functions for calculating distances, particularly between geographic points
//Author:		Connor Russell
//Date:			11/11/2024

//Compile Once
#pragma once

//Include system haeders
#include "SystemHeaders.h"

namespace XSLGeoutils
{
	//Function to get the intersection of two line rays
	bool GetIntersectionOfLineRays(double x1, double y1, double heading1, double x2, double y2, double heading2, double* outX, double* outY);

	//Function to calculate the distance between two geological coordinates in decimal degrees. Returns distance in meters
	double GetWorldDistance(double lat1, double lng1, double lat2, double lng2);

	//Function to offset world coordinates by a given distance
	void GetWorldOffset(double lat, double lon, double offsetX, double offsetY, double* newLat, double* newLon);

	//Function to calculate the bearing between two coords
	double GetWorldHeading(double lat1, double lng1, double lat2, double lng2);

	//Function to get the pitch given a rise over run
	double GetPitch(double dblRise, double dblRun);

	//Fucntion to get the heading given a start xy and end xy
	double GetHeading(double startX, double startY, double endX, double endY);

	//Function to get the distance between a given start xy and end xy
	double GetDistance(double startX, double startY, double endX, double endY);

	//Function to rotate a point clockwise around another (in local coords)
	void RotatePoint(double x, double y, double centerX, double centerY, double angle, double* newX, double* newY);

	//Function to extrude a point x units from another point at x angle. Works by extruding along y, then rotating
	void ExtrudePoint(double centerX, double centerY, double distance, double angle, double* newX, double* newY);

	//Function to get a point on a simple quadratic curve
	void GetPointOnSimpleCurve(double StartX, double StartY, double ControlX, double ControlY, double EndX, double EndY, double Ratio, double* OutX, double* OutY);

	//Function to subdivide a simple quadratic curve
	void SubdivideSimpleCurve(double StartX, double StartY, double ControlX, double ControlY, double EndX, double EndY, int Segments, std::vector<double>* OutXs, std::vector<double>* OutYs);

	//Measures the length of a cubic simple quadratic curve by subdividing it and measuring the length of the resulting line segments
	double MeasureSimpleCurve(double StartX, double StartY, double ControlX, double ControlY, double EndX, double EndY, int numSegments);

	//Measures the length of a simple quadratic curve by subdividing it and measuring the length of the resulting line segments
	double MeasureSimpleCurveWorld(double StartX, double StartY, double ControlX, double ControlY, double EndX, double EndY, int numSegments);

	//Returns a point on a cubic bezeir curve
	void GetPointOnBezeir(double StartX, double StartY, double StartCtrlX, double StartCtrlY, double EndX, double EndY, double EndCtrlX, double EndCtrlY, double RatioThrough, double* OutX, double* OutY);

	//Function to subdivide a cubic bezeir curve
	void SubdivideBezierCurve(double StartX, double StartY, double StartCtrlX, double StartCtrlY, double EndX, double EndY, double EndCtrlX, double EndCtrlY, int numSegments, std::vector<double>* OutXs, std::vector<double>* OutYs);

	//Measures the length of a cubic bezeir curve by subdividing it and measuring the length of the resulting line segments
	double MeasureBezierCurve(double StartX, double StartY, double StartCtrlX, double StartCtrlY, double EndX, double EndY, double EndCtrlX, double EndCtrlY, int numSegments);

	//Measures the length of a cubic bezeir curve by subdividing it and measuring the length of the resulting line segments
	double MeasureBezierCurveWorld(double StartX, double StartY, double StartCtrlX, double StartCtrlY, double EndX, double EndY, double EndCtrlX, double EndCtrlY, int numSegments);

	//Function to get the angle between two points
	double ResolveHeading(double heading);

	//Function to average two headings. Clockwise, 0 up
	double AverageHeading(double heading1, double heading2);

	/// <summary>
	/// Get the intersection point of two lines
	/// </summary>
	/// <param name="x1">The x coordinate of the first point of the first line</param>
	/// <param name="y1">The y coordinate of the first point of the first line</param>
	/// <param name="x2">The x coordinate of the second point of the first line</param>
	/// <param name="y2">The y coordinate of the second point of the first line</param>
	/// <param name="x3">The x coordinate of the first point of the second line</param>
	/// <param name="y3">The y coordinate of the first point of the second line</param>
	/// <param name="x4">The x coordinate of the second point of the second line</param>
	/// <param name="y4">The y coordinate of the second point of the second line</param>
	/// <param name="x">Pointer to double to hold the x coordinate of the intersection point</param>
	/// <param name="y">Pointer to double to hold the y coordinate of the intersection point</param>
	/// <returns>True if the lines intersect, false otherwise</returns>
	bool GetIntersection(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, double* x, double* y);

	/// <summary>
	/// Determine if two lines are parallel within a given tolerance
	/// </summary>
	/// <param name="x1">The x coordinate of the first point of the first line</param>
	/// <param name="y1">The y coordinate of the first point of the first line</param>
	/// <param name="x2">The x coordinate of the second point of the first line</param>
	/// <param name="y2">The y coordinate of the second point of the first line</param>
	/// <param name="x3">The x coordinate of the first point of the second line</param>
	/// <param name="y3">The y coordinate of the first point of the second line</param>
	/// <param name="x4">The x coordinate of the second point of the second line</param>
	/// <param name="y4">The y coordinate of the second point of the second line</param>
	/// <param name="tolerance">The tolerance to check for parallelism</param>
	/// <returns>True if the lines are parallel, false otherwise</returns>
	bool AreParallel(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, double tolerance);

	//Function to get the distance between two lines, perpendicular from the center of the first line to the second
	/// <summary>
	/// Get the distance between two parallel lines
	/// </summary>
	/// <param name="x1">The x coordinate of the first point of the first line</param>
	/// <param name="y1">The y coordinate of the first point of the first line</param>
	/// <param name="x2">The x coordinate of the second point of the first line</param>
	/// <param name="y2">The ycoordinate of the second point of the first line</param>
	/// <param name="x3">The x coordinate of the first point of the second line</param>
	/// <param name="y3">The y coordinate of the first point of the second line</param>
	/// <param name="x4">The x coordinate of the second point of the second line</param>
	/// <param name="y4">The y coordinate of the second point of the second line</param>
	/// <param name="MaxDistance">The maximum distance to check for an intersection</param>
	/// <returns>The distance between the two lines, or -1 if they do not intersect</returns>
	double GetDistanceBetweenParallels(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, double MaxDistance);

};