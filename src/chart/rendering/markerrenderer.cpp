#include "markerrenderer.h"

#include "base/geom/angle.h"
#include "base/text/smartstring.h"
#include "chart/rendering/colorbuilder.h"
#include "chart/rendering/drawlabel.h"
#include "chart/rendering/markers/abstractmarker.h"
#include "chart/rendering/markers/circlemarker.h"
#include "chart/rendering/markers/connectingmarker.h"
#include "chart/rendering/markers/rectanglemarker.h"
#include "chart/rendering/orientedlabel.h"

namespace Vizzu::Draw
{

void MarkerRenderer::drawLines(Gfx::ICanvas &canvas,
    Painter &painter) const
{
	const auto &style = rootStyle.plot.marker.guides;

	if (style.color->isTransparent() || *style.lineWidth <= 0
	    || plot->anyAxisSet == false || !plot->guides.hasAnyGuides())
		return;

	canvas.setLineWidth(*style.lineWidth);

	auto origo = plot->measureAxises.origo();

	auto baseColor = *style.color * double{plot->anyAxisSet};

	auto xMarkerGuides = double{plot->guides.x.markerGuides};
	auto yMarkerGuides = double{plot->guides.y.markerGuides};
	auto xLineColor = baseColor * xMarkerGuides;
	auto yLineColor = baseColor * yMarkerGuides;

	for (const auto &blended : markers) {
		if (blended.marker.enabled == false
		    || blended.enabled == false)
			continue;

		if (xMarkerGuides > 0) {
			canvas.setLineColor(xLineColor);
			auto axisPoint = blended.center.xComp() + origo.yComp();
			const Geom::Line line(axisPoint, blended.center);

			auto guideElement =
			    Events::Targets::markerGuide(blended.marker, false);

			if (rootEvents.draw.plot.marker.guide->invoke(
			        Events::OnLineDrawEvent(*guideElement,
			            {line, true}))) {
				painter.drawLine(line);
				renderedChart.emplace(Draw::Line{line, true},
				    std::move(guideElement));
			}
		}
		if (yMarkerGuides > 0) {
			auto center = Geom::Point{blended.center};
			center.x = Math::interpolate(center.x,
			    1.0,
			    getOptions().coordSystem.factor<double>(
			        Gen::CoordSystem::polar));
			canvas.setLineColor(yLineColor);
			auto axisPoint = center.yComp() + origo.xComp();
			const Geom::Line line(center, axisPoint);

			auto guideElement =
			    Events::Targets::markerGuide(blended.marker, true);

			if (rootEvents.draw.plot.marker.guide->invoke(
			        Events::OnLineDrawEvent(*guideElement,
			            {line, true}))) {
				painter.drawLine(line);
				renderedChart.emplace(Draw::Line{line, true},
				    std::move(guideElement));
			}
		}
	}
	canvas.setLineWidth(0);
}

void MarkerRenderer::drawMarkers(Gfx::ICanvas &canvas,
    Painter &painter) const
{
	for (const auto &blended : markers) {
		if (getOptions().geometry.contains(Gen::ShapeType::line)
		    && getOptions().geometry.contains(
		        Gen::ShapeType::circle)) {
			const CircleMarker circle(blended.marker,
			    coordSys,
			    getOptions(),
			    rootStyle);

			draw(canvas, painter, circle, 1, false);

			blended.marker.prevMainMarkerIdx.visit(
			    [this, &blended, &canvas, &painter](int index,
			        auto value)
			    {
				    draw(canvas,
				        painter,
				        ConnectingMarker{ctx(),
				            blended.marker,
				            static_cast<size_t>(index),
				            Gen::ShapeType::line},
				        value.weight,
				        true);
			    });
		}
		else {
			std::optional<AbstractMarker> other;
			auto drawMarker =
			    [this, &blended, &other, &canvas, &painter](int index,
			        ::Anim::Weighted<uint64_t> value)
			{
				if (index == 1 && !other) {
					other.emplace(
					    AbstractMarker::createInterpolated(ctx(),
					        blended.marker,
					        1));
				}
				const auto &blended0 = index == 0 ? blended : *other;

				auto lineFactor =
				    getOptions().geometry.factor<double>(
				        Gen::ShapeType::line);

				draw(canvas,
				    painter,
				    blended0,
				    value.weight * (1 - lineFactor)
				        * (1 - lineFactor),
				    false);
				draw(canvas,
				    painter,
				    blended0,
				    value.weight * sqrt(lineFactor),
				    true);
			};

			auto containsConnected =
			    getOptions().geometry.contains(Gen::ShapeType::line)
			    || getOptions().geometry.contains(
			        Gen::ShapeType::area);

			auto containsSingle = getOptions().geometry.contains(
			                          Gen::ShapeType::rectangle)
			                   || getOptions().geometry.contains(
			                       Gen::ShapeType::circle);

			if (containsConnected) {
				if (containsSingle) {
					auto lineIndex =
					    Gen::isConnecting(
					        getOptions().geometry.get(0).value)
					        ? 0
					        : 1;

					drawMarker(lineIndex,
					    ::Anim::Weighted<uint64_t>(0));
				}
				else
					blended.marker.prevMainMarkerIdx.visit(
					    drawMarker);
			}
			else
				drawMarker(0, ::Anim::Weighted<uint64_t>(0));
		}
	}
}

void MarkerRenderer::drawLabels(Gfx::ICanvas &canvas) const
{
	for (const auto &blended : markers) {
		if (blended.marker.enabled == false) continue;
		drawLabel(canvas, blended, 0);
		drawLabel(canvas, blended, 1);
	}
}

bool MarkerRenderer::shouldDrawMarkerBody(
    const Gen::Marker &marker) const
{
	bool enabled = marker.enabled != false;
	if (!enabled
	    && getOptions().geometry.factor<Math::FuzzyBool>(
	           Gen::ShapeType::area)
	           != false) {

		if (const auto *prev0 = ConnectingMarker::getPrev(marker,
		        plot->getMarkers(),
		        0))
			enabled |= prev0->enabled != false;
		if (const auto *prev1 = ConnectingMarker::getPrev(marker,
		        plot->getMarkers(),
		        1))
			enabled |= prev1->enabled != false;
	}
	return enabled;
}

void MarkerRenderer::draw(Gfx::ICanvas &canvas,
    Painter &painter,
    const AbstractMarker &abstractMarker,
    double factor,
    bool isLine) const
{
	if (abstractMarker.enabled == false || factor == 0) return;

	painter.setPolygonToCircleFactor(
	    isLine ? 0.0
	           : static_cast<double>(abstractMarker.morphToCircle));
	painter.setPolygonStraightFactor(
	    static_cast<double>(abstractMarker.linear));
	painter.setResMode(ResolutionMode::High);

	auto colors = getColor(abstractMarker, factor);

	canvas.save();

	canvas.setLineColor(colors.first);
	canvas.setLineWidth(*rootStyle.plot.marker.borderWidth);
	canvas.setBrushColor(colors.second);

	auto boundary = abstractMarker.getBoundary();

	auto markerElement =
	    Events::Targets::marker(abstractMarker.marker);

	if (isLine) {
		auto line = abstractMarker.getLine();

		auto p0 = coordSys.convert(line.begin);
		auto p1 = coordSys.convert(line.end);

		canvas.setLineColor(
		    colors.second
		    * static_cast<double>(abstractMarker.connected));

		if (rootEvents.draw.plot.marker.base->invoke(
		        Events::OnLineDrawEvent(*markerElement,
		            {Geom::Line(p0, p1), false}))) {
			painter.drawStraightLine(line,
			    abstractMarker.lineWidth,
			    static_cast<double>(abstractMarker.linear),
			    colors.second,
			    colors.second
			        * static_cast<double>(abstractMarker.connected));

			renderedChart.emplace(
			    Draw::Marker{abstractMarker.marker.enabled != false,
			        abstractMarker.shapeType,
			        abstractMarker.points,
			        abstractMarker.lineWidth},
			    std::move(markerElement));
		}
	}
	else {
		if (rootEvents.draw.plot.marker.base->invoke(
		        Events::OnRectDrawEvent(*markerElement,
		            {boundary, true}))) {
			painter.drawPolygon(abstractMarker.points);
			renderedChart.emplace(
			    Draw::Marker{abstractMarker.marker.enabled != false,
			        abstractMarker.shapeType,
			        abstractMarker.points,
			        abstractMarker.lineWidth},
			    std::move(markerElement));
		}
	}
	canvas.setLineWidth(0);

	canvas.restore();
}

void MarkerRenderer::drawLabel(Gfx::ICanvas &canvas,
    const AbstractMarker &abstractMarker,
    size_t index) const
{
	if (abstractMarker.labelEnabled == false) return;
	const auto &marker = abstractMarker.marker;

	auto weight = marker.label.values[index].weight;
	if (weight == 0.0) return;

	auto color = getColor(abstractMarker, 1, true).second;

	auto text = getLabelText(marker.label, index);
	if (text.empty()) return;

	const auto &labelStyle = rootStyle.plot.marker.label;

	auto labelPos = labelStyle.position->combine<Geom::Line>(
	    [this, &abstractMarker](int, const auto &position)
	    {
		    return abstractMarker.getLabelPos(position, coordSys);
	    });

	canvas.setTextColor((*labelStyle.filter)(color)*weight);

	auto centered = labelStyle.position->factor<double>(
	    Styles::MarkerLabel::Position::center);

	OrientedLabel{{ctx()}}.draw(canvas,
	    text,
	    labelPos,
	    labelStyle,
	    centered,
	    {},
	    weight,
	    *rootEvents.draw.plot.marker.label,
	    Events::Targets::markerLabel(text, marker));
}

std::string MarkerRenderer::getLabelText(
    const ::Anim::Interpolated<Gen::Marker::Label> &label,
    size_t index) const
{
	const auto &labelStyle = rootStyle.plot.marker.label;
	const auto &values = label.values;

	auto needsInterpolation =
	    label.count == 2
	    && (values[0].value.measureId == values[1].value.measureId);

	std::string valueStr;
	if (values[index].value.hasValue()) {
		auto value = needsInterpolation
		               ? label.combine<double>(
		                   [&](int, const auto &value)
		                   {
			                   return value.value.value_or(0);
		                   })
		               : values[index].value.value.value();
		valueStr = Text::SmartString::fromPhysicalValue(value,
		    *labelStyle.numberFormat,
		    static_cast<size_t>(*labelStyle.maxFractionDigits),
		    *labelStyle.numberScale,
		    values[index].value.unit);
	}

	auto indexStr = values[index].value.indexStr;

	typedef Styles::MarkerLabel::Format Format;
	switch (*labelStyle.format) {
	default:
	case Format::measureFirst: {
		auto text = valueStr;
		if (!indexStr.empty()) {
			if (!text.empty()) text += ", ";
			text += indexStr;
		}
		return text;
	}

	case Format::dimensionsFirst: {
		auto text = indexStr;
		if (!valueStr.empty()) {
			if (!text.empty()) text += ", ";
			text += valueStr;
		}
		return text;
	}
	}
}

std::pair<Gfx::Color, Gfx::Color> MarkerRenderer::getColor(
    const AbstractMarker &abstractMarker,
    double factor,
    bool label) const
{
	auto selectedColor =
	    getSelectedColor(abstractMarker.marker, label);

	auto borderAlpha = *rootStyle.plot.marker.borderOpacity;
	auto fillAlpha = *rootStyle.plot.marker.fillOpacity;

	auto fakeBgColor =
	    (*rootStyle.backgroundColor + *rootStyle.plot.backgroundColor)
	        .transparent(1.0);

	auto borderColor =
	    rootStyle.plot.marker.borderOpacityMode->combine<Gfx::Color>(
	        [&](int, const auto &mode)
	        {
		        if (mode
		            == Styles::Marker::BorderOpacityMode::
		                premultiplied)
			        return Math::interpolate(fakeBgColor,
			            selectedColor,
			            borderAlpha);
		        return selectedColor * borderAlpha;
	        });

	auto actBorderColor = Math::interpolate(selectedColor,
	    borderColor,
	    static_cast<double>(abstractMarker.border));

	const auto &enabled =
	    label ? abstractMarker.labelEnabled : abstractMarker.enabled;
	auto alpha = static_cast<double>(enabled) * factor;

	auto finalBorderColor = actBorderColor * alpha;
	auto itemColor = selectedColor * alpha * fillAlpha;

	double highlight = 0.0;
	double anyHighlight = 0.0;
	auto markerInfo = plot->getMarkersInfo();
	for (auto &info : markerInfo) {
		auto allHighlight = 0.0;
		info.second.visit(
		    [&highlight,
		        &allHighlight,
		        idx = abstractMarker.marker.idx](int,
		        const auto &info)
		    {
			    highlight += info.value.markerId == idx ? 1.0 : 0.0;
			    if (info.value.markerId.has_value())
				    allHighlight += info.weight;
		    });
		anyHighlight = std::max(anyHighlight, allHighlight);
	}

	auto highlightAlpha = 1 - (0.65 * anyHighlight) * (1 - highlight);
	finalBorderColor = (finalBorderColor * highlightAlpha);
	itemColor = (itemColor * highlightAlpha);

	return std::make_pair(finalBorderColor, itemColor);
}

Gfx::Color MarkerRenderer::getSelectedColor(const Gen::Marker &marker,
    bool label) const
{
	const ColorBuilder colorBuilder(
	    rootStyle.plot.marker.lightnessRange(),
	    *rootStyle.plot.marker.colorPalette,
	    *rootStyle.plot.marker.colorGradient);

	auto markerColor = colorBuilder.render(marker.colorBase);

	return label ? Math::interpolate(markerColor,
	           rootStyle.plot.marker.label.color->transparent(1.0),
	           rootStyle.plot.marker.label.color->alpha)
	             : markerColor;
}

MarkerRenderer MarkerRenderer::create(const DrawingContext &ctx)
{
	MarkerRenderer res{{ctx}, {}};
	for (auto &marker : res.plot->getMarkers()) {
		if (!res.shouldDrawMarkerBody(marker)) continue;

		res.markers.push_back(
		    AbstractMarker::createInterpolated(ctx, marker, 0));
	}
	return res;
}

}