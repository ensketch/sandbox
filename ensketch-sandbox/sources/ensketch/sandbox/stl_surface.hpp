#pragma once
#include <ensketch/sandbox/utility.hpp>

namespace ensketch::sandbox {

struct stl_surface {
  using header = array<uint8, 80>;
  using size_type = uint32;
  using attribute_byte_count_type = uint16;

  struct alignas(1) triangle {
    vec3 normal{};
    vec3 vertex[3]{};
  };

  //
  //
  struct parser_error : runtime_error {
    using runtime_error::runtime_error;
  };

  // We need tag dispatching to allow for multiple constructors
  // based on the file type.
  //
  struct ascii_tag {};
  struct binary_tag {};
  static constexpr ascii_tag ascii{};
  static constexpr binary_tag binary{};

  // The whole structure is meant as a typed cache with structure
  // for an underlying file.
  // So, no constructor extensions are used but only good old
  // standard constructors.
  //
  stl_surface(const filesystem::path& path);
  stl_surface(const filesystem::path& path, ascii_tag);
  stl_surface(const filesystem::path& path, binary_tag);

  void load_from_ascii_file(const filesystem::path& path);
  void load_from_binary_file(const filesystem::path& path);

  vector<triangle> triangles{};
};

}  // namespace ensketch::sandbox
