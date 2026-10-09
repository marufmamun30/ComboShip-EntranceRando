#ifndef RANDO_ENTRANCE_TRACKER_H
#define RANDO_ENTRANCE_TRACKER_H

#include <ship/window/gui/GuiWindow.h>

namespace Rando {

namespace Entrance {

class EntranceTrackerWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override{};
    void DrawElement() override;
    void UpdateElement() override{};
};

} // namespace Entrance

} // namespace Rando

#endif // RANDO_ENTRANCE_TRACKER_H
