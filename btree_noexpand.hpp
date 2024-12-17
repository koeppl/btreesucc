#ifndef LOAD_BALANCING_BTREE_HPP_
#define LOAD_BALANCING_BTREE_HPP_

#include <iostream>
#include <vector>
#include <cassert>

namespace lb
{
    template <class value_type,
              uint32_t M_KEY_SHARING,       // キーの共有を行う葉の最大数
              uint32_t M_LEAF,              // 葉の最大要素数
              uint32_t M_INTERNAL_CHILDREN, // 内部ノードの最大要素数 + 1（子が内部ノード）
              uint32_t M_LEAF_CHILDREN>     // 内部ノードの最大要素数 + 1 (子が葉)
    class lb_btree
    {
    public:
        lb_btree() : root(new internal_node1()) {}
        ~lb_btree()
        {
        }

        /*
         * 挿入
         */
        void insert(value_type x)
        {
            internal_node1 *new_root = root->insert(x);

            if (new_root != NULL)
            {
                root = new_root;
            }

            size_++;
        }

        void remove(value_type x)
        {
            internal_node1 *new_root = root->remove(x);

            if (new_root != NULL)
            {
                delete root;
                root = new_root;
            }

            size_--;
        }

        uint64_t byte_size()
        {
            assert(root != NULL);

            uint64_t bs = sizeof(lb_btree<value_type, M_KEY_SHARING, M_LEAF, M_INTERNAL_CHILDREN, M_LEAF_CHILDREN>);

            if (root != NULL)
                bs += root->byte_size();
            return bs;
        }

        uint64_t byte_leaf()
        {
            return sizeof(leaf_node);
        }

        uint64_t byte_internal1()
        {
            return sizeof(internal_node1);
        }

        uint64_t byte_internal2()
        {
            return sizeof(internal_node2);
        }

        uint64_t size() const
        {
            return size_;
        }

        uint64_t height() const
        {   
            assert(root != NULL);

            internal_node1 *node1 = root;
            uint64_t count = 0;
            
            while (node1->has_node1())
            {
                count++;
                node1 = static_cast<internal_node1 *>(node1->get_child(0));
            }

            return count + 3;
        }

        uint64_t internals() const
        {
            assert(root != NULL);

            std::vector<internal_node1 *> internals1 = {root};
            std::vector<internal_node2 *> internals2;

            for (uint64_t i = 0; i < internals1.size(); ++i)
            {
                for (int j = 0; j < internals1[i]->size(); ++j)
                {
                    if (internals1[i]->has_node1())
                    {
                        internals1.push_back(static_cast<internal_node1 *>(internals1[i]->get_child(j)));
                    }
                    else
                    {
                        internals2.push_back(static_cast<internal_node2 *>(internals1[i]->get_child(j)));
                    }
                }
            }

            return internals1.size() + internals2.size();
        }

        uint64_t leaves() const
        {
            assert(root != NULL);

            std::vector<internal_node1 *> internals1 = {root};
            std::vector<internal_node2 *> internals2;

            for (uint64_t i = 0; i < internals1.size(); ++i)
            {
                for (int j = 0; j < internals1[i]->size(); ++j)
                {
                    if (internals1[i]->has_node1())
                    {
                        internals1.push_back(static_cast<internal_node1 *>(internals1[i]->get_child(j)));
                    }
                    else
                    {
                        internals2.push_back(static_cast<internal_node2 *>(internals1[i]->get_child(j)));
                    }
                }
            }

            uint64_t ln = 0;
            for (int i = 0; i < internals2.size(); ++i)
            {
                ln += internals2[i]->size();
            }

            return ln;
        }

        void check_up() const
        {
            assert(root != NULL);

            std::vector<internal_node1 *> internals1 = {root};
            std::vector<internal_node2 *> internals2;

            for (uint64_t i = 0; i < internals1.size(); ++i)
            {
                for (int j = 0; j < internals1[i]->size(); ++j)
                {
                    if (internals1[i]->has_node1())
                    {
                        internals1.push_back(static_cast<internal_node1 *>(internals1[i]->get_child(j)));
                    }
                    else
                    {
                        internals2.push_back(static_cast<internal_node2 *>(internals1[i]->get_child(j)));
                    }
                }
            }

            for (int i = 0; i < internals1.size(); ++i)
            {
                internals1[i]->assert_all();
            }

            int x = -1;
            for (int i = 0; i < internals2.size(); ++i)
            {
                internals2[i]->assert_all();

                for (int j = 0; j < internals2[i]->size(); ++j)
                {
                    for (int z = 0; z < internals2[i]->get_leaf(j)->size(); ++z)
                    {
                        int key = internals2[i]->get_leaf(j)->get_key(z);
                        assert(x < key);
                        x = key;
                    }
                }
            }
        }


    private:
        class internal_node1;
        class internal_node2;
        class leaf_node;
        internal_node1 *root = NULL;
        uint64_t size_ = 0;
    };

    template <class value_type,
              uint32_t M_KEY_SHARING,       // キーの共有を行う葉の最大数
              uint32_t M_LEAF,              // 葉の最大要素数
              uint32_t M_INTERNAL_CHILDREN, // 内部ノードの最大要素数（子が内部ノード）
              uint32_t M_LEAF_CHILDREN>     // 内部ノードの最大要素数 (子が葉)
    class lb_btree<value_type, M_KEY_SHARING, M_LEAF, M_INTERNAL_CHILDREN, M_LEAF_CHILDREN>::internal_node1
    {
    public:
        /*
         * コンストラクタ
         */
        internal_node1()
        {
            num_of_children_ = 1;
            children[0] = new internal_node2(this);
        }

        // internal_node1を子として渡された時
        internal_node1(std::vector<internal_node1 *> &&c, internal_node1 *p = NULL)
        {
            has_node1_ = true;
            parent = p;
            num_of_children_ = c.size();
            keys_is_empty_ = false;

            for (int i = 0, trials = size(); i < trials; ++i)
            {
                // assert(c[i] != NULL);

                children[i] = c[i];
                keys[i] = c[i]->get_keys_back();
            }
        }

        // right_children を渡す時用
        internal_node1(std::vector<internal_node1 *> &c, internal_node1 *p = NULL)
        {
            has_node1_ = true;
            parent = p;
            num_of_children_ = c.size();
            keys_is_empty_ = false;

            for (int i = 0, trials = size(); i < trials; ++i)
            {
                // assert(c[i] != NULL);

                children[i] = c[i];
                keys[i] = c[i]->get_keys_back();
            }
        }

        // internal_node2を子として渡されたとき
        internal_node1(std::vector<internal_node2 *> &&c, internal_node1 *p = NULL)
        {
            parent = p;
            num_of_children_ = c.size();
            keys_is_empty_ = false;

            for (int i = 0, trials = size(); i < trials; ++i)
            {
                // assert(c[i] != NULL);

                children[i] = c[i];
                c[i]->set_pos(i);
                keys[i] = c[i]->get_keys_back();
            }
        }

        internal_node1(std::vector<internal_node2 *> &c, internal_node1 *p = NULL)
        {
            parent = p;
            num_of_children_ = c.size();
            keys_is_empty_ = false;

            for (int i = 0, trials = size(); i < trials; ++i)
            {
                // assert(c[i] != NULL);

                children[i] = c[i];
                c[i]->set_pos(i);
                keys[i] = c[i]->get_keys_back();
            }
        }

        ~internal_node1()
        {
        }

        uint64_t byte_size()
        {
            uint64_t bs = sizeof(internal_node1);

            if (has_node1())
            {
                for (int i = 0, trials = size(); i < trials; ++i)
                {
                    // assert(children[i] != NULL);
                    bs += static_cast<internal_node1 *>(children[i])->byte_size();
                }
            }
            else
            {
                for (int i = 0, trials = size(); i < trials; ++i)
                {
                    // assert(children[i] != NULL);
                    bs += static_cast<internal_node2 *>(children[i])->byte_size();
                }
            }

            return bs;
        }

        // 挿入類
        internal_node1 *insert(value_type x)
        {
            assert(not is_full());

            if (keys_empty())
            {
                // assert(is_root());
                // assert(children[0] != NULL);

                static_cast<internal_node2 *>(children[0])->insert(x);
                keys[0] = x;
                keys_is_empty_ = false;

                return NULL;
            }

            // assert(not keys_empty());

            uint32_t insert_pos = size() - 1;
            if (insert_pos == UINT32_MAX)
                insert_pos = 0;

            if (size() > 1 && x < get_keys_back())
                insert_pos = find_child(x);

            if (has_node1())
            {
                // assert(insert_pos < size());
                // assert(children[insert_pos] != NULL);
                // assert(static_cast<internal_node1 *>(children[insert_pos])->get_parent() == this);

                static_cast<internal_node1 *>(children[insert_pos])->insert(x);
            }
            else
            {
                // assert(insert_pos < size());
                // assert(children[insert_pos] != NULL);
                // assert(static_cast<internal_node2 *>(children[insert_pos])->get_parent() == this);

                static_cast<internal_node2 *>(children[insert_pos])->insert(x);
            }

            update_key(insert_pos);

            // キーの分割
            internal_node1 *new_root = NULL;

            if (size() == M_INTERNAL_CHILDREN)
            {
                internal_node1 *right = split();

                // assert(not is_full());
                // assert(right != NULL);
                // assert(not right->is_full());

                if (is_root())
                {
                    new_root = new internal_node1(std::vector<internal_node1 *>{this, right});

                    this->overwrite_parent(new_root);
                    right->overwrite_parent(new_root);
                }
                else
                {
                    // assert(parent != NULL);

                    (get_parent())->new_children(this, right); // parentのキー更新を含む
                }
            }

            return new_root;
        }

        internal_node1 *split()
        {
            internal_node1 *right = NULL;

            if (has_node1()) // 子がnode1の時
            {
                std::vector<internal_node1 *> right_children(size() / 2);

                for (int i = size() - size() / 2, trials = size(), j = 0; i < trials; ++i, ++j)
                {
                    right_children[j] = static_cast<internal_node1 *>(children[i]);
                }

                right = new internal_node1(right_children, parent);

                for (int i = 0, trials = size() / 2; i < trials; ++i)
                {
                    static_cast<internal_node1 *>(right->children[i])->set_parent(right);
                }
            }
            else // 子がnode2の時
            {
                std::vector<internal_node2 *> right_children(size() / 2);

                for (int i = size() - size() / 2, trials = size(), j = 0; i < trials; ++i, ++j)
                {
                    right_children[j] = static_cast<internal_node2 *>(children[i]);
                    // right_children[j]->set_pos(j);
                }

                right = new internal_node1(right_children, parent);

                static_cast<internal_node2 *>(children[size() - size() / 2 - 1])->set_right(NULL);
                static_cast<internal_node2 *>(right->get_children_front())->set_left(NULL);

                for (int i = 0, trials = size() / 2; i < trials; ++i)
                {
                    static_cast<internal_node2 *>(right->children[i])->set_parent(right);
                }
            }

            num_of_children_ = size() - size() / 2;

            return right;
        }

        void new_children(internal_node1 *left, internal_node1 *right)
        {
            // assert(not is_full());
            // assert(has_node1());

            for (int i = size(); i > 0; --i)
            {
                if (children[i - 1] == left)
                {
                    // assert(right != NULL);

                    children[i] = right;
                    keys[i] = right->get_keys_back();
                    break;
                }

                // assert(children[i - 1] != NULL);

                children[i] = children[i - 1];
                keys[i] = static_cast<internal_node1 *>(children[i - 1])->get_keys_back();
            }

            num_of_children_++;
        }

        void new_children(internal_node2 *left, internal_node2 *right)
        {
            // assert(not is_full());
            // assert(not has_node1());

            for (int i = size(); i > 0; --i)
            {
                if (children[i - 1] == left)
                {
                    // assert(right != NULL);

                    children[i] = right;
                    right->set_pos(i);
                    keys[i] = right->get_keys_back();
                    break;
                }

                // assert(children[i - 1] != NULL);

                children[i] = children[i - 1];
                static_cast<internal_node2 *>(children[i])->set_pos(i);
                keys[i] = static_cast<internal_node2 *>(children[i - 1])->get_keys_back();
            }

            num_of_children_++;
        }

        void overwrite_parent(internal_node1 *p)
        {
            parent = p;
        }

        void update_key(uint32_t i)
        {
            // assert(children[i] != NULL);

            if (has_node1())
            {
                keys[i] = static_cast<internal_node1 *>(children[i])->get_keys_back();
            }
            else
            {
                keys[i] = static_cast<internal_node2 *>(children[i])->get_keys_back();
            }
        }

        void update_keys(uint32_t start, uint32_t end)
        {
            // for (int i = start; i <= end; ++i)
            // {
            //     assert(children[i] != NULL);
            // }

            if (has_node1())
            {
                for (int i = start; i <= end; ++i)
                {
                    keys[i] = static_cast<internal_node1 *>(children[i])->get_keys_back();
                }
            }
            else
            {
                for (int i = start; i <= end; ++i)
                {
                    keys[i] = static_cast<internal_node2 *>(children[i])->get_keys_back();
                }
            }
        }

        void set_parent(internal_node1 *p)
        {
            parent = p;
        }

        internal_node1 *get_parent()
        {
            return parent;
        }

        uint32_t find_child(value_type x)
        {
            for (int i = 0, trials = size(); i < trials; ++i)
            {
                if (x <= keys[i])
                    return i;
            }

            return size() - 1;
        }

        bool has_node1()
        {
            return has_node1_;
        }

        bool is_root()
        {
            return parent == NULL;
        }

        bool keys_empty()
        {
            return keys_is_empty_;
        }

        bool is_full()
        {
            // assert(size() <= M_INTERNAL_CHILDREN);

            return size() == M_INTERNAL_CHILDREN;
        }

        value_type get_keys_back()
        {
            return keys[size() - 1];
        }

        void *get_child(uint32_t i) const
        {
            return children[i];
        }

        const uint32_t size()
        {
            return num_of_children_;
        }

        // 削除類
        internal_node1 *remove(value_type x) // このままではこの関数の返り値がrem_nodeかnew_rootか分からなくなってしまうが、どこに対する返り値化ですべに場合分け出来てる説。このままでいいかも。
        {
            if (keys_empty())
            {
                std::cout << x << " は存在しない" << std::endl;
                return NULL;
            }

            uint32_t remove_pos = find_child(x);

            if (has_node1())
            {
                internal_node1 *rem_node = static_cast<internal_node1 *>(children[remove_pos])->remove(x); // rem_nodeには、削除すべき一つ下の層のノードが返る

                if (rem_node != NULL)
                {
                    // rem_nodeを子から省く
                    remove_child(rem_node);
                    delete rem_node; // deleteには型情報が必要なので、重複は仕方ない
                }
            }
            else
            {
                internal_node2 *rem_node = static_cast<internal_node2 *>(children[remove_pos])->remove(x); // rem_nodeには、削除すべき一つ下の層のノードが返る

                if (rem_node != NULL)
                {
                    // rem_nodeを子から省く
                    remove_child(rem_node);
                    if (rem_node->get_left() != NULL)
                        (rem_node->get_left())->set_right(rem_node->get_right());
                    if (rem_node->get_right() != NULL)
                        (rem_node->get_right())->set_left(rem_node->get_left());
                    delete rem_node; // deleteには型情報が必要なので、重複は仕方ない
                }
            }

            update_keys(0, size() - 1); // これで共有による祖父への影響と、マージによるremove_pos±2個のノードの削除可能性に対応できる


            // nodeの調整・削除
            if (is_root())
            {
                if (size() == 1 && has_node1())
                {
                    internal_node1 *new_root = static_cast<internal_node1 *>(children[0]);
                    new_root->set_parent(NULL);

                    return new_root;
                }
            }
            else
            {
                uint32_t lower_bound = (M_INTERNAL_CHILDREN + 1) / 2;
                if (size() >= lower_bound)
                    return NULL;

                bool next_is_right;
                internal_node1 *next_node = find_next_node(&next_is_right);

                if (next_node == NULL)
                    return NULL;

                if (next_is_right && next_node->size() > lower_bound)
                {
                    // 右のnext_nodeから子を1つもらう
                    take_a_child_from_right_neighbor(next_node);
                    update_key(size() - 1);
                    next_node->update_keys(0, next_node->size() - 1);
                }
                else if (!next_is_right && next_node->size() > lower_bound)
                {
                    // 左のnext_nodeから子を1つもらう
                    take_a_child_from_left_neighbor(next_node);
                    update_keys(0, size() - 1);
                }
                else
                {
                    if (next_is_right)
                    {
                        merge(this, next_node);
                        return next_node;
                    }
                    else
                    {
                        merge(next_node, this);
                        return this;
                    }
                }
            }

            return NULL;
        }

        void take_a_child_from_right_neighbor(internal_node1 *next_node) // キー以外すべて更新
        {
            if (!has_node1())
            {
                static_cast<internal_node2 *>(get_children_back())->set_right(static_cast<internal_node2 *>(next_node->get_children_front()));
                static_cast<internal_node2 *>(next_node->get_children_front())->set_left(static_cast<internal_node2 *>(get_children_back()));
                static_cast<internal_node2 *>(next_node->get_children_front())->set_right(NULL);
            }

            push_children_back(next_node->get_children_front());
            next_node->pop_children_front();

            if (!has_node1())
            {
                static_cast<internal_node2 *>(next_node->get_children_front())->set_left(NULL);
            }
        }

        void take_a_child_from_left_neighbor(internal_node1 *next_node) // キー以外すべて更新
        {
            if (!has_node1())
            {
                static_cast<internal_node2 *>(get_children_front())->set_left(static_cast<internal_node2 *>(next_node->get_children_back()));
                static_cast<internal_node2 *>(next_node->get_children_back())->set_right(static_cast<internal_node2 *>(get_children_front()));
                static_cast<internal_node2 *>(next_node->get_children_back())->set_left(NULL);
            }

            push_children_front(next_node->get_children_back());
            next_node->pop_children_back();

            if (!has_node1())
            {
                static_cast<internal_node2 *>(next_node->get_children_back())->set_right(NULL);
            }
        }

        void push_children_back(void *child)
        {
            children[size()] = child;

            if (has_node1())
            {
                static_cast<internal_node1 *>(children[size()])->set_parent(this);
            }
            else
            {
                static_cast<internal_node2 *>(children[size()])->set_parent(this);
                static_cast<internal_node2 *>(children[size()])->set_pos(size());
            }

            num_of_children_++;
        }

        void push_children_front(void *child)
        {
            if (has_node1())
            {
                for (int i = size(); i > 0; --i)
                {
                    children[i] = children[i - 1];
                }
            }
            else
            {
                for (int i = size(); i > 0; --i)
                {
                    children[i] = children[i - 1];
                    static_cast<internal_node2 *>(children[i])->set_pos(i);
                }
            }

            children[0] = child;

            if (has_node1())
            {
                static_cast<internal_node1 *>(children[0])->set_parent(this);
            }
            else
            {
                static_cast<internal_node2 *>(children[0])->set_parent(this);
                static_cast<internal_node2 *>(children[0])->set_pos(0);
            }
            num_of_children_++;
        }

        void *get_children_back()
        {
            return children[size() - 1];
        }

        void *get_children_front()
        {
            return children[0];
        }

        void pop_children_back()
        {
            num_of_children_--;
        }

        void pop_children_front()
        {
            if (!has_node1())
            {
                for (int i = 0, trials = size() - 1; i < trials; ++i)
                {
                    children[i] = children[i + 1];
                }
            }
            else
            {
                for (int i = 0, trials = size() - 1; i < trials; ++i)
                {
                    children[i] = children[i + 1];
                    static_cast<internal_node2 *>(children[i])->set_pos(i);
                }
            }

            num_of_children_--;
        }

        void remove_child(internal_node1 *rem_node) // あとで全てのキーを更新する
        {
            uint32_t rem_pos;

            for (int i = 0, trials = size(); i < trials; ++i)
            {
                if (children[i] == rem_node)
                {
                    rem_pos = i;
                    break;
                }
            }

            for (int i = rem_pos, trials = size() - 1; i < trials; ++i)
            {
                children[i] = children[i + 1];
            }

            num_of_children_--;
        }

        void remove_child(internal_node2 *rem_node) // あとで全てのキーを更新する
        {
            uint32_t rem_pos;

            for (int i = 0, trials = size(); i < trials; ++i)
            {
                if (children[i] == rem_node)
                {
                    rem_pos = i;
                    break;
                }
            }

            for (int i = rem_pos, trials = size() - 1; i < trials; ++i)
            {
                children[i] = children[i + 1];
                static_cast<internal_node2 *>(children[i])->set_pos(i);
            }

            num_of_children_--;
        }

        internal_node1 *find_next_node(bool *next_is_right) // ここから始める
        {
            uint32_t pos;
            for (int i = 0, trials = get_parent()->size(); i < trials; ++i)
            {
                if (get_parent()->children[i] == this)
                {
                    pos = i;
                    break;
                }
            }

            if (pos == 0 && get_parent()->size() > 1)
            {
                *next_is_right = true;
                return static_cast<internal_node1 *>(get_parent()->children[1]);
            }
            else if (pos > 0)
            {
                *next_is_right = false;
                return static_cast<internal_node1 *>(get_parent()->children[pos - 1]);
            }

            return NULL;
        }

        // 常に左側のノードを残す
        void merge(internal_node1 *remaining_node, internal_node1 *disappearing_node)
        {
            if (remaining_node->has_node1())
            {
                for (int i = 0, j = remaining_node->size(), trials = disappearing_node->size(); i < trials; ++i, ++j)
                {
                    remaining_node->children[j] = disappearing_node->children[i];
                    static_cast<internal_node1 *>(remaining_node->children[j])->set_parent(remaining_node);
                    remaining_node->keys[j] = disappearing_node->keys[i];
                }
            }
            else
            {
                static_cast<internal_node2 *>(remaining_node->get_children_back())->set_right(static_cast<internal_node2 *>(disappearing_node->get_children_front()));
                static_cast<internal_node2 *>(disappearing_node->get_children_front())->set_left(static_cast<internal_node2 *>(remaining_node->get_children_back()));

                for (int i = 0, j = remaining_node->size(), trials = disappearing_node->size(); i < trials; ++i, ++j)
                {
                    remaining_node->children[j] = disappearing_node->children[i];
                    static_cast<internal_node2 *>(remaining_node->children[j])->set_parent(remaining_node);
                    static_cast<internal_node2 *>(remaining_node->children[j])->set_pos(j);
                    remaining_node->keys[j] = disappearing_node->keys[i];
                }
            }

            remaining_node->num_of_children_ += disappearing_node->size();
        }

        void assert_all(internal_node1 *p = NULL)
        {
            assert(p == parent || p == NULL);
            assert(num_of_children_ <= M_INTERNAL_CHILDREN);
            for (int i = 0, trials = size() - 1; i < trials; ++i)
            {
                assert(keys[i] <= keys[i + 1]);
            }
            for (int i = 0, trials = size(); i < trials; ++i)
            {
                if (has_node1())
                {
                    assert(keys[i] == static_cast<internal_node1 *>(children[i])->get_keys_back());
                }
                else
                {
                    assert(keys[i] == static_cast<internal_node2 *>(children[i])->get_keys_back());
                }
            }
        }

    private:
        value_type keys[M_INTERNAL_CHILDREN];
        void *children[M_INTERNAL_CHILDREN];

        internal_node1 *parent = NULL;

        uint32_t num_of_children_ = 0;

        bool has_node1_ = false;
        bool keys_is_empty_ = true;
    };

    template <class value_type,
              uint32_t M_KEY_SHARING,       // キーの共有を行う葉の最大数
              uint32_t M_LEAF,              // 葉の最大要素数
              uint32_t M_INTERNAL_CHILDREN, // 内部ノードの最大要素数（子が内部ノード）
              uint32_t M_LEAF_CHILDREN>     // 内部ノードの最大要素数 (子が葉)
    class lb_btree<value_type, M_KEY_SHARING, M_LEAF, M_INTERNAL_CHILDREN, M_LEAF_CHILDREN>::internal_node2
    {
    public:
        /*
         * コンストラクタ
         */
        internal_node2(internal_node1 *p)
        {
            parent = p;
            num_of_leaves_ = 1;
            leaves[0] = new leaf_node();
        }

        internal_node2(std::vector<leaf_node *> &c, internal_node1 *p, internal_node2 *l, internal_node2 *r)
        {
            parent = p;
            num_of_leaves_ = c.size();
            keys_is_empty_ = false;
            left_sibling = l;
            right_sibling = r;

            for (int i = 0, trials = size(); i < trials; ++i)
            {
                // assert(c[i] != NULL);

                leaves[i] = c[i];
                keys[i] = c[i]->get_keys_back();
            }
        }

        ~internal_node2()
        {
        }

        value_type get_keys_back()
        {
            return keys[size() - 1];
        }

        void set_pos(uint32_t pos)
        {
            pos_ = pos;
        }

        uint32_t get_pos()
        {
            return pos_;
        }

        void set_parent(internal_node1 *p)
        {
            parent = p;
        }

        internal_node1 *get_parent()
        {
            return parent;
        }

        internal_node2 *get_left()
        {
            return left_sibling;
        }

        internal_node2 *get_right()
        {
            return right_sibling;
        }

        uint64_t byte_size()
        {
            uint64_t bs = sizeof(internal_node2);

            for (int i = 0, trials = size(); i < trials; ++i)
            {
                // assert(leaves[i] != NULL);
                bs += leaves[i]->byte_size();
            }

            return bs;
        }

        void insert(value_type x) // キーの更新はこの階層で行う。
        {
            // assert(not is_full());

            if (keys_empty())
            {
                // assert(leaves[0] != NULL);

                leaves[0]->insert(x); // leafのinsertは空きのチェック機能を持っていない
                keys[0] = x;
                keys_is_empty_ = false;

                return;
            }

            uint32_t insert_pos = size() - 1;
            if (insert_pos == UINT32_MAX)
                insert_pos = 0;

            if (size() > 1 && x < get_keys_back())
                insert_pos = find_leaf(x);

            // assert(insert_pos < size());

            auto *new_leaf = find_vacant_leaf_and_insert(leaves[insert_pos], insert_pos, x);
            if (new_leaf)
            {
                new_leaves(leaves[insert_pos], new_leaf);
            }

            update_key(insert_pos);

            // キーの分割
            if (size() == M_LEAF_CHILDREN)
            {
                internal_node2 *right = split(); // メンバにrightがあることを忘れずに

                // assert(not is_full());
                // assert(right != NULL);
                // assert(not right->is_full());
                // assert(parent != NULL);

                get_parent()->new_children(this, right); // parentのキー更新を含む
            }

            // thisの確認
            // for (int i = 0, trials = size(); i < trials; ++i)
            // {
            //     assert(leaves[i] != NULL);
            //     leaves[i]->assert_all();
            //     assert(keys[i] == leaves[i]->get_keys_back());
            //     if (i + 1 < size())
            //         assert(keys[i] <= keys[i + 1]);
            // }

            // // 左右の確認
            // if (left_sibling != NULL)
            // {
            //     for (int i = 0, trials = left_sibling->size(); i < trials; ++i)
            //     {
            //         assert(left_sibling->leaves[i] != NULL);
            //         (left_sibling->leaves[i])->assert_all();

            //         assert(left_sibling->keys[i] == (left_sibling->leaves[i])->get_keys_back());
            //         if (i + 1 < left_sibling->size())
            //             assert(left_sibling->keys[i] <= left_sibling->keys[i + 1]);
            //     }
            // }

            // if (right_sibling != NULL)
            // {
            //     for (int i = 0, trials = right_sibling->size(); i < trials; ++i)
            //     {
            //         assert(right_sibling->leaves[i] != NULL);
            //         (right_sibling->leaves[i])->assert_all();

            //         assert(right_sibling->keys[i] == (right_sibling->leaves[i])->get_keys_back());
            //         if (i + 1 < right_sibling->size())
            //             assert(right_sibling->keys[i] <= right_sibling->keys[i + 1]);
            //     }
            // }
        }

        leaf_node *find_vacant_leaf_and_insert(leaf_node *leaf, uint32_t insert_pos, value_type x) // この関数は葉ノードをいじる関数なので、自家族や隣家族のキーの更新は行わない。
        {
            if (not leaf->is_full())
            {
                leaf->insert(x);

                return NULL;
            }

            // 左の兄弟を確認
            uint32_t vacant_pos;
            uint32_t num_of_keysharing_leaves = M_KEY_SHARING;
            bool there_is_vacant_sibling = find_vacant_leaf_on_the_left(insert_pos, &vacant_pos, &num_of_keysharing_leaves);

            // 左の兄弟に空きがあれば左にしシフト後、leafに挿入
            if (there_is_vacant_sibling)
            {
                // assert(leaf->is_full());
                // assert(not leaves[vacant_pos]->is_full());

                // leafに対してshiftを行ったかどうかを判断するフラグ
                bool shift_flag = shift_a_key_to_left(vacant_pos, insert_pos, x);

                if (shift_flag)
                {
                    // assert(not leaf->is_full());
                    leaf->insert(x);
                }
                else
                {
                    // assert(leaf->is_full());
                    leaves[insert_pos - 1]->insert(x);
                    update_key(insert_pos - 1);
                }

                return NULL;
            }

            // 左の兄弟に空きがなければ、右の兄弟を確認
            there_is_vacant_sibling = find_vacant_leaf_on_the_right(insert_pos, &vacant_pos, &num_of_keysharing_leaves);

            // 右の兄弟に空きがあれば右にシフト後、leafに挿入
            if (there_is_vacant_sibling)
            {
                // assert(leaf->is_full());
                // assert(not leaves[vacant_pos]->is_full());
                shift_a_key_to_right(insert_pos, vacant_pos);

                leaf->insert(x);

                return NULL;
            }

            // if (num_of_keysharing_leaves > 0) // 兄弟葉ノードが全て一杯かつ、指定した共有ノード数よりチェックしたノード数が少ない時、隣家族の空きも、それぞれチェックする
            // {
            //     // 兄弟に空きが無いため、左家族の空き確認
            //     bool there_is_vacant_family = false;
            //     if (left_sibling != NULL)
            //     {
            //         uint32_t num_of_keysharing_leaves_on_the_left = M_KEY_SHARING / 2;

            //         if (!(left_sibling->leaves[left_sibling->size() - 1])->is_full())
            //         {
            //             vacant_pos = left_sibling->size() - 1;
            //         }
            //         else
            //         {
            //             there_is_vacant_family = left_sibling->find_vacant_leaf_on_the_left(left_sibling->size() - 1, &vacant_pos, &num_of_keysharing_leaves_on_the_left);
            //         }
            //     }

            //     // 左家族に空きがある。この時左家族の最大値を変更するため、left->parent->keys_update()が必要
            //     if (there_is_vacant_family)
            //     {
            //         // 左家族の一番右の葉に空きを作る(shift_a_key_to_leftでやる。キーの更新も含まれている)
            //         bool shift_flag = left_sibling->shift_a_key_to_left(vacant_pos, left_sibling->size() - 1, x);

            //         // 左家族の一番右の葉にシフト
            //         // 左家族の一番右の葉の部分の親のキーを更新(左家族のupdate_keyを含む)
            //         if (insert_pos == 0 && x < get_leaf(0)->get_keys_front())
            //         {
            //             left_sibling->find_vacant_leaf_and_insert(left_sibling->get_leaf(left_sibling->size() - 1), left_sibling->size() - 1, x);
            //             left_sibling->update_key(left_sibling->size() - 1);
            //             (left_sibling->parent)->update_key(left_sibling->get_pos());
            //         }
            //         else
            //         {
            //             shift_a_key_to_left_family();
            //             find_vacant_leaf_and_insert(leaf, insert_pos, x);
            //         }

            //         return NULL;
            //     }

            //     // 左家族に空きがなければ、右家族を確認
            //     if (right_sibling != NULL)
            //     {
            //         uint32_t num_of_keysharing_leaves_on_the_right = M_KEY_SHARING / 2;

            //         if (!(right_sibling->leaves[0])->is_full())
            //         {
            //             vacant_pos = 0;
            //         }
            //         else
            //         {
            //             there_is_vacant_family = right_sibling->find_vacant_leaf_on_the_right(0, &vacant_pos, &num_of_keysharing_leaves_on_the_right);
            //         }
            //     }

            //     // 右家族に空きがあれば、右家族の一番左の葉にinsert_into_the_far_left_leaf。この時右家族の最大値は変更されないため、right->parent->keys_update()は必要ない
            //     if (there_is_vacant_family)
            //     {
            //         // 右家族の一番左の葉に空きを作る(shift_a_key_to_leftで。キーの更新も含まれている)
            //         right_sibling->shift_a_key_to_right(0, vacant_pos);

            //         // 右家族の一番左の葉にシフト
            //         // 右家族のキー更新は必要ない
            //         shift_a_key_to_right_family();

            //         find_vacant_leaf_and_insert(leaf, insert_pos, x);

            //         return NULL;
            //     }
            // }

            // 空きがなければ分割
            leaf_node *next = leaf->split();

            if (x <= leaf->get_keys_back())
            {
                leaf->insert(x);
            }
            else
            {
                next->insert(x);
            }

            return next;
        }

        bool find_vacant_family_on_the_left(uint32_t *v_pos)
        {
            for (int i = left_sibling->size() - 1; i > -1; --i)
            {
                if (!left_sibling->leaves[i]->is_full())
                {
                    *v_pos = i;
                    return true;
                }
            }

            return false;
        }

        bool find_vacant_family_on_the_right(uint32_t *v_pos)
        {
            for (int i = 0; i < right_sibling->size(); ++i)
            {
                if (!right_sibling->leaves[i]->is_full())
                {
                    *v_pos = i;
                    return true;
                }
            }

            return false;
        }

        void shift_a_key_to_left_family()
        {
            // assert(not left_sibling->leaves[left_sibling->size() - 1]->is_full());

            (left_sibling->leaves[left_sibling->size() - 1])->push_back(leaves[0]->get_keys_front());
            left_sibling->update_key(left_sibling->size() - 1); // これは上まで伝播しないので、下のupdate_keyが必要。それ以上上に伝播することはない。
            (left_sibling->parent)->update_key(left_sibling->get_pos());
            leaves[0]->pop_front();
        }

        void shift_a_key_to_right_family()
        {
            // assert(not right_sibling->leaves[0]->is_full());

            right_sibling->leaves[0]->push_front(leaves[size() - 1]->get_keys_back());
            leaves[size() - 1]->pop_back();
            update_key(size() - 1);
        }

        bool shift_a_key_to_left(uint32_t l_pos, uint32_t r_pos, value_type x) // shift_a_key_to_leftの場合はxがr_posの最小値より小さい場合、xがr_pos-1に挿入されるため、少し複雑
        {
            // assert(l_pos <= r_pos);
            // assert(leaves[l_pos] != NULL);
            // assert(leaves[r_pos] != NULL);

            bool flag = false;

            for (int i = l_pos; i < r_pos; ++i)
            {
                if (i < r_pos - 1)
                {
                    take_a_key_from_the_right(i); // キーの更新無くす
                }
                else if (x <= leaves[r_pos]->get_keys_front())
                {
                    flag = false;
                }
                else
                {
                    take_a_key_from_the_right(i);
                    flag = true;
                }
            }

            // for (int i = 0, trials = size(); i < trials; ++i)
            // {
            //     assert(leaves[i] != NULL);
            //     assert(keys[i] == leaves[i]->get_keys_back());
            // }

            return flag;
        }

        void shift_a_key_to_left(uint32_t l_pos, uint32_t r_pos)
        {
            // assert(l_pos <= r_pos);
            // assert(leaves[l_pos] != NULL);
            // assert(leaves[r_pos] != NULL);

            for (int i = l_pos; i < r_pos; ++i)
            {
                take_a_key_from_the_right(i);
            }
        }

        void shift_a_key_to_right(uint32_t l_pos, uint32_t r_pos)
        {
            // assert(l_pos <= r_pos);
            // assert(leaves[l_pos] != NULL);
            // assert(leaves[r_pos] != NULL);

            for (int i = r_pos; i > l_pos; --i)
            {
                take_a_key_from_the_left(i);
            }
        }

        void take_a_key_from_the_left(uint32_t i) // キー更新無くす
        {
            // assert(leaves[i - 1] != NULL);
            // assert(leaves[i] != NULL);

            value_type x = leaves[i - 1]->get_keys_back();
            leaves[i - 1]->pop_back();
            leaves[i]->push_front(x);
            keys[i - 1] = leaves[i - 1]->get_keys_back();
        }

        void take_a_key_from_the_right(uint32_t i) // キー更新無くす
        {
            // assert(leaves[i] != NULL);
            // assert(leaves[i + 1] != NULL);

            value_type x = leaves[i + 1]->get_keys_front();
            leaves[i + 1]->pop_front();
            leaves[i]->push_back(x);
            keys[i] = x;
        }

        // chatGPTによる簡潔化
        bool find_vacant_leaf_on_the_left(uint32_t l_pos, uint32_t *v_pos, uint32_t *n_keysharing)
        {
            // assert(leaves[l_pos] != NULL);
            // assert(*n_keysharing == M_KEY_SHARING || *n_keysharing == M_KEY_SHARING / 2);

            for (int i = l_pos - 1; i >= 0 && *n_keysharing > 0; --i)
            {
                if (!leaves[i]->is_full())
                {
                    *v_pos = i;
                    return true;
                }
                *n_keysharing -= 1;
            }

            return false;
        }

        // chatGPTによる簡潔化
        bool find_vacant_leaf_on_the_right(uint32_t l_pos, uint32_t *v_pos, uint32_t *n_keysharing)
        {
            // assert(leaves[l_pos] != NULL);

            for (int i = l_pos + 1, trials = size(); i < trials && *n_keysharing > 0; ++i)
            {
                if (!leaves[i]->is_full())
                {
                    *v_pos = i;
                    return true;
                }
                *n_keysharing -= 1;
            }

            return false;
        }

        internal_node2 *split() // right_sibling, left_siblingは共通の親を持つこととする。splitによって親が変わる場合は、right, leftの関係から切り離される。
        {
            // assert(size() == M_LEAF_CHILDREN);

            internal_node2 *right = NULL;

            std::vector<leaf_node *> right_leaves(size() / 2);

            for (int i = size() - size() / 2, j = 0, trials = size(); i < size(); ++i, ++j)
            {
                right_leaves[j] = leaves[i];
            }

            right = new internal_node2(right_leaves, parent, this, this->get_right());
            if (get_right() != NULL)
                get_right()->set_left(right);
            set_right(right);

            num_of_leaves_ = size() - size() / 2;

            return right;
        }

        void set_right(internal_node2 *right)
        {
            right_sibling = right;
        }

        void set_left(internal_node2 *left)
        {
            left_sibling = left;
        }

        void new_leaves(leaf_node *left, leaf_node *right)
        {
            // assert(not is_full());

            for (int i = size(); i > 0; --i)
            {
                if (leaves[i - 1] == left)
                {
                    // assert(right != NULL);

                    leaves[i] = right;
                    keys[i] = right->get_keys_back();
                    break;
                }

                // assert(leaves[i - 1] != NULL);

                leaves[i] = leaves[i - 1];
                keys[i] = leaves[i - 1]->get_keys_back();
            }

            num_of_leaves_++;
        }

        void update_key(uint32_t i)
        {
            // assert(i < size());
            // assert(leaves[i] != NULL);

            keys[i] = leaves[i]->get_keys_back();
        }

        void update_keys(uint32_t start, uint32_t end)
        {
            // for (int i = start; i <= end; ++i)
            // {
            //     assert(leaves[i] != NULL);
            // }

            for (int i = start; i <= end; ++i)
            {
                keys[i] = leaves[i]->get_keys_back();
            }
        }

        uint32_t find_leaf(value_type x)
        {
            for (int i = 0, trials = size(); i < trials; ++i)
            {
                if (x <= keys[i])
                    return i;
            }

            return size() - 1;
        }

        bool keys_empty()
        {
            return keys_is_empty_;
        }

        bool is_full()
        {
            // assert(size() <= M_LEAF_CHILDREN);

            return size() == M_LEAF_CHILDREN;
        }

        const uint32_t size()
        {
            return num_of_leaves_;
        }

        // 削除類
        internal_node2 *remove(value_type x) // 自分とその兄弟のキーは正してから返す,この階層でのみキーの更新を行う。削除するノード（mergeを行うとき）を返す。
        {
            uint32_t remove_pos = find_leaf(x);
            leaf_node *rem_leaf = NULL;
            char flag_of_vacant_leaf = '0'; // 0:キーの共有無し 1:自家族とキーの共有あり  2:右家族とキーの共有あり 3:左家族とキーの共有あり

            if (leaves[remove_pos]->is_full())
            {
                leaves[remove_pos]->remove(x);

                uint32_t vacant_pos = find_vacant_leaf_and_shift(leaves[remove_pos], remove_pos, &flag_of_vacant_leaf, x); // 自家族や隣家族から空きリーフを探し、キーを共有する。そして空きリーフの位置を返す。キーの更新はしなくてよい

                // 空の葉ノードを削除（必要に応じてキーの更新
                if (flag_of_vacant_leaf == '1' && leaves[vacant_pos]->keys_empty())
                {
                    rem_leaf = remove_leaf(vacant_pos); // 子からvacant_posの子供を省く関数
                }
                else if (flag_of_vacant_leaf == '2' && right_sibling->leaves[vacant_pos]->keys_empty())
                {
                    // rem_leaf = right_sibling->remove_leaf(vacant_pos); // 子からvacant_posに該当する子を省き、sizeを改める。
                    // if (right_sibling->size() > 0)
                    //     right_sibling->update_keys(vacant_pos, right_sibling->size() - 1);
                }
                else if (flag_of_vacant_leaf == '3' && left_sibling->leaves[vacant_pos]->keys_empty())
                {
                    // rem_leaf = left_sibling->remove_leaf(vacant_pos);
                }

                // キーの更新
                if (flag_of_vacant_leaf == '0')
                {
                    update_key(remove_pos);
                }
                else if (flag_of_vacant_leaf == '1') // うまく場合分けしても重複するため、ここでまとめて更新する
                {
                    if (remove_pos < vacant_pos)
                        update_keys(remove_pos, size() - 1);
                    else
                        update_keys(vacant_pos, size() - 1);
                }
                else if (flag_of_vacant_leaf == '2')
                {
                    // update_keys(remove_pos, size() - 1);
                    // right_sibling->update_keys(0, vacant_pos - 1);
                }
                else if (flag_of_vacant_leaf == '3')
                {
                    // update_keys(0, remove_pos);
                    // left_sibling->update_keys(vacant_pos, left_sibling->size() - 1);
                    // (left_sibling->get_parent())->update_key(left_sibling->get_pos()); // ここは親に伝播させる必要あり
                }
            }
            else
            {
                leaves[remove_pos]->remove(x);

                if (size() == 1 && parent->size() == 1 && parent->is_root()) // 木が最小サイズの時
                {
                    update_key(remove_pos);
                    return NULL; // 以下を飛ばして強制終了
                }

                if (leaves[remove_pos]->keys_empty()) // 空の葉ノード削除 自分含めすべての先祖がsize() == 1 の時はdeleteしない
                {
                    rem_leaf = remove_leaf(remove_pos);
                    if (size() > 0)
                        update_keys(remove_pos, size() - 1); // remove_pos以降のキー全てを更新
                }
                else
                {
                    update_key(remove_pos);
                }
            }

            if (rem_leaf != NULL)
            {
                // std::cout << "delete : " << rem_leaf << std::endl;
                delete rem_leaf; // leafをdeleteする関数
            }

            // nodeの調整
            uint32_t lower_bound = (M_LEAF_CHILDREN + 1) / 2;
            internal_node2 *rem_node = NULL;

            if (flag_of_vacant_leaf == '0' || flag_of_vacant_leaf == '1') // 自家族の葉が削除された可能性がある場合
            {
                if (size() >= lower_bound)
                    return NULL;

                // 子の共有
                if (right_sibling != NULL)
                {
                    if (right_sibling->size() > lower_bound)
                    {
                        take_a_leaf_from_rightsib(); // 両方のノードのsizeの更新も
                        update_key(size() - 1);
                        right_sibling->update_keys(0, right_sibling->size() - 1);
                        return NULL;
                    }
                }

                if (left_sibling != NULL)
                {
                    if (left_sibling->size() > lower_bound)
                    {
                        take_a_leaf_from_leftsib();
                        update_keys(0, size() - 1);
                        return NULL;
                    }
                }

                // マージ
                if (right_sibling != NULL)
                {
                    rem_node = right_sibling;
                    merge(this, right_sibling); // 左側のノードに子をまとめるだけ
                    return rem_node;
                }
                else if (left_sibling != NULL)
                {
                    merge(left_sibling, this);
                    return this;
                }
            }
            else if (flag_of_vacant_leaf == '2') // 右家族の葉が削除された可能性がある場合
            {
                // assert(right_sibling != NULL);

                // if (right_sibling->size() >= lower_bound)
                //     return NULL;

                // // 子の共有
                // if (right_sibling->right_sibling != NULL)
                // {
                //     if (right_sibling->right_sibling->size() > lower_bound)
                //     {
                //         right_sibling->take_a_leaf_from_rightsib(); // 両方のノードのsizeの更新も
                //         right_sibling->update_key(right_sibling->size() - 1);
                //         (right_sibling->get_right())->update_keys(0, right_sibling->right_sibling->size() - 1);
                //         return NULL;
                //     }
                // }

                // if (size() > lower_bound)
                // {
                //     right_sibling->take_a_leaf_from_leftsib();
                //     right_sibling->update_keys(0, right_sibling->size() - 1);
                //     return NULL;
                // }

                // // マージ
                // if (right_sibling->right_sibling != NULL)
                // {
                //     rem_node = right_sibling->right_sibling;
                //     merge(right_sibling, right_sibling->right_sibling); // 左側のノードに子をまとめるだけ
                //     return rem_node;
                // }

                // rem_node = right_sibling;
                // merge(this, right_sibling);
                // return rem_node;
            }
            else if (flag_of_vacant_leaf == '3') // 左家族の葉が削除された可能性がある場合
            {
                // assert(left_sibling != NULL);

                // if (left_sibling->size() >= lower_bound)
                //     return NULL;

                // // 子の共有
                // if (size() > lower_bound)
                // {
                //     left_sibling->take_a_leaf_from_rightsib(); // sizeの更新も
                //     left_sibling->update_key(left_sibling->size() - 1);
                //     update_keys(0, size() - 1);
                //     return NULL;
                // }

                // if (left_sibling->left_sibling != NULL)
                // {
                //     if ((left_sibling->get_left())->size() > lower_bound)
                //     {
                //         left_sibling->take_a_leaf_from_leftsib(); // 2つのノードのsize更新
                //         left_sibling->update_keys(0, left_sibling->size() - 1);
                //         return NULL;
                //     }
                // }

                // // マージ
                // merge(left_sibling, this); // 左側のノードに子をまとめるだけ
                // return this;
            }

            return NULL;
        }

        uint32_t find_vacant_leaf_and_shift(leaf_node *leaf, uint32_t remove_pos, char *flag, value_type x) // この関数の出力は空きのポジションを出力、本当に空きかどうかは見なくていい。この関数は葉ノードをいじる関数なので、内部ノード（this, sibling）のキーや子などは変更しなくてよい。
        {
            uint32_t vacant_pos;
            uint32_t num_of_keysharing_leaves = M_KEY_SHARING;
            bool there_is_vacant_sibling = find_vacant_leaf_on_the_left(remove_pos, &vacant_pos, &num_of_keysharing_leaves); // remove_posに変更したけどダイジョブ？

            if (there_is_vacant_sibling)
            {
                // leaves[vacant_pos]からleaves[remove_pos]へシフト
                shift_a_key_to_right(vacant_pos, remove_pos);
                *flag = '1';
                return vacant_pos;
            }

            // 左の兄弟に空きがなければ、右の兄弟を確認
            there_is_vacant_sibling = find_vacant_leaf_on_the_right(remove_pos, &vacant_pos, &num_of_keysharing_leaves);

            // 右の兄弟に空きがあれば右にシフト後、leafに挿入
            if (there_is_vacant_sibling)
            {
                // leaves[vacant_pos]からleaves[remove_pos]へシフト
                shift_a_key_to_left(remove_pos, vacant_pos); // insertとは違う関数を用いている。
                *flag = '1';
                return vacant_pos;
            }

            // if (num_of_keysharing_leaves > 0) // 兄弟葉ノードが全て一杯かつ、指定した共有ノード数よりチェックしたノード数が少ない時、隣家族の空きも、それぞれチェックする
            // {
            //     // 兄弟に空きが無いため、左家族の空き確認
            //     bool there_is_vacant_family = false;
            //     if (left_sibling != NULL)
            //     {
            //         uint32_t num_of_keysharing_leaves_on_the_left = M_KEY_SHARING / 2; // これで木が小さい時は M_KEY_SHARING*2 でキーを共有する
            //         there_is_vacant_family = left_sibling->find_vacant_leaf_on_the_left(left_sibling->size() - 1, &vacant_pos, &num_of_keysharing_leaves_on_the_left);
            //     }

            //     // 左家族に空きがある。この時左家族の最大値を変更するため、left->parent->keys_update()が必要
            //     if (there_is_vacant_family)
            //     {
            //         // 自家族の一番左からremove_posにシフト
            //         shift_a_key_to_right(0, remove_pos);
            //         // 左家族の一番右から自家族の一番左にシフト
            //         left_sibling->shift_a_key_to_right_family();
            //         // left_sibling->leaves[vacant_pos]からleft_sibling->leaves[left_sibling->size() - 1]にシフト
            //         left_sibling->shift_a_key_to_right(vacant_pos, left_sibling->size() - 1);
            //         *flag = '3';
            //         return vacant_pos;
            //     }

            //     // 左家族に空きがなければ、右家族を確認
            //     if (right_sibling != NULL)
            //     {
            //         uint32_t num_of_keysharing_leaves_on_the_right = M_KEY_SHARING / 2;
            //         there_is_vacant_family = right_sibling->find_vacant_leaf_on_the_right(0, &vacant_pos, &num_of_keysharing_leaves_on_the_right);
            //     }

            //     // 右家族に空きがある。この時右家族の最大値は変更されないため、right->parent->keys_update()は必要ない
            //     if (there_is_vacant_family)
            //     {
            //         // 自家族の一番右からremove_posにシフト
            //         shift_a_key_to_left(remove_pos, size() - 1);
            //         // 右家族の一番左から自家族の一番右にシフト
            //         right_sibling->shift_a_key_to_left_family();
            //         // 右家族のvacant_posから右家族の一番左にシフト
            //         right_sibling->shift_a_key_to_left(0, vacant_pos);
            //         *flag = '2';
            //         return vacant_pos;
            //     }
            // }

            *flag = '0';
            return 0;
        }

        leaf_node *remove_leaf(uint32_t pos)
        {
            leaf_node *rem_leaf = leaves[pos];

            for (int i = pos, trials = size() - 1; i < trials; ++i)
            {
                leaves[i] = leaves[i + 1];
            }

            num_of_leaves_--;

            return rem_leaf;
        }

        void take_a_leaf_from_rightsib() // キー更新以外は全部する
        {
            // assert(right_sibling != NULL);

            push_leaves_back(right_sibling->get_leaves_front());
            right_sibling->pop_leaves_front();
        }

        void take_a_leaf_from_leftsib()
        {
            // assert(left_sibling != NULL);

            push_leaves_front(left_sibling->get_leaves_back());
            left_sibling->pop_leaves_back();
        }

        void push_leaves_back(leaf_node *leaf)
        {
            // assert(num_of_leaves_ < M_LEAF_CHILDREN);

            leaves[size()] = leaf;
            num_of_leaves_++;
        }

        void push_leaves_front(leaf_node *leaf)
        {
            // assert(num_of_leaves_ < M_LEAF_CHILDREN);

            for (int i = size(); i > 0; --i)
            {
                leaves[i] = leaves[i - 1];
            }

            leaves[0] = leaf;
            num_of_leaves_++;
        }

        leaf_node *get_leaves_back()
        {
            return leaves[size() - 1];
        }

        leaf_node *get_leaves_front()
        {
            return leaves[0];
        }

        void pop_leaves_back()
        {
            num_of_leaves_--;
        }

        void pop_leaves_front()
        {
            for (int i = 0, trials = size() - 1; i < trials; ++i)
            {
                leaves[i] = leaves[i + 1];
            }

            num_of_leaves_--;
        }

        // 常に左側のノードを残す
        void merge(internal_node2 *remaining_node, internal_node2 *disappearing_node)
        {
            for (int i = 0, j = remaining_node->size(), trials = disappearing_node->size(); i < trials; ++i, ++j)
            {
                remaining_node->leaves[j] = disappearing_node->leaves[i];
                remaining_node->keys[j] = disappearing_node->keys[i];
            }

            remaining_node->num_of_leaves_ += disappearing_node->size();
            remaining_node->set_right(disappearing_node->get_right());
            if (disappearing_node->get_right() != NULL)
                (disappearing_node->get_right())->set_left(remaining_node);
        }

        void assert_all(internal_node1 *p = NULL)
        {
            assert(p == parent || p == NULL);
            assert(pos_ < M_INTERNAL_CHILDREN);
            assert(num_of_leaves_ <= M_LEAF_CHILDREN);
            for (int i = 0, trials = size() - 1; i < trials; ++i)
            {
                assert(keys[i] <= keys[i + 1]);
            }
            for (int i = 0, trials = size(); i < trials; ++i)
            {
                assert(keys[i] == leaves[i]->get_keys_back());
            }
            if (get_left() != NULL)
                assert(this == get_left()->get_right());
            if (get_right() != NULL)
                assert(this == get_right()->get_left());
        }

        leaf_node *get_leaf(uint32_t i)
        {
            return leaves[i];
        }

    private:
        value_type keys[M_LEAF_CHILDREN];
        leaf_node *leaves[M_LEAF_CHILDREN];

        internal_node1 *parent = NULL;
        internal_node2 *right_sibling = NULL;
        internal_node2 *left_sibling = NULL;

        uint32_t pos_ = 0;

        uint32_t num_of_leaves_ = 0;

        bool keys_is_empty_ = true;
    };

    template <class value_type,
              uint32_t M_KEY_SHARING,       // キーの共有を行う葉の最大数
              uint32_t M_LEAF,              // 葉の最大要素数
              uint32_t M_INTERNAL_CHILDREN, // 内部ノードの最大要素数（子が内部ノード）
              uint32_t M_LEAF_CHILDREN>     // 内部ノードの最大要素数 (子が葉)
    class lb_btree<value_type, M_KEY_SHARING, M_LEAF, M_INTERNAL_CHILDREN, M_LEAF_CHILDREN>::leaf_node
    {
    public:
        leaf_node()
        {
        }

        leaf_node(std::vector<value_type> &c)
        {
            num_of_keys_ = c.size();
            tail = size() - 1;

            for (int i = 0, trials = size(); i < trials; ++i)
            {
                keys[i] = c.at(i);
            }
        }

        ~leaf_node()
        {
        }

        uint32_t byte_size()
        {
            return sizeof(leaf_node);
        }

        void insert(value_type x)
        {
            // assert(not is_full());

            if (keys_empty())
            {
                keys[head] = x;
                tail++;
                num_of_keys_++;

                // assert_all();
                return;
            }

            if (x <= keys[head])
            {
                // 先頭に挿入
                push_front(x);

                // assert_all();
                return;
            }

            if (keys[tail] <= x)
            {
                // 後尾に挿入
                push_back(x);

                // assert_all();
                return;
            }

            int j1 = tail;

            tail++;
            if (tail == M_LEAF)
                tail = 0;

            int j2 = tail;

            for (int i = 0, trials = size(); i < trials; ++i)
            {
                if (x >= keys[j1])
                {
                    keys[j2] = x;
                    break;
                }

                keys[j2] = keys[j1];
                j1--;
                if (j1 == UINT32_MAX)
                    j1 = M_LEAF - 1;
                j2--;
                if (j2 == UINT32_MAX)
                    j2 = M_LEAF - 1;
            }

            num_of_keys_++;

            // 大小チェック
            // assert_all();
        }

        // 論理エラーの可能性あり
        leaf_node *split()
        {
            // assert(size() == M_LEAF);

            leaf_node *right = NULL;

            std::vector<value_type> right_keys(size() / 2);

            int j = (head + size() - size() / 2) % M_LEAF;
            for (int i = 0, trials = size() / 2; i < trials; ++i)
            {
                right_keys[i] = keys[j];

                j++;
                if (j == M_LEAF)
                    j = 0;
            }

            right = new leaf_node(right_keys);

            num_of_keys_ = size() - size() / 2;
            tail = (head + size() - 1) % M_LEAF;

            // thisとrightの大小チェック
            // assert_all();
            // right->assert_all();

            return right;
        }

        bool keys_empty()
        {
            return size() == 0;
        }

        bool is_full()
        {
            // assert(size() <= M_LEAF);

            return size() == M_LEAF;
        }

        value_type get_keys_back()
        {
            return keys[tail];
        }

        value_type get_keys_front()
        {
            return keys[head];
        }

        void pop_back()
        {
            tail--;
            if (tail == UINT32_MAX)
                tail = M_LEAF - 1;

            num_of_keys_--;
        }

        void pop_front()
        {
            head++;
            if (head == M_LEAF)
                head = 0;

            num_of_keys_--;
        }

        void push_back(value_type x)
        {
            // assert(not is_full());

            tail++;
            if (tail == M_LEAF)
                tail = 0;

            keys[tail] = x;

            num_of_keys_++;
        }

        void push_front(value_type x)
        {
            // assert(not is_full());

            head--;
            if (head == UINT32_MAX)
                head = M_LEAF - 1;

            keys[head] = x;

            num_of_keys_++;
        }

        const uint32_t size()
        {
            return num_of_keys_;
        }

        // 削除類
        void remove(value_type x)
        {
            if (x == get_keys_front())
            {
                pop_front();
            }
            else if (x == get_keys_back())
            {
                pop_back();
            }
            else
            {
                // uint32_t i = tail - 1;
                // if (i == UINT32_MAX)
                //     i = M_LEAF - 1;

                uint32_t remove_pos;

                for (int i = head + 1, count = 0, trials = size() - 2; count < trials; ++i, ++count)
                {
                    if (x == keys[i % M_LEAF])
                    {
                        remove_pos = i % M_LEAF;
                        break;
                    }
                }

                for (int i = remove_pos; i % M_LEAF != tail; ++i)
                {
                    keys[i % M_LEAF] = keys[(i + 1) % M_LEAF];
                }

                tail--;
                if (tail == UINT32_MAX)
                    tail = M_LEAF - 1;
                num_of_keys_--;
            }

            // 大小チェック
            // assert_all();
        }

        void assert_all()
        {
            assert(head < M_LEAF);
            assert(tail < M_LEAF);
            assert(num_of_keys_ <= M_LEAF);
            for (int i = head, j = head + 1, count = 0, trials = size() - 1; count < trials; ++i, ++j, ++count)
            {
                assert(keys[i % M_LEAF] <= keys[j % M_LEAF]);
            }
        }

        value_type get_key(uint32_t i)
        {
            return keys[(head + i) % M_LEAF];
        }

    private:
        value_type keys[M_LEAF];
        uint32_t head = 0;
        uint32_t tail = UINT32_MAX;

        uint32_t num_of_keys_ = 0;
    };

} // namespace lb

#endif /* LOAD_BALANCING_BTREE_HPP_ */
