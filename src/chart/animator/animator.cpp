#include "animator.h"

#include "base/io/log.h"
#include "chart/animator/keyframe.h"
#include "chart/generator/selector.h"

using namespace Vizzu;
using namespace Vizzu::Anim;
using namespace std::chrono;

Animator::Animator() : running(false)
{
	actAnimation = std::make_shared<Animation>(Diag::PlotPtr());
	nextAnimation = std::make_shared<Animation>(Diag::PlotPtr());
}

void Animator::addKeyframe(const Diag::PlotPtr &plot,
    const Options::Keyframe &options)
{
	nextAnimation->addKeyframe(plot, options);
}

void Animator::setAnimation(const Anim::AnimationPtr &animation)
{
	nextAnimation = animation;
}

void Animator::animate(const Options::Control &options,
    Animation::OnComplete onThisCompletes)
{
	if (running)
		throw std::logic_error("animation already in progress");

	auto completionCallback =
	    [=, this](Diag::PlotPtr plot, bool ok)
	{
		nextAnimation = std::make_shared<Animation>(plot);
		this->running = false;
		onThisCompletes(plot, ok);
	};

	running = true;
	stripActAnimation();
	actAnimation = nextAnimation;
	nextAnimation = AnimationPtr();
	setupActAnimation();
	actAnimation->animate(options, completionCallback);
}

void Animator::setupActAnimation()
{
	actAnimation->onPlotChanged.attach(
	    [&](const Diag::PlotPtr &actual)
	    {
		    onProgress();
		    onDraw(actual);
	    });

	actAnimation->onBegin.attach(onBegin);
	actAnimation->onComplete.attach(onComplete);
}

void Animator::stripActAnimation()
{
	actAnimation->onPlotChanged.detachAll();
	actAnimation->onBegin.detachAll();
	actAnimation->onComplete.detachAll();
}
