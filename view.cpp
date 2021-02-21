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

#include "view.hpp"

#include "imgui_internal.h"


std::tuple<bool, int> drawView(
  const ImVec2& windowSize,
  const std::string& windowTitle,
  const std::string& inputText,
  const bool showYesNoButtons,
  bool running,
  int exitCode)
{
  ImGui::SetNextWindowSize(windowSize);
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::Begin(
    windowTitle.c_str(),
    &running,
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoResize);

  const auto buttonSpaceRequired =
    ImGui::CalcTextSize("Close", nullptr, true).y +
    ImGui::GetStyle().FramePadding.y * 2.0f;
  const auto maxTextHeight = ImGui::GetContentRegionAvail().y -
    ImGui::GetStyle().ItemSpacing.y -
    buttonSpaceRequired;

  if (ImGui::IsWindowAppearing() && !showYesNoButtons)
  {
    ImGui::SetNextWindowFocus();
  }

  ImGui::BeginChild(
    "#scroll_area",
    {0, maxTextHeight},
    true,
    ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::TextUnformatted(inputText.c_str());
  ImGui::EndChild();

  // Draw buttons
  if (showYesNoButtons) {
    const auto buttonWidth = windowSize.x / 3.0f;
    ImGui::SetCursorPosX(
      (windowSize.x - (buttonWidth * 2 + ImGui::GetStyle().ItemSpacing.x))
      / 2.0f);

    if (ImGui::Button("Yes", {buttonWidth, 0.0f}))
    {
      // return 21 if selected yes, this is for checking return code in bash scripts
      exitCode = 21;
      running = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("No", {buttonWidth, 0.0f}))
    {
      running = false;
    }

    // Auto-focus the yes button
    if (ImGui::IsWindowAppearing())
    {
      ImGui::SetFocusID(ImGui::GetID("Yes"), ImGui::GetCurrentWindow());
      ImGui::GetCurrentContext()->NavDisableHighlight = false;
      ImGui::GetCurrentContext()->NavDisableMouseHover = true;
    }
  } else {
    const auto buttonWidth = windowSize.x / 3.0f;
    ImGui::SetCursorPosX((windowSize.x - buttonWidth) / 2.0f);
    if (ImGui::Button("Close", {buttonWidth, 0.0f}))
    {
      running = false;
    }
  }

  ImGui::End();

  return {running, exitCode};
}
