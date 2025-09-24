#pragma once
#include <string>
#include <format>
#include <type_traits>
#include <utility>
#include <vector>
#include <meta>
#include <atomic>


#include <rsl/serialize>
#include <rsl/repr>

#include <kwargs.h>

namespace rsl::logging {
class Field {
  template <typename T>
  struct Impl {
    struct RefCounted {
      mutable std::atomic_unsigned_lock_free references;
      T data;
      std::string name;

      RefCounted(T obj, std::string_view name) : references(0), data(std::move(obj)), name(name) {}
      ~RefCounted()                            = default;
      RefCounted(RefCounted const&)            = delete;
      RefCounted(RefCounted&&)                 = delete;
      RefCounted& operator=(RefCounted const&) = delete;
      RefCounted& operator=(RefCounted&&)      = delete;
    };

    static std::string to_string(Field const* field) { return std::format("{}", *get(field)); }
    static std::string to_repr(Field const* field) { return rsl::repr(*get(field)); }
    static std::string to_json(Field const* field) { return ""; }

    static T* get(Field* field) {
      if (field->vtable->destroy != nullptr) {
        return &static_cast<RefCounted*>(field->ptr)->data;
      } else {
        return static_cast<T*>(field->ptr);
      }
    }

    static T const* get(Field const* field) { return get(const_cast<Field*>(field)); }

    static Field transition(Field const* field) {
      RefCounted* ref = new RefCounted(*static_cast<T const*>(field->ptr), field->name);
      return Field(ref, make_vtable<T, true>(), ref->name);
    }

    static Field clone(Field const* field) {
      auto const& control_block = *static_cast<RefCounted const*>(field->ptr);
      // control_block.references++;
      auto val = control_block.references.fetch_add(1);
      return Field(field->ptr, field->vtable, field->name);
    }

    static void destroy(void* p) {
      auto* control_block = static_cast<RefCounted*>(p);
      auto val            = control_block->references.fetch_sub(1);
      if (val == 0) {
        delete control_block;
      }
    }
  };

  struct VTable {
    std::string_view type_name;

    std::string (*to_string)(Field const*);
    std::string (*to_repr)(Field const*);
    std::string (*to_json)(Field const*);
    Field (*clone)(Field const*);
    void (*destroy)(void* p);

    bool is_owning() const { return destroy != nullptr; }
  };

  template <typename T, bool Heap>
  constexpr static VTable const* make_vtable() {
    static constexpr VTable table{.type_name = rsl::type_name<T>,
                                  .to_string = &Impl<T>::to_string,
                                  .to_repr   = &Impl<T>::to_repr,
                                  .to_json   = &Impl<T>::to_json,
                                  .clone     = Heap ? &Impl<T>::clone : &Impl<T>::transition,
                                  .destroy   = Heap ? &Impl<T>::destroy : nullptr};
    return &table;
  }

  void* ptr                   = nullptr;
  VTable const* vtable = nullptr;

  void destroy() {
    if (vtable != nullptr && vtable->is_owning()) {
      vtable->destroy(ptr);
    }
    ptr    = nullptr;
    vtable = nullptr;
  }

  Field(void* ptr, VTable const* vtable, std::string_view name)
      : ptr(ptr)
      , vtable(vtable)
      , name(name) {}

  static Field copy_from(Field const& other) {
    if (other.vtable->destroy != nullptr) {
      return other.vtable->clone(&other);
    } else {
      return {other.ptr, other.vtable, other.name};
    }
  }

public:
  std::string_view name;

  Field() = default;
  Field(const Field& other) : Field(copy_from(other)) {}

  // move constructor
  Field(Field&& other) noexcept
      : ptr(other.ptr)
      , vtable(other.vtable)
      , name(other.name) {
    other.ptr    = nullptr;
    other.vtable = nullptr;
  }

  // copy assignment
  Field& operator=(const Field& other) {
    if (this != &other) {
      destroy();
      *this = copy_from(other);
    }
    return *this;
  }

  // move assignment
  Field& operator=(Field&& other) noexcept {
    if (this != &other) {
      destroy();
      ptr    = std::exchange(other.ptr, nullptr);
      vtable = std::exchange(other.vtable, nullptr);
      name   = other.name;
    }
    return *this;
  }

  template <typename T>
    requires(not std::same_as<T, void>)
  Field(std::string_view name, T* value)
      : ptr(value)
      , vtable(make_vtable<T, false>())
      , name(name) {}

  ~Field() noexcept { destroy(); }

  [[nodiscard]] Field clone() const {
    if (vtable == nullptr || ptr == nullptr) {
      return {};
    }
    return vtable->clone(this);
  }

  template <class T>
  friend T* any_cast(Field* field) noexcept {
    if (!field || !field->ptr) {
      return nullptr;
    }

    // checking one function in the table is sufficient to determine equality
    if (field->vtable->to_string != &Impl<T>::to_string) {
      return nullptr;
    }
    return Impl<T>::get(field);
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

  [[nodiscard]] std::string_view type_name() const { return vtable->type_name; }
  [[nodiscard]] std::string to_string() const { return vtable->to_string(this); }
  [[nodiscard]] std::string to_json() const { return vtable->to_json(this); }
  [[nodiscard]] std::string to_repr() const { return vtable->to_repr(this); }
};

struct ExtraFields {
  //TODO this could be a span that views memory on the stack and transitions to heap if its elts do
  std::vector<Field> fields;

  ExtraFields() = default;
  explicit(false) ExtraFields(std::vector<Field> fields) : fields(std::move(fields)) {}
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

}  // namespace rsl::logging