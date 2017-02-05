#ifndef BOOST_CONTAINER_ROPE_NODE
#define BOOST_CONTAINER_ROPE_NODE

#include <memory>
#include <string>
#include <vector>

#include <boost/make_unique.hpp>

namespace boost { namespace container
{
// A rope_node represents a string as a binary tree of string fragments
//
// A rope_node consists of:
//   - a non-negative integer weight
//   - a pointer to a left child rope_node
//   - a pointer to a right child rope_node
//   - a string fragment
//
// INVARIANTS:
//   - a leaf is represented as a rope_node with null child pointers
//   - a leaf node's weight is equal to the length of the string fragment it contains
//   - an internal node is represented as a rope_node with non-null children and
//     an empty string fragment
//   - an internal node's weight is equal to the length of the string fragment
//     contained in (the leaf nodes of) its left subtree

auto ERROR_OOB_NODE = std::out_of_range("Error: out of range exception.");

template <typename T>
class rope_node
{

public:
    using cont_type = T;
    using value_type = typename T::value_type;
    using size_type = size_t;
    using reference = value_type&;
    using const_reference = const value_type&;

    using handle = std::unique_ptr<rope_node>;

    // CONSTRUCTORS
    // Construct internal node by concatenating the given nodes
    rope_node(handle l, handle r)
            : fragment_(cont_type())
    {
        this->left_ = move(l);
        this->right_ = move(r);
        this->weight_ = this->left_->length();
    }

    // Construct leaf node from the given string
    rope_node(const cont_type& str)
            : weight_(str.size()), left_(nullptr), right_(nullptr), fragment_(str)
    {}

    // Copy constructor
    rope_node(const rope_node& aNode)
            : weight_(aNode.weight_), fragment_(aNode.fragment_)
    {
        rope_node* tmpLeft = aNode.left_.get();
        rope_node* tmpRight = aNode.right_.get();
        if (tmpLeft == nullptr)
        {
            this->left_ = nullptr;
        }
        else
        {
            this->left_ = boost::make_unique<rope_node>(*tmpLeft);
        }
        if (tmpRight == nullptr)
        {
            this->right_ = nullptr;
        }
        else
        {
            this->right_ = boost::make_unique<rope_node>(*tmpRight);
        }
    }

    // ACCESSORS
    size_t length() const
    {
        if (this->is_leaf())
        {
            return this->weight_;
        }
        size_t tmp = (this->right_ == nullptr) ? 0 : this->right_->length();
        return this->weight_ + tmp;
    }


    reference get_item(size_t index)
    {
        size_t w = this->weight_;
        // if node is a leaf, return the character at the specified index
        if (this->is_leaf())
        {
            return this->fragment_[index];
            // else search the appropriate child node
        }
        else
        {
            if (index < w)
            {
                return this->left_->get_item(index);
            }
            else
            {
                return this->right_->get_item(index - w);
            }
        }
    }

    reference get_item_safe(size_t index)
    {
        size_t w = this->weight_;
        // if node is a leaf, return the character at the specified index
        if (this->is_leaf())
        {
            if(index >= weight_)
            {
                throw ERROR_OOB_NODE;
            }
            return this->fragment_[index];
            // else search the appropriate child node
        }
        else
        {
            if (index < w)
            {
                return this->left_->get_item_safe(index);
            }
            else
            {
                return this->right_->get_item_safe(index - w);
            }
        }
    }


    // Get the substring of (len) chars beginning at index (start)
    cont_type substr(size_t start, size_t len) const
    {
        size_t w = this->weight_;
        if (this->is_leaf())
        {
            if (len < w)
            {
                return this->fragment_.substr(start, len);
            }
            else
            {
                return this->fragment_;
            }
        }
        else
        {
            // check if start index in left subtree
            if (start < w)
            {
                cont_type lResult = (this->left_ == nullptr) ? cont_type() : this->left_->substr(start, len);
                if ((start + len) > w)
                {
                    // get number of characters in left subtree
                    size_t tmp = w - start;
                    cont_type rResult = (this->right_ == nullptr) ? cont_type() : this->right_->substr(w, len - tmp);
                    return lResult.append(rResult);
                }
                else
                {
                    return lResult;
                }
                // if start index is in the right subtree...
            }
            else
            {
                return (this->right_ == nullptr) ? cont_type() : this->right_->substr(start - w, len);
            }
        }
    }

    // Get string contained in current node and its children
    cont_type tree_to_string() const
    {
        if (this->is_leaf())
        {
            return this->fragment_;
        }
        cont_type lResult = (this->left_ == nullptr) ? cont_type() : this->left_->tree_to_string();
        cont_type rResult = (this->right_ == nullptr) ? cont_type() : this->right_->tree_to_string();
        return lResult.append(rResult);
    }

    // MUTATORS
    // Split the represented string at the specified index
    friend std::pair<handle, handle> split(handle node, size_t index)
    {
        size_t w = node->weight_;
        // if the given node is a leaf, split the leaf
        if (node->is_leaf())
        {
            return std::pair<handle, handle>{
                    boost::make_unique<rope_node>(node->fragment_.substr(0, index)),
                    boost::make_unique<rope_node>(node->fragment_.substr(index, w - index))
            };
        }

        // if the given node is a concat (internal) node, compare index to weight and handle
        //   accordingly
        handle oldRight = move(node->right_);
        if (index < w)
        {
            node->right_ = nullptr;
            node->weight_ = index;
            std::pair<handle, handle> splitLeftResult = split(move(node->left_), index);
            node->left_ = move(splitLeftResult.first);
            return std::pair<handle, handle>{
                    move(node),
                    boost::make_unique<rope_node>(move(splitLeftResult.second), move(oldRight))
            };
        }
        else if (w < index)
        {
            std::pair<handle, handle> splitRightResult = split(move(oldRight), index - w);
            node->right_ = move(splitRightResult.first);
            return std::pair<handle, handle>{
                    move(node),
                    move(splitRightResult.second)
            };
        }
        else
        {
            return std::pair<handle, handle>{
                    move(node->left_), move(oldRight)
            };
        }
    }

    // HELPERS
    // Functions used in balancing
    size_t depth() const
    {
        if (this->is_leaf())
        {
            return 0;
        }
        size_t lResult = (this->left_ == nullptr) ? 0 : this->left_->depth();
        size_t rResult = (this->right_ == nullptr) ? 0 : this->right_->depth();
        return std::max(++lResult, ++rResult);
    }

    void getLeaves(std::vector<rope_node*>& v)
    {
        if (this->is_leaf())
        {
            v.push_back(this);
        }
        else
        {
            rope_node* tmpLeft = this->left_.get();
            if (tmpLeft != nullptr)
            {
                tmpLeft->getLeaves(v);
            }
            rope_node* tmpRight = this->right_.get();
            if (tmpRight != nullptr)
            {
                tmpRight->getLeaves(v);
            }
        }
    }

private:

    // Determine whether a node is a leaf
    bool is_leaf() const
    {
        return this->left_ == nullptr && this->right_ == nullptr;
    }

    size_t weight_;
    handle left_, right_;
    cont_type fragment_;

}; // class rope_node

} // namespace container
} // namespace boost

#endif //BOOST_CONTAINER_ROPE_NODE