#pragma once
#include <ensketch/opengl/opengl.hpp>
#include <ensketch/sandbox/defaults.hpp>
#include <ensketch/sandbox/main_thread.hpp>
//
#include <SFML/Graphics.hpp>

namespace ensketch::sandbox {

///
///
class opengl_window_state {
 protected:
  opengl_window_state(int width, int height, std::string_view title) {
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

 public:
  /// This routine basically represents the coroutine.
  ///
  void update(this auto&& self) {
    // Events must be polled in the window's thread and, especially
    // on MacOS, windows and events must be managed in the main thread.
    // See: https://www.sfml-dev.org/tutorials/2.6/window-window.php
    main_thread::invoke([&self] {
      sf::Event event;
      while (self.window.pollEvent(event))
        // The processing of specific events will still be done by the viewer's
        // thread. For this, enqueue all received events to the event queue.
        self.events.push(event);
    });

    // Now, process all enqueued events on the current thread of execution.
    // The call to `main_thread::invoke` will automatically synchronize.
    while (not self.events.empty()) {
      // Call the custom event processing for the current event and discard it.
      self.process(self.events.front());
      self.events.pop();
    }

    // Call the custom render function and swap buffers for display.
    self.render();
    self.window.display();
  }

  /// Make the window visible.
  ///
  void show() { window.setVisible(true); }

  /// Make the window invisible.
  ///
  void hide() { window.setVisible(false); }

  /// Check whether the window has the input focus.
  ///
  bool focused() { return window.hasFocus(); }

  /// Request the current window to be made the active foreground window.
  ///
  void focus() { window.requestFocus(); }

  /// Change the position of the window on screen.
  ///
  void set_position(int x, int y) { window.setPosition({x, y}); }

  /// Receive the current position of the OpenGL window on screen in pixels.
  ///
  auto position() const noexcept -> glm::ivec2 {
    const auto pos = window.getPosition();
    return {pos.x, pos.y};
  }

  /// Change the size of the rendering region of the window.
  /// Non-positive sizes are invalid and will be ignored.
  ///
  void resize(int width, int height) {
    // Non-positive values lead to error and program termination.
    if ((width <= 0) || (height <= 0)) return;
    window.setSize(
        {static_cast<unsigned int>(width), static_cast<unsigned int>(height)});
  }

  /// Change the title of the window to `str`.
  ///
  void set_title(std::string_view str) { window.setTitle(std::string(str)); }

  /// Receive the current mouse cursor position
  /// relative to the window's location.
  ///
  auto mouse_position() { return sf::Mouse::getPosition(window); }

 private:
  sf::Window window{};
  std::queue<sf::Event> events{};
};

///
///
template <typename derived>
class opengl_window_api {
 public:
  using state_type = opengl_window_state;

  constexpr decltype(auto) self() noexcept {
    return static_cast<derived&>(*this);
  }

  /// Make the window visible.
  ///
  void show() {
    return self().async_invoke_and_discard(
        [](state_type& state) { state.show(); });
  }

  /// Make the window invisible.
  ///
  void hide() {
    return self().async_invoke_and_discard(
        [](state_type& state) { state.hide(); });
  }

  /// Check whether the window has the input focus.
  ///
  bool focused() {
    return self().invoke([](state_type& state) { return state.focused(); });
  }

  /// Request the current window to be made the active foreground window.
  ///
  void focus() {
    return self().async_invoke_and_discard(
        [](state_type& state) { state.focus(); });
  }

  /// Change the position of the window on screen.
  ///
  void set_position(int x, int y) {
    self().async_invoke_and_discard(
        [x, y](state_type& state) { state.set_position(x, y); });
  }

  /// Change the size of the rendering region of the window.
  /// Non-positive sizes are invalid and will be ignored.
  ///
  void resize(int width, int height) {
    self().async_invoke_and_discard(
        [width, height](state_type& state) { state.resize(width, height); });
  }

  /// Change the title of the window to `str`.
  ///
  void set_title(std::string_view str) {
    self().async_invoke_and_discard(
        [str = std::string{str}](state_type& state) { state.set_title(str); });
  }

  /// Receive the current mouse cursor position
  /// relative to the window's location.
  ///
  auto mouse_position() -> std::pair<int, int> {
    const auto pos =
        self().invoke([](state_type& state) { return state.mouse_position(); });
    return {pos.x, pos.y};
  }
};

}  // namespace ensketch::sandbox
