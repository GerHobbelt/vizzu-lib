#ifndef LINEITEM_H
#define LINEITEM_H

#include "chart/rendering/painter/coordinatesystem.h"

#include "connectingitem.h"

namespace Vizzu
{
namespace Draw
{

class LineItem : public ConnectingDrawItem
{
public:
	LineItem(const Diag::Marker &marker,
	    const Diag::Options &options,
	    const Styles::Chart &style,
	    const CoordinateSystem &coordSys,
	    const Diag::Plot::Markers &markers,
	    size_t lineIndex);
	bool bounds(const Geom::Point &p) override;
};

}
}

#endif
