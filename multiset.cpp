#include <iostream>

#include "multiset.hpp"

//////////////////////////////////////////////////////////////////////////////
void dump(auto n, decltype(n) p = {})
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

        std::cout << '{' << n->key() << '}';
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
    [](auto&& p) noexcept { return std::get<0>(p); }));
}

//////////////////////////////////////////////////////////////////////////////
int main()
{
  xsg::multiset<int> st;

  st.emplace(-1);
  st.insert(0);
  st.insert({1, 1});
  st.emplace(2);
  st.emplace(3);
  st.emplace(4);

  dump(st.root());

  std::cout << "count: " << st.count(1) << std::endl;
  std::cout << "height: " << xsg::detail::height(st.root(), {}) << std::endl;
  std::cout << "size: " << st.size() << std::endl;

  std::for_each(
    st.cbegin(),
    st.cend(),
    [](auto&& k) noexcept
    {
      std::cout << '(' << k << ')' << std::endl;
    }
  );

  return 0;
}
