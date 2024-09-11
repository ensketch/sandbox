namespace ensketch::luarepl {

template <typename rep, typename period>
constexpr auto animation(auto spinner,
                         std::chrono::duration<rep, period> current) {
  const auto index =
      (spinner.size() * current / spinner.total_time()) % spinner.size();
  return spinner[index];
}

struct dot_spinner_t {
  using size_type = std::size_t;

  // See: https://github.com/sindresorhus/cli-spinners/blob/main/spinners.json
  // MIT License
  static constexpr czstring states[] = {
      "  ", "⢀⠀", "⡀⠀", "⠄⠀", "⢂⠀", "⡂⠀", "⠅⠀", "⢃⠀", "⡃⠀", "⠍⠀", "⢋⠀", "⡋⠀",
      "⠍⠁", "⢋⠁", "⡋⠁", "⠍⠉", "⠋⠉", "⠋⠉", "⠉⠙", "⠉⠙", "⠉⠩", "⠈⢙", "⠈⡙", "⢈⠩",
      "⡀⢙", "⠄⡙", "⢂⠩", "⡂⢘", "⠅⡘", "⢃⠨", "⡃⢐", "⠍⡐", "⢋⠠", "⡋⢀", "⠍⡁", "⢋⠁",
      "⡋⠁", "⠍⠉", "⠋⠉", "⠋⠉", "⠉⠙", "⠉⠙", "⠉⠩", "⠈⢙", "⠈⡙", "⠈⠩", "⠀⢙", "⠀⡙",
      "⠀⠩", "⠀⢘", "⠀⡘", "⠀⠨", "⠀⢐", "⠀⡐", "⠀⠠", "⠀⢀", "⠀⡀",
  };

  static constexpr auto size() noexcept -> std::size_t {
    return sizeof(states) / sizeof(czstring);
  }

  static constexpr auto total_time() noexcept { return 3s; }

  static constexpr auto operator[](size_type index) noexcept {
    return states[index];
  }
};

inline constexpr dot_spinner_t dot_spinner{};

}  // namespace ensketch::luarepl
