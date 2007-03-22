// Copyright (c) 2005, Google Inc.
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
// Author: Craig Silverstein
//
// This tests <google/sparsehash/densehashtable.h>
// This tests <google/dense_hash_set>
// This tests <google/dense_hash_map>
// This tests <google/sparsehash/sparsehashtable.h>
// This tests <google/sparse_hash_set>
// This tests <google/sparse_hash_map>

// Since {dense,sparse}hashtable is templatized, it's important that
// we test every function in every class in this file -- not just to
// see if it works, but even if it compiles.

#include <google/sparsehash/config.h>
#include <stdio.h>
#include <sys/stat.h>          // for stat()
#ifdef HAVE_UNISTD_H
#include <unistd.h>            // for unlink()
#endif
#include <string.h>
#include <time.h>              // for silly random-number-seed generator
#include <math.h>              // for sqrt()
#include <map>
#include <iostream>
#include <iomanip>             // for setprecision()
#include <string>
#include <google/sparsehash/hash_fun.h>
#include <google/dense_hash_map>
#include <google/dense_hash_set>
#include <google/sparsehash/densehashtable.h>
#include <google/sparse_hash_map>
#include <google/sparse_hash_set>
#include <google/sparsehash/sparsehashtable.h>

using GOOGLE_NAMESPACE::sparse_hash_map;
using GOOGLE_NAMESPACE::dense_hash_map;
using GOOGLE_NAMESPACE::sparse_hash_set;
using GOOGLE_NAMESPACE::dense_hash_set;
using GOOGLE_NAMESPACE::sparse_hashtable;
using GOOGLE_NAMESPACE::dense_hashtable;
using STL_NAMESPACE::map;
using STL_NAMESPACE::pair;
using STL_NAMESPACE::string;
using STL_NAMESPACE::allocator;
using STL_NAMESPACE::equal_to;
using STL_NAMESPACE::ostream;

#define LOGF  STL_NAMESPACE::cout   // where we log to; LOGF is a historical name

#define CHECK(cond)  do {                       \
  if (!(cond)) {                                \
    LOGF << "Test failed: " #cond "\n";         \
    exit(1);                                    \
  }                                             \
} while (0)

const char *words[] = {"Baffin\n",        // in /usr/dict/words
                       "Boffin\n",        // not in
                       "baffin\n",        // not in
                       "genial\n",        // last word in
                       "Aarhus\n",        // first word alphabetically
                       "Zurich\n",        // last word alphabetically
};

const char *nwords[] = {"Boffin\n",
                        "baffin\n",
};

struct strcmp_fnc {
  bool operator()(const char* s1, const char* s2) const {
    return ((s1 == 0 && s2 == 0) ||
            (s1 && s2 && *s1 == *s2 && strcmp(s1, s2) == 0));
  }
};


template <class SparseTable, class T>
static void set_empty_key(SparseTable *sp, T val) {
}

template <class T, class H, class I, class C, class A>
static void set_empty_key(dense_hashtable<T,T,H,I,C,A> *ht, T val) {
  ht->set_empty_key(val);
}

template <class T, class H, class C>
static void set_empty_key(dense_hash_set<T,H,C> *ht, T val) {
  ht->set_empty_key(val);
}

template <class K, class V, class H, class C>
static void set_empty_key(dense_hash_map<K,V,H,C> *ht, K val) {
  ht->set_empty_key(val);
}


template <class T, class H, class I, class C, class A>
static void insert(dense_hashtable<T,T,H,I,C,A> *ht, T val) {
  ht->insert(val);
}

template <class T, class H, class C>
static void insert(dense_hash_set<T,H,C> *ht, T val) {
  ht->insert(val);
}

template <class K, class V, class H, class C>
static void insert(dense_hash_map<K,V,H,C> *ht, K val) {
  ht->insert(pair<K,V>(val,V()));
}

template <class T, class H, class I, class C, class A>
static void insert(sparse_hashtable<T,T,H,I,C,A> *ht, T val) {
  ht->insert(val);
}

template <class T, class H, class C>
static void insert(sparse_hash_set<T,H,C> *ht, T val) {
  ht->insert(val);
}

template <class K, class V, class H, class C>
static void insert(sparse_hash_map<K,V,H,C> *ht, K val) {
  ht->insert(pair<K,V>(val,V()));
}

template <class HT, class Iterator>
static void insert(HT *ht, Iterator begin, Iterator end) {
  ht->insert(begin, end);
}

// For hashtable's and hash_set's, the iterator insert works fine (and
// is used). But for the hash_map's, the iterator insert expects the
// iterators to point to pair's. So by looping over and calling insert
// on each element individually, the code below automatically expands
// into inserting a pair.
template <class K, class V, class H, class C, class Iterator>
static void insert(dense_hash_map<K,V,H,C> *ht, Iterator begin, Iterator end) {
  while (begin != end) {
    insert(ht, *begin);
    ++begin;
  }
}

template <class K, class V, class H, class C, class Iterator>
static void insert(sparse_hash_map<K,V,H,C> *ht, Iterator begin, Iterator end) {
  while (begin != end) {
    insert(ht, *begin);
    ++begin;
  }
}


static void write_item(FILE *fp, const char *val) {
  fwrite(val, strlen(val), 1, fp);   // \n serves to separate
}

// The weird 'const' declarations are desired by the compiler. Yucko.
static void write_item(FILE *fp, const pair<char*const,int> &val) {
  fwrite(val.first, strlen(val.first), 1, fp);
}

static char* read_line(FILE* fp, char* line, int linesize) {
  if ( fgets(line, linesize, fp) == NULL )
    return NULL;
  // normalize windows files :-(
  const int linelen = strlen(line);
  if ( linelen >= 2 && line[linelen-2] == '\r' && line[linelen-1] == '\n' ) {
    line[linelen-2] = '\n';
    line[linelen-1] = '\0';
  }
  return line;
}

static void read_item(FILE *fp, char*const* val) {
  char line[1024];
  read_line(fp, line, sizeof(line));
  char **p = const_cast<char**>(val);
  *p = strdup(line);
}

static void read_item(FILE *fp, pair<char*const,int> *val) {
  char line[1024];
  read_line(fp, line, sizeof(line));
  char **p = const_cast<char**>(&val->first);
  *p = strdup(line);
}

static void free_item(char*const* val) {
  free(*val);
}

static void free_item(pair<char*const,int> *val) {
  free(val->first);
}


// Apparently identity is not stl-standard, so we define our own
template<class Value>
struct Identity {
  Value& operator()(Value& v) const { return v; }
  const Value& operator()(const Value& v) const { return v; }
};

// Let us log the pairs that make up a hash_map
template<class P1, class P2>
ostream& operator<<(ostream& s, const pair<P1, P2>& p) {
  s << "pair(" << p.first << ", " << p.second << ")";
  return s;
}

// The read_write parameters specifies whether the read/write tests
// should be performed. Note that densehashtable::write_metdata is not
// implemented, so we only do the read/write tests for the
// sparsehashtable varieties.
template<class ht, class htint>
void test(bool read_write) {
  htint y(1000);
  htint z(64);
  set_empty_key(&y, 0xefefef);
  set_empty_key(&z, 0xefefef);

  CHECK(y.empty());
  insert(&y, 1);
  CHECK(!y.empty());
  insert(&y, 11);
  insert(&y, 111);
  insert(&y, 1111);
  insert(&y, 11111);
  insert(&y, 111111);
  insert(&y, 1111111);     // 1M, more or less
  insert(&y, 11111111);
  insert(&y, 111111111);
  insert(&y, 1111111111);  // 1B, more or less
  for ( int i = 0; i < 64; ++i )
    insert(&z, i);

  for ( typename htint::const_iterator it = y.begin(); it != y.end(); ++it )
    LOGF << "y: " << *it << "\n";
  z.insert(y.begin(), y.end());
  swap(y,z);
  for ( typename htint::iterator it = y.begin(); it != y.end(); ++it )
    LOGF << "y+z: " << *it << "\n";
  LOGF << "z has " << z.bucket_count() << " buckets\n";
  LOGF << "y has " << y.bucket_count() << " buckets\n";
  LOGF << "z size: " << z.size() << "\n";

  CHECK(z.size() == 10);
  z.set_deleted_key(1010101010);      // an unused value
  z.erase(11111);
  CHECK(z.size() == 9);
  insert(&z, 11111);                  // should retake deleted value
  CHECK(z.size() == 10);
  // Do the delete/insert again.  Last time we probably resized; this time no
  z.erase(11111);
  insert(&z, 11111);                  // should retake deleted value
  CHECK(z.size() == 10);

  z.erase(-11111);                    // shouldn't do anything
  CHECK(z.size() == 10);
  z.erase(1);
  CHECK(z.size() == 9);
  typename htint::iterator itdel = z.find(1111);
  z.erase(itdel);
  CHECK(z.size() == 8);
  itdel = z.find(2222);               // should be end()
  z.erase(itdel);                     // shouldn't do anything
  CHECK(z.size() == 8);
  for ( typename htint::const_iterator it = z.begin(); it != z.end(); ++it )
    LOGF << "y: " << *it << "\n";
  z.set_deleted_key(1010101011);      // a different unused value
  for ( typename htint::const_iterator it = z.begin(); it != z.end(); ++it )
    LOGF << "y: " << *it << "\n";
  LOGF << "That's " << z.size() << " elements\n";
  z.erase(z.begin(), z.end());
  CHECK(z.empty());

  y.clear();
  CHECK(y.empty());
  LOGF << "y has " << y.bucket_count() << " buckets\n";

  ht w;
  set_empty_key(&w, (char*) NULL);
  insert(&w, const_cast<char **>(nwords),
         const_cast<char **>(nwords) + sizeof(nwords) / sizeof(*nwords));
  LOGF << "w has " << w.size() << " items\n";
  CHECK(w.size() == 2);
  CHECK(w == w);

  ht x;
  set_empty_key(&x, (char*) NULL);
  long dict_size = 1;        // for size stats -- can't be 0 'cause of division

  map<string, int> counts;
  // Hash the dictionary
  {
    // automake says 'look for all data files in $srcdir.'  OK.
    string filestr = (string(getenv("srcdir") ? getenv("srcdir") : ".") +
                      "/src/words");
    const char* file = filestr.c_str();
    FILE *fp = fopen(file, "rb");
    if ( fp == NULL ) {
      LOGF << "Can't open " << file << " skipping dictionary hash...";
    } else {
      char line[1024];
      while ( read_line(fp, line, sizeof(line)) ) {
        insert(&x, strdup(line));
        counts[line] = 0;
      }
      LOGF << "Read " << x.size() << " words from " << file << "\n";
      fclose(fp);
      struct stat buf;
      stat(file, &buf);
      dict_size = buf.st_size;
      LOGF << "Size of " << file << ": " << buf.st_size << " bytes\n";
      for ( char **word = const_cast<char **>(words);
            word < const_cast<char **>(words) + sizeof(words) / sizeof(*words);
            ++word ) {
        if (x.find(*word) == x.end()) {
          CHECK(w.find(*word) != w.end());
        } else {
          CHECK(w.find(*word) == w.end());
        }
      }
    }
  }
  CHECK(counts.size() == x.size());

  // Save the hashtable.
  if (read_write) {
    const char* file = "/tmp/#hashtable_unittest_dicthash";
    FILE *fp = fopen(file, "wb");
    if ( fp == NULL ) {
      // maybe we can't write to /tmp/.  Try the current directory
      file = "#hashtable_unittest_dicthash";
      fp = fopen(file, "wb");
    }
    if ( fp == NULL ) {
      LOGF << "Can't open " << file << " skipping hashtable save...";
    } else {
      x.write_metadata(fp);        // this only writes meta-information
      int count = 0;
      for ( typename ht::iterator it = x.begin(); it != x.end(); ++it ) {
        write_item(fp, *it);
        free_item(&(*it));
        ++count;
      }
      LOGF << "Wrote " << count << " words to " << file << "\n";
      fclose(fp);
      struct stat buf;
      stat(file, &buf);
      LOGF << "Size of " << file << ": " << buf.st_size << " bytes\n";
      LOGF << STL_NAMESPACE::setprecision(3)
           << "Hashtable overhead "
           << (buf.st_size - dict_size) * 100.0 / dict_size
           << "% ("
           << (buf.st_size - dict_size) * 8.0 / count
           << " bits/entry)\n";
      x.clear();

      // Load the hashtable
      fp = fopen(file, "rb");
      if ( fp == NULL ) {
        LOGF << "Can't open " << file << " skipping hashtable reload...";
      } else {
        x.read_metadata(fp);      // reads metainformation
        LOGF << "Hashtable size: " << x.size() << "\n";
        int count = 0;
        for ( typename ht::iterator it = x.begin(); it != x.end(); ++it ) {
          read_item(fp, &(*it));
          ++count;
        }
        LOGF << "Read " << count << " words from " << file << "\n";
        fclose(fp);
        unlink(file);
        for ( char **word = const_cast<char **>(words);
              word < const_cast<char **>(words) + sizeof(words) / sizeof(*words);
              ++word ) {
          if (x.find(*word) == x.end()) {
            CHECK(w.find(*word) != w.end());
          } else {
            CHECK(w.find(*word) == w.end());
          }
        }
      }
    }
  }
  for ( typename ht::iterator it = x.begin(); it != x.end(); ++it ) {
    free_item(&(*it));
  }
}

int main(int argc, char **argv) {
  // First try with the low-level hashtable interface
  LOGF << "\n\nTEST WITH DENSE_HASHTABLE\n\n";
  test<dense_hashtable<char *, char *, HASH_NAMESPACE::hash<char *>,
                       Identity<char *>, strcmp_fnc, allocator<char *> >,
       dense_hashtable<int, int, HASH_NAMESPACE::hash<int>,
                       Identity<int>, equal_to<int>, allocator<int> > >(
                         false);

  // Now try with hash_set, which should be equivalent
  LOGF << "\n\nTEST WITH DENSE_HASH_SET\n\n";
  test<dense_hash_set<char *, HASH_NAMESPACE::hash<char *>, strcmp_fnc>,
       dense_hash_set<int> >(false);

  // Now try with hash_map, which differs only in insert()
  LOGF << "\n\nTEST WITH DENSE_HASH_MAP\n\n";
  test<dense_hash_map<char *, int, HASH_NAMESPACE::hash<char *>, strcmp_fnc>,
    dense_hash_map<int, int> >(false);

  // First try with the low-level hashtable interface
  LOGF << "\n\nTEST WITH SPARSE_HASHTABLE\n\n";
  test<sparse_hashtable<char *, char *, HASH_NAMESPACE::hash<char *>,
                       Identity<char *>, strcmp_fnc, allocator<char *> >,
       sparse_hashtable<int, int, HASH_NAMESPACE::hash<int>,
                       Identity<int>, equal_to<int>, allocator<int> > >(
                         true);

  // Now try with hash_set, which should be equivalent
  LOGF << "\n\nTEST WITH SPARSE_HASH_SET\n\n";
  test<sparse_hash_set<char *, HASH_NAMESPACE::hash<char *>, strcmp_fnc>,
       sparse_hash_set<int> >(true);

  // Now try with hash_map, which differs only in insert()
  LOGF << "\n\nTEST WITH SPARSE_HASH_MAP\n\n";
  test<sparse_hash_map<char *, int, HASH_NAMESPACE::hash<char *>, strcmp_fnc>,
       sparse_hash_map<int, int> >(true);

  LOGF << "\nAll tests pass.\n";
  return 0;
}
