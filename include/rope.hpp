#ifndef BOOST_CONTAINER_ROPE
#define BOOST_CONTAINER_ROPE

#include <cmath>
#include <algorithm>

#include <boost/make_unique.hpp>

#include "node.hpp"

namespace boost { namespace container
{
// A rope represents a string as a binary tree wherein the leaves contain fragments of the
//   string. More accurately, a rope consists of a pointer to a root rope_node, which
//   describes a binary tree of string fragments.

// Examples:
//
//        X        |  null root pointer, represents empty string
//
//      "txt"      |
//     /     \     |  root is a leaf node containing a string fragment
//    X       X    |
//
//        O        |
//       / \       |
//  "some" "text"  |  root is an internal node formed by the concatenation of two distinct
//    /\     /\    |  ropes containing the strings "some" and "text"
//   X  X   X  X   |



//Utilities
// Compute the nth Fibonacci number, in O(log(n)) time
//https://en.wikibooks.org/w/index.php?title=Algorithm_Implementation/Mathematics/Fibonacci_Number_Program
size_t fib(size_t n)
{
    size_t a = 1, ta,
            b = 1, tb,
            c = 1, rc = 0, tc,
            d = 0, rd = 1;

    while (n)
    {
        if (n & 1)
        {
            tc = rc;
            rc = rc * a + rd * c;
            rd = tc * b + rd * d;
        }

        ta = a;
        tb = b;
        tc = c;
        a = a * a + b * c;
        b = ta * b + b * d;
        c = c * ta + d * c;
        d = tc * tb + d * d;

        n >>= 1;

    }
    return rc;
}

// Compute approximately the nth Fibonacci number, in O(1) time
// https://en.wikibooks.org/w/index.php?title=Algorithm_Implementation/Mathematics/Fibonacci_Number_Program
size_t fast_fib(size_t n)
{
    double fi = (1.0 + sqrt(5.0)) / 2.0;
    return static_cast<size_t>((pow(fi, n) - pow(1 - fi, n)) / sqrt(5.0));
}

// Construct a vector where the nth entry represents the interval between the
//   Fibonacci numbers F(n+2) and F(n+3), and the final entry includes the number
//   specified in the length parameter
// e.g. buildFibList(0) -> {}
//      buildFibList(1) -> {[1,2)}
//      buildFibList(8) -> {[1,2),[2,3),[3,5),[5,8),[8,13)}
std::vector<size_t> build_fib_list(size_t len)
{
    // initialize a and b to the first and second fib numbers respectively
    int a = 0, b = 1, next;
    std::vector<size_t> intervals = std::vector<size_t>();
    while (a <= len)
    {
        if (a > 0)
        {
            intervals.push_back(b);
        }
        next = a + b;
        a = b;
        b = next;
    }
    return intervals;
}


auto ERROR_OOB_ROPE = std::out_of_range("Error: out of range exception.");

template<typename T>
class rope
{

public:
    using cont_type = T;
    using value_type = typename T::value_type;
    using size_type = size_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using handle = std::unique_ptr<rope_node<cont_type>>;

    // CONSTRUCTORS
    // Default constructor - produces a rope representing the empty string
    rope() : rope(cont_type())
    {}

    // Construct a rope from the given string
    rope(const T& str)
    {
        this->root_ = boost::make_unique<rope_node<cont_type>>(str);
    }

    // Copy constructor
    rope(const rope& r)
    {
        rope_node<cont_type> newRoot = rope_node<cont_type>(*r.root_);
        this->root_ = boost::make_unique<rope_node<cont_type>>(newRoot);
    }

    // Get the string stored in the rope
    cont_type to_string() const
    {
        if (this->root_ == nullptr)
        {
            return cont_type();
        }
        return this->root_->tree_to_string();
    }

    // Get the length of the stored string
    size_t length() const
    {
        if (this->root_ == nullptr)
        {
            return 0;
        }
        return this->root_->length();
    }

    // Get the character at the given position in the represented string
    const_reference at(size_t index) const
    {
        if (this->root_ == nullptr)
        {
            throw ERROR_OOB_ROPE;
        }
        return this->root_->getCharByIndex(index);
    }

    // Get the character at the given position in the represented string
    const_reference operator[](size_t index) const
    {
        return this->root_->getCharByIndex(index);
    }

    // Return the substring of length (len) beginning at the specified index
    cont_type substr(size_t start, size_t len) const
    {
        size_t actualLength = this->length();
        if (start > actualLength || (start + len) > actualLength)
        {
            throw ERROR_OOB_ROPE;
        }
        return this->root_->getSubstring(start, len);
    }

    // Determine if rope is balanced
    bool is_balanced() const
    {
        if (this->root_ == nullptr)
        {
            return true;
        }
        size_t d = this->root_->depth();
        return this->length() >= fib(d + 2);
    }

    // Balance the rope
    void balance()
    {
        // initiate rebalancing only if rope is unbalanced
        if (!this->is_balanced())
        {
            // build vector representation of Fibonacci intervals
            std::vector<size_t> intervals = build_fib_list(this->length());
            std::vector<handle> nodes(intervals.size());

            // get leaf nodes
            std::vector<rope_node<cont_type>*> leaves;
            this->root_->getLeaves(leaves);

            size_t i;
            size_t max_i = intervals.size() - 1;
            size_t currMaxInterval = 0;
            handle acc = nullptr;
            handle tmp = nullptr;

            // attempt to insert each leaf into nodes vector based on length
            for (auto& leaf : leaves)
            {
                size_t len = leaf->length();

                // ignore empty leaf nodes
                if (len > 0)
                {
                    acc = boost::make_unique<rope_node<cont_type>>(*leaf);
                    i = 0;

                    bool inserted = false;
                    while (!inserted)
                    {
                        // find appropriate slot for the acc node to be inserted,
                        // concatenating with nodes encountered along the way
                        while (i < max_i && len >= intervals[i + 1])
                        {
                            if (nodes[i].get() != nullptr)
                            {
                                // concatenate encountered entries with node to be inserted
                                tmp = boost::make_unique<rope_node<cont_type>>(*nodes[i].get());
                                acc = boost::make_unique<rope_node<cont_type>>(*acc.get());
                                acc = boost::make_unique<rope_node<cont_type>>(move(tmp), move(acc));

                                // update len
                                len = acc->length();

                                // if new length is sufficiently great that the node must be
                                //   moved to a new slot, we clear out the existing entry
                                if (len >= intervals[i + 1])
                                {
                                    nodes[i] = nullptr;
                                }
                            }
                            ++i;
                        }

                        // target slot found -- check if occupied
                        if (nodes[i].get() == nullptr)
                        {
                            nodes[i].swap(acc);
                            inserted = true;
                            // update currMaxInterval if necessary
                            if (i > currMaxInterval)
                            {
                                currMaxInterval = i;
                            }
                        }
                        else
                        {
                            // concatenate encountered entries with node to be inserted
                            tmp = boost::make_unique<rope_node<cont_type>>(*nodes[i].get());
                            acc = boost::make_unique<rope_node<cont_type>>(*acc.get());
                            acc = boost::make_unique<rope_node<cont_type>>(move(tmp), move(acc));

                            // update len
                            len = acc->length();

                            // if new length is sufficiently great that the node must be
                            //   moved to a new slot, we clear out the existing entry
                            if (len >= intervals[i + 1])
                            {
                                nodes[i] = nullptr;
                            }
                        }
                    }
                }
            }

            // concatenate remaining entries to produce balanced rope
            acc = move(nodes[currMaxInterval]);
            for (int idx = currMaxInterval; idx >= 0; --idx)
            {
                if (nodes[idx] != nullptr)
                {
                    tmp = boost::make_unique<rope_node<cont_type>>(*nodes[idx].get());
                    acc = boost::make_unique<rope_node<cont_type>>(move(acc), move(tmp));
                }
            }
            this->root_ = move(acc); // reset root

        }
    }

    // MUTATORS
    // Insert the given string/rope into the rope, beginning at the specified index (i)
    void insert(size_t i, const cont_type& str)
    {
        this->insert(i, rope(str));
    }

    void insert(size_t i, const rope& r)
    {
        if (this->length() < i)
        {
            throw ERROR_OOB_ROPE;
        }
        else
        {
            rope tmp = rope(r);
            std::pair<handle, handle> origRopeSplit = split(move(this->root_), i);
            handle tmpConcat = boost::make_unique<rope_node<cont_type>>(move(origRopeSplit.first), move(tmp.root_));
            this->root_ = boost::make_unique<rope_node<cont_type>>(move(tmpConcat), move(origRopeSplit.second));
        }
    }

    // Concatenate the existing string/rope with the argument
    void append(const cont_type& str)
    {
        rope tmp = rope(str);
        this->root_ = boost::make_unique<rope_node<cont_type>>(move(this->root_), move(tmp.root_));
    }

    void append(const rope& r)
    {
        rope tmp = rope(r);
        this->root_ = boost::make_unique<rope_node<cont_type>>(move(this->root_), move(tmp.root_));
    }

    // Delete the substring of (len) characters beginning at index (start)
    void erase(size_t start, size_t len)
    {
        size_t actualLength = this->length();
        if (start > actualLength || start + len > actualLength)
        {
            throw ERROR_OOB_ROPE;
        }
        else
        {
            std::pair<handle, handle> firstSplit = split(move(this->root_), start);
            std::pair<handle, handle> secondSplit = split(move(firstSplit.second), len);
            secondSplit.first.reset();
            this->root_ = boost::make_unique<rope_node<cont_type>>(move(firstSplit.first), move(secondSplit.second));
        }
    }

    // OPERATORS
    rope& operator=(const rope& rhs)
    {
        // check for self-assignment
        if (this == &rhs)
        {
            return *this;
        }
        // delete existing rope to recover memory
        this->root_.reset();
        // invoke copy constructor
        this->root_ = boost::make_unique<rope_node<cont_type>>(*(rhs.root_.get()));
        return *this;
    }

    bool operator==(const rope& rhs) const
    {
        return this->to_string() == rhs.to_string();
    }

    bool operator!=(const rope& rhs) const
    {
        return !(*this == rhs);
    }

    friend std::ostream& operator<<(std::ostream& out, const rope& r)
    {
        return out << r.to_string();
    }

private:

    // Pointer to the root of the rope tree
    handle root_;

}; // class rope

} // namespace container
} // namespace boost
#endif //BOOST_CONTAINER_ROPE