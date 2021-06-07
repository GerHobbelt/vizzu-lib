#ifndef GUI_MAINWIDGET
#define GUI_MAINWIDGET

#include "screeninfo.h"
#include "transformerwidget.h"

namespace GUI
{

class MainWidget : public TransformerWidget
{
public:
	typedef TransformerWidget BaseWidget;

	MainWidget(const GUI::ScreenInfo &screenInfo);

	void onUpdateSize(Gfx::ICanvas &info, Geom::Size &size) override;
	DragObjectPtr onMouseDown(const Geom::Point &pos) override;
	bool onMouseUp(const Geom::Point &pos, DragObjectPtr dragObject) override;
	bool onMouseMove(const Geom::Point &pos, DragObjectPtr &dragObject) override;

protected:
	DragObjectPtr dragObject;
};

}

#endif
