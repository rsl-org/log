#include <rsl/logging/hierarchy.hpp>
#include <rsl/test>
#include "rsl/logging/logger.hpp"

namespace rsl::logging::_test_namespace {
  static void assert_min_level(const Namespace& root, std::string_view path, LogLevel expected) {
  auto o = root.get_overrides_for(path);
  ASSERT(o.has_value(), "Override missing", path);
  ASSERT(o->min_level == expected, "Override ID mismatch", path);
}

static void assert_no_override(const Namespace& root, std::string_view path) {
  ASSERT(!root.get_overrides_for(path)->min_level.has_value(), "Override should NOT exist", path);
}

[[=test]]
void add_overrides_and_structure() {
  Namespace root;

  auto* ns_foo = root.add_overrides("foo", {.min_level=LogLevel::DEBUG});
  auto* ns_bar = root.add_overrides("foo::bar", {.min_level=LogLevel::INFO});
  auto* ns_baz = root.add_overrides("foo::bar::baz", {.min_level=LogLevel::ERROR});

  assert_min_level(root, "foo", LogLevel::DEBUG);
  assert_min_level(root, "foo::bar", LogLevel::INFO);
  assert_min_level(root, "foo::bar::baz", LogLevel::ERROR);
  assert_min_level(root, "foo::bar::baz::extra", LogLevel::ERROR); // fallback to closest parent
  assert_min_level(root, "foo::bar::unknown", LogLevel::INFO);

  ASSERT(ns_foo->name == "foo", "Namespace name for 'foo'");
  ASSERT(ns_foo->parent == &root, "Parent of 'foo' is root");
  ASSERT(ns_bar->name == "bar", "Namespace name for 'foo::bar'");
  ASSERT(ns_bar->parent == ns_foo, "Parent of 'foo::bar' is 'foo'");
  ASSERT(ns_baz->name == "baz", "Namespace name for 'foo::bar::baz'");
  ASSERT(ns_baz->parent == ns_bar, "Parent of 'foo::bar::baz' is 'foo::bar'");
  ASSERT(root.name.empty(), "Root namespace name");
  ASSERT(root.parent == nullptr, "Root namespace parent");
}

[[=test]]
void remove_overrides_wildcards() {
  Namespace root;

  root.add_overrides("foo::bar::baz", {.min_level=LogLevel::DEBUG});
  root.add_overrides("foo::bar::qux", {.min_level=LogLevel::INFO});
  root.add_overrides("foo::baz::bar",{.min_level=LogLevel::WARNING});
  root.add_overrides("foo::qux", {.min_level=LogLevel::DEBUG});
  root.add_overrides("bar::baz",{.min_level=LogLevel::TRACE});
  root.add_overrides("bar::qux::bar", {.min_level=LogLevel::DEBUG});
  root.add_overrides("foo::deep::very::bar", {.min_level=LogLevel::DEBUG});

  root.remove_overrides("foo::*::baz");
  assert_no_override(root, "foo::bar::baz");
  assert_min_level(root, "foo::bar::qux", LogLevel::INFO);
  assert_min_level(root, "foo::baz::bar", LogLevel::WARNING);

  root.remove_overrides("**::bar");
  assert_no_override(root, "foo::baz::bar");
  assert_no_override(root, "bar::qux::bar");
  assert_no_override(root, "foo::deep::very::bar");

  root.add_overrides("foo::x::y::bar", {.min_level=LogLevel::DEBUG});
  root.add_overrides("foo::x::bar", {.min_level=LogLevel::DEBUG});
  root.add_overrides("foo::bar", {.min_level=LogLevel::DEBUG});

  root.remove_overrides("foo::**::bar");
  assert_no_override(root, "foo::x::y::bar");
  assert_no_override(root, "foo::x::bar");
  assert_no_override(root, "foo::bar");

  root.remove_overrides("foo::**");
  assert_no_override(root, "foo::bar::qux");
  assert_no_override(root, "foo::qux");

  assert_min_level(root, "bar::baz", LogLevel::TRACE);
}

[[=test]]
void remove_all_and_empty_string() {
  Namespace root;

  root.add_overrides("foo::bar", {.min_level=LogLevel::DEBUG});
  root.add_overrides("foo::baz", {.min_level=LogLevel::DEBUG});
  root.add_overrides("bar::baz::bar", {.min_level=LogLevel::DEBUG});
  root.add_overrides("", {.min_level=LogLevel::WARNING});
  assert_min_level(root, "unknown::path", LogLevel::WARNING);

  root.remove_overrides("**");
  assert_no_override(root, "foo::bar");
  assert_no_override(root, "foo::baz");
  assert_no_override(root, "bar::baz::bar");

  root.add_overrides("", {.min_level=LogLevel::ERROR});
  assert_min_level(root, "unknown::path", LogLevel::ERROR);

  root.remove_overrides("");
  assert_no_override(root, "unknown::path");
}

// [[=rsl::test]]
// void full_name_reconstruction() {
//   Namespace root;
//   auto* ns_foo = root.add_overrides("foo", Overrides{1});
//   auto* ns_bar = root.add_overrides("foo::bar", Overrides{2});
//   auto* ns_baz = root.add_overrides("foo::bar::baz", Overrides{3});

//   ASSERT(ns_foo->full_name() == "foo", "Full name of 'foo'");
//   ASSERT(ns_bar->full_name() == "foo::bar", "Full name of 'foo::bar'");
//   ASSERT(ns_baz->full_name() == "foo::bar::baz", "Full name of 'foo::bar::baz'");
//   ASSERT(root.full_name().empty(), "Root full name empty");
// }
}