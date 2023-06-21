#include "connectingitem.h"

#include <algorithm>

using namespace Vizzu;
using namespace Vizzu::Draw;

ConnectingDrawItem::ConnectingDrawItem(const Gen::Marker &marker,
    const Gen::Options &options,
    const Gen::Plot::Markers &markers,
    size_t lineIndex,
    Gen::ShapeType type) :
    DrawItem(marker),
    lineIndex(lineIndex)
{
	color = marker.color;

	enabled = options.shapeType.get().factor(type);
	labelEnabled = enabled && marker.enabled;

	auto weight = marker.prevMainMarkerIdx.values[lineIndex].weight;
	weight = std::max(0.0, 3 * weight - 2);

	connected = enabled && Math::FuzzyBool(weight);

	if (weight > 0.0) {
		const auto *prev = getPrev(marker, markers, lineIndex);
		if (prev) {
			labelEnabled =
			    enabled && (marker.enabled || prev->enabled);
			connected =
			    connected && (prev->enabled || marker.enabled);
			if (prev->mainId.itemId > marker.mainId.itemId) {
				connected = connected && options.polar.get().more();
				enabled = enabled && options.polar.get();
			}
		}
		else
			connected = 0;
	}
}

const Gen::Marker *ConnectingDrawItem::getPrev(
    const Gen::Marker &marker,
    const Gen::Plot::Markers &markers,
    size_t lineIndex)
{
	const auto &prevId = marker.prevMainMarkerIdx.values[lineIndex];
	return (prevId.weight > 0.0) ? &markers[prevId.value] : nullptr;
}
