#pragma once
#include <ensketch/opengl/opengl.hpp>
#include <ensketch/sandbox/defaults.hpp>
#include <ensketch/sandbox/main_thread.hpp>
//
#include <SFML/Graphics.hpp>

namespace ensketch::sandbox {

/// OpenGL comes with several important considerations:
///
/// - An OpenGL context must be created and activated on the thread that will
///   issue OpenGL commands.
/// - Issuing OpenGL commands without an active context or without creating one
///   for the current thread leads to errors and may terminate the application.
/// - Since the graphics driver batches OpenGL commands for processing on the
///   GPU, sharing a single OpenGL context across multiple threads is generally
///   discouraged. Instead, either use multiple contexts on separate threads or
///   restrict the context to a dedicated thread.
///
/// See: https://www.khronos.org/opengl/wiki/OpenGL_and_multithreading
///
/// We require an OpenGL context to run on a dedicated thread to avoid context
/// switching and ensuring the context is used exclusively on that thread.
/// However, we still want to allow issuing OpenGL commands from other threads,
/// with those commands being executed on the dedicated OpenGL thread.
/// This approach avoids the need for manually activating the OpenGL context
/// on specific threads, and, with the use of RAII, guarantees that an OpenGL
/// context is active when commands are issued.
///
/// Further Requirements:
/// - Automatic creation and asynchronous execution of the OpenGL loop.
///   => Use multi-threading with `std::jthread`.
/// - Ensure each function is executed on the dedicated OpenGL thread.
///   => Use a task queue that is processed exclusively within the OpenGL loop.
/// - Ensure a reasonable level of exception safety.
///   => Use RAII for automatic cleanup of OpenGL state.
///   => Catch any thrown exceptions and store them using a future-promise pair.
/// - Support state and API specialization through inheritance and static
///   polymorphism to allow the construction of various viewers.
///   => Maintain separation between state management and API logic.
/// - Provide a mechanism for stopping the OpenGL loop externally using stop
///   sources.
///
/// `opengl_window_state` is the base type for any OpenGL application state.
/// It is meant to be constructed and executed on its own dedicated thread to
/// ensure the availability of a valid OpenGL context for all the following
/// OpenGL commands. Throwing exceptions or manually quitting the thread of
/// execution will, according to RAII, call the state's destructor and properly
/// clean up.
///
class opengl_window_state {
  /// State Data
  ///
  /// OpenGL Window and Context
  sf::Window window{};
  /// Queue to asynchronously receive events and process them.
  std::queue<sf::Event> events{};

 protected:
  /// This state type is not meant to executed by itself but must first be used
  /// as a base class to add the required the functionality and then be executed
  /// by a thread executor. Consequently, the constructor is protected.
  ///
  opengl_window_state(int width, int height, std::string_view title);

 public:
  /// This is the inner update function that must be executed
  /// in a loop to keep the OpenGL window and context running.
  ///
  void update(this auto&& self);

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
};

/// `opengl_window_api` describes the respective thread-safe API for
/// `opengl_window_state` that may be called from any other thread.
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

#include <ensketch/sandbox/opengl_window.ipp>
