#ifndef XSG_SET_HPP
# define XSG_SET_HPP
# pragma once

#include <vector>

#include "utils.hpp"

#include "mapiterator.hpp"

namespace xsg
{

template <typename Key, class Compare = std::compare_three_way>
class set
{
public:
  struct node;

  using key_type = Key;
  using value_type = Key;

  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;
  using reference = value_type&;
  using const_reference = value_type const&;

  using const_iterator = mapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using iterator = mapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  struct node
  {
    using value_type = set::value_type;

    static constinit inline auto const cmp{Compare{}};

    std::uintptr_t l_, r_;
    Key const kv_;

    explicit node(auto&& ...a) noexcept(noexcept(Key())):
      kv_(std::forward<decltype(a)>(a)...)
    {
    }

    //
    auto&& key() const noexcept { return kv_; }

    //
    static auto emplace(auto& r, auto&& ...a)
    {
      node* q, *qp;
      bool s{}; // success

      key_type k(std::forward<decltype(a)>(a)...);

      auto const create_node([&](decltype(q) const p)
        {
          auto const q(new node(std::move(k)));
          q->l_ = q->r_ = detail::conv(p);

          return q;
        }
      );

      auto const f([&](auto&& f, auto const n, decltype(n) p) noexcept ->
        std::tuple<node*, size_type>
        {
          //
          size_type sl, sr;

          if (auto const c(cmp(k, n->key())); c < 0)
          {
            if (auto const l(detail::left_node(n, p)); l)
            {
              if (auto const [nn, sz](f(f, l, n)); nn)
              {
                n->l_ = detail::conv(nn, p);

                return {nullptr, {}};
              }
              else if (!sz)
              {
                return {nullptr, {}};
              }
              else
              {
                sl = sz;
              }
            }
            else
            {
              s = (q = create_node(n));

              n->l_ = detail::conv(q, p);

              qp = n;
              sl = 1;
            }

            sr = detail::size(detail::right_node(n, p), n);
          }
          else if (c > 0)
          {
            if (auto const r(detail::right_node(n, p)); r)
            {
              if (auto const [nn, sz](f(f, r, n)); nn)
              {
                n->r_ = detail::conv(nn, p);

                return {nullptr, {}};
              }
              else if (!sz)
              {
                return {nullptr, {}};
              }
              else
              {
                sr = sz;
              }
            }
            else
            {
              s = (q = create_node(n));

              n->r_ = detail::conv(q, p);

              qp = n;
              sr = 1;
            }

            sl = detail::size(detail::left_node(n, p), n);
          }
          else
          {
            q = n;
            qp = p;

            return {nullptr, {}};
          }

          //
          auto const s(1 + sl + sr), S(2 * s);

          return (3 * sl > S) || (3 * sr > S) ?
            std::tuple(rebuild(n, p, q, qp), std::size_t{}) :
            std::tuple(nullptr, s);
        }
      );

      if (r)
      {
        if (auto const [nn, s](f(f, r, {})); nn)
        {
          r = nn;
        }
      }
      else
      {
        s = (r = q = create_node({}));
        qp = {};
      }

      return std::tuple(q, qp, s);
    }

    static auto rebuild(auto const n, decltype(n) p, auto& qp, auto& q)
    {
      std::vector<node*> l;
      l.reserve(1024);

      {
        auto t(detail::first_node(n, p));

        do
        {
          l.emplace_back(std::get<0>(t));
        }
        while (std::get<0>(t =
          detail::next_node(n, std::get<0>(t), std::get<1>(t))));
      }

      auto const f([&](auto&& f, auto const p,
        auto const a, auto const b) noexcept -> node*
        {
          auto const i((a + b) / 2);
          auto const n(l[i]);

          if (n == q)
          {
            qp = p;
          }

          switch (b - a)
          {
            case 0:
              n->l_ = n->r_ = detail::conv(p);

              break;

            case 1:
              {
                // n - nb
                auto const nb(l[b]);

                nb->l_ = nb->r_ = detail::conv(n);
                n->l_ = detail::conv(p); n->r_ = detail::conv(nb, p);

                if (nb == q)
                {
                  qp = n;
                }
              }

              break;

            default:
              n->l_ = detail::conv(f(f, n, a, i - 1), p);
              n->r_ = detail::conv(f(f, n, i + 1, b), p);
          }

          return n;
        }
      );

      //
      return f(f, p, 0, l.size() - 1);
    }
  };

private:
  using this_class = set;
  node* root_{};

public:
  set() = default;

  set(std::initializer_list<Key> il)
    noexcept(noexcept(*this = il))
    requires(std::is_copy_constructible_v<Key>)
  {
    *this = il;
  }

  set(set const& o) 
    noexcept(noexcept(*this = o))
    requires(std::is_copy_constructible_v<Key>)
  {
    *this = o;
  }

  set(set&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  set(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
    requires(std::is_constructible_v<Key, decltype(*i)>)
  {
    insert(i, j);
  }

  ~set() noexcept(noexcept(delete root_)) { detail::destroy(root_, {}); }

# include "common.hpp"

  //
  auto size() const noexcept { return detail::size(root_, {}); }

  //
  size_type count(Key const& k) const noexcept
  {
    return bool(detail::find(root_, {}, k));
  }

  //
  auto emplace(auto&& ...a)
  {
    auto const [n, p, s](node::emplace(root_, std::forward<decltype(a)>(a)...));

    return std::tuple(iterator(&root_, n, p), s);
  }

  //
  auto equal_range(Key const& k) noexcept
  {
    auto const [e, g](detail::equal_range(root_, {}, k));

    return std::pair(
      iterator(&root_, std::get<0>(e) ? e : g),
      iterator(&root_, g)
    );
  }

  auto equal_range(Key const& k) const noexcept
  {
    auto const [e, g](detail::equal_range(root_, {}, k));

    return std::pair(
      const_iterator(&root_, std::get<0>(e) ? e : g),
      const_iterator(&root_, g)
    );
  }

  auto equal_range(auto const& k) noexcept
  {
    auto const [e, g](detail::equal_range(root_, {}, k));

    return std::pair(
      iterator(&root_, std::get<0>(e) ? e : g),
      iterator(&root_, g)
    );
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [e, g](detail::equal_range(root_, {}, k));

    return std::pair(
      const_iterator(&root_, std::get<0>(e) ? e : g),
      const_iterator(&root_, g)
    );
  }

  //
  size_type erase(Key const& k)
  {
    return bool(std::get<0>(detail::erase(root_, k)));
  }

  iterator erase(const_iterator const i)
  {
    //return {&root_, detail::erase(root_, *i)};
    return {
      &root_,
      detail::erase(
        root_,
        const_cast<node*>(i.n()),
        const_cast<node*>(i.p())
      )
    };
  }

  //
  auto insert(value_type const& v)
  {
    auto const [n, p, s](node::emplace(root_, v));

    return std::tuple(iterator(&root_, n, p), s);
  }

  auto insert(value_type&& v)
  {
    auto const [n, p, s](node::emplace(root_, std::move(v)));

    return std::tuple(iterator(&root_, n, p), s);
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
  {
    std::for_each(
      i,
      j,
      [&](auto&& v)
      {
        emplace(std::forward<decltype(v)>(v));
      }
    );
  }
};

}

#endif // XSG_SET_HPP
