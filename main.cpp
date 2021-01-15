/** Copyright (c) 2021 Nikolai Wuttke
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in all
  * copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  * SOFTWARE.
  */

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <cxxopts.hpp>
#include <GLES2/gl2.h>
#include <SDL.h>

#include <cstdlib>
#include <iostream>
#include <fstream>


namespace
{

std::optional<cxxopts::ParseResult> parseArgs(int argc, char** argv)
{
  try
  {
    cxxopts::Options options(argv[0], "TvTextViewer - a full-screen text viewer");

    options
      .positional_help("[input file]")
      .show_positional_help()
      .add_options()
        ("input_file", "text file to view", cxxopts::value<std::string>())
        ("f,font_size", "font size in pixels", cxxopts::value<int>())
        ("t,title", "window title (filename by default)", cxxopts::value<std::string>())
        ("h,help", "show help")
      ;

    options.parse_positional({"input_file"});

    try
    {
      const auto result = options.parse(argc, argv);

      if (result.count("help"))
      {
        std::cout << options.help({""}) << '\n';
        std::exit(0);
      }

      if (!result.count("input_file"))
      {
        std::cerr << "Error: No input given\n\n";
        std::cerr << options.help({""}) << '\n';
        return {};
      }

      return result;
    }
    catch (const cxxopts::OptionParseException& e)
    {
      std::cerr << "Error: " << e.what() << "\n\n";
      std::cerr << options.help({""}) << '\n';
    }
  }
  catch (const cxxopts::OptionSpecException& e)
  {
    std::cerr << "Error defining options: " << e.what() << '\n';
  }

  return {};
}


void run(SDL_Window* pWindow, const cxxopts::ParseResult& args)
{
  const auto& inputFilename = args["input_file"].as<std::string>();

  std::string inputText;

  {
    std::ifstream file(inputFilename, std::ios::ate);
    const auto fileSize = file.tellg();
    file.seekg(0);
    inputText.resize(fileSize);
    file.read(&inputText[0], fileSize);
  }

  const auto windowTitle = args.count("title")
    ? args["title"].as<std::string>() : inputFilename;

  auto& io = ImGui::GetIO();

  auto running = true;
  auto focused = false;
  while (running)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (
          event.type == SDL_QUIT ||
          (event.type == SDL_WINDOWEVENT &&
           event.window.event == SDL_WINDOWEVENT_CLOSE &&
           event.window.windowID == SDL_GetWindowID(pWindow))
        ) {
          running = false;
        }
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(pWindow);
    ImGui::NewFrame();

    // Draw single window with scrollable text
    const auto& windowSize = io.DisplaySize;
    ImGui::SetNextWindowSize(windowSize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin(
      windowTitle.c_str(),
      &running,
      ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoSavedSettings);

    const auto buttonSpaceRequired =
      ImGui::CalcTextSize("Close", nullptr, true).y +
      ImGui::GetStyle().FramePadding.y * 2.0f;
    const auto maxTextHeight = ImGui::GetContentRegionAvail().y -
      ImGui::GetStyle().ItemSpacing.y -
      buttonSpaceRequired;

    if (!focused)
    {
      ImGui::SetNextWindowFocus();
      focused = true;
    }

    ImGui::BeginChild("#scroll_area", {0, maxTextHeight}, true);
    ImGui::TextUnformatted(inputText.c_str());
    ImGui::EndChild();

    const auto buttonWidth = windowSize.x / 3.0f;
    ImGui::SetCursorPosX((windowSize.x - buttonWidth) / 2.0f);
    if (ImGui::Button("Close", {buttonWidth, 0.0f}))
    {
      running = false;
    }

    ImGui::End();

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(pWindow);
  }
}

}


int main(int argc, char** argv)
{
  const auto oArgs = parseArgs(argc, argv);
  if (!oArgs)
  {
    return -2;
  }

  const auto& args = *oArgs;

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
  {
    std::cerr << "Error: " << SDL_GetError() << '\n';
    return -1;
  }

  // Setup window and OpenGL
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  SDL_DisplayMode displayMode;
  SDL_GetDesktopDisplayMode(0, &displayMode);

  auto pWindow = SDL_CreateWindow(
    "Log Viewer",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    displayMode.w,
    displayMode.h,
    SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);

  auto pGlContext = SDL_GL_CreateContext(pWindow);
  SDL_GL_MakeCurrent(pWindow, pGlContext);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  if (args.count("font_size"))
  {
    ImFontConfig config;
    config.SizePixels = args["font_size"].as<int>();
    ImGui::GetIO().Fonts->AddFontDefault(&config);
  }

  // Setup Platform/Renderer bindings
  ImGui_ImplSDL2_InitForOpenGL(pWindow, pGlContext);
  ImGui_ImplOpenGL3_Init(nullptr);

  // Main loop
  run(pWindow, args);

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(pGlContext);
  SDL_DestroyWindow(pWindow);
  SDL_Quit();

  return 0;
}
