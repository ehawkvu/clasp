/*
    File: hashTable.h
*/

/*
Copyright (c) 2014, Christian E. Schafmeister

CLASP is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

See directory 'clasp/licenses' for full details.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/* -^- */
#ifndef _core_HashTable_H
#define _core_HashTable_H

#include <clasp/core/object.h>
#include <clasp/core/record.h>
#include <clasp/core/array.h>
#include <clasp/core/hashTableBase.h>
#include <clasp/core/mpPackage.fwd.h>
#include <clasp/core/corePackage.fwd.h>

//#define DEBUG_HASH_TABLE_DEBUG

namespace core {
double maybeFixRehashThreshold(double rt);
#define DEFAULT_REHASH_THRESHOLD 0.7

T_sp cl__make_hash_table(T_sp test, Fixnum_sp size, Number_sp rehash_size, Real_sp orehash_threshold, Symbol_sp weakness = nil<T_O>(), T_sp debug = nil<T_O>(), T_sp thread_safe = nil<T_O>(), T_sp hashf = nil<T_O>());

size_t next_hash_table_id();

};

namespace core{

struct KeyValuePair {
  KeyValuePair(T_sp k, T_sp v) : _Key(k), _Value(v) {};
  core::T_sp _Key;
  core::T_sp _Value;
};

  FORWARD(HashTable);
  class HashTable_O : public HashTableBase_O {
    struct metadata_bootstrap_class {};
    friend T_sp cl__make_hash_table(T_sp test, Fixnum_sp size, Number_sp rehash_size, Real_sp orehash_threshold, Symbol_sp weakness, T_sp debug, T_sp thread_safe, T_sp hashf);
    friend class HashTableReadLock;
    friend class HashTableWriteLock;
    LISP_CLASS(core, ClPkg, HashTable_O, "HashTable",HashTableBase_O);
    bool fieldsp() const override { return true; };
    void fields(Record_sp node) override;

    friend T_sp cl__maphash(T_sp function_desig, T_sp hash_table);
  HashTable_O() :
#ifdef DEBUG_REHASH_COUNT
    _HashTableId(next_hash_table_id()),
    _RehashCount(0),
    _InitialSize(0),
#endif
    _RehashSize(nil<Number_O>()),
    _RehashThreshold(maybeFixRehashThreshold(0.7)),
    _HashTableCount(0)
#ifdef DEBUG_HASH_TABLE_DEBUG
    ,_Debug(false)
    ,_History(nil<T_O>())
#endif
    {};
  //	DEFAULT_CTOR_DTOR(HashTable_O);
    friend class HashTableEq_O;
    friend class HashTableEql_O;
    friend class HashTableEqual_O;
    friend class HashTableEqualp_O;
    friend class HashTableCustom_O;
    friend T_sp cl__maphash(T_sp function_desig, HashTable_sp hash_table);
    friend T_sp cl__clrhash(HashTable_sp hash_table);
  public: // instance variables here
#ifdef DEBUG_REHASH_COUNT
    size_t    _HashTableId;
    size_t    _RehashCount;
    size_t    _InitialSize;
#endif
    Number_sp _RehashSize;
    double _RehashThreshold;
    gctools::Vec0<KeyValuePair> _Table;
    size_t _HashTableCount;
#ifdef DEBUG_HASH_TABLE_DEBUG
    bool   _Debug;
    std::atomic<T_sp> _History;
#endif
#ifdef CLASP_THREADS
    mutable mp::SharedMutex_sp _Mutex;
#endif
  public:
    static HashTable_sp create(T_sp test); // set everything up with defaults
    static HashTable_sp create_thread_safe(T_sp test, SimpleBaseString_sp readLockName, SimpleBaseString_sp writeLockName); // set everything up with defaults

  public:
    static void sxhash_eq(Hash1Generator &running_hash, T_sp obj );
    static void sxhash_eql(Hash1Generator &running_hash, T_sp obj );
    static void sxhash_eq(HashGenerator &running_hash, T_sp obj );
    static void sxhash_eql(HashGenerator &running_hash, T_sp obj );
    static void sxhash_equal(HashGenerator &running_hash, T_sp obj );
    static void sxhash_equalp(HashGenerator &running_hash, T_sp obj );
    void setupThreadSafeHashTable();
    void setupDebug();

  private:
    void setup(uint sz, Number_sp rehashSize, double rehashThreshold);
    uint resizeEmptyTable_no_lock(size_t sz);
    uint calculateHashTableCount() const;

  public:
    List_sp hash_table_bucket(size_t index);
  /*! If findKey is defined then search it as you rehash and return resulting keyValuePair CONS */
    KeyValuePair* rehash_no_lock(bool expandTable, T_sp findKey);
    KeyValuePair* rehash_upgrade_write_lock(bool expandTable, T_sp findKey);
    CL_LISPIFY_NAME("hash-table-buckets");
//    CL_DEFMETHOD ComplexVector_T_sp hash_table_buckets() const { return this->_HashTable; };
    CL_LISPIFY_NAME("hash-table-shared-mutex");
    CL_DEFMETHOD T_sp hash_table_shared_mutex() const { if (this->_Mutex) return this->_Mutex; else return nil<T_O>(); };
//    void set_thread_safe(bool thread_safe);
  public: // Functions here
    virtual bool is_eq_hashtable() const { return false;}
    virtual bool equalp(T_sp other) const override;

  /*! See CLHS */
    virtual T_sp hashTableTest() const { SUBIMP(); };

  /*! Return a count of the number of keys */
    size_t hashTableCount() const override;
    size_t hashTableSize() const override;
    size_t size() { return this->hashTableCount(); };

    T_sp operator[](const std::string& key);
    
    virtual gc::Fixnum sxhashKey(T_sp key, gc::Fixnum bound, HashGenerator& hg) const;
    virtual bool keyTest(T_sp entryKey, T_sp searchKey) const;

  /*! I'm not sure I need this and tableRef */
    List_sp bucketsFind_no_lock(T_sp key) const;
  /*! I'm not sure I need this and bucketsFind */
    virtual KeyValuePair* searchTable_no_read_lock(T_sp key, cl_index index);
    KeyValuePair* tableRef_no_read_lock(T_sp key, cl_index index );
//    List_sp findAssoc_no_lock(gc::Fixnum index, T_sp searchKey) const;

    T_sp hash_table_average_search_length();

  /*! Return true if the key is within the hash table */
    bool contains(T_sp key);

  /*! Return the key/value pair in a CONS if found or NIL if not */
    KeyValuePair* find(T_sp key);

    T_mv gethash(T_sp key, T_sp defaultValue = nil<T_O>()) override;
    gc::Fixnum hashIndex(T_sp key) const;

    T_sp hash_table_setf_gethash(T_sp key, T_sp value) override;
    T_sp setf_gethash_no_write_lock(T_sp key, T_sp value);
    void setf_gethash(T_sp key, T_sp val) { this->hash_table_setf_gethash(key, val); };

    Number_sp rehash_size() override;
    double rehash_threshold() override;
    T_sp hash_table_test() override;
    
    T_sp clrhash() override;

    bool remhash(T_sp key) override;

    string __repr__() const override;

    string hash_table_dump();
    void hash_table_pointers_dump();
    void hash_table_early_dump();

    void lowLevelMapHash(KeyValueMapper *mapper) const;

    void maphash(T_sp fn) override; 

    void mapHash(std::function<void(T_sp, T_sp)> const &fn);
    void maphash(std::function<void(T_sp, T_sp)> const &fn) { this->mapHash(fn); };

  /*! maps function across a hash table until the function returns false */
    bool /*terminatingMapHash*/ map_while_true(std::function<bool(T_sp, T_sp)> const &fn) const;

  /*! Return the number of entries in the HashTable Vector0 */
    int hashTableNumberOfHashes() const;
  /*! Return the start of the alist in the HashTable Vector0 at hash value */
//    List_sp hashTableAlistAtHash(int hash) const;

    string keysAsString();

  /*! Look like a set */
    void insert(T_sp obj) { this->setf_gethash(obj, nil<T_O>()); };
  /*! Return a Cons of all keys */
    List_sp keysAsCons();
  };

//HashTable_mv af_make_hash_table(T_sp test, Fixnum_sp size, Number_sp rehash_size, DoubleFloat_sp orehash_threshold);

T_mv clasp_gethash_safe(T_sp key, T_sp hashTable, T_sp default_);



}; /* core */


#endif /* _core_HashTable_H */
