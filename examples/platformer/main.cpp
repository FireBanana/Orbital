#include "graphics.h"
#include "window.h"
#include <passes/forward_pass.h>
#include <passes/gui_pass.h>

int main()
{
    Window w{};
    Graphics g{&w};

    GuiPass gPass{&g};
    ForwardPass fPass{&g};

    std::vector<Pass *> gPasses{&gPass};
    std::vector<Pass *> cPasses{};

    g.beginRenderLoop(gPasses, cPasses, [](double t) {});
}