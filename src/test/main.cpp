#include <array>
#include <cassert>
#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "prettyprint.h"

struct Foo {};

struct Bar
{
  void operator()() {}
};

struct Baz
{
  void foo() {}
  void operator()() {}
};

ostream& operator<<(ostream& s, const Baz&)
{
  return s << "Baz";
}

void foobar()
{
}

void foobarbind(int, int)
{
}

union U {};

enum Quux
{
  FOO,
  BAR,
  BAZ
};

enum class Garply
{
  FOO,
  BAR,
  BAZ
};

struct deque_formatter : public default_formatter
{
  // use [] for deques
  template <typename T>
  constexpr const char* opener(const deque<T>&) const
  { return ">"; }

  template <typename T>
  constexpr const char* closer(const deque<T>&) const
  { return ">"; }
};

#define TEST(decl, expected, ...)               \
  do {                                          \
    ostringstream oss;                          \
    decl;                                       \
    oss << prettyprint(__VA_ARGS__);            \
    assert(oss.str() == expected);              \
  } while (false)

int main(int, char* [])
{
  // zero-extent array
  TEST(extern int x0[], "<array (unknown bounds)>", x0);
  TEST(extern const int x1[], "<array (unknown bounds)>", x1);

  // array with extents
  TEST(int x[1] = {1}, "[1]", x);
  TEST(const int x[2] = {1}, "[1,0]", x);

  // array without extents
  TEST(int x[] = {1}, "[1]", x);
  TEST(const int x[] = {1}, "[1]", x);

  // std::array
  array<int, 3> a{{1,2,3}};
  TEST(a, "[1,2,3]", a);

  // vector
  TEST(vector<int> x{1}, "[1]", x);
  TEST(vector<int> x{}, "[]", x);

  // deque and custom formatter
  TEST(deque<int> x{1}, "{1}", x);
  TEST(deque<int> x{1}, ">1>", x, deque_formatter());

  // bool
  TEST(bool b = true, "true", b);
  TEST(bool b = false, "false", b);

  // enum and enum class
  TEST(, "1", BAR);
  TEST(, "2", Garply::BAZ);

  // nullptr
  TEST(, "<nullptr>", nullptr);

  // various char pointers
  char hw[] = "Hello, world!";
  TEST(char* x = hw, "\"Hello, world!\"", x);
  TEST(const char* x = "Hello, world!", "\"Hello, world!\"", x);
  TEST(char* const x = hw, "\"Hello, world!\"", x);
  TEST(const char* const x = hw, "\"Hello, world!\"", x);

  // char arrays and literals
  TEST(char x[] = "Hello, world!", "\"Hello, world!\"", x);
  TEST(const char x[] = "Hello, world!", "\"Hello, world!\"", x);
  TEST(, "\"Hello, world!\"", "Hello, world!");

  // strings
  TEST(string x = "Hello, world!", "\"Hello, world!\"", x);
  TEST(const string x = "Hello, world!", "\"Hello, world!\"", x);
  TEST(string x = "Hello, world!", "\"Hello, world!\"", std::move(x));

  // zero-extent char arrays
  TEST(extern char c0[], "\"Hello, world!\"", c0);
  TEST(extern const char c1[], "\"Hello, world!\"", c1);

  // unprintable aggregates
  TEST(, "<class>", Foo());
  TEST(, "<union>", U());

  // pairs and tuples
  TEST(, "(1,2)", (make_pair(1,2)));
  TEST(, "(\"Hello\",42)", (make_tuple("Hello",42)));

  // object with operator<<
  TEST(, "Baz", Baz());

  // callables
  TEST(, "<callable (function object)>", Bar());
  TEST(std::function<void()> f = [](){}, "<callable (std::function)>", f);
  TEST(, "<callable (function object)>", [](){});
  TEST(, "<callable (function)>", foobar);
  TEST(, "<callable (bind expression)>", (std::bind(&foobarbind, 1, placeholders::_1)));

  return 0;
}
