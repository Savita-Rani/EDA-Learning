#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include <eda/Netlist.h>
#include <eda/WaveformBuffer.h>

// ============================================================================
// main.cpp — console-turned-GUI driver for Phase 9.
//
// The first real GUI in this project. Everything before this phase
// printed to stdout; this one opens an actual window.
//
// Built on Phase 3's Netlist (reused — new capability, an interactive
// GUI, built on an existing piece, same relationship Phase 5's
// NetlistLoader had to Netlist) — the same 4-NAND XOR circuit every
// prior demo used, now driven interactively instead of looped over all
// 4 input combinations automatically.
//
// Concepts introduced here:
//  - The immediate-mode GUI event loop: poll events -> update app state
//    -> build this frame's entire UI from scratch (ImGui::Checkbox/
//    PlotLines/etc. — nothing persists between frames except the plain
//    C++ variables WE keep, like `inputA`/`outHistory`) -> render ->
//    swap buffers -> repeat. Nothing like this project's prior main()
//    functions, which ran once and printed output.
//  - Vendoring a third-party library as source (see CMakeLists.txt):
//    Dear ImGui ships as source specifically meant to be compiled
//    directly into your own project — there's no "ImGui.dll" to link
//    against.
//  - --max-frames=N: an automated-verification affordance, NOT how a
//    real user runs this. See docs/phases/phase9.md, "What this phase
//    does and doesn't unit-test," for why: there's no practical way to
//    assert against a rendered frame's pixels, so basic
//    functional correctness (does it open a window, run its loop, and
//    shut down cleanly without crashing?) was instead verified by
//    actually running the binary in this auto-exiting mode.
// ============================================================================

using namespace eda;

namespace {

void glfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

int parseMaxFrames(int argc, char** argv) {
    const std::string prefix = "--max-frames=";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind(prefix, 0) == 0) {
            return std::stoi(arg.substr(prefix.size()));
        }
    }
    return -1; // run until the window is closed normally
}

} // namespace

int main(int argc, char** argv) {
    int maxFrames = parseMaxFrames(argc, argv);

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(900, 600, "EDA Logic Simulator -- Phase 9: ImGui Frontend", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    Netlist netlist;
    netlist.addComponent(std::make_unique<NandGate>(), {"A", "B"}, "n1");
    netlist.addComponent(std::make_unique<NandGate>(), {"A", "n1"}, "n2");
    netlist.addComponent(std::make_unique<NandGate>(), {"B", "n1"}, "n3");
    netlist.addComponent(std::make_unique<NandGate>(), {"n2", "n3"}, "OUT");

    bool inputA = false;
    bool inputB = false;
    WaveformBuffer outHistory(200);

    int frameCount = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        netlist.setInput("A", inputA);
        netlist.setInput("B", inputB);
        netlist.simulate();
        outHistory.push(netlist.wireValue("OUT") ? 1.0f : 0.0f);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Schematic -- 4-NAND XOR");
        ImGui::Text("Same circuit as the Phase 3/4/5/8 console demos, driven interactively.");
        ImGui::Checkbox("A", &inputA);
        ImGui::Checkbox("B", &inputB);
        ImGui::Separator();

        auto wireRow = [&](const char* name) {
            bool value = netlist.wireValue(name);
            ImVec4 color = value ? ImVec4(0.3f, 0.9f, 0.3f, 1.0f) : ImVec4(0.7f, 0.3f, 0.3f, 1.0f);
            ImGui::TextColored(color, "%-4s = %d", name, value ? 1 : 0);
        };
        wireRow("A");
        wireRow("B");
        wireRow("n1");
        wireRow("n2");
        wireRow("n3");
        wireRow("OUT");
        ImGui::End();

        ImGui::Begin("Waveform -- OUT over time");
        const std::vector<float>& samples = outHistory.samples();
        if (!samples.empty()) {
            ImGui::PlotLines("OUT", samples.data(), static_cast<int>(samples.size()),
                              0, nullptr, -0.2f, 1.2f, ImVec2(0, 120));
        }
        ImGui::End();

        ImGui::Render();

        int displayW;
        int displayH;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        ++frameCount;
        if (maxFrames >= 0 && frameCount >= maxFrames) {
            glfwSetWindowShouldClose(window, true);
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    std::printf("Exited cleanly after %d frames.\n", frameCount);
    return 0;
}
