#include <bitset>
#include <cstdint>
#include <print>
#include <variant>
#include <vector>
//
#include <SFML/Graphics.hpp>

struct keyboard {
  enum class key : uint8_t {
    esc,

    ctrl,
    alt,
    shift,

    left,
    right,
    up,
    down,

    space,
    enter,
    backspace,
    tab,

    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12,

    _0,
    _1,
    _2,
    _3,
    _4,
    _5,
    _6,
    _7,
    _8,
    _9,

    a,
    b,
    c,
    d,
    e,
    f,
    g,
    h,
    i,
    j,
    k,
    l,
    m,
    n,
    o,
    p,
    q,
    r,
    s,
    t,
    u,
    v,
    w,
    x,
    y,
    z,

    count
  };

  struct state : std::bitset<size_t(key::count)> {
    template <typename... keys>
      requires(std::same_as<keys, key> && ...)
    constexpr state(keys... k) noexcept {
      (operator[](size_t(k)) = true, ...);
    }

    friend constexpr auto operator<=>(const state&,
                                      const state&) noexcept = default;
  };

  template <key k>
  static constexpr auto mask = state{state{1} << size_t{k}};

  struct event {
    enum key key : 7 {};
    bool released : 1 {};

    friend constexpr auto operator<=>(const event&,
                                      const event&) noexcept = default;
  };
};

// static_assert(sizeof(keyboard::event) == 1);

struct mouse {
  enum class button : uint8_t { left, middle, right, count };

  struct state {
    int16_t x;
    int16_t y;
    std::bitset<size_t(button::count)> buttons;

    friend constexpr auto operator<=>(const state&,
                                      const state&) noexcept = default;
  };

  struct button_event {
    enum button button : 7 {};
    bool released : 1 {};
    friend constexpr auto operator<=>(const button_event&,
                                      const button_event&) noexcept = default;
  };

  struct move_event {
    int16_t x;
    int16_t y;
    friend constexpr auto operator<=>(const move_event&,
                                      const move_event&) noexcept = default;
  };

  struct wheel_event {
    int16_t delta;
    friend constexpr auto operator<=>(const wheel_event&,
                                      const wheel_event&) noexcept = default;
  };
};

struct peripheral {
  using keyboard_type = keyboard;
  using mouse_type = mouse;

  struct state {
    keyboard_type::state keyboard{};
    mouse_type::state mouse{};

    friend constexpr auto operator<=>(const state&,
                                      const state&) noexcept = default;
  };

  using event_base = std::variant<std::monostate,
                                  keyboard_type::event,
                                  mouse_type::button_event,
                                  mouse_type::move_event,
                                  mouse_type::wheel_event>;
  struct event : event_base {
    using base = event_base;
    using base::base;
  };
};

auto keyboard_key_from(sf::Keyboard::Key in) -> keyboard::key {
  switch (in) {
    case sf::Keyboard::Escape:
      return keyboard::key::esc;

    case sf::Keyboard::LControl:
    case sf::Keyboard::RControl:
      return keyboard::key::ctrl;

    case sf::Keyboard::LAlt:
    case sf::Keyboard::RAlt:
      return keyboard::key::alt;

    case sf::Keyboard::LShift:
    case sf::Keyboard::RShift:
      return keyboard::key::shift;

    case sf::Keyboard::Left:
      return keyboard::key::left;
    case sf::Keyboard::Right:
      return keyboard::key::right;
    case sf::Keyboard::Up:
      return keyboard::key::up;
    case sf::Keyboard::Down:
      return keyboard::key::down;

    case sf::Keyboard::Space:
      return keyboard::key::space;
    case sf::Keyboard::Enter:
      return keyboard::key::enter;
    case sf::Keyboard::Backspace:
      return keyboard::key::backspace;
    case sf::Keyboard::Tab:
      return keyboard::key::tab;

    case sf::Keyboard::F1:
      return keyboard::key::f1;
    case sf::Keyboard::F2:
      return keyboard::key::f2;
    case sf::Keyboard::F3:
      return keyboard::key::f3;
    case sf::Keyboard::F4:
      return keyboard::key::f4;
    case sf::Keyboard::F5:
      return keyboard::key::f5;
    case sf::Keyboard::F6:
      return keyboard::key::f6;
    case sf::Keyboard::F7:
      return keyboard::key::f7;
    case sf::Keyboard::F8:
      return keyboard::key::f8;
    case sf::Keyboard::F9:
      return keyboard::key::f9;
    case sf::Keyboard::F10:
      return keyboard::key::f10;
    case sf::Keyboard::F11:
      return keyboard::key::f11;
    case sf::Keyboard::F12:
      return keyboard::key::f12;

    case sf::Keyboard::Num1:
    case sf::Keyboard::Numpad1:
      return keyboard::key::_1;
    case sf::Keyboard::Num2:
    case sf::Keyboard::Numpad2:
      return keyboard::key::_2;
    case sf::Keyboard::Num3:
    case sf::Keyboard::Numpad3:
      return keyboard::key::_3;
    case sf::Keyboard::Num4:
    case sf::Keyboard::Numpad4:
      return keyboard::key::_4;
    case sf::Keyboard::Num5:
    case sf::Keyboard::Numpad5:
      return keyboard::key::_5;
    case sf::Keyboard::Num6:
    case sf::Keyboard::Numpad6:
      return keyboard::key::_6;
    case sf::Keyboard::Num7:
    case sf::Keyboard::Numpad7:
      return keyboard::key::_7;
    case sf::Keyboard::Num8:
    case sf::Keyboard::Numpad8:
      return keyboard::key::_8;
    case sf::Keyboard::Num9:
    case sf::Keyboard::Numpad9:
      return keyboard::key::_9;
    case sf::Keyboard::Num0:
    case sf::Keyboard::Numpad0:
      return keyboard::key::_0;

    case sf::Keyboard::A:
      return keyboard::key::a;
    case sf::Keyboard::B:
      return keyboard::key::b;
    case sf::Keyboard::C:
      return keyboard::key::c;
    case sf::Keyboard::D:
      return keyboard::key::d;
    case sf::Keyboard::E:
      return keyboard::key::e;
    case sf::Keyboard::F:
      return keyboard::key::f;
    case sf::Keyboard::G:
      return keyboard::key::g;
    case sf::Keyboard::H:
      return keyboard::key::h;
    case sf::Keyboard::I:
      return keyboard::key::i;
    case sf::Keyboard::J:
      return keyboard::key::j;
    case sf::Keyboard::K:
      return keyboard::key::k;
    case sf::Keyboard::L:
      return keyboard::key::l;
    case sf::Keyboard::M:
      return keyboard::key::m;
    case sf::Keyboard::N:
      return keyboard::key::n;
    case sf::Keyboard::O:
      return keyboard::key::o;
    case sf::Keyboard::P:
      return keyboard::key::p;
    case sf::Keyboard::Q:
      return keyboard::key::q;
    case sf::Keyboard::R:
      return keyboard::key::r;
    case sf::Keyboard::S:
      return keyboard::key::s;
    case sf::Keyboard::T:
      return keyboard::key::t;
    case sf::Keyboard::U:
      return keyboard::key::u;
    case sf::Keyboard::V:
      return keyboard::key::v;
    case sf::Keyboard::W:
      return keyboard::key::w;
    case sf::Keyboard::X:
      return keyboard::key::x;
    case sf::Keyboard::Y:
      return keyboard::key::y;
    case sf::Keyboard::Z:
      return keyboard::key::z;

    default:
      return keyboard::key::count;
  }
}

auto keyboard_key_from(sf::Keyboard::Scan::Scancode in) -> keyboard::key {
  switch (in) {
    case sf::Keyboard::Scan::Escape:
      return keyboard::key::esc;

    case sf::Keyboard::Scan::LControl:
    case sf::Keyboard::Scan::RControl:
      return keyboard::key::ctrl;

    case sf::Keyboard::Scan::LAlt:
    case sf::Keyboard::Scan::RAlt:
      return keyboard::key::alt;

    case sf::Keyboard::Scan::LShift:
    case sf::Keyboard::Scan::RShift:
      return keyboard::key::shift;

    case sf::Keyboard::Scan::Left:
      return keyboard::key::left;
    case sf::Keyboard::Scan::Right:
      return keyboard::key::right;
    case sf::Keyboard::Scan::Up:
      return keyboard::key::up;
    case sf::Keyboard::Scan::Down:
      return keyboard::key::down;

    case sf::Keyboard::Scan::Space:
      return keyboard::key::space;
    case sf::Keyboard::Scan::Enter:
      return keyboard::key::enter;
    case sf::Keyboard::Scan::Backspace:
      return keyboard::key::backspace;
    case sf::Keyboard::Scan::Tab:
      return keyboard::key::tab;

    case sf::Keyboard::Scan::F1:
      return keyboard::key::f1;
    case sf::Keyboard::Scan::F2:
      return keyboard::key::f2;
    case sf::Keyboard::Scan::F3:
      return keyboard::key::f3;
    case sf::Keyboard::Scan::F4:
      return keyboard::key::f4;
    case sf::Keyboard::Scan::F5:
      return keyboard::key::f5;
    case sf::Keyboard::Scan::F6:
      return keyboard::key::f6;
    case sf::Keyboard::Scan::F7:
      return keyboard::key::f7;
    case sf::Keyboard::Scan::F8:
      return keyboard::key::f8;
    case sf::Keyboard::Scan::F9:
      return keyboard::key::f9;
    case sf::Keyboard::Scan::F10:
      return keyboard::key::f10;
    case sf::Keyboard::Scan::F11:
      return keyboard::key::f11;
    case sf::Keyboard::Scan::F12:
      return keyboard::key::f12;

    case sf::Keyboard::Scan::Num1:
    case sf::Keyboard::Scan::Numpad1:
      return keyboard::key::_1;
    case sf::Keyboard::Scan::Num2:
    case sf::Keyboard::Scan::Numpad2:
      return keyboard::key::_2;
    case sf::Keyboard::Scan::Num3:
    case sf::Keyboard::Scan::Numpad3:
      return keyboard::key::_3;
    case sf::Keyboard::Scan::Num4:
    case sf::Keyboard::Scan::Numpad4:
      return keyboard::key::_4;
    case sf::Keyboard::Scan::Num5:
    case sf::Keyboard::Scan::Numpad5:
      return keyboard::key::_5;
    case sf::Keyboard::Scan::Num6:
    case sf::Keyboard::Scan::Numpad6:
      return keyboard::key::_6;
    case sf::Keyboard::Scan::Num7:
    case sf::Keyboard::Scan::Numpad7:
      return keyboard::key::_7;
    case sf::Keyboard::Scan::Num8:
    case sf::Keyboard::Scan::Numpad8:
      return keyboard::key::_8;
    case sf::Keyboard::Scan::Num9:
    case sf::Keyboard::Scan::Numpad9:
      return keyboard::key::_9;
    case sf::Keyboard::Scan::Num0:
    case sf::Keyboard::Scan::Numpad0:
      return keyboard::key::_0;

    case sf::Keyboard::Scan::A:
      return keyboard::key::a;
    case sf::Keyboard::Scan::B:
      return keyboard::key::b;
    case sf::Keyboard::Scan::C:
      return keyboard::key::c;
    case sf::Keyboard::Scan::D:
      return keyboard::key::d;
    case sf::Keyboard::Scan::E:
      return keyboard::key::e;
    case sf::Keyboard::Scan::F:
      return keyboard::key::f;
    case sf::Keyboard::Scan::G:
      return keyboard::key::g;
    case sf::Keyboard::Scan::H:
      return keyboard::key::h;
    case sf::Keyboard::Scan::I:
      return keyboard::key::i;
    case sf::Keyboard::Scan::J:
      return keyboard::key::j;
    case sf::Keyboard::Scan::K:
      return keyboard::key::k;
    case sf::Keyboard::Scan::L:
      return keyboard::key::l;
    case sf::Keyboard::Scan::M:
      return keyboard::key::m;
    case sf::Keyboard::Scan::N:
      return keyboard::key::n;
    case sf::Keyboard::Scan::O:
      return keyboard::key::o;
    case sf::Keyboard::Scan::P:
      return keyboard::key::p;
    case sf::Keyboard::Scan::Q:
      return keyboard::key::q;
    case sf::Keyboard::Scan::R:
      return keyboard::key::r;
    case sf::Keyboard::Scan::S:
      return keyboard::key::s;
    case sf::Keyboard::Scan::T:
      return keyboard::key::t;
    case sf::Keyboard::Scan::U:
      return keyboard::key::u;
    case sf::Keyboard::Scan::V:
      return keyboard::key::v;
    case sf::Keyboard::Scan::W:
      return keyboard::key::w;
    case sf::Keyboard::Scan::X:
      return keyboard::key::x;
    case sf::Keyboard::Scan::Y:
      return keyboard::key::y;
    case sf::Keyboard::Scan::Z:
      return keyboard::key::z;

    default:
      return keyboard::key::count;
  }
}

auto peripheral_event_from(const sf::Event& in) -> peripheral::event {
  peripheral::event out{};

  switch (in.type) {
    case sf::Event::KeyPressed:
      out.emplace<keyboard::event>(keyboard_key_from(in.key.code), false);
      break;
    case sf::Event::KeyReleased:
      out.emplace<keyboard::event>(keyboard_key_from(in.key.code), true);
      break;
    default:
      return out;
  }

  return out;
}

void update(peripheral::state& state, keyboard::event event) {
  state.keyboard[size_t(event.key)] = !event.released;
}

void update(peripheral::state& state, mouse::button_event event) {
  state.mouse.buttons[size_t(event.button)] = !event.released;
}

void update(peripheral::state& state, mouse::move_event event) {
  state.mouse.x += event.x;
  state.mouse.y += event.y;
}

void update(peripheral::state& state, mouse::wheel_event event) {
  // does not need to update the state
}

void update(peripheral::state& state, std::monostate event) {
  // does not need to update the state
}

void update(peripheral::state& state, const peripheral::event& event) {
  visit([&state](auto e) { update(state, e); }, event);
}

void update(keyboard::state& state) {
  state[size_t(keyboard::key::esc)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Escape);
  //
  state[size_t(keyboard::key::ctrl)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
  state[size_t(keyboard::key::alt)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt);
  state[size_t(keyboard::key::shift)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
  //
  state[size_t(keyboard::key::left)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
  state[size_t(keyboard::key::right)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
  state[size_t(keyboard::key::up)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
  state[size_t(keyboard::key::down)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
  //
  state[size_t(keyboard::key::space)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
  state[size_t(keyboard::key::enter)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Enter);
  state[size_t(keyboard::key::backspace)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Backspace);
  state[size_t(keyboard::key::tab)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Tab);
  //
  state[size_t(keyboard::key::f1)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::F1);
  state[size_t(keyboard::key::f2)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::F2);
  state[size_t(keyboard::key::f3)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::F3);
  state[size_t(keyboard::key::f4)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::F4);
  state[size_t(keyboard::key::f5)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::F5);
  state[size_t(keyboard::key::f6)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::F6);
  state[size_t(keyboard::key::f7)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::F7);
  state[size_t(keyboard::key::f8)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::F8);
  state[size_t(keyboard::key::f9)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::F9);
  state[size_t(keyboard::key::f10)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::F10);
  state[size_t(keyboard::key::f11)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::F11);
  state[size_t(keyboard::key::f12)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::F12);
  //
  state[size_t(keyboard::key::_0)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Num0) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad0);
  state[size_t(keyboard::key::_1)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Num1) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad1);
  state[size_t(keyboard::key::_2)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Num2) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad2);
  state[size_t(keyboard::key::_3)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Num3) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad3);
  state[size_t(keyboard::key::_4)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Num4) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad4);
  state[size_t(keyboard::key::_5)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Num5) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad5);
  state[size_t(keyboard::key::_6)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Num6) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad6);
  state[size_t(keyboard::key::_7)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Num7) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad7);
  state[size_t(keyboard::key::_8)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Num8) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad8);
  state[size_t(keyboard::key::_9)] =
      sf::Keyboard::isKeyPressed(sf::Keyboard::Num9) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad9);
  //
  state[size_t(keyboard::key::a)] = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
  state[size_t(keyboard::key::b)] = sf::Keyboard::isKeyPressed(sf::Keyboard::B);
  state[size_t(keyboard::key::c)] = sf::Keyboard::isKeyPressed(sf::Keyboard::C);
  state[size_t(keyboard::key::d)] = sf::Keyboard::isKeyPressed(sf::Keyboard::D);
  state[size_t(keyboard::key::e)] = sf::Keyboard::isKeyPressed(sf::Keyboard::E);
  state[size_t(keyboard::key::f)] = sf::Keyboard::isKeyPressed(sf::Keyboard::F);
  state[size_t(keyboard::key::g)] = sf::Keyboard::isKeyPressed(sf::Keyboard::G);
  state[size_t(keyboard::key::h)] = sf::Keyboard::isKeyPressed(sf::Keyboard::H);
  state[size_t(keyboard::key::i)] = sf::Keyboard::isKeyPressed(sf::Keyboard::I);
  state[size_t(keyboard::key::j)] = sf::Keyboard::isKeyPressed(sf::Keyboard::J);
  state[size_t(keyboard::key::k)] = sf::Keyboard::isKeyPressed(sf::Keyboard::K);
  state[size_t(keyboard::key::l)] = sf::Keyboard::isKeyPressed(sf::Keyboard::L);
  state[size_t(keyboard::key::m)] = sf::Keyboard::isKeyPressed(sf::Keyboard::M);
  state[size_t(keyboard::key::n)] = sf::Keyboard::isKeyPressed(sf::Keyboard::N);
  state[size_t(keyboard::key::o)] = sf::Keyboard::isKeyPressed(sf::Keyboard::O);
  state[size_t(keyboard::key::p)] = sf::Keyboard::isKeyPressed(sf::Keyboard::P);
  state[size_t(keyboard::key::q)] = sf::Keyboard::isKeyPressed(sf::Keyboard::Q);
  state[size_t(keyboard::key::r)] = sf::Keyboard::isKeyPressed(sf::Keyboard::R);
  state[size_t(keyboard::key::s)] = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
  state[size_t(keyboard::key::t)] = sf::Keyboard::isKeyPressed(sf::Keyboard::T);
  state[size_t(keyboard::key::u)] = sf::Keyboard::isKeyPressed(sf::Keyboard::U);
  state[size_t(keyboard::key::v)] = sf::Keyboard::isKeyPressed(sf::Keyboard::V);
  state[size_t(keyboard::key::w)] = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
  state[size_t(keyboard::key::x)] = sf::Keyboard::isKeyPressed(sf::Keyboard::X);
  state[size_t(keyboard::key::y)] = sf::Keyboard::isKeyPressed(sf::Keyboard::Y);
  state[size_t(keyboard::key::z)] = sf::Keyboard::isKeyPressed(sf::Keyboard::Z);
}

void update(peripheral::state& state) {
  update(state.keyboard);
}

struct key_binding {
  bool triggered() const noexcept { return true; }
  bool activated() const noexcept {}

  keyboard::event event{};
  keyboard::state mods{};
};

struct binding {
  peripheral::event event{};
  peripheral::state state{};
};

std::vector<std::pair<std::string, binding>> bindings{
    {"quit by esc", {keyboard::event{keyboard::key::esc, true}, {}}},
    {"copy",
     {keyboard::event{keyboard::key::c},
      {{keyboard::key::ctrl, keyboard::key::c}}}},
    {"paste",
     {keyboard::event{keyboard::key::v},
      {{keyboard::key::ctrl, keyboard::key::v}}}},
    // {"activate", {mouse::button_event{mouse::button::left}, {{},{}}}},
};

int main() {
  peripheral::state input_state{};

  sf::RenderWindow window(sf::VideoMode(200, 200), "SFML works!");
  window.setKeyRepeatEnabled(false);
  sf::CircleShape shape(100.f);
  shape.setFillColor(sf::Color::Green);

  while (window.isOpen()) {
    // update(input_state);

    sf::Event event;
    while (window.pollEvent(event)) {
      switch (event.type) {
        case sf::Event::Closed:
          std::println("closed event");
          break;

        case sf::Event::Resized:
          std::println("resized event");
          break;

        case sf::Event::LostFocus:
          std::println("LostFocus event");
          break;
        case sf::Event::GainedFocus:
          std::println("GainedFocus event");
          break;

        case sf::Event::TextEntered:
          std::println("TextEntered event");
          break;

        case sf::Event::KeyPressed:
          std::println("KeyPressed event");
          break;
        case sf::Event::KeyReleased:
          std::println("KeyReleased event");
          break;

        case sf::Event::MouseWheelScrolled:
          std::println("MouseWheelScrolled event");
          break;

        case sf::Event::MouseButtonPressed:
          std::println("MouseButtonPressed event");
          break;
        case sf::Event::MouseButtonReleased:
          std::println("MouseButtonReleased event");
          break;

        case sf::Event::MouseMoved:
          std::println("MouseMovedEvent event");
          break;

        case sf::Event::MouseEntered:
          std::println("MouseEntered event");
          break;
        case sf::Event::MouseLeft:
          std::println("MouseLeft event");
          break;

        case sf::Event::JoystickButtonPressed:
          std::println("JoystickButtonPressed event");
          break;
        case sf::Event::JoystickButtonReleased:
          std::println("JoystickButtonReleased event");
          break;
        case sf::Event::JoystickMoved:
          std::println("JoystickMoved event");
          break;
        case sf::Event::JoystickConnected:
          std::println("JoystickConnected event");
          break;
        case sf::Event::JoystickDisconnected:
          std::println("JoystickDisconnected event");
          break;

        default:
          std::println("unknown event");
      }

      if (event.type == sf::Event::Closed) window.close();
      // if (event.type)
      // if (event.type == sf::Event::KeyPressed) {
      //   if (event.key.code == sf::Keyboard::X) std::println("x pressed");
      //   std::println("{}", keys.to_string());
      //   std::println(
      //       "{:>3}:{:>3} {}", int(event.key.code),
      //       int(event.key.scancode),
      //       std::string{sf::Keyboard::getDescription(event.key.scancode)});
      // }
      const auto e = peripheral_event_from(event);
      update(input_state, e);
      // std::println("{}", input_state.keyboard.to_string());

      for (const auto& [name, binding] : bindings) {
        if (binding.event != e) continue;
        if (binding.state != input_state) continue;
        std::println("{}", name);
      }
    }

    window.clear();
    window.draw(shape);
    window.display();
  }
}

// #include <ensketch/xstd/xstd.hpp>
// // #include "luarepl/luarepl.hpp"

// using namespace ensketch::xstd;

// template <typename type>
// concept signal_parameter = std::copyable<type>;

// template <meta::string str, typename... args>
// struct signal {
//   static consteval auto name() noexcept { return str; }
//   static consteval auto arguments() noexcept {
//     return meta::type_list<args...>{};
//   }
// };

// template <typename type>
// concept generic_signal = requires {
//   { type::name() } -> meta::string_instance;
//   { type::arguments() } -> meta::type_list_instance;
// };

// template <typename type, typename signal>
// concept slot_for =
//     generic_signal<signal> && fold(signal::arguments(), []<typename... args>
//     {
//       return std::invocable<type, args...>;
//     });

// template <generic_signal signal, slot_for<signal>... types>
// struct signal_entry : signal, std::tuple<types...> {
//   using base = std::tuple<types...>;
//   using signal_type = signal;
//   constexpr signal_entry(auto&&... args)
//       : base{std::forward<decltype(args)>(args)...} {}
//   constexpr auto slots() & noexcept { return static_cast<base&>(*this); }
//   constexpr auto slots() const& noexcept {
//     return static_cast<const base&>(*this);
//   }
//   constexpr auto slots() && noexcept { return static_cast<base&&>(*this); }
//   constexpr auto slots() const&& noexcept {
//     return static_cast<const base&&>(*this);
//   }
// };

// template <generic_signal signal>
// constexpr auto entry(slot_for<signal> auto&&... slots) {
//   return signal_entry<signal, std::unwrap_ref_decay_t<decltype(slots)>...>{
//       std::forward<decltype(slots)>(slots)...};
// }

// constexpr auto table(auto&&... entries) {
//   return named_tuple<
//       meta::name_list<std::decay_t<decltype(entries)>::name()...>,
//       std::tuple<std::unwrap_ref_decay_t<decltype(entries)>...>>{
//       std::forward<decltype(entries)>(entries)...};
// }

// template <meta::string name>
// constexpr auto sync_signal(auto&& signals, auto... args) {
//   for_each(value<name>(signals).slots(),
//            [&](auto slot) { std::invoke(slot, args...); });
// }

// template <meta::string name>
// constexpr auto async_signal(task_queue& queue, auto&& signals, auto... args)
// {
//   for_each(value<name>(signals).slots(), [&](auto slot) {
//     queue.push_and_discard([slot, args...] { std::invoke(slot, args...); });
//   });
// }

// template <meta::string str, typename... params>
// struct dynamic_signal_entry : std::list<std::function<void(params...)>> {
//   using base = std::list<std::function<void(params...)>>;
//   static consteval auto name() noexcept { return str; }
//   static consteval auto parameters() noexcept {
//     return meta::type_list<params...>{};
//   }

//   constexpr auto slots() & noexcept { return static_cast<base&>(*this); }
//   constexpr auto slots() const& noexcept {
//     return static_cast<const base&>(*this);
//   }
//   constexpr auto slots() && noexcept { return static_cast<base&&>(*this); }
//   constexpr auto slots() const&& noexcept {
//     return static_cast<const base&&>(*this);
//   }

//   // static constexpr void invoke(std::invocable<params...> auto&& f,
//   //                              params... args) {
//   //   std::invoke(std::forward<decltype(f)>(f), args...);
//   // }

//   // void operator()(params... args) const {
//   //   std::ranges::for_each(*this, [&](auto& slot) {});
//   // }
// };

// template <meta::string name, typename... params>
// constexpr auto emit(dynamic_signal_entry<name, params...>& signal,
//                     task_queue& queue,
//                     params... args) {
//   std::ranges::for_each(signal.slots(), [&](auto& slot) {
//     queue.push_and_discard([&] { std::invoke(slot, args...); });
//   });
// }

// template <meta::string name>
// constexpr auto emit_signal(auto& signals, task_queue& queue, auto... args) {
//   emit(value<name>(signals), queue, args...);
// }

// auto string_from(std::source_location location) -> std::string {
//   return std::format(
//       "{}:{}:{}:{}",
//       relative(std::filesystem::path(location.file_name())).string(),
//       location.line(), location.column(), location.function_name());
// }

// struct debug_mutex_timeout : std::runtime_error {
//   using base = std::runtime_error;
//   debug_mutex_timeout(std::source_location owner, std::source_location
//   attempt)
//       : base{std::format(
//             "timeout during attempt to lock mutex at {} previously owned at
//             {}", string_from(attempt), string_from(owner))} {}
// };

// template <auto timeout_ms>
// struct debug_mutex {
//   void lock(std::source_location location = std::source_location::current())
//   {
//     if (!mutex.try_lock_for(std::chrono::milliseconds{timeout_ms}))
//       throw debug_mutex_timeout{source, location};
//     source = location;
//   }

//   bool try_lock() { return mutex.try_lock(); }

//   template <typename rep, typename period>
//   bool try_lock_for(
//       const std::chrono::duration<rep, period>& timeout_duration) {
//     return mutex.try_lock_for(timeout_duration);
//   }

//   template <typename clock, typename duration>
//   bool try_lock_until(
//       const std::chrono::time_point<clock, duration>& timeout_time) {
//     return mutex.try_lock_until(timeout_time);
//   }

//   void unlock() { mutex.unlock(); }

//   std::timed_mutex mutex{};
//   std::source_location source{};
// };

// template <typename Mutex>
// struct debug_lock {
//   using mutex_type = Mutex;
//   mutex_type& mutex;
//   explicit debug_lock(
//       mutex_type& m,
//       std::source_location location = std::source_location::current())
//       : mutex{m} {
//     mutex.lock(location);
//   }
//   debug_lock(const debug_lock&) = delete;
//   ~debug_lock() { mutex.unlock(); }
// };

// // std::mutex mutex1;
// // std::mutex mutex2;
// using mutex_type = debug_mutex<2000>;
// mutex_type mutex1;
// mutex_type mutex2;
// int main() {
//   // auto p1 = async([] {
//   //   debug_lock lock1{mutex1};
//   //   std::this_thread::sleep_for(1s);
//   //   debug_lock lock2{mutex2};
//   // });
//   // auto p2 = async([] {
//   //   debug_lock lock2{mutex2};
//   //   std::this_thread::sleep_for(1s);
//   //   debug_lock lock1{mutex1};
//   // });
//   // p1.get();
//   // p2.get();

//   // task_queue tasks{};

//   // dynamic_signal_entry<"number", int> number_signal{};

//   // auto signals = table(dynamic_signal_entry<"quit">{},
//   //                      dynamic_signal_entry<"number", int>{});

//   // value<"quit">(signals).push_back([] { std::println("First quit slot.");
//   });
//   // value<"quit">(signals).push_back([] { std::println("Second quit slot.");
//   });

//   // emit_signal<"quit">(signals, tasks);

//   // value<"quit">(signals).pop_back();

//   // emit_signal<"quit">(signals, tasks);

//   // number_signal.emplace_back(
//   //     [](int x) { std::println("First number slot. x = {}", x); });
//   // number_signal.emplace_back(
//   //     [](int x) { std::println("Second number slot. x = {}", x); });

//   // emit(number_signal, tasks, 10);
//   // emit_signal<"number">(signals, tasks, 13);

//   // auto signals =
//   //     table(entry<signal<"quit">>([] { std::println("Quit the
//   application."); },
//   //                                 [] { std::println("Second quit slot");
//   }),
//   //           entry<signal<"number", int>>(
//   //               [](int x) { std::println("first number slot x = {}.", x);
//   },
//   //               [](int x) { std::println("Second number slot x = {}.", x);
//   }));

//   // async_signal<"quit">(tasks, signals);
//   // async_signal<"number">(tasks, signals, 10);
//   // async_signal<"number">(tasks, signals, 13);

//   // auto p1 = async([&] { tasks.process_all(); });
//   // auto p2 = async([&] { tasks.process_all(); });
//   // tasks.process_all();

//   // using namespace ensketch::xstd;
//   // namespace luarepl = ensketch::luarepl;
//   // std::atomic<bool> done{false};
//   // luarepl::set_done_and_quit([&done] { return done.load(); },
//   //                            [&done] { done = true; });
//   // luarepl::set_function("quit", [] { luarepl::quit(); });
//   // luarepl::set_function("print", [](czstring str) { luarepl::log(str); });
//   // auto repl_task = async(luarepl::run);
// }
