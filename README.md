# xsg
c++ associative containers based on the XOR [scapegoat tree](https://en.wikipedia.org/wiki/Scapegoat_tree). XOR [BST](https://en.wikipedia.org/wiki/Binary_search_tree) trees are cousins of the [XOR linked list](https://en.wikipedia.org/wiki/XOR_linked_list). Eye-popping performance and modest resource requirements make XOR [scapegoat trees](https://en.wikipedia.org/wiki/Scapegoat_tree) stand out of the [BST](https://en.wikipedia.org/wiki/Binary_search_tree) crowd.

All iterators, but not references and pointers, are invalidated when adding to or erasing from XOR [binary trees](https://en.wikipedia.org/wiki/Binary_tree). You will be able to get away dereferencing invalidated iterators, but you won't be able to iterate with them.

# how XOR binary trees work
Left and right node addresses of each node are XORed with the parent node address. The address of the parent of the root node is set arbitrarily (e.g. 0). When traversing an XOR [BST](https://en.wikipedia.org/wiki/Binary_search_tree) we need a pair `(parent, child)`. To obtain the address of a grandparent node, we first make a comparison of a node's key with the key of its parent, then, based on this comparison, XOR either the left or the right node address in the parent with this node's address. To obtain the left and right node addresses of a node we XOR them with the address of the parent. This way we can traverse an XOR [binary tree](https://en.wikipedia.org/wiki/Binary_tree) in two directions, away and towards the root node.

Since rebalancing can change node parent-child relations, all iterators are invalidated after adding or erasing from an XOR [binary tree](https://en.wikipedia.org/wiki/Binary_tree).

# build instructions
    g++ -std=c++20 -Ofast set.cpp -o s
    g++ -std=c++20 -Ofast map.cpp -o m
