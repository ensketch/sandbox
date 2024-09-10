namespace ensketch::luarepl {

/// Global Variables
/// These are needed here to allow for use inside function templates.
///
extern std::function<bool()> done;
extern std::function<void()> quit;
extern std::mutex mutex;
extern sol::state lua;

void set_done_and_quit(auto&& is_done, auto&& shall_quit) {
  done = std::forward<decltype(is_done)>(is_done);
  quit = std::forward<decltype(shall_quit)>(shall_quit);
}

void set_function(std::string_view name, auto&& f) {
  std::scoped_lock lock{mutex};
  lua.set_function(name, std::forward<decltype(f)>(f));
}

}  // namespace ensketch::luarepl
