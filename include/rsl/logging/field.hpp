#pragma once
#include <string>
#include <format>
#include <type_traits>
#include <vector>
#include <meta>

#include <print>

#include <rsl/serialize>
#include <rsl/repr>

#include <kwargs.h>

namespace rsl::_log_impl {
namespace _impl {
template <typename T>
struct Impl {
  static std::string to_string(void const* p) {
    return std::format("{}", *static_cast<const T*>(p));
  }
  static std::string to_repr(void const* p) { return rsl::repr(*static_cast<const T*>(p)); }
  static std::string to_json(void const* p) { return ""; }

  static void* clone(void const* p) { return new T(*static_cast<T const*>(p)); }
  static void destroy(void* p) { delete static_cast<T*>(p); }
};

struct VTable {
  std::string (*type_name)(void const*);
  std::string (*to_string)(void const*);
  std::string (*to_repr)(void const*);
  std::string (*to_json)(void const*);
  void* (*clone)(void const*);
  void (*destroy)(void* p);
};

template <typename T>
constexpr VTable const* make_vtable() {
  static constexpr VTable table{.to_string = &Impl<T>::to_string,
                                .to_repr   = &Impl<T>::to_repr,
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
  std::string_view type_name;

  Field() = default;

  Field(Field const& other)            = default;
  Field& operator=(Field const& other) = default;

  Field(Field&& other) noexcept { swap(other); }

  void swap(Field& other) noexcept {
    using std::swap;
    vtable = other.vtable;
    swap(ptr, other.ptr);
    swap(owning, other.owning);
    swap(name, other.name);
    swap(type_name, other.type_name);
  }

  Field& operator=(Field&& other) {
    swap(other);
    return *this;
  }

  template <typename T>
    requires(!std::same_as<T, void>)
  Field(std::string_view name, T* value, bool needs_cleanup = false)
      : name(name)
      , type_name(rsl::type_name<T>)
      , ptr(value)
      , vtable(_impl::make_vtable<T>())
      , owning(needs_cleanup) {}

  ~Field() noexcept {
    if (owning) {
      //! TODO
      // vtable->destroy(ptr);
    }
  }

  [[nodiscard]] Field clone() const {
    Field copy = *this;
    copy.ptr = vtable->clone(ptr);
    copy.owning = true;
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
  [[nodiscard]] std::string to_repr() const { return vtable->to_repr(ptr); }
};

struct ExtraFields {
  std::vector<Field> fields;

  ExtraFields() = default;
  explicit(false) ExtraFields(std::vector<Field> fields) : fields(fields) {}
  template <typename T>
    requires is_kwargs<std::remove_cvref_t<T>>
  explicit(false) ExtraFields(T&& kwargs) {
    template for (constexpr auto member : std::define_static_array(
                      nonstatic_data_members_of(^^typename std::remove_cvref_t<T>::type,
                                                std::meta::access_context::current()))) {
      static constexpr auto name = std::define_static_string(identifier_of(member));
      // we need to clone here
      // the kwargs wrapper will not live long enough, it'll be destroyed
      // after the full expression in which this container is created
      fields.push_back(Field(name, &kwargs.[:member:]).clone());
    }
  }

  [[nodiscard]] bool is_empty() const { return fields.empty(); }
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