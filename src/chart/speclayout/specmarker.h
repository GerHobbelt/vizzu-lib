#ifndef CHART_SPECMARKER_H
#define CHART_SPECMARKER_H

#include <variant>

#include "base/geom/point.h"
#include "base/geom/rect.h"
#include "base/geom/circle.h"

namespace Vizzu
{
namespace Charts
{

struct SpecMarker
{
	size_t index;
	double size;
	std::variant<Geom::Rect, Geom::Circle> shape;

	SpecMarker(size_t index, double size) {
		this->index = index;
		this->size = size;
	}

	void emplaceCircle(const Geom::Circle &circle) {
		shape.emplace<Geom::Circle>(circle);
	}

	void emplaceRect(const Geom::Point &p0, const Geom::Point &p1) {
		shape.emplace<Geom::Rect>(p0, p1 - p0);
	}

	const Geom::Rect &rect() const { return *std::get_if<Geom::Rect>(&shape); }
	const Geom::Circle &circle() const { return *std::get_if<Geom::Circle>(&shape); }

	static bool indexOrder(const SpecMarker &a, const SpecMarker &b)
	{
		return a.index < b.index;
	}

	static bool sizeOrder(const SpecMarker &a, const SpecMarker &b)
	{
		return b.size < a.size;
	}
};

}
}

#endif
