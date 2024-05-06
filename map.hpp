#ifndef XSG_MAP_HPP
# define XSG_MAP_HPP
# pragma once

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

  using difference_type = detail::difference_type;
  using size_type = detail::size_type;
  using reference = value_type&;
  using const_reference = value_type const&;

  using iterator = mapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_iterator = mapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  struct node
  {
    using value_type = map::value_type;

    static constinit inline Compare const cmp;

    std::uintptr_t l_, r_;
    value_type kv_;

    explicit node(auto&& k, auto&& ...a)
      noexcept(noexcept(
          value_type(
            std::piecewise_construct_t{},
            std::forward_as_tuple(std::forward<decltype(k)>(k)),
            std::forward_as_tuple(std::forward<decltype(a)>(a)...)
          )
        )
      ):
      kv_(
        std::piecewise_construct_t{},
        std::forward_as_tuple(std::forward<decltype(k)>(k)),
        std::forward_as_tuple(std::forward<decltype(a)>(a)...)
      )
    {
    }

    //
    auto& key() const noexcept { return std::get<0>(kv_); }

    //
    static auto emplace(auto& r, auto&& k, auto&& ...a)
      noexcept(noexcept(new node(std::forward<decltype(k)>(k),
        std::forward<decltype(a)>(a)...)))
      requires(detail::Comparable<Compare, decltype(k), key_type>)
    {
      auto const create_node([&](node* const p)
        noexcept(noexcept(new node(std::forward<decltype(k)>(k),
          std::forward<decltype(a)>(a)...)))
        {
          auto const q(new node(std::forward<decltype(k)>(k),
            std::forward<decltype(a)>(a)...));

          q->l_ = q->r_ = detail::conv(p);

          return q;
        }
      );

      return r ? detail::emplace(r, k, create_node) :
        std::tuple<node*, node*, bool>(r = create_node({}), {}, true);
    }
  };

private:
  using this_class = map;
  node* root_{};

public:
  map() = default;

  map(map const& o)
    noexcept(noexcept(map(o.begin(), o.end())))
    requires(std::is_copy_constructible_v<value_type>):
    map(o.begin(), o.end())
  {
  }

  map(map&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  map(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
  {
    insert(i, j);
  }

  map(std::initializer_list<value_type> l)
    noexcept(noexcept(map(l.begin(), l.end()))):
    map(l.begin(), l.end())
  {
  }

  ~map() noexcept(noexcept(detail::destroy(root_, {})))
  {
    detail::destroy(root_, {});
  }

# include "common.hpp"

  //
  auto size() const noexcept { return detail::size(root_, {}); }

  //
  template <int = 0>
  auto& operator[](auto&& k)
    noexcept(noexcept(node::emplace(root_, std::forward<decltype(k)>(k))))
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return std::get<1>(std::get<0>(
      node::emplace(root_, std::forward<decltype(k)>(k)))->kv_);
  }

  auto& operator[](key_type k)
    noexcept(noexcept(operator[]<0>(std::move(k))))
  {
    return operator[]<0>(std::move(k));
  }

  template <int = 0>
  auto& operator[](auto const& k) const
    noexcept(noexcept(at(k)))
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return at(k);
  }

  auto& operator[](key_type const k) const
    noexcept(noexcept(operator[]<0>(k)))
  {
    return operator[]<0>(k);
  }

  template <int = 0>
  auto& at(auto&& k) noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return std::get<1>(detail::find(root_, k)->kv_);
  }

  auto& at(key_type k) noexcept { return at<0>(std::move(k)); }

  template <int = 0>
  auto const& at(auto&& k) const noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return std::get<1>(detail::find(root_, std::forward<decltype(k)>(k))->kv_);
  }

  auto& at(key_type k) const noexcept { return at<0>(std::move(k)); }

  //
  template <int = 0>
  size_type count(auto const& k) const noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return bool(detail::find(root_, {}, k));
  }

  auto count(key_type const k) const noexcept { return count<0>(k); }

  //
  template <int = 0>
  auto emplace(auto&& k, auto&& ...a)
    noexcept(noexcept(
        node::emplace(
          root_,
          std::forward<decltype(k)>(k),
          std::forward<decltype(a)>(a)...
        )
      )
    )
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [n, p, s](
      node::emplace(
        root_,
        std::forward<decltype(k)>(k),
        std::forward<decltype(a)>(a)...
      )
    );

    return std::pair(iterator(&root_, n, p), s);
  }

  auto emplace(key_type k, auto&& ...a)
    noexcept(noexcept(
        emplace<0>(std::move(k), std::forward<decltype(a)>(a)...)
      )
    )
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return emplace<0>(std::move(k), std::forward<decltype(a)>(a)...);
  }

  //
  template <int = 0>
  auto equal_range(auto&& k) noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [nl, g](
      detail::equal_range(root_, {}, std::forward<decltype(k)>(k))
    );

    return std::pair(iterator(&root_, nl), iterator(&root_, g));
  }

  auto equal_range(key_type k) noexcept
  {
    return equal_range<0>(std::move(k));
  }

  template <int = 0>
  auto equal_range(auto&& k) const noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [nl, g](
      detail::equal_range(root_, {}, std::forward<decltype(k)>(k))
    );

    return std::pair(const_iterator(&root_, nl), const_iterator(&root_, g));
  }

  auto equal_range(key_type k) const noexcept
  {
    return equal_range<0>(std::move(k));
  }

  //
  template <int = 0>
  size_type erase(auto&& k)
    noexcept(noexcept(detail::erase(root_, k)))
    requires(detail::Comparable<Compare, decltype(k), key_type> &&
      !std::convertible_to<decltype(k), const_iterator>)
  {
    return bool(
        std::get<0>(detail::erase(root_, std::forward<decltype(k)>(k)))
      );
  }

  auto erase(key_type k) noexcept(noexcept(erase<0>(std::move(k))))
  {
    return erase<0>(std::move(k));
  }

  iterator erase(const_iterator const i)
    noexcept(noexcept(
        detail::erase(
          root_,
          const_cast<node*>(i.n_),
          const_cast<node*>(i.p_)
        )
      )
    )
  {
    return {
        &root_,
        detail::erase(
          root_,
          const_cast<node*>(i.n_),
          const_cast<node*>(i.p_)
        )
      };
  }

  //
  template <int = 0>
  auto insert(auto&& v)
    noexcept(noexcept(
        node::emplace(
          root_,
          std::get<0>(std::forward<decltype(v)>(v)),
          std::get<1>(std::forward<decltype(v)>(v))
        )
      )
    )
    requires(
      detail::Comparable<
        Compare,
        decltype(std::get<0>(std::forward<decltype(v)>(v))),
        key_type
      >
    )
  {
    auto const [n, p, s](
      node::emplace(
        root_,
        std::get<0>(std::forward<decltype(v)>(v)),
        std::get<1>(std::forward<decltype(v)>(v))
      )
    );

    return std::pair(iterator(&root_, n, p), s);
  }

  auto insert(value_type const v)
    noexcept(noexcept(insert<0>(v)))
  {
    return insert<0>(v);
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(emplace(std::get<0>(*i), std::get<1>(*i))))
  {
    std::for_each(
      i,
      j,
      [&](auto&& v)
        noexcept(noexcept(
            emplace(
              std::get<0>(std::forward<decltype(v)>(v)),
              std::get<1>(std::forward<decltype(v)>(v))
            )
          )
        )
      {
        emplace(
          std::get<0>(std::forward<decltype(v)>(v)),
          std::get<1>(std::forward<decltype(v)>(v))
        );
      }
    );
  }

  //
  template <int = 0>
  auto insert_or_assign(auto&& k, auto&& ...a)
    noexcept(noexcept(
        node::emplace(
          root_,
          std::forward<decltype(k)>(k),
          std::forward<decltype(a)>(a)...
        )
      )
    )
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [n, p, s](
      node::emplace(
        root_,
        std::forward<decltype(k)>(k),
        std::forward<decltype(a)>(a)...
      )
    );

    if (!s)
    {
      if constexpr(sizeof...(a))
      {
        std::get<1>(n->kv_) = mapped_type(std::forward<decltype(a)>(a)...);
      }
      else
      {
        std::get<1>(n->kv_) = (std::forward<decltype(a)>(a), ...);
      }
    }

    return std::pair(iterator(&root_, n, p), s);
  }

  auto insert_or_assign(key_type k, auto&& ...a)
    noexcept(noexcept(insert_or_assign<0>(std::move(k),
      std::forward<decltype(a)>(a)...)))
  {
    return insert_or_assign<0>(std::move(k), std::forward<decltype(a)>(a)...);
  }
};

//////////////////////////////////////////////////////////////////////////////
template <int = 0, typename K, typename V, class C>
inline auto erase(map<K, V, C>& c, auto const& k)
  noexcept(noexcept(c.erase(K(k))))
  requires(!detail::Comparable<C, decltype(k), K>)
{
  return c.erase(K(k));
}

template <int = 0, typename K, typename V, class C>
inline auto erase(map<K, V, C>& c, auto const& k)
  noexcept(noexcept(c.erase(k)))
  requires(detail::Comparable<C, decltype(k), K>)
{
  return c.erase(k);
}

template <typename K, typename V, class C>
inline auto erase(map<K, V, C>& c, K const k)
  noexcept(noexcept(erase<0>(c, k)))
{
  return erase<0>(c, k);
}

template <typename K, typename V, class C>
inline auto erase_if(map<K, V, C>& c, auto pred)
  noexcept(noexcept(pred(std::declval<K const&>()), c.erase(c.begin())))
{
  typename std::remove_reference_t<decltype(c)>::size_type r{};

  for (auto i(c.begin()); i; pred(*i) ? ++r, i = c.erase(i) : ++i);

  return r;
}

//////////////////////////////////////////////////////////////////////////////
template <typename K, typename V, class C>
inline void swap(map<K, V, C>& l, decltype(l) r) noexcept { l.swap(r); }

}

#endif // XSG_MAP_HPP
