#include "graphics.h"
#include "window.h"

int main()
{
    Window w{};
    Graphics g{&w};

    std::vector<Pass *> gPasses{};
    std::vector<Pass *> cPasses{};

    g.beginRenderLoop(gPasses, cPasses, [](double t) {});
}