#pragma once

#include <algorithm>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
template <typename...>
using void_t = void;

#define SFINAE_DETECT(name, expr)                                       \
  template <typename T>                                                 \
  using name##_t = decltype(expr);                                      \
  template <typename T, typename = void>                                \
  struct has_##name : public std::false_type {};                        \
  template <typename T>                                                 \
  struct has_##name<T, void_t<name##_t<T>>> : public std::true_type {};

SFINAE_DETECT(operator_output, std::cout << std::declval<T>())
SFINAE_DETECT(begin, std::declval<T>().begin())
SFINAE_DETECT(end, std::declval<T>().end())

template<typename T>
using is_iterable = typename std::conditional<
  has_begin<T>::value && has_end<T>::value,
  std::true_type, std::false_type>::type;

// -----------------------------------------------------------------------------
template <typename T, bool, bool>
struct stringifier_select;

template <typename T>
using stringifier = stringifier_select<T,
                                       has_operator_output<T>::value,
                                       is_iterable<T>::value>;

template <typename T>
std::ostream& operator<<(std::ostream& s, const stringifier<T>& t)
{
  return t.output(s);
}

template <typename T>
stringifier<T> prettyprint(T&& t)
{
  return stringifier<T>(std::forward<T>(t));
}

// -----------------------------------------------------------------------------
// default: not stringifiable
template <typename T, bool, bool>
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
template <typename T, bool b>
struct stringifier_select<T, true, b>
{
  explicit stringifier_select(const T& t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    return s << m_t;
  }

  const T& m_t;
};

// -----------------------------------------------------------------------------
// If t is iterable, do that, with customization points for
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
struct stringifier_select<T, false, true>
{
  explicit stringifier_select(const T& t) : m_t(t) {}

  std::ostream& output(std::ostream& s) const
  {
    s << iterable_opener<T>::value;
    auto b = m_t.begin();
    auto e = m_t.end();
    if (e != b)
      s << prettyprint(*b);
    std::for_each(++b, e,
                  [&s] (auto& e) { s << iterable_separator<T>::value
                                     << prettyprint(e); });
    return s << iterable_closer<T>::value;
  }

  const T& m_t;
};
