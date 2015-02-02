#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <type_traits>
#include <utility>

// -----------------------------------------------------------------------------
// A pretty-printer for (pretty much) any type.
//
// * If operator<< is available, it will be used.
// * If the type is not printable (like a union) we will attempt to
//   characterize it anyway using type_traits and print something like
//   <union>.
// * Containers (iterable, with begin() and end()) get printed with customizable
//   openers, closers and separators. (The default is { , })
// * Pairs are printed (like,this), as are tuples.
// * Enum values and enum class values are printed as ints.
// * Objects with operator() that can implicitly convert to bool are output as
//   <callable> even though operator<< would work. An example is non-capturing
//   lambdas, which can implicit convert to pointer-to-function and thus to
//   bool.
// * Strings and char arrays are printed with surrounding quotes.

// -----------------------------------------------------------------------------
// SFINAE member/functionality detection
#define SFINAE_DETECT(name, expr)                                       \
  template <typename T>                                                 \
  using name##_t = decltype(expr);                                      \
  template <typename T, typename = void>                                \
  struct has_##name : public std::false_type {};                        \
  template <typename T>                                                 \
  struct has_##name<T, detail::void_t<name##_t<T>>> : public std::true_type {};

namespace detail
{

  // ---------------------------------------------------------------------------
  template <typename...>
  using void_t = void;

  // ---------------------------------------------------------------------------
  // Is the type iterable (has begin() and end())?
  SFINAE_DETECT(begin, std::declval<T>().begin())
  SFINAE_DETECT(end, std::declval<T>().end())

  template<typename T>
  using is_iterable = typename std::conditional<
    has_begin<T>::value && has_end<T>::value,
    std::true_type, std::false_type>::type;

  struct is_iterable_tag {};

  // ---------------------------------------------------------------------------
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

  // ---------------------------------------------------------------------------
  // Is the type a callable of some kind?
  SFINAE_DETECT(call_operator, &T::operator())

  template <typename T>
  struct is_std_function : public std::false_type {};
  template <typename T>
  struct is_std_function<std::function<T>> : public std::true_type {};

  template<typename T>
  using is_callable = typename std::conditional<
    has_call_operator<T>::value
    || is_std_function<T>::value
    || std::is_function<T>::value
    || std::is_bind_expression<T>::value,
    std::true_type, std::false_type>::type;

  struct is_callable_tag {};

  template <typename T>
  constexpr static std::enable_if_t<is_std_function<T>::value, const char*>
  callable_type() { return "(std::function)"; }

  template <typename T>
  constexpr static std::enable_if_t<std::is_function<T>::value, const char*>
  callable_type() { return "(function)"; }

  template <typename T>
  constexpr static
  std::enable_if_t<std::is_bind_expression<T>::value,
                   const char*>
  callable_type() { return "(bind expression)"; }

  template <typename T>
  constexpr static
  std::enable_if_t<!is_std_function<T>::value && has_call_operator<T>::value,
                   const char*>
    callable_type() { return "(function object)"; }

  // ---------------------------------------------------------------------------
  // Does the type support operator<< ?
  SFINAE_DETECT(operator_output, std::cout << std::declval<T>())

  // Non-capturing lambdas (and some other callables) may implicitly convert to
  // bool, which will make operator<< work. We want to treat them as callables,
  // not outputtables.
  void bool_conversion_test(bool);
  SFINAE_DETECT(bool_conversion, bool_conversion_test(std::declval<T>()))

  template<typename T>
  using is_outputtable = typename std::conditional<
    has_operator_output<T>::value &&
    !std::is_function<T>::value &&
    !(has_call_operator<T>::value && has_bool_conversion<T>::value),
    std::true_type, std::false_type>::type;

  struct is_outputtable_tag {};

  // ---------------------------------------------------------------------------
  // Is the type an enum or enum class?
  struct is_enum_tag {};

  // ---------------------------------------------------------------------------
  // Is the type something unprintable?
  template<typename T>
  using is_unprintable = typename std::conditional<
    std::is_union<T>::value ||
    std::is_class<T>::value ||
    std::is_null_pointer<T>::value,
    std::true_type, std::false_type>::type;

  struct is_unprintable_tag {};

  template <typename T>
  constexpr static std::enable_if_t<is_union<T>::value, const char*>
  unprintable_type() { return "<union>"; }

  template <typename T>
  constexpr static std::enable_if_t<std::is_class<T>::value, const char*>
  unprintable_type() { return "<class>"; }

  template <typename T>
  constexpr static
  std::enable_if_t<std::is_null_pointer<T>::value, const char*>
  unprintable_type() { return "<nullptr>"; }

} // detail

// -----------------------------------------------------------------------------
template <typename T, typename TAG>
struct stringifier_select;

// The way we want to treat a type, in preference order.
template <typename T>
using stringifier_tag = std::conditional_t<
  detail::is_outputtable<T>::value,
  detail::is_outputtable_tag,
  std::conditional_t<
    detail::is_callable<T>::value,
    detail::is_callable_tag,
    std::conditional_t<
      detail::is_iterable<T>::value,
      detail::is_iterable_tag,
      std::conditional_t<
        detail::is_pair<T>::value,
        detail::is_pair_tag,
        std::conditional_t<
          detail::is_tuple<T>::value,
          detail::is_tuple_tag,
          std::conditional_t<
            std::is_enum<T>::value,
            detail::is_enum_tag,
            std::conditional_t<
              detail::is_unprintable<T>::value,
              detail::is_unprintable_tag,
              void>>>>>>>;

template <typename T>
using stringifier = stringifier_select<T, stringifier_tag<T>>;

template <typename T>
inline std::ostream& operator<<(std::ostream& s, const stringifier<T>& t)
{
  return t.output(s);
}

// -----------------------------------------------------------------------------
// The function that drives it all
template <typename T>
inline stringifier<std::remove_reference_t<T>> prettyprint(T&& t)
{
  return stringifier<std::remove_reference_t<T>>(std::forward<T>(t));
}

// -----------------------------------------------------------------------------
// Customization points for printing iterable things (including arrays)
template <typename T>
struct iterable_opener
{
  constexpr const char* operator()(const T&) const
  { return "{"; }
};

template <typename T>
struct iterable_closer
{
  constexpr const char* operator()(const T&) const
  { return "}"; }
};

template <typename T>
struct iterable_separator
{
  constexpr const char* operator()(const T&) const
  { return ","; }
};

// -----------------------------------------------------------------------------
// Default: not stringifiable
template <typename T, typename TAG>
struct stringifier_select
{
  explicit stringifier_select(const T&) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << "<unknown>";
  }
};

// -----------------------------------------------------------------------------
// Specialization for outputtable
template <typename T>
struct stringifier_select<T, detail::is_outputtable_tag>
{
  explicit stringifier_select(const T& t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << m_t;
  }

  const T& m_t;
};

// -----------------------------------------------------------------------------
// Specialize for string
template <typename C, typename T, typename A>
struct stringifier_select<std::basic_string<C,T,A>, detail::is_outputtable_tag>
{
  using S = std::basic_string<C,T,A>;
  explicit stringifier_select(const S& t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << C('\"') << m_t << C('\"');
  }

  const S& m_t;
};

// -----------------------------------------------------------------------------
// Specialize for char* pointer-to-const and const-pointer varieties
template <>
struct stringifier_select<char*, detail::is_outputtable_tag>
{
  explicit stringifier_select(const char* const t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << '\"' << m_t << '\"';
  }

  const char* const m_t;
};

template <>
struct stringifier_select<char* const, detail::is_outputtable_tag>
  : public stringifier_select<char*, detail::is_outputtable_tag>
{
  explicit stringifier_select(char* const t)
    : stringifier_select<char*, detail::is_outputtable_tag>(t)
  {}
};

template <>
struct stringifier_select<const char*, detail::is_outputtable_tag>
  : public stringifier_select<char*, detail::is_outputtable_tag>
{
  explicit stringifier_select(const char* t)
    : stringifier_select<char*, detail::is_outputtable_tag>(t)
  {}
};

template <>
struct stringifier_select<const char* const, detail::is_outputtable_tag>
  : public stringifier_select<char*, detail::is_outputtable_tag>
{
  explicit stringifier_select(const char* const t)
    : stringifier_select<char*, detail::is_outputtable_tag>(t)
  {}
};

// -----------------------------------------------------------------------------
// Specialize for const char[] and char[]
template <size_t N>
struct stringifier_select<char[N], detail::is_outputtable_tag>
{
  using S = char[N];
  explicit stringifier_select(const S& t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << '\"' << m_t << '\"';
  }

  const S& m_t;
};

template <size_t N>
struct stringifier_select<const char[N], detail::is_outputtable_tag>
  : public stringifier_select<char[N], detail::is_outputtable_tag>
{
  using S = typename stringifier_select<char[N], detail::is_outputtable_tag>::S;
  explicit stringifier_select(const S& t)
    : stringifier_select<char[N], detail::is_outputtable_tag>(t)
  {}
};

// -----------------------------------------------------------------------------
// Specialize for arrays
namespace detail
{
  template <typename T>
  std::ostream& output_iterable(std::ostream& s, const T& t)
  {
    s << iterable_opener<T>()(t);
    auto b = std::begin(t);
    auto e = std::end(t);
    if (b != e)
      s << prettyprint(*b);
    std::for_each(++b, e,
                  [&s, &t] (auto& e)
                  { s << iterable_separator<T>()(t)
                      << prettyprint(e); });
    return s << iterable_closer<T>()(t);
  }
}

template <typename T, size_t N>
struct stringifier_select<T[N], detail::is_outputtable_tag>
{
  using S = T[N];
  explicit stringifier_select(const S& t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return detail::output_iterable<T[N]>(s, m_t);
  }

  const S& m_t;
};

// -----------------------------------------------------------------------------
// Specialize for bool: do boolalpha explicitly so as not to affect stream state
template <>
struct stringifier_select<bool, detail::is_outputtable_tag>
{
  explicit stringifier_select(bool t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << (m_t ? "true" : "false");
  }

  bool m_t;
};

// -----------------------------------------------------------------------------
// Specialize for iterable, with customization of opener/closer/separator
template <typename T>
struct stringifier_select<T, detail::is_iterable_tag>
{
  explicit stringifier_select(const T& t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return detail::output_iterable<decay_t<T>>(s, m_t);
  }

  const T& m_t;
};

// -----------------------------------------------------------------------------
// Specialization for callable object
template <typename T>
struct stringifier_select<T, detail::is_callable_tag>
{
  explicit stringifier_select(T) {}
  std::ostream& output(std::ostream& s) const
  {
    return s << "<callable "
             << detail::callable_type<T>()
             << '>';
  }
};

// -----------------------------------------------------------------------------
// Specialization for unprintable object
template <typename T>
struct stringifier_select<T, detail::is_unprintable_tag>
{
  explicit stringifier_select(T) {}
  std::ostream& output(std::ostream& s) const
  {
    return s << detail::unprintable_type<T>();
  }
};

// -----------------------------------------------------------------------------
// Specialization for enum
template <typename T>
struct stringifier_select<T, detail::is_enum_tag>
{
  explicit stringifier_select(T t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << static_cast<std::underlying_type_t<T>>(m_t);
  }

  T m_t;
};

// -----------------------------------------------------------------------------
// Specialization for pair
template <typename T>
struct stringifier_select<T, detail::is_pair_tag>
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
// Specialization for tuple
namespace detail
{
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
} // detail

template <typename T>
struct stringifier_select<T, detail::is_tuple_tag>
{
  explicit stringifier_select(const T& t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    s << '(';
    detail::for_each_in_tuple(m_t,
                              [&s] (auto& e, size_t i)
                              { if (i > 0) s << ',';
                                s << prettyprint(e); });
    return s << ')';
  }

  const T& m_t;
};

#undef SFINAE_DETECT
