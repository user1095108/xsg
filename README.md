# xsg
XOR [BST](https://en.wikipedia.org/wiki/Binary_search_tree) implementations are related to the [XOR linked list](https://en.wikipedia.org/wiki/XOR_linked_list), a [doubly linked list](https://en.wikipedia.org/wiki/Doubly_linked_list) variant, from where we borrow the idea about how links between nodes are to be implemented.

Modest resource requirements and simplicity make XOR [scapegoat trees](https://en.wikipedia.org/wiki/Scapegoat_tree) stand out of the [BST](https://en.wikipedia.org/wiki/Binary_search_tree) crowd. All iterators (except `end()` iterators), but not references and pointers, are invalidated, after inserting or erasing from this XOR [scapegoat tree](https://en.wikipedia.org/wiki/Scapegoat_tree) implementation. You can dereference invalidated iterators, except the erased iterator, but you cannot iterate with them. `end()` iterators are constant and always valid, but dereferencing them results in undefined behavior.

# how XOR BST trees work
Left and right node addresses of each node are XORed with the parent node's address. The address of the parent of the root node is set arbitrarily (e.g. 0). To traverse an XOR [BST](https://en.wikipedia.org/wiki/Binary_search_tree) we need a `(node, parent_node) = (n, p)` pointer pair. To obtain the address of a grandparent node, we first make a comparison of a node's key with the key of its parent, then, based on this comparison, we XOR either the left or the right link in the parent with the node's address, thereby obtaining `g`. To obtain the left and right node addresses of a node's children we XOR the left and right links with the address of the parent. This way we can traverse an XOR [BST](https://en.wikipedia.org/wiki/Binary_search_tree) in two directions, away and towards the root node.

![example.svg](example.svg?raw=true)

Since tree rebalancing can alter node parent-child relations, all iterators, except `end()` iterators, are invalidated, after inserting or erasing a node.

# evaluation of the XOR scapegoat BST
Important advantages of scapegoat BSTs over other [BST](https://en.wikipedia.org/wiki/Binary_search_tree)s are simplicity and reduced memory consumption. XOR scapegoat BSTs introduce additional complexity with an associated performance hit, compared to [plain scapegoat BSTs](https://github.com/user1095108/sg); insertion and erasure by key can perform worse than when using a plain scapegoat BST. This leaves tree traversal and erasure by iterator as the only advantages the XOR scapegoat BST can offer.

This implementation is usually outperformed by `std::` associative containers, as it trades performance for a smaller memory footprint and binary size.

# build instructions
    git submodule update --init
    g++ -std=c++20 -Ofast set.cpp -o s
    g++ -std=c++20 -Ofast map.cpp -o m
