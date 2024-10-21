#include <ensketch/sandbox/opengl_window.hpp>

namespace ensketch::sandbox {

opengl_window_state::opengl_window_state(int width,
                                         int height,
                                         std::string_view title) {
  // On MacOS, windows and events must be managed in the main thread.
  // See: https://www.sfml-dev.org/tutorials/2.6/window-window.php
  main_thread::invoke([this, width, height, title] {
    // For now, always try to acquire the latest OpenGL version.
    // In the future, the debug should be disabled when `NDEBUG` is enabled.
    sf::ContextSettings opengl_context_settings{
        /*.depthBits = */ 24,
        /*.stencilBits = */ 8,
        /*.antialiasingLevel = */ 4,
        /*.majorVersion = */ 4,
        /*.minorVersion = */ 6,
        /*.attributeFlags = */
        sf::ContextSettings::Core | sf::ContextSettings::Debug,
        /*.sRgbCapable = */ false};

    // Create a standard window with border and default interaction buttons.
    // This class is not meant to provide fullscreen windows.
    window.create(sf::VideoMode(width, height), std::string(title),  //
                  sf::Style::Default, opengl_context_settings);

    // "A window is active only on the current thread,
    // if you want to make it active on another thread
    // you have to deactivate it on the previous thread
    // first if it was active."
    // See: https://www.sfml-dev.org/documentation/2.6.1/classsf_1_1Window.php
    window.setActive(false);
  });

  // Activate the window as the current target for OpenGL rendering.
  // Make the window's OpenGL context available on the current thread.
  window.setActive(true);
  // Initialize all OpenGL functions for the current thread and context.
  glbinding::initialize(sf::Context::getFunction);

  // Initialize the window's default settings.
  window.setVerticalSyncEnabled(true);
  window.setKeyRepeatEnabled(false);
}

}  // namespace ensketch::sandbox
