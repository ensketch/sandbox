namespace ensketch::sandbox {

void traverse(scene::node& node, auto&& f) {
  std::invoke(f, node);
  for (auto& child : node.children) traverse(child, f);
}

void traverse(const scene::node& node, auto&& f) {
  std::invoke(f, node);
  for (auto& child : node.children) traverse(child, f);
}

}  // namespace ensketch::sandbox
