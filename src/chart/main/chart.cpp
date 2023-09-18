#include "chart.h"

#include "chart/rendering/drawbackground.h"
#include "chart/rendering/drawlabel.h"
#include "chart/rendering/drawlegend.h"
#include "chart/rendering/drawmarkerinfo.h"
#include "chart/rendering/drawplot.h"
#include "chart/rendering/logo.h"
#include "data/datacube/datacube.h"

Vizzu::Chart::Chart() :
    animator(std::make_shared<Anim::Animator>()),
    stylesheet(Styles::Chart::def()),
    computedStyles(stylesheet.getDefaultParams()),
    events(*this)
{
	stylesheet.setActiveParams(actStyles);
	nextOptions = std::make_shared<Gen::Options>();

	animator->onDraw.attach(
	    [this](const Gen::PlotPtr &actPlot)
	    {
		    this->actPlot = actPlot;
		    if (onChanged) onChanged();
	    });
	animator->onProgress.attach(
	    [this]()
	    {
		    events.animation.update->invoke(
		        Events::OnUpdateEvent(animator->getControl()));
	    });
	animator->onBegin = [this]()
	{
		events.animation.begin->invoke();
	};
	animator->onComplete = [this]()
	{
		events.animation.complete->invoke();
	};
}

void Vizzu::Chart::setBoundRect(const Geom::Rect &rect,
    Gfx::ICanvas &info)
{
	if (actPlot) {
		actPlot->getStyle().setup();
		layout.setBoundary(rect, *actPlot, info);
	}
	else {
		layout.setBoundary(rect, info);
	}
}

void Vizzu::Chart::animate(const OnComplete &onComplete)
{
	auto f = [this, onComplete](const Gen::PlotPtr &plot, bool ok)
	{
		actPlot = plot;
		if (ok) {
			prevOptions = *nextOptions;
			prevStyles = actStyles;
		}
		else {
			*nextOptions = prevOptions;
			actStyles = prevStyles;
			computedStyles = plot->getStyle();
		}
		if (onComplete) onComplete(ok);
	};
	animator->animate(nextAnimOptions.control, f);
	nextAnimOptions = Anim::Options();
	nextOptions = std::make_shared<Gen::Options>(*nextOptions);
}

void Vizzu::Chart::setKeyframe()
{
	animator->addKeyframe(plot(nextOptions),
	    nextAnimOptions.keyframe);
	nextAnimOptions.keyframe = Anim::Options::Keyframe();
	nextOptions = std::make_shared<Gen::Options>(*nextOptions);
}

void Vizzu::Chart::setAnimation(const Anim::AnimationPtr &animation)
{
	animator->setAnimation(animation);
}

Vizzu::Gen::Config Vizzu::Chart::getConfig()
{
	return Gen::Config{getSetter()};
}

Vizzu::Gen::OptionsSetter Vizzu::Chart::getSetter()
{
	Gen::OptionsSetter setter(*nextOptions);
	setter.setTable(&table);
	return setter;
}

void Vizzu::Chart::draw(Gfx::ICanvas &canvas)
{
	if (actPlot
	    && (!events.draw.begin
	        || events.draw.begin->invoke(
	            Util::EventDispatcher::Params{}))) {

		auto coordSys = getCoordSystem();

		Draw::RenderedChart rendered(coordSys, actPlot.get());

		Draw::DrawingContext context(canvas,
		    layout,
		    events,
		    *actPlot,
		    coordSys,
		    rendered);

		Draw::DrawBackground(context,
		    layout.boundary.outline(Geom::Size::Square(1)),
		    actPlot->getStyle(),
		    events.draw.background,
		    std::make_unique<Events::Targets::Root>());

		Draw::DrawPlot{context};

		actPlot->getOptions()->legend.visit(
		    [&](int, const auto &legend)
		    {
			    if (legend.value)
				    Draw::DrawLegend(context,
				        *legend.value,
				        legend.weight);
		    });

		actPlot->getOptions()->title.visit(
		    [this, &context](int, const auto &title)
		    {
			    if (title.value.has_value()) {
				    Draw::DrawLabel(context,
				        Geom::TransformedRect::fromRect(layout.title),
				        *title.value,
				        actPlot->getStyle().title,
				        events.draw.title,
				        std::make_unique<Events::Targets::ChartTitle>(
				            *title.value),
				        Draw::DrawLabel::Options(true,
				            std::max(title.weight * 2 - 1, 0.0)));
			    }
		    });

		Draw::DrawMarkerInfo(layout, canvas, *actPlot);

		renderedChart = std::move(rendered);
	}

	auto logoRect = getLogoBoundary();
	if (auto logoElement = std::make_unique<Events::Targets::Logo>();
	    events.draw.logo->invoke(Events::OnRectDrawEvent(*logoElement,
	        {logoRect, false}))) {
		auto filter = *(actPlot ? actPlot->getStyle()
		                        : stylesheet.getDefaultParams())
		                   .logo.filter;

		Draw::Logo(canvas).draw(logoRect.pos,
		    logoRect.width(),
		    filter);

		renderedChart.emplace(
		    Geom::TransformedRect::fromRect(logoRect),
		    std::move(logoElement));
	}

	if (events.draw.complete)
		events.draw.complete->invoke(Util::EventDispatcher::Params{});
}

Geom::Rect Vizzu::Chart::getLogoBoundary() const
{
	const auto &logoStyle = (actPlot ? actPlot->getStyle()
	                                 : stylesheet.getDefaultParams())
	                            .logo;

	auto logoWidth =
	    logoStyle.width->get(layout.boundary.size.minSize(),
	        Styles::Sheet::baseFontSize(layout.boundary.size, false));

	auto logoHeight = Draw::Logo::height(logoWidth);

	auto logoPad =
	    logoStyle.toMargin(Geom::Size{logoWidth, logoHeight},
	        Styles::Sheet::baseFontSize(layout.boundary.size, false));

	return {layout.boundary.topRight()
	            - Geom::Point{logoPad.right + logoWidth,
	                logoPad.bottom + logoHeight},
	    Geom::Size{logoWidth, logoHeight}};
}

Vizzu::Gen::PlotPtr Vizzu::Chart::plot(
    const Gen::PlotOptionsPtr &options)
{
	options->setAutoParameters();

	computedStyles =
	    stylesheet.getFullParams(options, layout.boundary.size);

	return std::make_shared<Gen::Plot>(table,
	    options,
	    computedStyles,
	    false);
}

Vizzu::Draw::CoordinateSystem Vizzu::Chart::getCoordSystem() const
{
	if (actPlot) {
		const auto &rootStyle = actPlot->getStyle();

		auto plotArea = rootStyle.plot.contentRect(layout.plot,
		    rootStyle.calculatedSize());

		const auto &options = *actPlot->getOptions();

		return {plotArea,
		    options.angle,
		    options.coordSystem,
		    actPlot->keepAspectRatio};
	}
	return {layout.plotArea,
	    0.0,
	    ::Anim::Interpolated<Gen::CoordSystem>{
	        Gen::CoordSystem::cartesian},
	    Math::FuzzyBool()};
}

const Vizzu::Gen::Marker *Vizzu::Chart::markerByIndex(
    size_t index) const
{
	if (actPlot) {
		auto &markers = actPlot->getMarkers();
		if (index < markers.size()) return &markers[index];
	}
	return nullptr;
}
