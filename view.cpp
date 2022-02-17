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

#include <poll.h>
#include <unistd.h>

#include <sstream>
#include <stdexcept>


View::View(
  std::string windowTitle,
  std::string inputTextOrScriptFile,
  const bool showYesNoButtons,
  const bool wrapLines,
  const bool inputTextIsScriptFile)
  : mTitle(std::move(windowTitle))
  , mText([&]() -> decltype(mText) {
      if (inputTextIsScriptFile)
      {
        if (wrapLines)
        {
          return std::vector<std::string>{};
        }
        else
        {
          return "";
        }
      }

      return std::move(inputTextOrScriptFile);
    }())
  , mpScriptPipe(nullptr)
  , mScriptPipeFd(-1)
  , mShowYesNoButtons(showYesNoButtons)
{
  // We are executing a script instead of showing some text.
  // Start executing it, and grab the file descriptor for polling.
  if (inputTextIsScriptFile)
  {
    mpScriptPipe = popen((inputTextOrScriptFile + " 2>&1 ").c_str(), "r");
    if (!mpScriptPipe)
    {
      throw std::runtime_error("Failed to execute script");
    }

    mScriptPipeFd = fileno(mpScriptPipe);
    if (mScriptPipeFd == -1)
    {
      pclose(mpScriptPipe);
      throw std::runtime_error("Failed to execute script");
    }
  }
  else if (wrapLines)
  {
    std::stringstream stream{std::get<std::string>(mText)};
    std::vector<std::string> lines;
    for (std::string line; std::getline(stream, line); )
    {
      lines.push_back(line);
    }

    mText = std::move(lines);
  }
}


View::~View()
{
  closeScriptPipe();
}


std::optional<int> View::draw(const ImVec2& windowSize)
{
  ImGui::SetNextWindowSize(windowSize);
  ImGui::SetNextWindowPos(ImVec2(0, 0));

  bool scroll = false;
  auto running = true;
  ImGui::Begin(
    mTitle.c_str(),
    &running,
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoResize);

  const auto buttonSpaceRequired =
    ImGui::CalcTextSize("Close", nullptr, true).y +
    ImGui::GetStyle().FramePadding.y * 2.0f;
  const auto maxTextHeight = ImGui::GetContentRegionAvail().y -
    ImGui::GetStyle().ItemSpacing.y -
    buttonSpaceRequired;

  if (ImGui::IsWindowAppearing() && !mShowYesNoButtons)
  {
    ImGui::SetNextWindowFocus();
  }

  ImGui::BeginChild(
    "#scroll_area",
    {0, maxTextHeight},
    true,
    ImGuiWindowFlags_HorizontalScrollbar);

  // We are executing a script instead of showing some text.
  // Fetch output from the script and append it to our text buffer.
  if (mpScriptPipe)
  {
    scroll = fetchScriptOutput();
  }

  // Draw the text buffer.
  if (const auto pText = std::get_if<std::string>(&mText))
  {
    ImGui::TextUnformatted(pText->c_str());
  }
  else if (const auto pLines = std::get_if<std::vector<std::string>>(&mText))
  {
    for (const auto& line : *pLines)
    {
      ImGui::TextWrapped(line.c_str());
    }
  }

  if (scroll)
  {
    ImGui::SetScrollHere(1.0);
  }

  ImGui::EndChild();

  // Draw buttons
  if (mShowYesNoButtons) {
    const auto buttonWidth = windowSize.x / 3.0f;
    ImGui::SetCursorPosX(
      (windowSize.x - (buttonWidth * 2 + ImGui::GetStyle().ItemSpacing.x))
      / 2.0f);

    if (ImGui::Button("Yes", {buttonWidth, 0.0f}))
    {
      // return 21 if selected yes, this is for checking return code in bash scripts
      mExitCode = 21;
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

  if (!running && !mExitCode)
  {
    mExitCode = 0;
  }

  return mExitCode;
}


bool View::fetchScriptOutput()
{
  bool gotNewData = false;

  // Check if there is new data available from the script's output
  struct pollfd pollData{mScriptPipeFd, POLLIN, 0};
  const auto result = poll(&pollData, 1, 0);

  if (result > 0)
  {
    if (pollData.revents & POLLIN)
    {
      // Data is available.
      char bytes[1024];
      const auto bytesRead = read(mScriptPipeFd, bytes, sizeof(bytes));
      if (bytesRead == -1)
      {
        // Error reading the pipe
        throw std::runtime_error("Error read()-ing script fd");
      }

      if (bytesRead > 0)
      {
        gotNewData = true;

        // We read some output bytes, append them to our message,
        // taking word-wrapping into account as needed.
        if (const auto pText = std::get_if<std::string>(&mText))
        {
          // Word-wrapping is disabled, simply append the bytes we read to our
          // string.
          pText->insert(pText->end(), bytes, bytes + bytesRead);
        }
        else if (const auto pLines = std::get_if<std::vector<std::string>>(&mText))
        {
          // Word-wrapping is enabled, look for linebreaks and move on to
          // the next line in the list of lines when we encouter one.
          if (pLines->empty())
          {
            pLines->emplace_back(bytes, bytes + bytesRead);
          }
          else
          {
            for (auto pChar = bytes; pChar != bytes + bytesRead; ++pChar)
            {
              pLines->back().push_back(*pChar);

              if (*pChar == '\n')
              {
                pLines->emplace_back();
              }
            }
          }
        }
      }
    }

    if (pollData.revents & POLLHUP || pollData.revents & POLLERR)
    {
      // The script is done, or an error occured - close the pipe
      closeScriptPipe();
    }
  }
  else if (result < 0)
  {
    // Error polling the pipe
    throw std::runtime_error("Error poll()-ing script fd");
  }

  return gotNewData;
}


void View::closeScriptPipe()
{
    if (mpScriptPipe)
    {
        pclose(mpScriptPipe);
        mpScriptPipe = nullptr;
        mScriptPipeFd = -1;
    }
}
