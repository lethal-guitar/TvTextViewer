# TvTextViewer

This is a small utility for showing text files in a full-screen window with gamepad controls.
It's meant for showing log files, instructions, error messages etc. on devices like TV boxes or gaming handhelds
which typically don't have a keyboard/mouse.

Using OpenGL ES 2.0, SDL, and the excellent [Dear ImGui library](https://github.com/ocornut/imgui).
Target platform is Linux only for now.

## Building

This project uses C++ 17 and needs at least gcc 8.

First, install SDL2 if you haven't already. It's available in the package managers of most Linux distros.
On Ubuntu/Debian/Mint etc. you can grab it by running `sudo apt-get install libsdl2-dev`, for example.

Next, you need an OpenGL ES 2.0 implementation (`libGLESv2.so`).
On ARM-based devices, this is typically included with the system.
On Desktop/x86, you can use the Mesa GLES library: `sudo apt-get install libgles2-mesa-dev`.

For all remaining dependencies, this project uses git submodules.
Before you can build, these submodules need to be initialized.
You can either use `git clone --recursive` when cloning the repo, or run:

```bash
git submodule update --init --recursive
```

Once everything is installed and submodules are initialized,
you can build using the supplied `Makefile` by running `make` in the repository root.

## Usage

Basic usage is:

```
text_viewer <file>
```

With `<file>` being a text file you'd like to show.

You can also customize various options like font size, window title etc.
Run `text_viewer --help` to learn more.

## Controls

You can scroll up and down using the analog sticks or d-pad.
Holding RB while scrolling makes it faster, LB makes it slower.

To quit, press button B to unfocus the text display.
You can now use the d-pad to toggle between the close button and the text.
Press button A once the close button is selected to quit.
When having the text selected instead,
button A will enter scrolling mode again.

