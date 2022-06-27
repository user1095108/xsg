#include <iostream>

#include "xl/list.hpp"

#include "set.hpp"

//////////////////////////////////////////////////////////////////////////////
void dump(auto n, decltype(n) p)
{
  xl::list<std::pair<decltype(n), decltype(n)>> q{{n, p}};

  do
  {
    for (auto m(q.size()); m--;)
    {
      auto const [n, p](q.front());
      q.erase(q.begin());

      if (n)
      {
        q.insert(
          q.end(),
          {
            {xsg::detail::left_node(n, p), n},
            {xsg::detail::right_node(n, p), n}
          }
        );

        std::cout << '{' << n->kv_ << '}';
      }
      else
      {
        std::cout << "{null}";
      }

      std::cout << ' ';
    }

    std::cout << std::endl;
  }
  while (q.size() && std::any_of(q.cbegin(), q.cend(),
    [](auto const p) noexcept { return std::get<0>(p); }));
}

//////////////////////////////////////////////////////////////////////////////
int main()
{
  xsg::set<int> s{4, 3, 2, 1};

  dump(s.root(), {});

  std::cout << s.contains(3u) << std::endl;
  std::cout << s.contains(100ull) << std::endl;

  std::for_each(
    s.cbegin(),
    s.cend(),
    [](auto&& p) noexcept
    {
      std::cout << p << std::endl;
    }
  );

  std::cout << "eq: " << (s == xsg::set<int>{1, 2, 3, 4}) << std::endl;
  //s.erase(std::next(s.cbegin()));
  s.erase(std::next(s.cbegin()));
  s.erase(std::prev(s.cend()));

  dump(s.root(), {});

  std::for_each(
    s.cbegin(),
    s.cend(),
    [](auto&& p) noexcept
    {
      std::cout << p << std::endl;
    }
  );

  return 0;
}
