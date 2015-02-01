#include <deque>
#include <iostream>
#include <vector>
using namespace std;

#include "prettyprint.h"

struct Foo {};

struct Bar
{
  void operator()() {};
};

void foobar()
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

template <typename T>
struct iterable_opener<deque<T>>
{
  constexpr static const char* value = ">";
};

template <typename T>
struct iterable_closer<deque<T>>
{
  constexpr static const char* value = ">";
};

int main(int argc, char* argv[])
{
  vector<int> v{1,2,3};
  cout << prettyprint(v) << endl; // {1,2,3}
  deque<int> d{1,2,3};
  cout << prettyprint(d) << endl; // >1,2,3>

  cout << prettyprint(true) << endl; // true
  cout << prettyprint(BAR) << endl; // 1
  cout << prettyprint(Garply::BAZ) << endl; // 2
  cout << prettyprint(nullptr) << endl; // nullptr
  cout << prettyprint("Hello, world!") << endl; // Hello, world!
  cout << prettyprint(Foo()) << endl; // <object>
  cout << prettyprint(U()) << endl; // <union>
  cout << prettyprint(make_pair(1,2)) << endl; // (1,2)
  cout << prettyprint(make_tuple("Hello", 42)) << endl; // (Hello, 42)
  cout << prettyprint(Bar()) << endl; // <callable object>

  std::function<void()> f = [](){};
  cout << prettyprint(f) << endl; // <std::function>

  cout << prettyprint([](){}) << endl; // <callable object>
  cout << prettyprint(foobar) << endl; // <function>

  return 0;
}
