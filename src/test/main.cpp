#include <deque>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "prettyprint.h"

struct Foo {};

struct Bar
{
  void operator()() {};
};

struct Baz
{
  void operator()() {};
};

ostream& operator<<(ostream& s, const Baz&)
{
  return s << "Baz";
}

void foobar()
{
}

void foobarbind(int i, int j)
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
  constexpr const char* operator()(const deque<T>&) const
  { return ">"; }
};

template <typename T>
struct iterable_closer<deque<T>>
{
  constexpr const char* operator()(const deque<T>&) const
  { return ">"; }
};

int main(int argc, char* argv[])
{
  const int x0[3] = {1,2,3};
  int x1[3] = {1,2,3};
  int x2[] = {1,2,3};
  int* x3 = x2;
  cout << prettyprint(x0) << endl; // {1,2,3}
  cout << prettyprint(x1) << endl; // {1,2,3}
  cout << prettyprint(x2) << endl; // {1,2,3}
  cout << prettyprint(x3) << endl; // some pointer

  vector<int> v{1,2,3};
  cout << prettyprint(v) << endl; // {1,2,3}
  deque<int> d{1,2,3};
  cout << prettyprint(d) << endl; // >1,2,3>

  cout << prettyprint(true) << endl; // true
  cout << prettyprint(BAR) << endl; // 1
  cout << prettyprint(Garply::BAZ) << endl; // 2
  cout << prettyprint(nullptr) << endl; // <nullptr>

  const char* hw = "Hello, world!";
  cout << prettyprint(hw) << endl; // "Hello, world!"
  cout << prettyprint(std::move(hw)) << endl; // "Hello, world!"

  char hw2[] = "Hello, world!";
  char* hw3 = hw2;
  cout << prettyprint(hw2) << endl; // "Hello, world!"
  cout << prettyprint(hw3) << endl; // "Hello, world!"

  cout << prettyprint("Hello, world!") << endl; // "Hello, world!"
  string hws("Hello, world!");
  cout << prettyprint(hws) << endl; // "Hello, world!"
  cout << prettyprint(std::move(hws)) << endl; // "Hello, world!"

  cout << prettyprint(Foo()) << endl; // <class>
  cout << prettyprint(U()) << endl; // <union>
  cout << prettyprint(make_pair(1,2)) << endl; // (1,2)
  cout << prettyprint(make_tuple("Hello", 42)) << endl; // ("Hello", 42)
  cout << prettyprint(Bar()) << endl; // <callable (function object)>
  cout << prettyprint(Baz()) << endl; // Baz

  std::function<void()> f = [](){};
  cout << prettyprint(f) << endl; // <callable (std::function)>

  cout << prettyprint([](){}) << endl; // <callable (function object)>
  cout << prettyprint(foobar) << endl; // <callable (function)>

  auto b = std::bind(&foobarbind, 1, placeholders::_1);
  cout << prettyprint(b) << endl; // <callable (bind expression)>

  return 0;
}
