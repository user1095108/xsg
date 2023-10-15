// self-assign neglected
auto& operator=(this_class const& o)
  noexcept(noexcept(clear()) && noexcept(insert(o.begin(), o.end())))
  requires(std::is_copy_constructible_v<value_type>)
{
  clear();
  insert(o.begin(), o.end());

  return *this;
}

auto& operator=(this_class&& o)
  noexcept(noexcept(detail::destroy(root_, {})))
{
  detail::destroy(root_, {});

  root_ = o.root_;
  o.root_ = {};

  return *this;
}

auto& operator=(std::initializer_list<value_type> l)
  noexcept(noexcept(clear()) && noexcept(insert(l.begin(), l.end())))
  requires(std::is_copy_constructible_v<value_type>)
{
  clear();
  insert(l.begin(), l.end());

  return *this;
}

//
friend bool operator==(this_class const& l, this_class const& r)
  noexcept(noexcept(
      std::equal(
        l.begin(), l.end(),
        r.begin(), r.end(),
        [](auto&&, auto&&) noexcept { return true; }
      )
    )
  )
  requires(requires{
      std::equal(
        l.begin(), l.end(),
        r.begin(), r.end(),
        [](auto&& a, auto&& b) noexcept
        {
          return std::remove_cvref_t<decltype(l)>::node::cmp(a, b) == 0;
        }
      );
    }
  )
{
  return std::equal(
    l.begin(), l.end(),
    r.begin(), r.end(),
    [](auto&& a, auto&& b) noexcept
    {
      return std::remove_cvref_t<decltype(l)>::node::cmp(a, b) == 0;
    }
  );
}

friend auto operator<=>(this_class const& l, this_class const& r)
  noexcept(noexcept(
      std::lexicographical_compare_three_way(
        l.begin(), l.end(),
        r.begin(), r.end(),
        std::remove_cvref_t<decltype(l)>::node::cmp
      )
    )
  )
  requires(requires{
    std::lexicographical_compare_three_way(
        l.begin(), l.end(),
        r.begin(), r.end(),
        std::remove_cvref_t<decltype(l)>::node::cmp
      );
    }
  )
{
  return std::lexicographical_compare_three_way(
    l.begin(), l.end(),
    r.begin(), r.end(),
    std::remove_cvref_t<decltype(l)>::node::cmp
  );
}

// iterators
iterator begin() noexcept
{
  return root_ ?
    iterator(&root_, detail::first_node(root_, {})) :
    iterator(&root_);
}

iterator end() noexcept { return iterator(&root_); }

// const iterators
const_iterator begin() const noexcept
{
  return root_ ?
    const_iterator(&root_, detail::first_node(root_, {})) :
    const_iterator(&root_);
}

const_iterator end() const noexcept { return const_iterator(&root_); }

const_iterator cbegin() const noexcept
{
  return root_ ?
    const_iterator(&root_, detail::first_node(root_, {})) :
    const_iterator(&root_);
}

const_iterator cend() const noexcept { return const_iterator(&root_); }

// reverse iterators
reverse_iterator rbegin() noexcept
{
  return reverse_iterator(iterator(&root_));
}

reverse_iterator rend() noexcept
{
  return root_ ?
    reverse_iterator(iterator(&root_, detail::first_node(root_, {}))) :
    reverse_iterator(iterator(&root_));
}

// const reverse iterators
const_reverse_iterator crbegin() const noexcept
{
  return const_reverse_iterator(const_iterator(&root_));
}

const_reverse_iterator crend() const noexcept
{
  return root_ ?
    const_reverse_iterator(
      const_iterator(&root_, detail::first_node(root_, {}))
    ) :
    const_reverse_iterator(const_iterator(&root_));
}

//
auto root() const noexcept { return root_; }

//
static constexpr size_type max_size() noexcept { return ~size_type{} / 3; }

void clear() noexcept(noexcept(delete root_))
{
  detail::destroy(root_, {});
  root_ = {};
}

bool empty() const noexcept { return !root_; }
void swap(this_class& o) noexcept { std::swap(root_, o.root_); }

//
template <int = 0>
bool contains(auto&& k) const noexcept
  requires(detail::Comparable<Compare, key_type, decltype(k)>)
{
  return std::get<0>(detail::find(root_, {}, k));
}

auto contains(key_type k) const noexcept { return contains<0>(std::move(k)); }

//
iterator erase(const_iterator a, const_iterator const b)
  noexcept(noexcept(erase(a)))
{
  iterator i(b);

  for (; a != b; i = erase(a), a = i);

  return i;
}

//
template <int = 0>
iterator find(auto&& k) noexcept
  requires(detail::Comparable<Compare, key_type, decltype(k)>)
{
  return {&root_, detail::find(root_, {}, k)};
}

auto find(key_type k) noexcept { return find<0>(std::move(k)); }

template <int = 0>
const_iterator find(auto&& k) const noexcept
  requires(detail::Comparable<Compare, key_type, decltype(k)>)
{
  return {&root_, detail::find(root_, {}, k)};
}

auto find(key_type k) const noexcept { return find<0>(std::move(k)); }

//
void insert(std::initializer_list<value_type> l)
  noexcept(noexcept(insert(l.begin(), l.end())))
  requires(std::is_copy_constructible_v<value_type>)
{
  insert(l.begin(), l.end());
}

//
template <int = 0>
iterator lower_bound(auto&& k) noexcept
{
  return std::get<0>(equal_range(k));
}

auto lower_bound(key_type k) noexcept
{
  return lower_bound<0>(std::move(k));
}

template <int = 0>
const_iterator lower_bound(auto&& k) const noexcept
{
  return std::get<0>(equal_range(k));
}

auto lower_bound(key_type k) const noexcept
{
  return lower_bound<0>(std::move(k));
}

//
template <int = 0>
iterator upper_bound(auto&& k) noexcept
{
  return std::get<1>(equal_range(k));
}

auto upper_bound(key_type k) noexcept
{
  return upper_bound<0>(std::move(k));
}

template <int = 0>
const_iterator upper_bound(auto&& k) const noexcept
{
  return std::get<1>(equal_range(k));
}

auto upper_bound(key_type k) const noexcept
{
  return upper_bound<0>(std::move(k));
}
