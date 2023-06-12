#include "drawaxes.h"

#include "chart/rendering/drawguides.h"
#include "chart/rendering/drawinterlacing.h"
#include "chart/rendering/draworientedlabel.h"

#include "drawlabel.h"

using namespace Geom;
using namespace Vizzu;
using namespace Vizzu::Base;
using namespace Vizzu::Draw;
using namespace Vizzu::Diag;

drawAxes::drawAxes(const DrawingContext &context) :
    DrawingContext(context)
{}

void drawAxes::drawBase()
{
	drawInterlacing(*this, false);

	drawAxis(Diag::ChannelId::x);
	drawAxis(Diag::ChannelId::y);

	drawGuides(*this);
}

void drawAxes::drawLabels()
{
	drawInterlacing(*this, true);

	drawDiscreteLabels(true);
	drawDiscreteLabels(false);

	drawTitle(Diag::ChannelId::x);
	drawTitle(Diag::ChannelId::y);
}

Geom::Line drawAxes::getAxis(Diag::ChannelId axisIndex) const
{
	auto horizontal = axisIndex == Diag::ChannelId::x;

	auto offset = diagram.axises.other(axisIndex).origo();

	auto direction = Point::Ident(horizontal);

	auto p0 = direction.flip() * offset;
	auto p1 = p0 + direction;

	if (offset >= 0 && offset <= 1)
		return Geom::Line(p0, p1);
	else
		return Geom::Line();
}

void drawAxes::drawAxis(Diag::ChannelId axisIndex)
{
	const char *element =
	    axisIndex == Diag::ChannelId::x ? "plot.xAxis" : "plot.yAxis";

	auto lineBaseColor = *style.plot.getAxis(axisIndex).color
	                   * (double)diagram.anyAxisSet;

	if (lineBaseColor.alpha <= 0) return;

	auto line = getAxis(axisIndex);

	if (!line.isPoint()) {
		auto lineColor =
		    lineBaseColor * (double)diagram.guides.at(axisIndex).axis;

		canvas.setLineColor(lineColor);
		canvas.setLineWidth(1.0);

		if (events.plot.axis.base->invoke(
		        Events::OnLineDrawParam(element, line))) {
			painter.drawLine(line);
		}
	}
}

Geom::Point drawAxes::getTitleBasePos(Diag::ChannelId axisIndex,
    int index) const
{
	typedef Styles::AxisTitle::Position Pos;
	typedef Styles::AxisTitle::VPosition VPos;

	const auto &titleStyle = style.plot.getAxis(axisIndex).title;

	double orthogonal;

	switch (titleStyle.position->get(index).value) {
	default:
	case Pos::min_edge: orthogonal = 0.0; break;
	case Pos::max_edge: orthogonal = 1.0; break;
	case Pos::axis:
		orthogonal = diagram.axises.other(axisIndex).origo();
		break;
	}

	double parallel;

	switch (titleStyle.vposition->get(index).value) {
	default:
	case VPos::end: parallel = 1.0; break;
	case VPos::middle: parallel = 0.5; break;
	case VPos::begin: parallel = 0.0; break;
	}

	return axisIndex == Diag::ChannelId::x
	         ? Geom::Point(parallel, orthogonal)
	         : Geom::Point(orthogonal, parallel);
}

Geom::Point drawAxes::getTitleOffset(Diag::ChannelId axisIndex,
    int index,
    bool fades) const
{
	const auto &titleStyle = style.plot.getAxis(axisIndex).title;

	auto calcSide = [](int, auto side)
	{
		typedef Styles::AxisTitle::Side Side;
		switch (side) {
		default:
		case Side::positive: return 1.0;
		case Side::negative: return -1.0;
		case Side::upon: return 0.0;
		}
	};

	auto orthogonal =
	    fades ? calcSide(0, titleStyle.side->get(index).value)
	          : titleStyle.side->combine<double>(calcSide);

	auto calcVSide = [](int, auto side) -> double
	{
		typedef Styles::AxisTitle::VSide Side;
		switch (side) {
		default:
		case Side::positive: return 1.0;
		case Side::negative: return -1.0;
		case Side::upon: return 0.0;
		}
	};

	auto parallel =
	    fades ? calcVSide(0, titleStyle.vside->get(index).value)
	          : titleStyle.vside->combine<double>(calcVSide);

	return axisIndex == Diag::ChannelId::x
	         ? Geom::Point(parallel, -orthogonal)
	         : Geom::Point(orthogonal, -parallel);
}

void drawAxes::drawTitle(Diag::ChannelId axisIndex)
{
	const auto &titleString = diagram.axises.at(axisIndex).title;
	const char *element = axisIndex == Diag::ChannelId::x
	                        ? "plot.xAxis.title"
	                        : "plot.yAxis.title";

	const auto &titleStyle = style.plot.getAxis(axisIndex).title;

	auto fades = titleStyle.position->interpolates()
	          || titleStyle.vposition->interpolates()
	          || titleString.interpolates();

	for (int index = 0; index < (fades ? 2 : 1); index++) {
		auto title = titleString.get(index);

		if (!title.value.empty()) {
			auto weight = title.weight
			            * titleStyle.position->get(index).weight
			            * titleStyle.vposition->get(index).weight;

			Gfx::Font font(titleStyle);
			canvas.setFont(font);
			auto textBoundary = canvas.textBoundary(title.value);
			auto textMargin =
			    titleStyle.toMargin(textBoundary, font.size);
			auto size = textBoundary + textMargin.getSpace();

			auto relCenter = getTitleBasePos(axisIndex, index);

			auto normal = Geom::Point::Ident(true);

			auto offset = getTitleOffset(axisIndex, index, fades);

			auto posDir = coordSys.convertDirectionAt(
			    Geom::Line(relCenter, relCenter + normal));

			auto posAngle = posDir.getDirection().angle();

			canvas.save();

			Geom::AffineTransform transform(posDir.begin,
			    1.0,
			    -posAngle);

			auto calcOrientation = [&](int, auto orientation)
			{
				return orientation
				            == Styles::AxisTitle::Orientation::
				                   vertical
				         ? size.flip()
				         : size;
			};

			auto angle =
			    -M_PI / 2.0
			    * (fades ? titleStyle.orientation->get(index).value
			                   == Styles::AxisTitle::Orientation::
			                       vertical
			             : titleStyle.orientation->factor(Styles::
			                     AxisTitle::Orientation::vertical));

			auto orientedSize =
			    (fades ? calcOrientation(0,
			         titleStyle.orientation->get(index).value)
			           : titleStyle.orientation->combine<Geom::Size>(
			               calcOrientation));

			auto center = offset * (orientedSize / 2.0);

			transform = transform
			          * Geom::AffineTransform(center, 1.0, angle)
			          * Geom::AffineTransform((orientedSize / -2.0));

			canvas.transform(transform);

			canvas.setTextColor(*titleStyle.color * weight);

			auto realAngle = Geom::Angle(-posAngle + angle).rad();
			auto upsideDown =
			    realAngle > M_PI / 2.0 && realAngle < 3 * M_PI / 2.0;

			Events::Events::OnTextDrawParam param(element);
			drawLabel(Geom::Rect(Geom::Point(), size),
			    title.value,
			    titleStyle,
			    events.plot.axis.title,
			    std::move(param),
			    canvas,
			    drawLabel::Options(false, 1.0, upsideDown));

			canvas.restore();
		}
	}
}

void drawAxes::drawDiscreteLabels(bool horizontal)
{
	auto axisIndex = horizontal ? Diag::ChannelId::x : Diag::ChannelId::y;

	const auto &labelStyle = style.plot.getAxis(axisIndex).label;

	auto textColor = *labelStyle.color;
	if (textColor.alpha == 0.0) return;

	auto origo = diagram.axises.origo();
	const auto &axises = diagram.discreteAxises;
	const auto &axis = axises.at(axisIndex);

	if (axis.enabled) {
		canvas.setFont(Gfx::Font(labelStyle));

		Diag::DiscreteAxis::Values::const_iterator it;
		for (it = axis.begin(); it != axis.end(); ++it) {
			drawDiscreteLabel(horizontal, origo, it);
		}
	}
}

void drawAxes::drawDiscreteLabel(bool horizontal,
    const Geom::Point &origo,
    Diag::DiscreteAxis::Values::const_iterator it)
{
	const char *element =
	    horizontal ? "plot.xAxis.label" : "plot.yAxis.label";
	auto &enabled = horizontal ? diagram.guides.x : diagram.guides.y;
	auto axisIndex = horizontal ? Diag::ChannelId::x : Diag::ChannelId::y;
	const auto &labelStyle = style.plot.getAxis(axisIndex).label;
	auto textColor = *labelStyle.color;

	auto text = it->second.label;
	auto weight = it->second.weight * (double)enabled.labels;
	if (weight == 0) return;

	auto ident = Geom::Point::Ident(horizontal);
	auto normal = Geom::Point::Ident(!horizontal);

	typedef Styles::AxisLabel::Position Pos;
	labelStyle.position->visit(
	    [&](int index, const auto &position)
	    {
		    if (labelStyle.position->interpolates()
		        && !it->second.presentAt(index))
			    return;

		    Geom::Point refPos;

		    switch (position.value) {
		    case Pos::max_edge:
			    refPos = Geom::Point::Ident(!horizontal);
			    break;
		    case Pos::axis: refPos = origo.comp(!horizontal); break;
		    default:
		    case Pos::min_edge: refPos = Geom::Point(); break;
		    }

		    auto relCenter =
		        refPos + ident * it->second.range.middle();

		    double under =
		        labelStyle.position->interpolates()
		            ? labelStyle.side->get(index).value
		                  == Styles::AxisLabel::Side::negative
		            : labelStyle.side->factor(
		                Styles::AxisLabel::Side::negative);

		    auto sign = 1 - 2 * under;

		    auto posDir = coordSys.convertDirectionAt(
		        Geom::Line(relCenter, relCenter + normal));

		    posDir = posDir.extend(sign);

		    drawOrientedLabel(*this,
		        text,
		        posDir,
		        labelStyle,
		        events.plot.axis.label,
		        std::move(Events::Events::OnTextDrawParam(element)),
		        0,
		        textColor * weight * position.weight,
		        *labelStyle.backgroundColor);
	    });
}
