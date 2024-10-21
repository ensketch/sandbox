namespace ensketch::sandbox {

void opengl_window_state::update(this auto&& self) {
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

}  // namespace ensketch::sandbox
