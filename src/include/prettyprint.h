#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <type_traits>
#include <utility>

// -----------------------------------------------------------------------------
// A pretty-printer for (pretty much) any type.
//
// * If operator<< is available, it will be used.
// * If the type is not printable (like a function) we will attempt to
//   characterize it anyway using type_traits and print something like
//   <function>.
// * Containers (iterable, with begin() and end()) get printed with customizable
//   openers, closers and separators. (The default is { , })
// * Pairs are printed (like,this) as are tuples.
// * Enum values and enum class values are printed as ints.
// * Objects with operator() that can implicitly convert to bool are output as
//   <callable object> even though operator<< would work. An example is
//   non-capturing lambdas.

// -----------------------------------------------------------------------------
// SFINAE member/functionality detection
template <typename...>
using void_t = void;

#define SFINAE_DETECT(name, expr)                                       \
  template <typename T>                                                 \
  using name##_t = decltype(expr);                                      \
  template <typename T, typename = void>                                \
  struct has_##name : public std::false_type {};                        \
  template <typename T>                                                 \
  struct has_##name<T, void_t<name##_t<T>>> : public std::true_type {};

// -----------------------------------------------------------------------------
// Is the type iterable (has begin() and end())?
SFINAE_DETECT(begin, std::declval<T>().begin())
SFINAE_DETECT(end, std::declval<T>().end())

template<typename T>
using is_iterable = typename std::conditional<
  has_begin<T>::value && has_end<T>::value,
  std::true_type, std::false_type>::type;

struct is_iterable_tag {};

// -----------------------------------------------------------------------------
// Is the type a pair or tuple?
template <typename T>
struct is_pair : public std::false_type {};
template <typename T, typename U>
struct is_pair<std::pair<T, U>> : public std::true_type {};

struct is_pair_tag {};

template <typename T>
struct is_tuple : public std::false_type {};
template <typename... Ts>
struct is_tuple<std::tuple<Ts...>> : public std::true_type {};

struct is_tuple_tag {};

// -----------------------------------------------------------------------------
// Is the type a callable of some kind?

SFINAE_DETECT(call_operator, &T::operator())

template <typename T>
struct is_std_function : public std::false_type {};
template <typename T>
struct is_std_function<std::function<T>> : public std::true_type {};

struct is_std_function_tag {};
struct is_function_tag {};
struct is_callable_tag {};

// -----------------------------------------------------------------------------
// Does the type support operator<< ?
SFINAE_DETECT(operator_output, std::cout << std::declval<T>())

// Non-capturing lambdas (and some other callables) may implicitly convert to
// bool, which will make operator<< work. We want to treat them as callables,
// not outputtables.
void bool_test(bool);
SFINAE_DETECT(bool_conversion, bool_test(std::declval<T>()))

template<typename T>
using is_outputtable = typename std::conditional<
  has_operator_output<T>::value &&
  !(has_call_operator<T>::value && has_bool_conversion<T>::value),
  std::true_type, std::false_type>::type;

struct is_outputtable_tag {};

// -----------------------------------------------------------------------------
// Is the type an enum or enum class?

struct is_enum_tag {};

// -----------------------------------------------------------------------------
// Is the type something else we want to distinguish?

struct is_union_tag {};
struct is_class_tag {};
struct is_nullptr_tag {};

// -----------------------------------------------------------------------------
template <typename T, typename TAG>
struct stringifier_select;

// The way we want to treat a type, in preference order.

template <typename T>
using stringifier_tag = std::conditional_t<
  std::is_null_pointer<T>::value,
  is_nullptr_tag,
  std::conditional_t<
    is_std_function<std::remove_reference_t<T>>::value,
    is_std_function_tag,
    std::conditional_t<
      std::is_function<std::remove_reference_t<T>>::value,
      is_function_tag,
      std::conditional_t<
        std::is_enum<T>::value,
        is_enum_tag,
        std::conditional_t<
          is_outputtable<T>::value,
          is_outputtable_tag,
          std::conditional_t<
            has_call_operator<T>::value,
            is_callable_tag,
            std::conditional_t<
              is_iterable<T>::value,
              is_iterable_tag,
              std::conditional_t<
                is_pair<T>::value,
                is_pair_tag,
                std::conditional_t<
                  is_tuple<T>::value,
                  is_tuple_tag,
                  std::conditional_t<
                    std::is_union<T>::value,
                    is_union_tag,
                    std::conditional_t<
                      std::is_class<T>::value,
                      is_class_tag,
                      void>>>>>>>>>>>;

template <typename T>
using stringifier = stringifier_select<T, stringifier_tag<T>>;

template <typename T>
inline std::ostream& operator<<(std::ostream& s, const stringifier<T>& t)
{
  return t.output(s);
}

template <typename T>
inline stringifier<T> prettyprint(T&& t)
{
  return stringifier<T>(std::forward<T>(t));
}

// -----------------------------------------------------------------------------
// default: not stringifiable
template <typename T, typename TAG>
struct stringifier_select
{
  explicit stringifier_select(T&&) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << "<unknown>";
  }
};

// -----------------------------------------------------------------------------
// If T has an output operator, use it
template <typename T>
struct stringifier_select<T, is_outputtable_tag>
{
  explicit stringifier_select(const T& t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << m_t;
  }

  const T& m_t;
};

// -----------------------------------------------------------------------------
// Specialize for bool: do boolalpha explicitly so as not to affect stream state
template <>
struct stringifier_select<bool, is_outputtable_tag>
{
  explicit stringifier_select(bool t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << (m_t ? "true" : "false");
  }

  bool m_t;
};

// -----------------------------------------------------------------------------
// If t is iterable, show that, with customization points for
// opener/closer/separator

template <typename T>
struct iterable_opener
{
  constexpr static const char* value = "{";
};

template <typename T>
struct iterable_closer
{
  constexpr static const char* value = "}";
};

template <typename T>
struct iterable_separator
{
  constexpr static const char* value = ",";
};

template <typename T>
struct stringifier_select<T, is_iterable_tag>
{
  explicit stringifier_select(const T& t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    s << iterable_opener<decay_t<T>>::value;
    auto b = m_t.begin();
    auto e = m_t.end();
    if (e != b)
      s << prettyprint(*b);
    std::for_each(++b, e,
                  [&s] (auto& e) { s << iterable_separator<decay_t<T>>::value
                                     << prettyprint(e); });
    return s << iterable_closer<decay_t<T>>::value;
  }

  const T& m_t;
};

// -----------------------------------------------------------------------------
#define SS_SIMPLE_SPEC(TAG, NAME)               \
  template <typename T>                         \
  struct stringifier_select<T, TAG>             \
  {                                             \
    explicit stringifier_select(T&&) {}         \
    std::ostream& output(std::ostream& s) const \
    {                                           \
      return s << #NAME;                        \
    }                                           \
};

// -----------------------------------------------------------------------------
// If T is a function, show that

SS_SIMPLE_SPEC(is_callable_tag, <callable object>)
SS_SIMPLE_SPEC(is_function_tag, <function>)
SS_SIMPLE_SPEC(is_std_function_tag, <std::function>)

// -----------------------------------------------------------------------------
// If T is a regular object, show that

SS_SIMPLE_SPEC(is_class_tag, <object>)
SS_SIMPLE_SPEC(is_union_tag, <union>)

// -----------------------------------------------------------------------------
// If T is a nullptr, show that

SS_SIMPLE_SPEC(is_nullptr_tag, nullptr)

// -----------------------------------------------------------------------------
// If T is an enum or enum class, show that
template <typename T>
struct stringifier_select<T, is_enum_tag>
{
  explicit stringifier_select(const T t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << static_cast<std::underlying_type_t<T>>(m_t);
  }

  T m_t;
};

// -----------------------------------------------------------------------------
// If T is a pair, show that
template <typename T>
struct stringifier_select<T, is_pair_tag>
{
  explicit stringifier_select(const T& t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << '(' << prettyprint(m_t.first)
             << ',' << prettyprint(m_t.second) << ')';
  }

  const T& m_t;
};

// -----------------------------------------------------------------------------
// If t is a tuple, show that
template <typename F, typename...Ts, std::size_t...Is>
inline void for_each_in_tuple(const std::tuple<Ts...>& t, F f,
                              std::index_sequence<Is...>)
{
  (void)(int[]) { 0, (f(std::get<Is>(t), Is), 0)... };
}

template <typename F, typename...Ts>
inline void for_each_in_tuple(const std::tuple<Ts...>& t, F f)
{
  for_each_in_tuple(t, f, std::make_index_sequence<sizeof...(Ts)>());
}

template <typename T>
struct stringifier_select<T, is_tuple_tag>
{
  explicit stringifier_select(const T& t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    s << '(';
    for_each_in_tuple(m_t,
                      [&s] (auto& e, size_t i)
                      { if (i > 0) s << ',';
                        s << prettyprint(e); });
    return s << ')';
  }

  const T& m_t;
};

#undef SFINAE_DETECT
#undef SS_SIMPLE_SPEC
