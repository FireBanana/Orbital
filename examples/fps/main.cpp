#include "window.h"
#include <passes/forward_pass.h>

int main()
{
    Window w{};
    Graphics g{&w};

    std::vector<Pass *> passes{new ForwardPass{&g}};
    std::vector<Pass *> cPasses{};

    auto map = AssetLoader::loadModel(ROOT "examples/fps/assets/testscene.glb");
    auto nmap = g.makeNativeModel(map);

    std::vector<NativeModel> models{nmap};

    passes[0]->attachModels(&models);
    static_cast<ForwardPass *>(passes[0])->setCameraPosition({2, 1, 2});

    g.beginRenderLoop(passes, cPasses, [](double, double) {});
}