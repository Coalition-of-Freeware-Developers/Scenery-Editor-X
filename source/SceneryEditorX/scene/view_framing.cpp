/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * view_framing.cpp
 * -------------------------------------------------------
 * Created: 10/11/2026
 * -------------------------------------------------------
 */
#include "view_framing.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX::ViewFraming
{
	namespace
	{
		double Dot(const Point &a, const Point &b)
		{
			return a.x * b.x + a.y * b.y + a.z * b.z;
		}

		Point Subtract(const Point &a, const Point &b)
		{
			return { a.x - b.x, a.y - b.y, a.z - b.z };
		}

		bool IsUsable(const double value)
		{
			return std::isfinite(value) && std::abs(value) <= MAX_COORDINATE;
		}

		/* @brief The half fields of view the box fills: the lens' own, times the fill (never zero). */
		void FilledTangents(const Lens &lens, const double fill, double &tanX, double &tanY)
		{
			const double share = std::isfinite(fill) ? std::clamp(fill, 0.05, 1.0) : DEFAULT_FILL;
			tanX = std::max(std::abs(lens.tanHalfX) * share, 1e-6);
			tanY = std::max(std::abs(lens.tanHalfY) * share, 1e-6);
		}
	}

	bool IsUsable(const Point &point)
	{
		return IsUsable(point.x) && IsUsable(point.y) && IsUsable(point.z);
	}

	// -------------------------------------------------------

	bool Box::Add(const Point &point)
	{
		if (!IsUsable(point))
			return false;
		if (!valid)
		{
			min = point;
			max = point;
			valid = true;
			return true;
		}
		min = { std::min(min.x, point.x), std::min(min.y, point.y), std::min(min.z, point.z) };
		max = { std::max(max.x, point.x), std::max(max.y, point.y), std::max(max.z, point.z) };
		return true;
	}

	bool Box::Add(const Point &boxMin, const Point &boxMax)
	{
		// An "empty" box (min above max: +inf / -inf, as a mesh without vertices has) or a corner out in the void
		if (!IsUsable(boxMin) || !IsUsable(boxMax) || boxMin.x > boxMax.x || boxMin.y > boxMax.y || boxMin.z > boxMax.z)
			return false;
		Add(boxMin);
		Add(boxMax);
		return true;
	}

	bool Box::Add(const Box &box)
	{
		return box.valid && Add(box.min, box.max);
	}

	Point Box::GetCenter() const
	{
		return { 0.5 * (min.x + max.x), 0.5 * (min.y + max.y), 0.5 * (min.z + max.z) };
	}

	Point Box::GetSize() const
	{
		return valid ? Point{ max.x - min.x, max.y - min.y, max.z - min.z } : Point{};
	}

	double Box::GetRadius() const
	{
		const Point size = GetSize();
		return 0.5 * std::sqrt(size.x * size.x + size.y * size.y + size.z * size.z);
	}

	void Box::Inflate(const double minimumSize)
	{
		if (!valid || !(minimumSize > 0.0))
			return;
		const auto widen = [minimumSize](double &low, double &high)
		{
			if (high - low >= minimumSize)
				return;
			const double middle = 0.5 * (low + high);
			low = middle - 0.5 * minimumSize;
			high = middle + 0.5 * minimumSize;
		};
		widen(min.x, max.x);
		widen(min.y, max.y);
		widen(min.z, max.z);
	}

	// -------------------------------------------------------

	double FitDistance(const Box &box, const Lens &lens, const double fill, const double nearMargin)
	{
		if (!box.valid)
			return 0.0;
		double tanX = 0.0, tanY = 0.0;
		FilledTangents(lens, fill, tanX, tanY);

		// For each corner (camera axes about the box's center: f along the view, r and u across it), the distance that
		// puts it on the edge of the view; the farthest of them puts every corner inside. Perspective: the corner is
		// d + f in front of the eye, where the view is (d + f) * tan wide. Orthographic: the view is d * tan wide anywhere.
		const Point center = box.GetCenter();
		double distance = 0.0;
		for (int corner = 0; corner < 8; ++corner)
		{
			const Point point{ (corner & 1) ? box.max.x : box.min.x, (corner & 2) ? box.max.y : box.min.y, (corner & 4) ? box.max.z : box.min.z };
			const Point offset = Subtract(point, center);
			const double f = Dot(offset, lens.forward);
			const double across = std::max(std::abs(Dot(offset, lens.right)) / tanX, std::abs(Dot(offset, lens.up)) / tanY);
			const double needed = lens.orthographic ? across : across - f;
			distance = std::max({ distance, needed, nearMargin - f });
		}
		return distance;
	}

	double FitSphereDistance(const double radius, const Lens &lens, const double fill, const double nearMargin)
	{
		const double r = std::isfinite(radius) ? std::max(radius, 0.0) : 0.0;
		double tanX = 0.0, tanY = 0.0;
		FilledTangents(lens, fill, tanX, tanY);
		const double tanHalf = std::min(tanX, tanY);

		// Perspective: the sphere touches the view's sides when distance * sin(half angle) = radius
		const double fit = lens.orthographic ? r / tanHalf : r * std::sqrt(1.0 + tanHalf * tanHalf) / tanHalf;
		return std::max(fit, r + nearMargin);
	}

	bool IsInView(const Point &point, const Point &pivot, const double distance, const Lens &lens, const double fill, const double nearMargin)
	{
		const Point eye{ pivot.x - lens.forward.x * distance, pivot.y - lens.forward.y * distance, pivot.z - lens.forward.z * distance };
		const Point offset = Subtract(point, eye);
		const double depth = Dot(offset, lens.forward);
		const double r = std::abs(Dot(offset, lens.right));
		const double u = std::abs(Dot(offset, lens.up));
		const double tolerance = 1e-9 * (1.0 + std::abs(depth) + r + u + std::abs(distance));
		if (depth < nearMargin - tolerance)
			return false;
		const double share = std::isfinite(fill) ? std::clamp(fill, 0.0, 1.0) : 1.0;	// A bad fill checks the whole view
		const double reach = lens.orthographic ? distance : depth;	// Orthographic views are as wide everywhere
		return r <= reach * std::abs(lens.tanHalfX) * share + tolerance && u <= reach * std::abs(lens.tanHalfY) * share + tolerance;
	}
}

// -------------------------------------------------------
