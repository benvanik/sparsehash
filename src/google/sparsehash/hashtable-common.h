// Copyright (c) 2010, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ---
//
// Provides classes shared by both sparse and dense hashtable.
//
// sh_hashtable_settings has parameters for growing and shrinking
// a hashtable.  It also packages zero-size functor (ie. hasher).
//
// Other functions and classes provide common code for serializing
// and deserializing hashtables to a stream (such as a FILE*).

#ifndef UTIL_GTL_HASHTABLE_COMMON_H_
#define UTIL_GTL_HASHTABLE_COMMON_H_

#include <google/sparsehash/sparseconfig.h>
#include <assert.h>
#include <stdio.h>
#include <stddef.h>                  // for size_t
#include <stdexcept>                 // For length_error

_START_GOOGLE_NAMESPACE_

namespace sparsehash_internal {

// Settings contains parameters for growing and shrinking the table.
// It also packages zero-size functor (ie. hasher).
//
// It does some munging of the hash value in cases where we think
// (fear) the original hash function might not be very good.  In
// particular, the default hash of pointers is the identity hash,
// so probably all the low bits are 0.  We identify when we think
// we're hashing a pointer, and chop off the low bits.  Note this
// isn't perfect: even when the key is a pointer, we can't tell
// for sure that the hash is the identity hash.  If it's not, this
// is needless work (and possibly, though not likely, harmful).

template<typename Key, typename HashFunc,
         typename SizeType, int HT_MIN_BUCKETS>
class sh_hashtable_settings : public HashFunc {
 public:
  typedef Key key_type;
  typedef HashFunc hasher;
  typedef SizeType size_type;

 public:
  sh_hashtable_settings(const hasher& hf,
                        const float ht_occupancy_flt,
                        const float ht_empty_flt)
      : hasher(hf),
        enlarge_threshold_(0),
        shrink_threshold_(0),
        consider_shrink_(false),
        use_empty_(false),
        use_deleted_(false),
        num_ht_copies_(0) {
    set_enlarge_factor(ht_occupancy_flt);
    set_shrink_factor(ht_empty_flt);
  }

  size_type hash(const key_type& v) const {
    // We munge the hash value when we don't trust hasher::operator().
    return hash_munger<Key>::MungedHash(hasher::operator()(v));
  }

  float enlarge_factor() const {
    return enlarge_factor_;
  }
  void set_enlarge_factor(float f) {
    enlarge_factor_ = f;
  }
  float shrink_factor() const {
    return shrink_factor_;
  }
  void set_shrink_factor(float f) {
    shrink_factor_ = f;
  }

  size_type enlarge_threshold() const {
    return enlarge_threshold_;
  }
  void set_enlarge_threshold(size_type t) {
    enlarge_threshold_ = t;
  }
  size_type shrink_threshold() const {
    return shrink_threshold_;
  }
  void set_shrink_threshold(size_type t) {
    shrink_threshold_ = t;
  }

  size_type enlarge_size(size_type x) const {
    return static_cast<size_type>(x * enlarge_factor_);
  }
  size_type shrink_size(size_type x) const {
    return static_cast<size_type>(x * shrink_factor_);
  }

  bool consider_shrink() const {
    return consider_shrink_;
  }
  void set_consider_shrink(bool t) {
    consider_shrink_ = t;
  }

  bool use_empty() const {
    return use_empty_;
  }
  void set_use_empty(bool t) {
    use_empty_ = t;
  }

  bool use_deleted() const {
    return use_deleted_;
  }
  void set_use_deleted(bool t) {
    use_deleted_ = t;
  }

  size_type num_ht_copies() const {
    return static_cast<size_type>(num_ht_copies_);
  }
  void inc_num_ht_copies() {
    ++num_ht_copies_;
  }

  // Reset the enlarge and shrink thresholds
  void reset_thresholds(size_type num_buckets) {
    set_enlarge_threshold(enlarge_size(num_buckets));
    set_shrink_threshold(shrink_size(num_buckets));
    // whatever caused us to reset already considered
    set_consider_shrink(false);
  }

  // Caller is resposible for calling reset_threshold right after
  // set_resizing_parameters.
  void set_resizing_parameters(float shrink, float grow) {
    assert(shrink >= 0.0);
    assert(grow <= 1.0);
    if (shrink > grow/2.0f)
      shrink = grow / 2.0f;     // otherwise we thrash hashtable size
    set_shrink_factor(shrink);
    set_enlarge_factor(grow);
  }

  // This is the smallest size a hashtable can be without being too crowded
  // If you like, you can give a min #buckets as well as a min #elts
  size_type min_buckets(size_type num_elts, size_type min_buckets_wanted) {
    float enlarge = enlarge_factor();
    size_type sz = HT_MIN_BUCKETS;             // min buckets allowed
    while ( sz < min_buckets_wanted ||
            num_elts >= static_cast<size_type>(sz * enlarge) ) {
      // This just prevents overflowing size_type, since sz can exceed
      // max_size() here.
      if (static_cast<size_type>(sz * 2) < sz) {
        throw std::length_error("resize overflow");  // protect against overflow
      }
      sz *= 2;
    }
    return sz;
  }

 private:
  template<class HashKey> class hash_munger {
   public:
    static size_t MungedHash(size_t hash) {
      return hash;
    }
  };
  // This matches when the hashtable key is a pointer.
  template<class HashKey> class hash_munger<HashKey*> {
   public:
    static size_t MungedHash(size_t hash) {
      // TODO(csilvers): consider rotating instead:
      //    static const int shift = (sizeof(void *) == 4) ? 2 : 3;
      //    return (hash << (sizeof(hash) * 8) - shift)) | (hash >> shift);
      // This matters if we ever change sparse/dense_hash_* to compare
      // hashes before comparing actual values.  It's speedy on x86.
      return hash / sizeof(void*);   // get rid of known-0 bits
    }
  };

  size_type enlarge_threshold_;  // table.size() * enlarge_factor
  size_type shrink_threshold_;   // table.size() * shrink_factor
  float enlarge_factor_;         // how full before resize
  float shrink_factor_;          // how empty before resize
  // consider_shrink=true if we should try to shrink before next insert
  bool consider_shrink_;
  bool use_empty_;    // used only by densehashtable, not sparsehashtable
  bool use_deleted_;  // false until delkey has been set
  // num_ht_copies is a counter incremented every Copy/Move
  unsigned int num_ht_copies_;
};


// Adaptor methods for reading/writing data from an INPUT or OUPTUT
// variable passed to serialize() or unserialize().  The rule for
// these methods is their INPUT/OUTPUT variable must either be a
// FILE* or else something that supports a Read()/Write() method.
// The serializer can pass their INPUT/OUTPUT variable here and it
// will handle both cases.
inline bool read_data(FILE* fp, void* data, size_t length) {
  return fread(data, length, 1, fp) == 1;
}

inline bool write_data(FILE* fp, const void* data, size_t length) {
  return fwrite(data, length, 1, fp) == 1;
}

// The INPUT type needs to support a Read() method that takes a
// buffer and a length and returns the number of bytes read.
template <typename INPUT>
inline bool read_data(INPUT* fp, void* data, size_t length) {
  return static_cast<size_t>(fp->Read(data, length)) == length;
}

// The OUTPUT type needs to support a Write() operation that takes
// a buffer and a length and returns the number of bytes written.
template <typename OUTPUT>
inline bool write_data(OUTPUT* fp, const void* data, size_t length) {
  return static_cast<size_t>(fp->Write(data, length)) == length;
}

// Uses read_data() and write_data() to read/write an integer.
// length is the number of bytes to read/write (which may differ
// from sizeof(IntType), allowing us to save on a 32-bit system
// and load on a 64-bit system).  Excess bytes are taken to be 0.
// INPUT and OUTPUT must match legal inputs to read/write_data (above).
template <typename INPUT, typename IntType>
bool read_bigendian_number(INPUT* fp, IntType* value, size_t length) {
  *value = 0;
  unsigned char byte;
  for (size_t i = 0; i < length; ++i) {
    if (!read_data(fp, &byte, sizeof(byte))) return false;
    *value |= static_cast<IntType>(byte) << (i * 8);
  }
  return true;
}

template <typename OUTPUT, typename IntType>
bool write_bigendian_number(OUTPUT* fp, IntType value, size_t length) {
  unsigned char byte;
  for (size_t i = 0; i < length; ++i) {
    byte = i >= sizeof(value) ? 0 : value >> (i * 8);
    if (!write_data(fp, &byte, sizeof(byte))) return false;
  }
  return true;
}

// If your keys and values are simple enough, you can pass this
// serializer to serialize()/unserialize().  "Simple enough" means
// value_type is a POD type that contains no pointers.  Note,
// however, we don't try to normalize endianness.
// This is the type used for NopointerSerializer.
template <typename value_type> struct pod_serializer {
  template <typename INPUT>
  bool operator()(INPUT* fp, value_type* value) const {
    return read_data(fp, value, sizeof(*value));
  }

  template <typename OUTPUT>
  bool operator()(OUTPUT* fp, const value_type& value) const {
    return write_data(fp, &value, sizeof(value));
  }
};

}  // namespace sparsehash_internal

_END_GOOGLE_NAMESPACE_

#endif  // UTIL_GTL_HASHTABLE_COMMON_H_
