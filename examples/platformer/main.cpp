#include "graphics.h"
#include "twodpass.h"
#include "window.h"
#include <passes/gui_pass.h>

int main()
{
    Window w{};
    Graphics g{&w};

    GuiPass gPass{&g};
    TwoDPass tdPass{&g};

    std::vector<Pass *> gPasses{&tdPass, &gPass};
    std::vector<Pass *> cPasses{};

    g.beginRenderLoop(gPasses, cPasses, [](double t) {});
}