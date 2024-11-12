//Module:		Geoutils
//Purpose:		Provide functions for calculating distances, particularly between geographic points
//Author:		Connor Russell
//Date:			11/11/2024

//Include our header
#include "GeoUtils.h"
#include <cmath>
#include "XSLMacros.h"
#include "SystemHeaders.h"

namespace XSLGeoutils
{

	//Constants
	#define PI ((double)3.141592653589793)			//Haversine stuff
	#define EARTH_RADIUS ((double)6372797.56085)	//Haversine stuff	SHOULD BE 6372797.56085
	#define PI_RADIANS (PI / ((double)180.0))		//Haversine stuff
	#define PROBE_ALT 10000							//Altitude in meters to probe terrain from
	#define PROBE_NEW_DIST 0.00001

	//Function to get the intersection of two line rays
	bool GetIntersectionOfLineRays(double x1, double y1, double heading1, double x2, double y2, double heading2, double* outX, double* outY)
	{
		// Convert headings from degrees to radians
		double radHeading1 = heading1 * PI / 180.0;
		double radHeading2 = heading2 * PI / 180.0;

		// Convert headings to slopes
		double m1 = std::tan(PI / 2 - radHeading1);
		double m2 = std::tan(PI / 2 - radHeading2);

		// Check if lines are parallel (slopes are equal)
		if (std::abs(m1 - m2) < 1e-9) {
			return false; // No intersection, lines are parallel
		}

		// Calculate intersection point
		double x = (m1 * x1 - m2 * x2 + y2 - y1) / (m1 - m2);
		double y = m1 * (x - x1) + y1;

		// Output the intersection point
		*outX = x;
		*outY = y;

		return true;
	}

	//Function to calculate the distance between two geological coordinates in decimal degrees. Returns distance in meters
	double GetWorldDistance(double lat1, double lng1, double lat2, double lng2)
	{
		//Convert to radians
		lat1 = PI_RADIANS * lat1;
		lng1 = PI_RADIANS * lng1;
		lat2 = PI_RADIANS * lat2;
		lng2 = PI_RADIANS * lng2;

		//Calculate the distance
		double dLat = lat2 - lat1;
		double dLng = lng2 - lng1;
		double a = sin(dLat / 2) * sin(dLat / 2) + cos(lat1) * cos(lat2) * sin(dLng / 2) * sin(dLng / 2);
		double c = 2 * atan2(sqrt(a), sqrt(1 - a));
		double d = EARTH_RADIUS * c;

		//Return the distance
		return d;
	}

	//Function to offset world coordinates by a given distance
	void GetWorldOffset(double lat, double lon, double offsetX, double offsetY, double* newLat, double* newLon)
	{
		// Coordinate offsets in radians
		double dLat = offsetY / EARTH_RADIUS;
		double dLon = offsetX / (EARTH_RADIUS * cos(PI * lat / 180));

		// Offset position, decimal degrees
		*newLat = lat + dLat * 180 / PI;
		*newLon = lon + dLon * 180 / PI;
	}

	//Function to calculate the bearing between two coords
	double GetWorldHeading(double lat1, double lng1, double lat2, double lng2)
	{
		lat1 = PI_RADIANS * lat1;
		lng1 = PI_RADIANS * lng1;
		lat2 = PI_RADIANS * lat2;
		lng2 = PI_RADIANS * lng2;

		double dLng = lng2 - lng1;
		double dPhi = log(tan(lat2 / 2.0 + PI / 4.0) / tan(lat1 / 2.0 + PI / 4.0));

		if (abs(dLng) > PI) {
			if (dLng > 0.0)
				dLng = -(2.0 * PI - dLng);
			else
				dLng = (2.0 * PI + dLng);
		}

		return fmod((atan2(dLng, dPhi) * (180.0 / PI)) + 360.0, 360);
	}

	//Function to get the pitch given a rise over run
	double GetPitch(double dblRise, double dblRun)
	{
		//Make sure the run isn't zero (so no dbz)
		if (dblRun <= 0)
		{
			return 0;
		}

		//Check if the rise is positive
		if (dblRise > 0)
		{
			//Return the inverse tangent of the rise over run
			return atan((dblRise / (float)dblRun)) * 180 / PI;
		}

		//Check if this is negative
		else if (dblRise < 0)
		{
			//Return the inverse tangent of rise over run times -1 to make it negative
			return (atan((dblRise / (float)dblRun)) * 180 / PI);
		}

		//Otherwise return 0
		return 0;
	}

	//Fucntion to get the heading given a start xy and end xy. Clockwise, 0 up
	double GetHeading(double startX, double startY, double endX, double endY)
	{
		double deltaX = endX - startX;
		double deltaY = endY - startY; // invert deltaY
		double heading = atan2(deltaY, deltaX) * 180 / PI;
		heading = 90 - heading; // rotate by 90 degrees
		if (heading < 0) heading += 360;
		return heading;
	}

	//Function to get the distance between a given start xy and end xy
	double GetDistance(double startX, double startY, double endX, double endY)
	{
		double deltaX = endX - startX;
		double deltaY = endY - startY;
		return sqrt(deltaX * deltaX + deltaY * deltaY);
	}

	//Function to rotate a point clockwise around another (in local coords). Clockwise, 0 up
	void RotatePoint(double x, double y, double centerX, double centerY, double angle, double* newX, double* newY)
	{
		// Convert the angle to radians
		double rad = angle * PI / 180.0;

		// Translate point back to origin
		x -= centerX;
		y -= centerY;

		// Perform rotation
		*newX = x * cos(rad) + y * sin(rad);
		*newY = y * cos(rad) - x * sin(rad);

		// Translate point back
		*newX += centerX;
		*newY += centerY;
	}

	//Function to calculate the position of a point x units away at y angle from a given point. Clockwise, 0 up
	void ExtrudePoint(double centerX, double centerY, double distance, double angle, double* newX, double* newY)
	{
		//Define a new point distance above Y
		*newX = centerX;
		*newY = centerY + distance;

		//Rotate
		RotatePoint(*newX, *newY, centerX, centerY, angle, newX, newY);
	}

	//Function to get a point on a simple quadratic curve
	void GetPointOnSimpleCurve(double StartX, double StartY, double ControlX, double ControlY, double EndX, double EndY, double Ratio, double* OutX, double* OutY)
	{
		double dblXA = INTERPOLATE(StartX, ControlX, Ratio);
		double dblYA = INTERPOLATE(StartY, ControlY, Ratio);
		double dblXB = INTERPOLATE(ControlX, EndX, Ratio);
		double dblYB = INTERPOLATE(ControlY, EndY, Ratio);
		*OutX = INTERPOLATE(dblXA, dblXB, Ratio);
		*OutY = INTERPOLATE(dblYA, dblYB, Ratio);
	}

	//Function to subdivide a simple quadratic curve
	void SubdivideSimpleCurve(double StartX, double StartY, double ControlX, double ControlY, double EndX, double EndY, int Segments, std::vector<double>* OutXs, std::vector<double>* OutYs)
	{
		OutXs->push_back(StartX);
		OutYs->push_back(StartY);

		for (int i = 1; i <= Segments; ++i) {
			double RatioThrough = i / static_cast<double>(Segments);
			double u = 1.0 - RatioThrough;

			double OutX, OutY;
			GetPointOnSimpleCurve(StartX, StartY, ControlX, ControlY, EndX, EndY, RatioThrough, &OutX, &OutY);

			OutXs->push_back(OutX);
			OutYs->push_back(OutY);
		}
	}

	//Measures the length of a cubic simple quadratic curve by subdividing it and measuring the length of the resulting line segments
	double MeasureSimpleCurve(double StartX, double StartY, double ControlX, double ControlY, double EndX, double EndY, int numSegments)
	{
		std::vector<double> Xs, Ys;
		SubdivideSimpleCurve(StartX, StartY, ControlX, ControlY, EndX, EndY, numSegments, &Xs, &Ys);

		double dblLen = 0;

		for (size_t i = 0; i < Xs.size() - 1; i++)
		{
			dblLen += GetDistance(Xs[i], Ys[i], Xs[i + 1], Ys[i + 1]);
		}

		return dblLen;
	}

	//Measures the length of a simple quadratic curve by subdividing it and measuring the length of the resulting line segments
	double MeasureSimpleCurveWorld(double StartX, double StartY, double ControlX, double ControlY, double EndX, double EndY, int numSegments)
	{
		std::vector<double> Xs, Ys;
		SubdivideSimpleCurve(StartX, StartY, ControlX, ControlY, EndX, EndY, numSegments, &Xs, &Ys);

		double dblLen = 0;

		for (size_t i = 0; i < Xs.size() - 1; i++)
		{
			dblLen += GetWorldDistance(Ys[i], Xs[i], Ys[i + 1], Xs[i + 1]);
		}

		return dblLen;
	}

	//Returns a point on a cubic bezeir curve
	void GetPointOnBezeir(double StartX, double StartY, double StartCtrlX, double StartCtrlY, double EndX, double EndY, double EndCtrlX, double EndCtrlY, double RatioThrough, double* OutX, double* OutY)
	{
		double u = 1.0 - RatioThrough;


		*OutX = std::pow(u, 3) * StartX + 3 * std::pow(u, 2) * RatioThrough * StartCtrlX + 3 * u * std::pow(RatioThrough, 2) * EndCtrlX + std::pow(RatioThrough, 3) * EndX;
		*OutY = std::pow(u, 3) * StartY + 3 * std::pow(u, 2) * RatioThrough * StartCtrlY + 3 * u * std::pow(RatioThrough, 2) * EndCtrlY + std::pow(RatioThrough, 3) * EndY;
	}

	//Function to subdivide a cubic bezeir curve
	void SubdivideBezierCurve(double StartX, double StartY, double StartCtrlX, double StartCtrlY, double EndX, double EndY, double EndCtrlX, double EndCtrlY, int numSegments, std::vector<double>* OutXs, std::vector<double>* OutYs)
	{
		OutXs->push_back(StartX);
		OutYs->push_back(StartY);

		for (int i = 1; i <= numSegments; ++i) {
			double RatioThrough = i / static_cast<double>(numSegments);
			double u = 1.0 - RatioThrough;

			double OutX = std::pow(u, 3) * StartX + 3 * std::pow(u, 2) * RatioThrough * StartCtrlX + 3 * u * std::pow(RatioThrough, 2) * EndCtrlX + std::pow(RatioThrough, 3) * EndX;
			double OutY = std::pow(u, 3) * StartY + 3 * std::pow(u, 2) * RatioThrough * StartCtrlY + 3 * u * std::pow(RatioThrough, 2) * EndCtrlY + std::pow(RatioThrough, 3) * EndY;

			OutXs->push_back(OutX);
			OutYs->push_back(OutY);
		}
	}

	//Measures the length of a cubic bezeir curve by subdividing it and measuring the length of the resulting line segments
	double MeasureBezierCurve(double StartX, double StartY, double StartCtrlX, double StartCtrlY, double EndX, double EndY, double EndCtrlX, double EndCtrlY, int numSegments)
	{
		std::vector<double> Xs, Ys;
		SubdivideBezierCurve(StartX, StartY, StartCtrlX, StartCtrlY, EndX, EndY, EndCtrlX, EndCtrlY, numSegments, &Xs, &Ys);

		double dblLen = 0;

		for (size_t i = 0; i < Xs.size() - 1; i++)
		{
			dblLen += GetDistance(Xs[i], Ys[i], Xs[i + 1], Ys[i + 1]);
		}

		return dblLen;
	}

	//Measures the length of a cubic bezeir curve by subdividing it and measuring the length of the resulting line segments
	double MeasureBezierCurveWorld(double StartX, double StartY, double StartCtrlX, double StartCtrlY, double EndX, double EndY, double EndCtrlX, double EndCtrlY, int numSegments)
	{
		std::vector<double> Xs, Ys;
		SubdivideBezierCurve(StartX, StartY, StartCtrlX, StartCtrlY, EndX, EndY, EndCtrlX, EndCtrlY, numSegments, &Xs, &Ys);

		double dblLen = 0;

		for (size_t i = 0; i < Xs.size() - 1; i++)
		{
			dblLen += GetWorldDistance(Ys[i], Xs[i], Ys[i + 1], Xs[i + 1]);
		}

		return dblLen;
	}

	//Function to get the angle between two points. Clockwise, 0 up
	double ResolveHeading(double heading)
	{
		while (heading > 360)
		{
			heading -= 360;
		}

		while (heading < 0)
		{
			heading += 360;
		}

		return heading;
	}

	//Function to average two headings. Clockwise, 0 up
	double AverageHeading(double heading1, double heading2)
	{
		double x = cos(heading1 * PI / 180.0) + cos(heading2 * PI / 180.0);
		double y = sin(heading1 * PI / 180.0) + sin(heading2 * PI / 180.0);
		double avg_heading = atan2(y, x) * 180.0 / PI;
		return avg_heading < 0 ? avg_heading + 360 : avg_heading;
	}

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
	bool GetIntersection(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, double* x, double* y)
	{
		double a_dx = x2 - x1;
		double a_dy = y2 - y1;
		double b_dx = x4 - x3;
		double b_dy = y4 - y3;

		double s = (-a_dy * (x1 - x3) + a_dx * (y1 - y3)) / (-b_dx * a_dy + a_dx * b_dy);
		double t = (b_dx * (y1 - y3) - b_dy * (x1 - x3)) / (-b_dx * a_dy + a_dx * b_dy);

		if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
		{
			*x = x1 + (t * a_dx);
			*y = y1 + (t * a_dy);
			return true;
		}

		return false;
	}

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
	bool AreParallel(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, double tolerance)
	{
		double dblHeadign1 = GetHeading(x1, y1, x2, y2);
		double dblHeading2 = GetHeading(x3, y3, x4, y4);

		return abs(dblHeadign1 - dblHeading2) < tolerance;
	}

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
	double GetDistanceBetweenParallels(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, double MaxDistance)
	{
		//Get the heading of the first line
		double dblHeading = GetHeading(x1, y1, x2, y2);

		//Get the center of the first line
		double dblCenterX = (x1 + x2) / 2;
		double dblCenterY = (y1 + y2) / 2;

		//Take the point, extrude it along the heading, and rotate it 90 degrees
		double dblX1, dblY1;
		double dblX2, dblY2;
		ExtrudePoint(dblCenterX, dblCenterY, MaxDistance, dblHeading + 90, &dblX1, &dblY1);
		ExtrudePoint(dblCenterX, dblCenterY, MaxDistance, dblHeading - 90, &dblX2, &dblY2);

		//Get the intersection point between these extruded points and the second line
		double dblIntersectX, dblIntersectY;
		if (GetIntersection(dblX1, dblY1, dblX2, dblY2, x3, y3, x4, y4, &dblIntersectX, &dblIntersectY))
		{
			//Return the distance between the center of the first line and the intersection point
			return GetDistance(dblCenterX, dblCenterY, dblIntersectX, dblIntersectY);
		}

		return -1;
	}
}