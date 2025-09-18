#pragma once
#include <span>
#include <vector>
#include <optional>
#include <unordered_map>
#include <string_view>
#include <memory>

#include "logger.hpp"
#include <rsl/_impl/hash.hpp>

namespace rsl::logging {
struct Namespace {
  // TODO remove backreference
  Namespace* parent = nullptr;
  
  std::string_view name;
  // TODO remove unique_ptr
  std::unordered_map<_impl::hash_t, std::unique_ptr<Namespace>> children;
  
  // TODO fold into this class
  std::optional<Overrides> overrides;

  LogLevel min_level;
  std::vector<Sink> sinks;


  Namespace() = default;
  Namespace(Namespace* p, std::string_view n) : parent(p), name(n) {}

  Namespace* add_overrides(std::string_view dotted, Overrides ovr) {
    Namespace* cur = this;
    size_t pos     = 0;
    while (pos < dotted.size()) {
      size_t next = dotted.find("::", pos);
      if (next == std::string_view::npos) {
        next = dotted.size();
      }

      std::string_view seg = dotted.substr(pos, next - pos);
      if (seg.empty()) {
        break;
      }

      _impl::hash_t h = _impl::fnv1a(seg);

      auto it = cur->children.find(h);
      if (it == cur->children.end()) {
        auto ns   = std::make_unique<Namespace>(cur, seg);
        auto* ptr = ns.get();
        cur->children.emplace(h, std::move(ns));
        cur = ptr;
      } else {
        cur = it->second.get();
      }

      pos = next + 2;
    }
    cur->overrides = std::move(ovr);
    return cur;
  }

  Overrides get_overrides_for(std::span<_impl::hash_t> hashes) const {
    const Namespace* cur                = this;
    std::optional<Overrides> last_found = overrides;
    Overrides result;
    for (_impl::hash_t seg : hashes) {
      auto it = cur->children.find(seg);
      if (it == cur->children.end()) {
        break;
      }
      cur = it->second.get();
      if (cur->overrides) {
        last_found = cur->overrides;
        if (cur->overrides->min_level.has_value()) {
          result.min_level = cur->overrides->min_level;
        }

        if (!cur->overrides->sinks.empty()) {
          result.sinks = cur->overrides->sinks;
        }
      }
    }

    return result;
  }

  std::optional<Overrides> get_overrides_for(std::string_view dotted) const {
    auto hashes = split_and_hash(dotted);
    return get_overrides_for(hashes);
  }

  void remove_overrides(std::string_view pattern) {
    auto hashes = split_and_hash(pattern);
    remove_overrides_recursive(this, hashes, 0);
  }

private:
  static constexpr _impl::hash_t HASH_STAR  = _impl::fnv1a("*");
  static constexpr _impl::hash_t HASH_DSTAR = _impl::fnv1a("**");

  static std::vector<_impl::hash_t> split_and_hash(std::string_view dotted) {
    std::vector<_impl::hash_t> hashes;
    size_t pos = 0;
    while (pos < dotted.size()) {
      size_t next = dotted.find("::", pos);
      if (next == std::string_view::npos) {
        next = dotted.size();
      }
      std::string_view seg = dotted.substr(pos, next - pos);
      if (!seg.empty()) {
        hashes.push_back(_impl::fnv1a(seg));
      }
      pos = next + 2;
    }
    return hashes;
  }

  static void remove_overrides_recursive(Namespace* node,
                                         const std::vector<_impl::hash_t>& pattern,
                                         size_t i) {
    if (i == pattern.size()) {
      node->overrides.reset();
      return;
    }

    _impl::hash_t seg = pattern[i];

    if (seg == HASH_DSTAR) {
      // '**' matches zero or more segments
      // Match zero segments: advance pattern index
      remove_overrides_recursive(node, pattern, i + 1);
      // Match one or more segments: recurse into all children keeping pattern index
      for (auto& [child_hash, child_ptr] : node->children) {
        remove_overrides_recursive(child_ptr.get(), pattern, i);
      }
    } else if (seg == HASH_STAR) {
      // '*' matches exactly one segment: recurse into all children advancing pattern index
      for (auto& [child_hash, child_ptr] : node->children) {
        remove_overrides_recursive(child_ptr.get(), pattern, i + 1);
      }
    } else {
      // Literal segment match
      auto it = node->children.find(seg);
      if (it != node->children.end()) {
        remove_overrides_recursive(it->second.get(), pattern, i + 1);
      }
    }
  }
};
}  // namespace rsl::logging