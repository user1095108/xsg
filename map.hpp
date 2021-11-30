#ifndef XSG_MAP_HPP
# define XSG_MAP_HPP
# pragma once

#include <vector>

#include "utils.hpp"

#include "mapiterator.hpp"

namespace xsg
{

template <typename Key, typename Value,
  class Compare = std::compare_three_way>
class map
{
public:
  struct node;

  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<Key const, Value>;

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
    using value_type = map::value_type;

    struct empty_t{};

    static constinit inline Compare const cmp{};

    std::uintptr_t l_, r_;
    value_type kv_;

    explicit node(auto&& k) noexcept(noexcept(Key())):
      kv_(std::forward<decltype(k)>(k), Value())
    {
    }

    explicit node(auto&& k, auto&& v) noexcept(noexcept(Key(), Value())):
      kv_(std::forward<decltype(k)>(k), std::forward<decltype(v)>(v))
    {
    }

    //
    auto&& key() const noexcept { return std::get<0>(kv_); }

    //
    static auto emplace(auto& r, auto&& a, auto&& v)
    {
      enum Direction: bool { LEFT, RIGHT };

      bool s{}; // success
      node* q, *qp;

      key_type k(std::forward<decltype(a)>(a));

      auto const create_node([&](decltype(q) const p)
        {
          node* q;

          if constexpr(std::is_same_v<decltype(v), empty_t&&>)
          {
            q = new node(std::move(k));
          }
          else
          {
            q = new node(std::move(k), std::forward<decltype(v)>(v));
          }

          q->l_ = q->r_ = detail::conv(p);

          return q;
        }
      );

      auto const f([&](auto&& f, auto const n, decltype(n) p,
        enum Direction const d) -> size_type
        {
          size_type sl, sr;

          if (auto const c(cmp(k, n->key())); c < 0)
          {
            if (auto const l(detail::left_node(n, p)); l)
            {
              if (auto const sz(f(f, l, n, LEFT)); sz)
              {
                sl = sz;
              }
              else
              {
                return {};
              }
            }
            else
            {
              sl = s = (q = create_node(qp = n));
              n->l_ = detail::conv(q, p);
            }

            sr = detail::size(detail::right_node(n, p), n);
          }
          else if (c > 0)
          {
            if (auto const r(detail::right_node(n, p)); r)
            {
              if (auto const sz(f(f, r, n, RIGHT)); sz)
              {
                sr = sz;
              }
              else
              {
                return {};
              }
            }
            else
            {
              sr = s = (q = create_node(qp = n));
              n->r_ = detail::conv(q, p);
            }

            sl = detail::size(detail::left_node(n, p), n);
          }
          else
          {
            qp = p;
            q = n;

            return {};
          }

          //
          if (auto const s(1 + sl + sr), S(2 * s);
            (3 * sl > S) || (3 * sr > S))
          {
            if (auto const nn(rebuild(n, p, q, qp)); p)
            {
              d ?
                p->r_ = detail::conv(nn, detail::right_node(p, n)) :
                p->l_ = detail::conv(nn, detail::left_node(p, n));
            }
            else
            {
              r = nn;
            }

            return {};
          }
          else
          {
            return s;
          }
        }
      );

      if (r)
      {
        f(f, r, {}, {});
      }
      else
      {
        s = (r = q = create_node(qp = {}));
      }

      return std::tuple(q, qp, s);
    }

    static auto rebuild(auto const n, decltype(n) p, decltype(n) q, auto& qp)
    {
      std::vector<node*> l;
      l.reserve(1024);

      {
        auto const f([&](auto&& f, auto const n, decltype(n) const p) -> void
          {
            if (n)
            {
              f(f, detail::left_node(n, p), n);

              l.emplace_back(n);

              f(f, detail::right_node(n, p), n);
            }
          }
        );

        f(f, n, p);
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
  using this_class = map;
  node* root_{};

public:
  map() = default;

  map(std::initializer_list<value_type> il)
    noexcept(noexcept(*this = il))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = il;
  }

  map(map const& o)
    noexcept(noexcept(*this = o))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = o;
  }

  map(map&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  map(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
    requires(std::is_constructible_v<value_type, decltype(*i)>)
  {
    insert(i, j);
  }

  ~map() noexcept(noexcept(delete root_)) { detail::destroy(root_, {}); }

# include "common.hpp"

  //
  auto size() const noexcept { return detail::size(root_, {}); }

  //
  auto& operator[](Key const& k)
  {
    return std::get<1>(
      std::get<0>(
        node::emplace(root_, k, typename node::empty_t())
      )->kv_
    );
  }

  auto& operator[](Key&& k)
  {
    return std::get<1>(
      std::get<0>(
        node::emplace(root_, std::move(k), typename node::empty_t())
      )->kv_
    );
  }

  auto& at(Key const& k) noexcept
  {
    return std::get<1>(detail::find(root_, k)->kv_);
  }

  auto const& at(Key const& k) const noexcept
  {
    return std::get<1>(detail::find(root_, k)->kv_);
  }

  //
  size_type count(Key const& k) const noexcept
  {
    return bool(detail::find(root_, k));
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
    auto const [e, g](detail::equal_range(root_, k));
    return std::pair(iterator(&root_, e), iterator(&root_, g));
  }

  auto equal_range(Key const& k) const noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));
    return std::pair(const_iterator(&root_, e), const_iterator(&root_, g));
  }

  auto equal_range(auto const& k) noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));
    return std::pair(iterator(&root_, e), iterator(&root_, g));
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));
    return std::pair(const_iterator(&root_, e), const_iterator(&root_, g));
  }

  //
  size_type erase(Key const& k)
  {
    return bool(std::get<0>(detail::erase(root_, k)));
  }

  iterator erase(const_iterator const i)
  {
    return {
      &root_,
      detail::erase(
        root_,
        const_cast<node*>(i.n()),
        const_cast<node*>(i.p())
      )
    };
  }

  iterator erase(std::initializer_list<const_iterator> il)
  {
    iterator r;

    std::for_each(
      il.begin(),
      il.end(),
      [&](auto const i)
      {
        r = {&root_, detail::erase(root_, std::get<0>(*i))};
      }
    );

    return r;
  }

  //
  auto insert(value_type const& v)
  {
    auto const [n, p, s](
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    );

    return std::tuple(iterator(&root_, n, p), s);
  }

  auto insert(value_type&& v)
  {
    auto const [n, p, s](
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    );

    return std::tuple(iterator(&root_, n, p), s);
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
  {
    std::for_each(
      i,
      j,
      [&](auto&& v)
      {
        emplace(std::get<0>(v), std::get<1>(v));
      }
    );
  }

  //
  auto insert_or_assign(key_type const& k, auto&& v)
  {
    auto const [n, p, s](
      node::emplace(root_, k, std::forward<decltype(v)>(v))
    );

    if (!s)
    {
      std::get<1>(n->kv_) = std::forward<decltype(v)>(v);
    }

    return std::tuple(iterator(&root_, n, p), s);
  }

  auto insert_or_assign(key_type&& k, auto&& v)
  {
    auto const [n, p, s](
      node::emplace(root_, std::move(k), std::forward<decltype(v)>(v))
    );

    if (!s)
    {
      std::get<1>(n->kv_) = std::forward<decltype(v)>(v);
    }

    return std::tuple(iterator(&root_, n, p), s);
  }
};

}

#endif // XSG_MAP_HPP
