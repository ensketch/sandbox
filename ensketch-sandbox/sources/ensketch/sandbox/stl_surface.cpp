#include <ensketch/sandbox/stl_surface.hpp>

namespace ensketch::sandbox {

void stl_surface::load_from_binary_file(const filesystem::path& path) {
  // Provide some static assertions that make sure the loading works properly.
  static_assert(offsetof(triangle, normal) == 0);
  static_assert(offsetof(triangle, vertex[0]) == 12);
  static_assert(offsetof(triangle, vertex[1]) == 24);
  static_assert(offsetof(triangle, vertex[2]) == 36);
  static_assert(sizeof(triangle) == 48);
  static_assert(alignof(triangle) == 4);

  fstream file{path, ios::in | ios::binary};
  if (!file.is_open())
    throw runtime_error(
        format("Failed to open STL file from path '{}'.", path.string()));

  // We will ignore the header.
  // It has no specific use to us.
  file.ignore(sizeof(header));

  // Read number of triangles.
  size_type size;
  file.read((char*)&size, sizeof(size));

  triangles.resize(size);

  // Due to padding and alignment issues concerning 'float32' and 'uint16',
  // we cannot read everything at once.
  // Instead, we use a simple loop for every triangle.
  for (auto& t : triangles) {
    file.read((char*)&t, sizeof(triangle));
    // Ignore the attribute byte count.
    // There should not be any information anyway.
    file.ignore(sizeof(attribute_byte_count_type));
  }
}

void stl_surface::load_from_ascii_file(const filesystem::path& path) {
  fstream file{path, ios::in | ios::binary};
  if (!file.is_open())
    throw runtime_error("Failed to open STL file from path '"s + path.string() +
                        "'.");

  string name;
  string line;
  getline(file, line);
  stringstream input{line};
  string keyword;
  input >> keyword;

  const auto match = [&](czstring str) {
    file >> keyword;
    if (keyword == str) return;
    throw parser_error("Failed to match keyword '"s + str +
                       "' in ASCII-based STL file.");
  };

  if (keyword != "solid")
    throw parser_error{"Failed to match keyword 'solid' at the start."};
  input >> name;

  while (file >> keyword) {
    if (keyword == "endsolid") {
      match(name.c_str());
      return;
    } else if (keyword == "facet") {
      triangle t{};
      match("normal");
      file >> t.normal.x >> t.normal.y >> t.normal.z;
      match("outer");
      match("loop");
      for (int i = 0; i < 3; ++i) {
        match("vertex");
        file >> t.vertex[i].x >> t.vertex[i].y >> t.vertex[i].z;
      }
      match("endloop");
      match("endfacet");
      triangles.push_back(t);
    } else
      throw parser_error{"Failed to match keyword 'facet' or 'endsolid'."};
  }
}

stl_surface::stl_surface(const filesystem::path& path, binary_tag) {
  load_from_binary_file(path);
}

stl_surface::stl_surface(const filesystem::path& path, ascii_tag) {
  load_from_ascii_file(path);
}

stl_surface::stl_surface(const filesystem::path& path) {
  try {
    load_from_ascii_file(path);
  } catch (parser_error&) {
    load_from_binary_file(path);
  }
}
}  // namespace ensketch::sandbox
