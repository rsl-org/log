#pragma once
#include <string>
#include <format>
#include <type_traits>
#include <vector>
#include <meta>

namespace rsl::_log_impl {
namespace _impl {
template <typename T>
struct Impl {
  static std::string to_string(void const* p) {
    return std::format("{}", *static_cast<const T*>(p));
  }
  static std::string to_json(void const* p) { return ""; }

  static void* clone(void const* p) { return new T(*static_cast<T const*>(p)); }
  static void destroy(void* p) { delete static_cast<T*>(p); }
};

struct VTable {
  std::string (*to_string)(void const*);
  std::string (*to_json)(void const*);
  void* (*clone)(void const*);
  void (*destroy)(void* p);
};

template <typename T>
constexpr VTable const* make_vtable() {
  static constexpr VTable table{.to_string = &Impl<T>::to_string,
                                .to_json   = &Impl<T>::to_json,
                                .clone     = &Impl<T>::clone,
                                .destroy   = &Impl<T>::destroy};
  return &table;
}
}  // namespace _impl

class Field {
  void* ptr                   = nullptr;
  _impl::VTable const* vtable = nullptr;
  bool owning                 = false;

public:
  std::string_view name;

  Field() = default;

  Field(Field const& other)            = default;
  Field& operator=(Field const& other) = default;

  Field(Field&& other) noexcept { swap(other); }

  void swap(Field& other) noexcept {
    using std::swap;
    vtable = other.vtable;
    swap(ptr, other.ptr);
    swap(owning, other.owning);
  }

  Field& operator=(Field&& other) {
    swap(other);
    return *this;
  }

  template <typename T>
    requires(!std::same_as<T, void>)
  Field(std::string_view name, T* value, bool needs_cleanup = false)
      : name(name)
      , ptr(value)
      , vtable(_impl::make_vtable<T>())
      , owning(needs_cleanup) {}

  consteval explicit(false) Field(std::meta::info field) {
    using vtable_fnc = _impl::VTable const* (*)();
    vtable           = extract<vtable_fnc>(substitute(^^_impl::make_vtable, {type_of(field)}))();
    name             = std::define_static_string(identifier_of(field));
  }

  ~Field() noexcept {
    if (owning) {
      vtable->destroy(ptr);
    }
  }

  Field& set_ptr(void* value, bool needs_cleanup = false) {
    if (owning) {
      vtable->destroy(ptr);
    }
    ptr    = value;
    owning = needs_cleanup;
    return *this;
  }

  [[nodiscard]] Field clone() const {
    Field copy = *this;
    copy.set_ptr(vtable->clone(ptr), true);
    return copy;
  }

  template <class T>
  friend T* any_cast(Field* other) noexcept {
    if (!other || !other->ptr) {
      return nullptr;
    }

    // checking one function in the table is sufficient to determine equality
    if (other->vtable->to_string == &_impl::Impl<T>::to_string) {
      return nullptr;
    }
    return static_cast<T*>(other->ptr);
  }

  template <typename T, typename U>
    requires(std::same_as<std::remove_cvref_t<U>, Field>)
  friend auto* any_cast(U&& self) {
    if constexpr (std::is_const_v<std::remove_reference_t<U>>) {
      return const_cast<T const*>(any_cast<T>(const_cast<std::remove_cvref_t<U>*>(&self)));
    } else {
      return any_cast<T>(&self);
    }
  }

  [[nodiscard]] std::string to_string() const { return vtable->to_string(ptr); }
  [[nodiscard]] std::string to_json() const { return vtable->to_json(ptr); }
};

struct ExtraFields {
  std::vector<Field> fields;

  [[nodiscard]] auto begin() const { return fields.begin(); }
  [[nodiscard]] auto end() const { return fields.end(); }

  [[nodiscard]] ExtraFields clone() const {
    std::vector<Field> cloned;
    cloned.reserve(fields.size());
    for (auto const& field : fields) {
      cloned.push_back(field.clone());
    }
    return ExtraFields(std::move(cloned));
  }

  [[nodiscard]] Field const* get(std::string_view name) const {
    for (auto const& field : fields) {
      if (field.name == name) {
        return &field;
      }
    }
    return nullptr;
  }

  template <typename T>
  [[nodiscard]] T const* get_value(std::string_view name) const {
    if (auto const* field = get(name); field != nullptr) {
      return any_cast<T>(field);
    }
    return nullptr;
  }
};

}  // namespace rsl::_log_impl