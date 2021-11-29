#include <chrono>
#include <iostream>

#include "map.hpp"

using namespace std::literals::string_literals;

//////////////////////////////////////////////////////////////////////////////
void dump(auto n, decltype(n) p)
{
  std::vector<std::pair<decltype(n), decltype(n)>> q{{n, p}};

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

        std::cout << '(' << n->kv_.first << ',' << n->kv_.second << ')';
      }
      else
      {
        std::cout << "(null)";
      }

      std::cout << ' ';
    }

    std::cout << std::endl;
  }
  while (q.size() && std::any_of(q.cbegin(), q.cend(),
    [](auto&& p) noexcept { return std::get<0>(p); }));
}

//////////////////////////////////////////////////////////////////////////////
int main()
{
  xsg::map<std::string, int> st{
    {"a", -1},
    {"c", 1}
  };

  st.insert_or_assign("b"s, 0);

  st.emplace("d"s, 2);
  st.insert({"e", 3});
  st["f"s] = 4;

  /*
  for (std::size_t i{}; 1000 != i; ++i)
  {
    st.emplace(std::string(rand() % 9 + 1, 48 + rand() % (123 - 48)), i);
  }
  */

  dump(st.root(), {});

  std::cout << "height: " << xsg::detail::height(st.root(), {}) << std::endl;
  std::cout << "size: " << st.size() << std::endl;
  std::cout << (st == st) << std::endl;

  st.erase(st.cbegin());

  std::for_each(
    st.crbegin(),
    st.crend(),
    [](auto&& p) noexcept
    {
      std::cout << "(" << p.first << " " << p.second << ")" << std::endl;
    }
  );

  srandom(time(nullptr));

	using timer_t = std::chrono::high_resolution_clock;

  auto const t0(timer_t::now());

  for (std::size_t i{}; 10000 != i; ++i)
  {
    st.emplace(std::string(rand() % 9 + 1, 48 + rand() % (123 - 48)), i);
  }

  while (st.size())
  {
    st.erase(std::next(st.begin(), random() % st.size()));
  }

  std::chrono::nanoseconds const d(timer_t::now() - t0);

  std::cout << d.count() << std::endl;

  return 0;
}
