/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2007
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_INTRUSIVE_RBTREE_HPP
#define BOOST_INTRUSIVE_RBTREE_HPP

#include <boost/intrusive/detail/config_begin.hpp>
#include <functional>
#include <iterator>
#include <utility>
#include <boost/intrusive/detail/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/intrusive/intrusive_fwd.hpp>
#include <boost/intrusive/detail/pointer_to_other.hpp>
#include <boost/intrusive/set_hook.hpp>
#include <boost/intrusive/detail/rbtree_node.hpp>
#include <boost/intrusive/detail/ebo_functor_holder.hpp>
#include <boost/intrusive/options.hpp>
#include <boost/intrusive/rbtree_algorithms.hpp>
#include <boost/intrusive/link_mode.hpp>
#include <cstddef>
#include <iterator>

namespace boost {
namespace intrusive {

/// @cond

template <class T>
struct internal_default_set_hook
{
   template <class U> static detail::one test(...);
   template <class U> static detail::two test(typename U::default_set_hook* = 0);
   static const bool value = sizeof(test<T>(0)) == sizeof(detail::two);
};

template <class T>
struct get_default_set_hook
{
   typedef typename T::default_set_hook type;
};

template <class ValueTraits, class Compare, class SizeType, bool ConstantTimeSize>
struct setopt
{
   typedef ValueTraits  value_traits;
   typedef Compare      compare;
   typedef SizeType     size_type;
   static const bool constant_time_size = ConstantTimeSize;
};

template <class T>
struct set_defaults
   :  pack_options
      < none
      , base_hook
         <  typename detail::eval_if_c
               < internal_default_set_hook<T>::value
               , get_default_set_hook<T>
               , detail::identity<none>
               >::type
         >
      , constant_time_size<true>
      , size_type<std::size_t>
      , compare<std::less<T> >
      >::type
{};

/// @endcond

//! The class template rbtree is an intrusive red-black tree container, that
//! is used to construct intrusive set and tree containers. The no-throw 
//! guarantee holds only, if the value_compare object 
//! doesn't throw.
//!
//! The template parameter \c T is the type to be managed by the container.
//! The user can specify additional options and if no options are provided
//! default options are used.
//!
//! The container supports the following options:
//! \c base_hook<>/member_hook<>/value_traits<>,
//! \c constant_time_size<>, \c size_type<> and
//! \c compare<>.
#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
template<class T, class ...Options>
#else
template<class Config>
#endif
class rbtree_impl
{
   public:
   typedef typename Config::value_traits                             value_traits;
   /// @cond
   static const bool external_value_traits =
      detail::external_value_traits_is_true<value_traits>::value;
   typedef typename detail::eval_if_c
      < external_value_traits
      , detail::eval_value_traits<value_traits>
      , detail::identity<value_traits>
      >::type                                                        real_value_traits;
   /// @endcond
   typedef typename real_value_traits::pointer                       pointer;
   typedef typename real_value_traits::const_pointer                 const_pointer;
   typedef typename std::iterator_traits<pointer>::value_type        value_type;
   typedef value_type                                                key_type;
   typedef typename std::iterator_traits<pointer>::reference         reference;
   typedef typename std::iterator_traits<const_pointer>::reference   const_reference;
   typedef typename std::iterator_traits<pointer>::difference_type   difference_type;
   typedef typename Config::size_type                                size_type;
   typedef typename Config::compare                                  value_compare;
   typedef value_compare                                             key_compare;
   typedef rbtree_iterator<rbtree_impl, false>                       iterator;
   typedef rbtree_iterator<rbtree_impl, true>                        const_iterator;
   typedef std::reverse_iterator<iterator>                           reverse_iterator;
   typedef std::reverse_iterator<const_iterator>                     const_reverse_iterator;
   typedef typename real_value_traits::node_traits                        node_traits;
   typedef typename node_traits::node                                node;
   typedef typename boost::pointer_to_other
      <pointer, node>::type                                          node_ptr;
   typedef typename boost::pointer_to_other
      <node_ptr, const node>::type                                   const_node_ptr;
   typedef rbtree_algorithms<node_traits>                            node_algorithms;

   static const bool constant_time_size = Config::constant_time_size;
   static const bool stateful_value_traits = detail::store_cont_ptr_on_it<rbtree_impl>::value;

   /// @cond
   private:
   typedef detail::size_holder<constant_time_size, size_type>        size_traits;

   //noncopyable
   rbtree_impl (const rbtree_impl&);
   rbtree_impl operator =(const rbtree_impl&);

   enum { safemode_or_autounlink  = 
            (int)real_value_traits::link_mode == (int)auto_unlink   ||
            (int)real_value_traits::link_mode == (int)safe_link     };

   //Constant-time size is incompatible with auto-unlink hooks!
   BOOST_STATIC_ASSERT(!(constant_time_size && ((int)real_value_traits::link_mode == (int)auto_unlink)));

   struct header_plus_size : public size_traits
   {  node header_;  };

   struct node_plus_pred_t : public detail::ebo_functor_holder<value_compare>
   {
      node_plus_pred_t(const value_compare &comp)
         :  detail::ebo_functor_holder<value_compare>(comp)
      {}
      header_plus_size header_plus_size_;
   };

   struct data_t : public rbtree_impl::value_traits
   {
      typedef typename rbtree_impl::value_traits value_traits;
      data_t(const value_compare & comp, const value_traits &val_traits)
         :  value_traits(val_traits), node_plus_pred_(comp)
      {}
      node_plus_pred_t node_plus_pred_;
   } data_;
  
   const value_compare &priv_comp() const
   {  return data_.node_plus_pred_.get();  }

   value_compare &priv_comp()
   {  return data_.node_plus_pred_.get();  }

   const node &priv_header() const
   {  return data_.node_plus_pred_.header_plus_size_.header_;  }

   node &priv_header()
   {  return data_.node_plus_pred_.header_plus_size_.header_;  }

   static node_ptr uncast(const_node_ptr ptr)
   {
      return node_ptr(const_cast<node*>(detail::get_pointer(ptr)));
   }

   size_traits &priv_size_traits()
   {  return data_.node_plus_pred_.header_plus_size_;  }

   const size_traits &priv_size_traits() const
   {  return data_.node_plus_pred_.header_plus_size_;  }

   const real_value_traits &get_real_value_traits(detail::bool_<false>) const
   {  return data_;  }

   const real_value_traits &get_real_value_traits(detail::bool_<true>) const
   {  return data_.get_value_traits(*this);  }

   real_value_traits &get_real_value_traits(detail::bool_<false>)
   {  return data_;  }

   real_value_traits &get_real_value_traits(detail::bool_<true>)
   {  return data_.get_value_traits(*this);  }

   /// @endcond

   public:

   const real_value_traits &get_real_value_traits() const
   {  return this->get_real_value_traits(detail::bool_<external_value_traits>());  }

   real_value_traits &get_real_value_traits()
   {  return this->get_real_value_traits(detail::bool_<external_value_traits>());  }

   typedef typename node_algorithms::insert_commit_data insert_commit_data;

   //! <b>Effects</b>: Constructs an empty tree. 
   //!   
   //! <b>Complexity</b>: Constant. 
   //! 
   //! <b>Throws</b>: Nothing unless the copy constructor of the value_compare object throws. 
   rbtree_impl( value_compare cmp = value_compare()
              , const value_traits &v_traits = value_traits()) 
      :  data_(cmp, v_traits)
   {  
      node_algorithms::init_header(&priv_header());  
      this->priv_size_traits().set_size(size_type(0));
   }

   //! <b>Requires</b>: Dereferencing iterator must yield an lvalue of type value_type.
   //!   cmp must be a comparison function that induces a strict weak ordering.
   //!
   //! <b>Effects</b>: Constructs an empty tree and inserts elements from
   //!   [b, e).
   //!
   //! <b>Complexity</b>: Linear in N if [b, e) is already sorted using
   //!   comp and otherwise N * log N, where N is last � first.
   //! 
   //! <b>Throws</b>: Nothing unless the copy constructor of the value_compare object throws.
   template<class Iterator>
   rbtree_impl( bool unique, Iterator b, Iterator e
              , value_compare cmp = value_compare()
              , const value_traits &v_traits = value_traits())
      : data_(cmp, v_traits)
   {
      node_algorithms::init_header(&priv_header());
      this->priv_size_traits().set_size(size_type(0));
      if(unique)
         this->insert_unique(b, e);
      else
         this->insert_equal(b, e);
   }

   //! <b>Effects</b>: Detaches all elements from this. The objects in the set 
   //!   are not deleted (i.e. no destructors are called), but the nodes according to 
   //!   the value_traits template parameter are reinitialized and thus can be reused. 
   //! 
   //! <b>Complexity</b>: Linear to elements contained in *this. 
   //! 
   //! <b>Throws</b>: Nothing.
   ~rbtree_impl() 
   {  this->clear(); }

   //! <b>Effects</b>: Returns an iterator pointing to the beginning of the tree.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   iterator begin()
   {  return iterator (node_traits::get_left(node_ptr(&priv_header())), this);   }

   //! <b>Effects</b>: Returns a const_iterator pointing to the beginning of the tree.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   const_iterator begin() const
   {  return cbegin();   }

   //! <b>Effects</b>: Returns a const_iterator pointing to the beginning of the tree.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   const_iterator cbegin() const
   {  return const_iterator (node_traits::get_left(const_node_ptr(&priv_header())), this);   }

   //! <b>Effects</b>: Returns an iterator pointing to the end of the tree.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   iterator end()
   {  return iterator (node_ptr(&priv_header()), this);  }

   //! <b>Effects</b>: Returns a const_iterator pointing to the end of the tree.
   //!
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   const_iterator end() const
   {  return cend();  }

   //! <b>Effects</b>: Returns a const_iterator pointing to the end of the tree.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   const_iterator cend() const
   {  return const_iterator (uncast(const_node_ptr(&priv_header())), this);  }

   //! <b>Effects</b>: Returns a reverse_iterator pointing to the beginning of the
   //!    reversed tree.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   reverse_iterator rbegin()
   {  return reverse_iterator(end());  }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the beginning
   //!    of the reversed tree.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   const_reverse_iterator rbegin() const
   {  return const_reverse_iterator(end());  }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the beginning
   //!    of the reversed tree.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   const_reverse_iterator crbegin() const
   {  return const_reverse_iterator(end());  }

   //! <b>Effects</b>: Returns a reverse_iterator pointing to the end
   //!    of the reversed tree.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   reverse_iterator rend()
   {  return reverse_iterator(begin());   }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the end
   //!    of the reversed tree.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   const_reverse_iterator rend() const
   {  return const_reverse_iterator(begin());   }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the end
   //!    of the reversed tree.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   const_reverse_iterator crend() const
   {  return const_reverse_iterator(begin());   }

   //! <b>Precondition</b>: end_iterator must be a valid end iterator
   //!   of rbtree.
   //! 
   //! <b>Effects</b>: Returns a const reference to the rbtree associated to the end iterator
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   static rbtree_impl &container_from_end_iterator(iterator end_iterator)
   {  return priv_container_from_end_iterator(end_iterator);   }

   //! <b>Precondition</b>: end_iterator must be a valid end const_iterator
   //!   of rbtree.
   //! 
   //! <b>Effects</b>: Returns a const reference to the rbtree associated to the end iterator
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   static const rbtree_impl &container_from_end_iterator(const_iterator end_iterator)
   {  return priv_container_from_end_iterator(end_iterator);   }

   //! <b>Effects</b>: Returns the value_compare object used by the tree.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: If value_compare copy-constructor throws.
   value_compare value_comp() const
   {  return priv_comp();   }

   //! <b>Effects</b>: Returns true is the container is empty.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   bool empty() const
   {  return node_algorithms::unique(const_node_ptr(&priv_header()));   }

   //! <b>Effects</b>: Returns the number of elements stored in the tree.
   //! 
   //! <b>Complexity</b>: Linear to elements contained in *this.
   //! 
   //! <b>Throws</b>: Nothing.
   size_type size() const
   {
      if(constant_time_size)
         return this->priv_size_traits().get_size();
      else
         return empty() ? 0 : node_algorithms::count(node_traits::get_parent(const_node_ptr(&priv_header())));
   }

   //! <b>Effects</b>: Swaps the contents of two multisets.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: If the comparison functor's none swap call throws.
   void swap(rbtree_impl& other)
   {
      //This can throw
      using std::swap;
      swap(priv_comp(), priv_comp());
      //These can't throw
      node_algorithms::swap_tree(node_ptr(&priv_header()), node_ptr(&other.priv_header()));
      if(constant_time_size){
         size_type backup = this->priv_size_traits().get_size();
         this->priv_size_traits().set_size(other.priv_size_traits().get_size());
         other.priv_size_traits().set_size(backup);
      }
   }

   //! <b>Requires</b>: value must be an lvalue
   //! 
   //! <b>Effects</b>: Inserts value into the tree before the upper bound.
   //! 
   //! <b>Complexity</b>: Average complexity for insert element is at
   //!   most logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   No copy-constructors are called.
   iterator insert_equal_upper_bound(reference value)
   {
      detail::key_nodeptr_comp<value_compare, rbtree_impl>
         key_node_comp(priv_comp(), this);
      node_ptr to_insert(get_real_value_traits().to_node_ptr(value));
      if(safemode_or_autounlink)
         BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(node_algorithms::unique(to_insert));
      this->priv_size_traits().increment();
      return iterator(node_algorithms::insert_equal_upper_bound
         (node_ptr(&priv_header()), to_insert, key_node_comp), this);
   }

   //! <b>Requires</b>: value must be an lvalue
   //! 
   //! <b>Effects</b>: Inserts value into the tree before the lower bound.
   //! 
   //! <b>Complexity</b>: Average complexity for insert element is at
   //!   most logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   No copy-constructors are called.
   iterator insert_equal_lower_bound(reference value)
   {
      detail::key_nodeptr_comp<value_compare, rbtree_impl>
         key_node_comp(priv_comp(), this);
      node_ptr to_insert(get_real_value_traits().to_node_ptr(value));
      if(safemode_or_autounlink)
         BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(node_algorithms::unique(to_insert));
      this->priv_size_traits().increment();
      return iterator(node_algorithms::insert_equal_lower_bound
         (node_ptr(&priv_header()), to_insert, key_node_comp), this);
   }

   //! <b>Requires</b>: value must be an lvalue, and "hint" must be
   //!   a valid iterator.
   //! 
   //! <b>Effects</b>: Inserts x into the tree, using "hint" as a hint to
   //!   where it will be inserted. If "hint" is the upper_bound
   //!   the insertion takes constant time (two comparisons in the worst case)
   //! 
   //! <b>Complexity</b>: Logarithmic in general, but it is amortized
   //!   constant time if t is inserted immediately before hint.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   No copy-constructors are called.
   iterator insert_equal(const_iterator hint, reference value)
   {
      detail::key_nodeptr_comp<value_compare, rbtree_impl>
         key_node_comp(priv_comp(), this);
      node_ptr to_insert(get_real_value_traits().to_node_ptr(value));
      if(safemode_or_autounlink)
         BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(node_algorithms::unique(to_insert));
      this->priv_size_traits().increment();
      return iterator(node_algorithms::insert_equal
         (node_ptr(&priv_header()), hint.pointed_node(), to_insert, key_node_comp), this);
   }

   //! <b>Requires</b>: Dereferencing iterator must yield an lvalue 
   //!   of type value_type.
   //! 
   //! <b>Effects</b>: Inserts a each element of a range into the tree
   //!   before the upper bound of the key of each element.
   //! 
   //! <b>Complexity</b>: Insert range is in general O(N * log(N)), where N is the
   //!   size of the range. However, it is linear in N if the range is already sorted
   //!   by value_comp().
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   No copy-constructors are called.
   template<class Iterator>
   void insert_equal(Iterator b, Iterator e)
   {
      if(this->empty()){
         iterator end(this->end());
         for (; b != e; ++b)
            this->insert_equal(end, *b);
      }
      else{
         for (; b != e; ++b)
            this->insert_equal_upper_bound(*b);
      }
   }

   //! <b>Requires</b>: value must be an lvalue
   //! 
   //! <b>Effects</b>: Inserts value into the tree if the value
   //!   is not already present.
   //! 
   //! <b>Complexity</b>: Average complexity for insert element is at
   //!   most logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   No copy-constructors are called.
   std::pair<iterator, bool> insert_unique(reference value)
   {
      insert_commit_data commit_data;
      std::pair<iterator, bool> ret = insert_unique_check(value, commit_data);
      if(!ret.second)
         return ret;
      return std::pair<iterator, bool> (insert_unique_commit(value, commit_data), true);
   }

   //! <b>Requires</b>: value must be an lvalue, and "hint" must be
   //!   a valid iterator
   //! 
   //! <b>Effects</b>: Tries to insert x into the tree, using "hint" as a hint
   //!   to where it will be inserted.
   //! 
   //! <b>Complexity</b>: Logarithmic in general, but it is amortized
   //!   constant time (two comparisons in the worst case)
   //!   if t is inserted immediately before hint.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   No copy-constructors are called.
   iterator insert_unique(const_iterator hint, reference value)
   {
      insert_commit_data commit_data;
      std::pair<iterator, bool> ret = insert_unique_check(hint, value, commit_data);
      if(!ret.second)
         return ret.first;
      return insert_unique_commit(value, commit_data);
   }

   //! <b>Requires</b>: Dereferencing iterator must yield an lvalue 
   //!   of type value_type.
   //! 
   //! <b>Effects</b>: Tries to insert each element of a range into the tree.
   //! 
   //! <b>Complexity</b>: Insert range is in general O(N * log(N)), where N is the 
   //!   size of the range. However, it is linear in N if the range is already sorted 
   //!   by value_comp().
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   No copy-constructors are called.
   template<class Iterator>
   void insert_unique(Iterator b, Iterator e)
   {
      if(this->empty()){
         iterator end(this->end());
         for (; b != e; ++b)
            this->insert_unique(end, *b);
      }
      else{
         for (; b != e; ++b)
            this->insert_unique(*b);
      }
   }

   std::pair<iterator, bool> insert_unique_check
      (const_reference value, insert_commit_data &commit_data)
   {  return insert_unique_check(value, priv_comp(), commit_data); }

   template<class KeyType, class KeyValueCompare>
   std::pair<iterator, bool> insert_unique_check
      (const KeyType &key, KeyValueCompare key_value_comp, insert_commit_data &commit_data)
   {
      detail::key_nodeptr_comp<KeyValueCompare, rbtree_impl>
         comp(key_value_comp, this);
      std::pair<node_ptr, bool> ret = 
         (node_algorithms::insert_unique_check
            (node_ptr(&priv_header()), key, comp, commit_data));
      return std::pair<iterator, bool>(iterator(ret.first, this), ret.second);
   }

   std::pair<iterator, bool> insert_unique_check
      (const_iterator hint, const_reference value, insert_commit_data &commit_data)
   {  return insert_unique_check(hint, value, priv_comp(), commit_data); }

   template<class KeyType, class KeyValueCompare>
   std::pair<iterator, bool> insert_unique_check
      (const_iterator hint, const KeyType &key
      ,KeyValueCompare key_value_comp, insert_commit_data &commit_data)
   {
      detail::key_nodeptr_comp<KeyValueCompare, rbtree_impl>
         comp(key_value_comp, this);
      std::pair<node_ptr, bool> ret = 
         (node_algorithms::insert_unique_check
            (node_ptr(&priv_header()), hint.pointed_node(), key, comp, commit_data));
      return std::pair<iterator, bool>(iterator(ret.first, this), ret.second);
   }

   iterator insert_unique_commit(reference value, const insert_commit_data &commit_data)
   {
      node_ptr to_insert(get_real_value_traits().to_node_ptr(value));
      if(safemode_or_autounlink)
         BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(node_algorithms::unique(to_insert));
      this->priv_size_traits().increment();
      node_algorithms::insert_unique_commit
               (node_ptr(&priv_header()), to_insert, commit_data);
      return iterator(to_insert, this);
   }

   //! <b>Effects</b>: Erases the element pointed to by pos. 
   //! 
   //! <b>Complexity</b>: Average complexity for erase element is constant time. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. No destructors are called.
   iterator erase(iterator i)
   {
      iterator ret(i);
      ++ret;
      node_ptr to_erase(i.pointed_node());
      if(safemode_or_autounlink)
         BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(!node_algorithms::unique(to_erase));
      node_algorithms::erase(&priv_header(), to_erase);
      this->priv_size_traits().decrement();
      if(safemode_or_autounlink)
         node_algorithms::init(to_erase);
      return ret;
   }

   //! <b>Effects</b>: Erases the range pointed to by b end e. 
   //! 
   //! <b>Complexity</b>: Average complexity for erase range is at most 
   //!   O(log(size() + N)), where N is the number of elements in the range.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. No destructors are called.
   iterator erase(iterator b, iterator e)
   {  size_type n;   return private_erase(b, e, n);   }

   //! <b>Effects</b>: Erases all the elements with the given value.
   //! 
   //! <b>Returns</b>: The number of erased elements.
   //! 
   //! <b>Complexity</b>: O(log(size() + N).
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. No destructors are called.
   size_type erase(const_reference value)
   {  return this->erase(value, priv_comp());   }

   //! <b>Effects</b>: Erases all the elements with the given key.
   //!   according to the comparison functor "comp".
   //!
   //! <b>Returns</b>: The number of erased elements.
   //! 
   //! <b>Complexity</b>: O(log(size() + N).
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. No destructors are called.
   template<class KeyType, class KeyValueCompare>
   size_type erase(const KeyType& key, KeyValueCompare comp)
   {
      std::pair<iterator,iterator> p = this->equal_range(key, comp);
      size_type n;
      private_erase(p.first, p.second, n);
      return n;
   }

   //! <b>Requires</b>: Disposer::operator()(pointer) shouldn't throw.
   //!
   //! <b>Effects</b>: Erases the element pointed to by pos. 
   //!   Disposer::operator()(pointer) is called for the removed element.
   //! 
   //! <b>Complexity</b>: Average complexity for erase element is constant time. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Invalidates the iterators 
   //!    to the erased elements.
   template<class Disposer>
   iterator erase_and_dispose(iterator i, Disposer disposer)
   {
      node_ptr to_erase(i.pointed_node());
      iterator ret(this->erase(i));
      disposer(get_real_value_traits().to_value_ptr(to_erase));
      return ret;
   }

   //! <b>Requires</b>: Disposer::operator()(pointer) shouldn't throw.
   //!
   //! <b>Effects</b>: Erases the range pointed to by b end e.
   //!   Disposer::operator()(pointer) is called for the removed elements.
   //! 
   //! <b>Complexity</b>: Average complexity for erase range is at most 
   //!   O(log(size() + N)), where N is the number of elements in the range.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Invalidates the iterators
   //!    to the erased elements.
   template<class Disposer>
   iterator erase_and_dispose(iterator b, iterator e, Disposer disposer)
   {  size_type n;   return private_erase(b, e, n, disposer);   }

   //! <b>Requires</b>: Disposer::operator()(pointer) shouldn't throw.
   //!
   //! <b>Effects</b>: Erases all the elements with the given value.
   //!   Disposer::operator()(pointer) is called for the removed elements.
   //! 
   //! <b>Returns</b>: The number of erased elements.
   //! 
   //! <b>Complexity</b>: O(log(size() + N).
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. No destructors are called.
   template<class Disposer>
   size_type erase_and_dispose(const_reference value, Disposer disposer)
   {
      std::pair<iterator,iterator> p = this->equal_range(value);
      size_type n;
      private_erase(p.first, p.second, n, disposer);
      return n;
   }

   //! <b>Requires</b>: Disposer::operator()(pointer) shouldn't throw.
   //!
   //! <b>Effects</b>: Erases all the elements with the given key.
   //!   according to the comparison functor "comp".
   //!   Disposer::operator()(pointer) is called for the removed elements.
   //!
   //! <b>Returns</b>: The number of erased elements.
   //! 
   //! <b>Complexity</b>: O(log(size() + N).
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Invalidates the iterators
   //!    to the erased elements.
   template<class KeyType, class KeyValueCompare, class Disposer>
   size_type erase_and_dispose(const KeyType& key, KeyValueCompare comp, Disposer disposer)
   {
      std::pair<iterator,iterator> p = this->equal_range(key, comp);
      size_type n;
      private_erase(p.first, p.second, n, disposer);
      return n;
   }

   //! <b>Effects</b>: Erases all of the elements. 
   //! 
   //! <b>Complexity</b>: Linear to the number of elements on the container.
   //!   if it's a safe-mode or auto-unlink value_type. Constant time otherwise.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. No destructors are called.
   void clear()
   {
      if(safemode_or_autounlink){
         this->clear_and_dispose(detail::null_disposer());
      }
      else{
         node_algorithms::init_header(&priv_header());
         this->priv_size_traits().set_size(0);
      }
   }

   //! <b>Effects</b>: Erases all of the elements calling disposer(p) for
   //!   each node to be erased.
   //! <b>Complexity</b>: Average complexity for is at most O(log(size() + N)),
   //!   where N is the number of elements in the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. Calls N times to disposer functor.
   template<class Disposer>
   void clear_and_dispose(Disposer disposer)
   {
      node_algorithms::clear_and_dispose(node_ptr(&priv_header())
         , detail::node_disposer<Disposer, rbtree_impl>(disposer, this));
      node_algorithms::init_header(&priv_header());
      this->priv_size_traits().set_size(0);
   }

   //! <b>Effects</b>: Returns the number of contained elements with the given value
   //! 
   //! <b>Complexity</b>: Logarithmic to the number of elements contained plus lineal
   //!   to number of objects with the given value.
   //! 
   //! <b>Throws</b>: Nothing.
   size_type count(const_reference value) const
   {  return this->count(value, priv_comp());   }

   //! <b>Effects</b>: Returns the number of contained elements with the given key
   //! 
   //! <b>Complexity</b>: Logarithmic to the number of elements contained plus lineal
   //!   to number of objects with the given key.
   //! 
   //! <b>Throws</b>: Nothing.
   template<class KeyType, class KeyValueCompare>
   size_type count(const KeyType &key, KeyValueCompare comp) const
   {
      std::pair<const_iterator, const_iterator> ret = this->equal_range(key, comp);
      return std::distance(ret.first, ret.second);
   }

   //! <b>Effects</b>: Returns an iterator to the first element whose
   //!   key is not less than k or end() if that element does not exist.
   //! 
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   iterator lower_bound(const_reference value)
   {  return this->lower_bound(value, priv_comp());   }

   //! <b>Effects</b>: Returns an iterator to the first element whose
   //!   key is not less than k or end() if that element does not exist.
   //! 
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   const_iterator lower_bound(const_reference value) const
   {  return this->lower_bound(value, priv_comp());   }

   //! <b>Effects</b>: Returns an iterator to the first element whose
   //!   key is not less than k or end() if that element does not exist.
   //! 
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   template<class KeyType, class KeyValueCompare>
   iterator lower_bound(const KeyType &key, KeyValueCompare comp)
   {
      detail::key_nodeptr_comp<KeyValueCompare, rbtree_impl>
         key_node_comp(comp, this);
      return iterator(node_algorithms::lower_bound
         (const_node_ptr(&priv_header()), key, key_node_comp), this);
   }

   //! <b>Effects</b>: Returns a const iterator to the first element whose
   //!   key is not less than k or end() if that element does not exist.
   //! 
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   template<class KeyType, class KeyValueCompare>
   const_iterator lower_bound(const KeyType &key, KeyValueCompare comp) const
   {
      detail::key_nodeptr_comp<KeyValueCompare, rbtree_impl>
         key_node_comp(comp, this);
      return const_iterator(node_algorithms::lower_bound
         (const_node_ptr(&priv_header()), key, key_node_comp), this);
   }

   //! <b>Effects</b>: Returns an iterator to the first element whose
   //!   key is greater than k or end() if that element does not exist.
   //! 
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   iterator upper_bound(const_reference value)
   {  return this->upper_bound(value, priv_comp());   }

   //! <b>Effects</b>: Returns an iterator to the first element whose
   //!   key is greater than k according to comp or end() if that element
   //!   does not exist.
   //!
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   template<class KeyType, class KeyValueCompare>
   iterator upper_bound(const KeyType &key, KeyValueCompare comp)
   {
      detail::key_nodeptr_comp<KeyValueCompare, rbtree_impl>
         key_node_comp(comp, this);
      return iterator(node_algorithms::upper_bound
         (const_node_ptr(&priv_header()), key, key_node_comp), this);
   }

   //! <b>Effects</b>: Returns an iterator to the first element whose
   //!   key is greater than k or end() if that element does not exist.
   //! 
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   const_iterator upper_bound(const_reference value) const
   {  return this->upper_bound(value, priv_comp());   }

   //! <b>Effects</b>: Returns an iterator to the first element whose
   //!   key is greater than k according to comp or end() if that element
   //!   does not exist.
   //!
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   template<class KeyType, class KeyValueCompare>
   const_iterator upper_bound(const KeyType &key, KeyValueCompare comp) const
   {
      detail::key_nodeptr_comp<KeyValueCompare, rbtree_impl>
         key_node_comp(comp, this);
      return const_iterator(node_algorithms::upper_bound
         (const_node_ptr(&priv_header()), key, key_node_comp), this);
   }

   //! <b>Effects</b>: Finds an iterator to the first element whose key is 
   //!   k or end() if that element does not exist.
   //!
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   iterator find(const_reference value)
   {  return this->find(value, priv_comp()); }

   //! <b>Effects</b>: Finds an iterator to the first element whose key is 
   //!   k or end() if that element does not exist.
   //!
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   template<class KeyType, class KeyValueCompare>
   iterator find(const KeyType &key, KeyValueCompare comp)
   {
      detail::key_nodeptr_comp<KeyValueCompare, rbtree_impl>
         key_node_comp(comp, this);
      return iterator
         (node_algorithms::find(const_node_ptr(&priv_header()), key, key_node_comp), this);
   }

   //! <b>Effects</b>: Finds a const_iterator to the first element whose key is 
   //!   k or end() if that element does not exist.
   //! 
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   const_iterator find(const_reference value) const
   {  return this->find(value, priv_comp()); }

   //! <b>Effects</b>: Finds a const_iterator to the first element whose key is 
   //!   k or end() if that element does not exist.
   //! 
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   template<class KeyType, class KeyValueCompare>
   const_iterator find(const KeyType &key, KeyValueCompare comp) const
   {
      detail::key_nodeptr_comp<KeyValueCompare, rbtree_impl>
         key_node_comp(comp, this);
      return const_iterator
         (node_algorithms::find(const_node_ptr(&priv_header()), key, key_node_comp), this);
   }

   //! <b>Effects</b>: Finds a range containing all elements whose key is k or
   //!   an empty range that indicates the position where those elements would be
   //!   if they there is no elements with key k.
   //! 
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   std::pair<iterator,iterator> equal_range(const_reference value)
   {  return this->equal_range(value, priv_comp());   }

   //! <b>Effects</b>: Finds a range containing all elements whose key is k or
   //!   an empty range that indicates the position where those elements would be
   //!   if they there is no elements with key k.
   //! 
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   template<class KeyType, class KeyValueCompare>
   std::pair<iterator,iterator> equal_range(const KeyType &key, KeyValueCompare comp)
   {
      detail::key_nodeptr_comp<KeyValueCompare, rbtree_impl>
         key_node_comp(comp, this);
      std::pair<node_ptr, node_ptr> ret
         (node_algorithms::equal_range(const_node_ptr(&priv_header()), key, key_node_comp));
      return std::pair<iterator, iterator>(iterator(ret.first, this), iterator(ret.second, this));
   }

   //! <b>Effects</b>: Finds a range containing all elements whose key is k or
   //!   an empty range that indicates the position where those elements would be
   //!   if they there is no elements with key k.
   //! 
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   std::pair<const_iterator, const_iterator>
      equal_range(const_reference value) const
   {  return this->equal_range(value, priv_comp());   }

   //! <b>Effects</b>: Finds a range containing all elements whose key is k or
   //!   an empty range that indicates the position where those elements would be
   //!   if they there is no elements with key k.
   //! 
   //! <b>Complexity</b>: Logarithmic.
   //! 
   //! <b>Throws</b>: Nothing.
   template<class KeyType, class KeyValueCompare>
   std::pair<const_iterator, const_iterator>
      equal_range(const KeyType &key, KeyValueCompare comp) const
   {
      detail::key_nodeptr_comp<KeyValueCompare, rbtree_impl>
         key_node_comp(comp, this);
      std::pair<node_ptr, node_ptr> ret
         (node_algorithms::equal_range(const_node_ptr(&priv_header()), key, key_node_comp));
      return std::pair<const_iterator, const_iterator>(const_iterator(ret.first, this), const_iterator(ret.second, this));
   }

   template <class Cloner, class Disposer>
   void clone_from(const rbtree_impl &src, Cloner cloner, Disposer disposer)
   {
      this->clear_and_dispose(disposer);
      if(!src.empty()){
         node_algorithms::clone
            (const_node_ptr(&src.priv_header())
            ,node_ptr(&this->priv_header())
            ,detail::node_cloner<Cloner, rbtree_impl>(cloner, this)
            ,detail::node_disposer<Disposer, rbtree_impl>(disposer, this));
         this->priv_size_traits().set_size(src.priv_size_traits().get_size());
      }
   }

   pointer unlink_leftmost_without_rebalance()
   {
      node_ptr to_be_disposed(node_algorithms::unlink_leftmost_without_rebalance
                           (node_ptr(&priv_header())));
      if(!to_be_disposed)
         return 0;
      this->priv_size_traits().decrement();
      if(safemode_or_autounlink)//If this is commented does not work with normal_link
         node_algorithms::init(to_be_disposed);
      return get_real_value_traits().to_value_ptr(to_be_disposed);
   }

   //! <b>Requires</b>: replace_this must be a valid iterator of *this
   //!   and with_this must not be inserted in any tree.
   //! 
   //! <b>Effects</b>: Replaces replace_this in its position in the
   //!   tree with with_this. The tree does not need to be rebalanced.
   //! 
   //! <b>Complexity</b>: Constant. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: This function will break container ordering invariants if
   //!   with_this is not equivalent to *replace_this according to the
   //!   ordering rules. This function is faster than erasing and inserting
   //!   the node, since no rebalancing or comparison is needed.
   void replace_node(iterator replace_this, reference with_this)
   {
      node_algorithms::replace_node( get_real_value_traits().to_node_ptr(*replace_this)
                                   , node_ptr(&priv_header())
                                   , get_real_value_traits().to_node_ptr(with_this));
   }

   //! <b>Requires</b>: value must be an lvalue and shall be in a set of
   //!   appropriate type. Otherwise the behavior is undefined.
   //! 
   //! <b>Effects</b>: Returns: a valid iterator i belonging to the set
   //!   that points to the value
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Note</b>: This static function is available only if the <i>value traits</i>
   //!   is stateless.
   static iterator s_iterator_to(reference value)
   {
      BOOST_STATIC_ASSERT((!stateful_value_traits));
      return iterator (value_traits::to_node_ptr(value), 0);
   }

   //! <b>Requires</b>: value must be an lvalue and shall be in a set of
   //!   appropriate type. Otherwise the behavior is undefined.
   //! 
   //! <b>Effects</b>: Returns: a valid const_iterator i belonging to the
   //!   set that points to the value
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.�
   //! 
   //! <b>Note</b>: This static function is available only if the <i>value traits</i>
   //!   is stateless.
   static const_iterator s_iterator_to(const_reference value) 
   {
      BOOST_STATIC_ASSERT((!stateful_value_traits));
      return const_iterator (value_traits::to_node_ptr(const_cast<reference> (value)), 0);
   }

   //! <b>Requires</b>: value must be an lvalue and shall be in a set of
   //!   appropriate type. Otherwise the behavior is undefined.
   //! 
   //! <b>Effects</b>: Returns: a valid iterator i belonging to the set
   //!   that points to the value
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   iterator iterator_to(reference value)
   {  return iterator (value_traits::to_node_ptr(value), this); }

   //! <b>Requires</b>: value must be an lvalue and shall be in a set of
   //!   appropriate type. Otherwise the behavior is undefined.
   //! 
   //! <b>Effects</b>: Returns: a valid const_iterator i belonging to the
   //!   set that points to the value
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Throws</b>: Nothing.
   const_iterator iterator_to(const_reference value) const
   {  return const_iterator (value_traits::to_node_ptr(const_cast<reference> (value)), this); }

   //! <b>Requires</b>: value shall not be in a tree.
   //! 
   //! <b>Effects</b>: init_node puts the hook of a value in a well-known default
   //!   state.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant time.
   //! 
   //! <b>Note</b>: This function puts the hook in the well-known default state
   //!   used by auto_unlink and safe hooks.
   static void init_node(reference value)
   { node_algorithms::init(value_traits::to_node_ptr(value)); }

/*
   //! <b>Effects</b>: removes x from a tree of the appropriate type. It has no effect,
   //! if x is not in such a tree. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant time.
   //! 
   //! <b>Note</b>: This static function is only usable with the "safe mode"
   //! hook and non-constant time size lists. Otherwise, the user must use
   //! the non-static "erase(reference )" member. If the user calls
   //! this function with a non "safe mode" or constant time size list
   //! a compilation error will be issued.
   template<class T>
   static void remove_node(T& value)
   {
      //This function is only usable for safe mode hooks and non-constant
      //time lists. 
      //BOOST_STATIC_ASSERT((!(safemode_or_autounlink && constant_time_size)));
      BOOST_STATIC_ASSERT((!constant_time_size));
      BOOST_STATIC_ASSERT((boost::is_convertible<T, value_type>::value));
      node_ptr to_remove(value_traits::to_node_ptr(value));
      node_algorithms::unlink_and_rebalance(to_remove);
      if(safemode_or_autounlink)
         node_algorithms::init(to_remove);
   }
*/

   /// @cond
   private:
   template<class Disposer>
   iterator private_erase(iterator b, iterator e, size_type &n, Disposer disposer)
   {
      for(n = 0; b != e; ++n)
        this->erase_and_dispose(b++, disposer);
      return b;
   }

   iterator private_erase(iterator b, iterator e, size_type &n)
   {
      for(n = 0; b != e; ++n)
        this->erase(b++);
      return b;
   }
   /// @endcond

   private:
   static rbtree_impl &priv_container_from_end_iterator(const const_iterator &end_iterator)
   {
      header_plus_size *r = detail::parent_from_member<header_plus_size, node>
         ( detail::get_pointer(end_iterator.pointed_node()), &header_plus_size::header_);
      node_plus_pred_t *n = detail::parent_from_member
         <node_plus_pred_t, header_plus_size>(r, &node_plus_pred_t::header_plus_size_);
      data_t *d = detail::parent_from_member<data_t, node_plus_pred_t>(n, &data_t::node_plus_pred_);
      rbtree_impl *rb  = detail::parent_from_member<rbtree_impl, data_t>(d, &rbtree_impl::data_);
      return *rb;
   }
};

#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
template<class T, class ...Options>
#else
template<class Config>
#endif
inline bool operator<
#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
(const rbtree_impl<T, Options...> &x, const rbtree_impl<T, Options...> &y)
#else
(const rbtree_impl<Config> &x, const rbtree_impl<Config> &y)
#endif
{  return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());  }

#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
template<class T, class ...Options>
#else
template<class Config>
#endif
bool operator==
#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
(const rbtree_impl<T, Options...> &x, const rbtree_impl<T, Options...> &y)
#else
(const rbtree_impl<Config> &x, const rbtree_impl<Config> &y)
#endif
{
   typedef rbtree_impl<Config> tree_type;
   typedef typename tree_type::const_iterator const_iterator;
   const bool CS = tree_type::constant_time_size;
   if(CS && x.size() != y.size()){
      return false;
   }
   const_iterator end1 = x.end();
   const_iterator i1 = x.begin();
   const_iterator i2 = y.begin();
   if(CS){
      while (i1 != end1 && *i1 == *i2) {
         ++i1;
         ++i2;
      }
      return i1 == end1;
   }
   else{
      const_iterator end2 = y.end();
      while (i1 != end1 && i2 != end2 && *i1 == *i2) {
         ++i1;
         ++i2;
      }
      return i1 == end1 && i2 == end2;
   }
}

#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
template<class T, class ...Options>
#else
template<class Config>
#endif
inline bool operator!=
#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
(const rbtree_impl<T, Options...> &x, const rbtree_impl<T, Options...> &y)
#else
(const rbtree_impl<Config> &x, const rbtree_impl<Config> &y)
#endif
{  return !(x == y); }

#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
template<class T, class ...Options>
#else
template<class Config>
#endif
inline bool operator>
#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
(const rbtree_impl<T, Options...> &x, const rbtree_impl<T, Options...> &y)
#else
(const rbtree_impl<Config> &x, const rbtree_impl<Config> &y)
#endif
{  return y < x;  }

#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
template<class T, class ...Options>
#else
template<class Config>
#endif
inline bool operator<=
#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
(const rbtree_impl<T, Options...> &x, const rbtree_impl<T, Options...> &y)
#else
(const rbtree_impl<Config> &x, const rbtree_impl<Config> &y)
#endif
{  return !(y < x);  }

#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
template<class T, class ...Options>
#else
template<class Config>
#endif
inline bool operator>=
#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
(const rbtree_impl<T, Options...> &x, const rbtree_impl<T, Options...> &y)
#else
(const rbtree_impl<Config> &x, const rbtree_impl<Config> &y)
#endif
{  return !(x < y);  }

#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
template<class T, class ...Options>
#else
template<class Config>
#endif
inline void swap
#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
(rbtree_impl<T, Options...> &x, rbtree_impl<T, Options...> &y)
#else
(rbtree_impl<Config> &x, rbtree_impl<Config> &y)
#endif
{  x.swap(y);  }

/// @cond
template<class T, class O1 = none, class O2 = none
                , class O3 = none, class O4 = none
                , class O5 = none, class O6 = none
                , class O7 = none
                >
struct make_rbtree_opt
{
   typedef typename pack_options
      < set_defaults<T>, O1, O2, O3, O4>::type packed_options;
   typedef typename detail::get_value_traits
      <T, typename packed_options::value_traits>::type value_traits;

   typedef setopt
         < value_traits
         , typename packed_options::compare
         , typename packed_options::size_type
         , packed_options::constant_time_size
         > type;
};
/// @endcond

//! Helper metafunction to define a \c rbtree that yields to the same type when the
//! same options (either explicitly or implicitly) are used.
#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
template<class T, class ...Options>
#else
template<class T, class O1 = none, class O2 = none
                , class O3 = none, class O4 = none>
#endif
struct make_rbtree
{
   /// @cond
   typedef rbtree_impl
      < typename make_rbtree_opt<T, O1, O2, O3, O4>::type
      > implementation_defined;
   /// @endcond
   typedef implementation_defined type;
};

#ifndef BOOST_INTRUSIVE_DOXYGEN_INVOKED
template<class T, class O1, class O2, class O3, class O4>
class rbtree
   :  public make_rbtree<T, O1, O2, O3, O4>::type
{
   typedef typename make_rbtree
      <T, O1, O2, O3, O4>::type   Base;

   public:
   typedef typename Base::value_compare      value_compare;
   typedef typename Base::value_traits       value_traits;
   typedef typename Base::real_value_traits  real_value_traits;
   typedef typename Base::iterator           iterator;
   typedef typename Base::const_iterator     const_iterator;

   //Assert if passed value traits are compatible with the type
   BOOST_STATIC_ASSERT((detail::is_same<typename real_value_traits::value_type, T>::value));

   rbtree( const value_compare &cmp = value_compare()
         , const value_traits &v_traits = value_traits())
      :  Base(cmp, v_traits)
   {}

   template<class Iterator>
   rbtree( bool unique, Iterator b, Iterator e
         , const value_compare &cmp = value_compare()
         , const value_traits &v_traits = value_traits())
      :  Base(unique, b, e, cmp, v_traits)
   {}

   static rbtree &container_from_end_iterator(iterator end_iterator)
   {  return static_cast<rbtree &>(Base::container_from_end_iterator(end_iterator));   }

   static const rbtree &container_from_end_iterator(const_iterator end_iterator)
   {  return static_cast<const rbtree &>(Base::container_from_end_iterator(end_iterator));   }
};

#endif


} //namespace intrusive 
} //namespace boost 

#include <boost/intrusive/detail/config_end.hpp>

#endif //BOOST_INTRUSIVE_RBTREE_HPP
