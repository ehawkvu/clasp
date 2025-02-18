/* NOTES:

(1) _deleted is not being used or updated properly.
(2) There is something wrong with WeakKeyHashTable - weak pointers end up pointing to memory that is not the start of an object
(3) The other weak objects (weak pointer, weak mapping) are doing allocations in their constructors.

*/


/*
    File: gcweak.cc
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
#include <clasp/core/foundation.h>
#include <clasp/gctools/gcweak.h>
#include <clasp/core/object.h>
#include <clasp/core/evaluator.h>
#include <clasp/core/mpPackage.h>
#ifdef USE_MPS
#include <clasp/mps/code/mps.h>
#endif



namespace gctools {

#ifdef CLASP_THREADS
struct WeakKeyHashTableReadLock {
  const WeakKeyHashTable* _hashTable;
  WeakKeyHashTableReadLock(const WeakKeyHashTable* ht) : _hashTable(ht) {
    if (this->_hashTable->_Mutex) {
      this->_hashTable->_Mutex->shared_lock();
    }
  }
  ~WeakKeyHashTableReadLock() {
    if (this->_hashTable->_Mutex) {
      this->_hashTable->_Mutex->shared_unlock();
    }
  }
};
struct WeakKeyHashTableWriteLock {
  const WeakKeyHashTable* _hashTable;
  WeakKeyHashTableWriteLock(const WeakKeyHashTable* ht,bool upgrade = false) : _hashTable(ht) {
    if (this->_hashTable->_Mutex) {
      this->_hashTable->_Mutex->write_lock(upgrade);
    }
  }
  ~WeakKeyHashTableWriteLock() {
    if (this->_hashTable->_Mutex) {
      this->_hashTable->_Mutex->write_unlock();
    }
  }
};
#endif

#ifdef CLASP_THREADS
#define HT_READ_LOCK(me) WeakKeyHashTableReadLock _zzz(me)
#define HT_WRITE_LOCK(me) WeakKeyHashTableWriteLock _zzz(me)
#define HT_UPGRADE_WRITE_LOCK(me) WeakKeyHashTableWriteLock _zzz(me,true)
#else
#define HT_READ_LOCK(me) 
#define HT_WRITE_LOCK(me) 
#define HT_UPGRADE_WRITE_LOCK(me)
#endif

void WeakKeyHashTable::initialize() {
  int length = this->_Length;
  /* round up to next power of 2 */
  if (length == 0)
    length = 2;
  size_t l;
  for (l = 1; l < length; l *= 2)
    ;
  this->_Keys = KeyBucketsAllocatorType::allocate(Header_s::BadgeStampWtagMtag(Header_s::WeakBucketKind),l);
  this->_Values = ValueBucketsAllocatorType::allocate(Header_s::BadgeStampWtagMtag(Header_s::StrongBucketKind),l);
  this->_Keys->dependent = this->_Values;
  //  GCTOOLS_ASSERT((reinterpret_cast<uintptr_t>(this->_Keys->dependent) & 0x3) == 0);
  this->_Values->dependent = this->_Keys;
}

void WeakKeyHashTable::setupThreadSafeHashTable() {
#ifdef CLASP_THREADS
  core::SimpleBaseString_sp sbsread  = core::SimpleBaseString_O::make("WEAKHSHR");
  core::SimpleBaseString_sp sbswrite = core::SimpleBaseString_O::make("WEAKHSHW");
  this->_Mutex = mp::SharedMutex_O::make_shared_mutex(sbsread,sbswrite);
#endif
}

uint WeakKeyHashTable::sxhashKey(const value_type &key) {
  GCWEAK_LOG(fmt::format("Calling lisp_hash for key: {}" , (void *)lisp_badge(key)));
  return core::lisp_hash(static_cast<uintptr_t>(gctools::lisp_badge(key)));
}

/*! Return 0 if there is no more room in the sequence of entries for the key
	  Return 1 if the element is found or an unbound or deleted entry is found.
	  Return the entry index in (b)
	*/
size_t WeakKeyHashTable::find_no_lock(gctools::tagged_pointer<KeyBucketsType> keys, const value_type &key
                        ,
                        size_t &b
#ifdef DEBUG_FIND
                        ,
                        bool debugFind, stringstream *reportP
#endif
                        ) {
  unsigned long i, h, probe;
  unsigned long l = keys->length() - 1;
  int result = 0;
  h = WeakKeyHashTable::sxhashKey(key);

#ifdef DEBUG_FIND
  if (debugFind) {
    *reportP << __FILE__ << ":" << __LINE__ << " starting find key = " << (void *)(key.raw_()) << " h = " << h << " l = " << l << std::endl;
  }
#endif
  probe = (h >> 8) | 1;
  h &= l;
  i = h;
  do {
    value_type &k = (*keys)[i];
#ifdef DEBUG_FIND
    if (debugFind) {
      *reportP << "  i = " << i << "   k = " << (void *)(k.raw_()) << std::endl;
    }
#endif
    if (k.unboundp() || k == key) {
      b = i;
#ifdef DEBUG_FIND
      if (debugFind && k != key) {
        *reportP << "find  returning 1 b= " << b << " k = " << k.raw_() << std::endl;
      }
#endif
      return 1;
    }
#if defined(USE_BOEHM)
    // Handle splatting
    if (!k.raw_()) {
      auto deleted = value_type(gctools::make_tagged_deleted<core::T_O*>());
      keys->set(i, deleted);
      ValueBucketsType *values = dynamic_cast<ValueBucketsType *>(&*keys->dependent);
      (*values)[i] = value_type(gctools::make_tagged_unbound<core::T_O*>());
    }
#else
    MISSING_GC_SUPPORT();
#endif
    if (result == 0 && (k.deletedp())) {
      b = i;
      result = 1;
    }
    i = (i + probe) & l;
  } while (i != h);
  return result;
}
int WeakKeyHashTable::rehash_not_safe(const value_type &key, size_t &key_bucket) {
  HT_WRITE_LOCK(this);
  size_t newLength;
  if (this->_RehashSize.fixnump()) {
    newLength = this->_Keys->length() + this->_RehashSize.unsafe_fixnum();
  } else if (gc::IsA<core::Float_sp>(this->_RehashSize)) {
    double size = core::clasp_to_double(this->_RehashSize);
    newLength = this->_Keys->length()*size;
  } else {
    SIMPLE_ERROR("Illegal rehash size {}", _rep_(this->_RehashSize));
  }
  int result;
  GCWEAK_LOG(fmt::format("entered rehash newLength = {}" , newLength ));
  size_t i, length;
		// buckets_t new_keys, new_values;
  result = 0;
  length = this->_Keys->length();
  MyType newHashTable(newLength,this->_RehashSize,this->_RehashThreshold);
  newHashTable.initialize();
		//new_keys = make_buckets(newLength, this->key_ap);
		//new_values = make_buckets(newLength, this->value_ap);
		//new_keys->dependent = new_values;
		//new_values->dependent = new_keys;
  for (i = 0; i < length; ++i) {
    value_type& old_key = (*this->_Keys)[i];
    if (!old_key.unboundp() && !old_key.deletedp() && old_key.raw_() ) {
      size_t found;
      size_t b;
      found = WeakKeyHashTable::find_no_lock(newHashTable._Keys, old_key, b);
      GCTOOLS_ASSERT(found);// assert(found);            /* new table shouldn't be full */
      if ( !(*newHashTable._Keys)[b].unboundp() ) {
        printf("%s:%d About to copy key: %12p   at index %zu    to newHashTable at index: %zu\n", __FILE__, __LINE__,
               old_key.raw_(), i, b );
        printf("Key = %s\n", core::lisp_rep(old_key).c_str());
        printf("    original value@%p = %s\n", (*this->_Values)[i].raw_(), core::lisp_rep((*this->_Values)[i]).c_str());
        printf("newHashTable value@%p = %s\n", (*newHashTable._Values)[b].raw_(), core::lisp_rep((*newHashTable._Values)[b]).c_str());
        printf("--------- Original table\n");
        printf("%s\n", this->dump("Original").c_str());
        printf("--------- New table\n");
        printf("%s\n", newHashTable.dump("Copy").c_str());
      }
      GCTOOLS_ASSERT((*newHashTable._Keys)[b].unboundp()); /* shouldn't be in new table */
      newHashTable._Keys->set(b,old_key);
      (*newHashTable._Values)[b] = (*this->_Values)[i];
      if (key && old_key == key ) {
        key_bucket = b;
        result = 1;
      }
      (*newHashTable._Keys).setUsed((*newHashTable._Keys).used()+1); // TAG_COUNT(UNTAG_COUNT(new_keys->used) + 1);
    }
  }
  GCTOOLS_ASSERT( (*newHashTable._Keys).used() == (newHashTable.tableSize()) );
		// assert(UNTAG_COUNT(new_keys->used) == table_size(tbl));
  this->swap(newHashTable);
  return result;
}

int WeakKeyHashTable::rehash( const value_type &key, size_t &key_bucket) {
  int result;
  safeRun<void()>([&result, this, &key, &key_bucket]() -> void {
      result = this->rehash_not_safe(key,key_bucket);
    });
  return result;
}

/*! trySet returns 0 only if there is no room in the hash-table */
int WeakKeyHashTable::trySet(core::T_sp tkey, core::T_sp value) {
  HT_WRITE_LOCK(this);
  GCWEAK_LOG(fmt::format("Entered trySet with key {}" , tkey.raw_()));
  size_t b;
  if (tkey == value) {
    value = gctools::make_tagged_same_as_key<core::T_O>();
  }
  value_type key(tkey);
#ifdef DEBUG_TRYSET
  stringstream report;
  report << "About to trySet with the key " << tkey.raw_() << std::endl;
  report << this->dump("trySet-incoming WeakKeyHashTable: ") << std::endl;
  // Count the number of times the key is in the table
  int alreadyThere = 0;
  int firstOtherIndex = 0;
  for (int i(0); i < (*this->_Keys).length(); ++i) {
    if ((*this->_Keys)[i] == tkey) {
      firstOtherIndex = i;
      ++alreadyThere;
    }
  }
#endif
  size_t result = WeakKeyHashTable::find_no_lock(this->_Keys, key, b);
  if ((!result || (*this->_Keys)[b] != key)) {
    GCWEAK_LOG(fmt::format("then case - Returned from find with result = {}     (*this->_Keys)[b={}] = {}" , result , b , (*this->_Keys)[b].raw_()));
#ifdef DEBUG_TRYSET
    if (alreadyThere) {
      report << "Could not find the key @" << key.raw_() << std::endl;
      if (!result) {
        report << " because find return result = " << result << " - which means that the hash table was searched and the key was not found before the hash list was exhausted" << std::endl;
      }
      if ((*this->_Keys)[b] != key) {
        report << " because find hit an empty (unbound or deleted) slot before it found the key" << std::endl;
      }
      report << " ...  about to check if mps_ld_isstale" << std::endl;
    }
#endif

  } else {
    GCWEAK_LOG(fmt::format("else case - Returned from find with result = {}     (*this->_Keys)[b={}] = {}" , result , b , (*this->_Keys)[b].raw_()));
    GCWEAK_LOG(fmt::format("Calling mps_ld_add for key: {}" , (void *)key.raw_()));
  }
  if ((*this->_Keys)[b].unboundp()) {
    GCWEAK_LOG(fmt::format("Writing key over unbound entry"));
    this->_Keys->set(b, key);
    (*this->_Keys).setUsed((*this->_Keys).used() + 1);
#ifdef DEBUG_GCWEAK
    printf("%s:%d key was unboundp at %zu  used = %d\n", __FILE__, __LINE__, b, this->_Keys->used());
#endif
  } else if ((*this->_Keys)[b].deletedp()) {
    GCWEAK_LOG(fmt::format("Writing key over deleted entry"));
    this->_Keys->set(b, key);
    GCTOOLS_ASSERT((*this->_Keys).deleted() > 0);
    (*this->_Keys).setDeleted((*this->_Keys).deleted() - 1);
#ifdef DEBUG_GCWEAK
    printf("%s:%d key was deletedp at %zu  deleted = %d\n", __FILE__, __LINE__, b, (*this->_Keys).deleted());
#endif // DEBUG_GCWEAK
  }
  GCWEAK_LOG(fmt::format("Setting value at b = {}" , b));
  (*this->_Values).set(b, value_type(value));
#ifdef DEBUG_TRYSET
  // Count the number of times the key is in the table
  int count = 0;
  int otherIndex;
  for (int i(0); i < (*this->_Keys).length(); ++i) {
    if ((*this->_Keys)[i] == tkey) {
      if (i != b) {
        otherIndex = i;
      }
      ++count;
    }
  }
  if (count > 1) {
    printf("Found %d duplicate keys %p in hash table\n", count, tkey.raw_());
    printf("Log of trySet action:\n%s\n", report.str().c_str());
    printf("The new key is at %zu and another instance of the key is at %d\n", b, otherIndex);
    printf("%s\n", this->dump("Final trySet table").c_str());
  }
#endif
  GCWEAK_LOG(fmt::format("Leaving trySet"));
  return 1;
}

string WeakKeyHashTable::dump(const string &prefix) {
  stringstream sout;
  safeRun<void()>([this, &prefix, &sout]() -> void {
      HT_READ_LOCK(this);
		size_t i, length;
		length = this->_Keys->length();
		sout << "===== Dumping WeakKeyHashTable length = " << length << std::endl;
		for (i = 0; i < length; ++i) {
		    value_type& old_key = (*this->_Keys)[i];
		    sout << prefix << "  [" << i << "]  key= " << old_key.raw_() << "  value = " << (*this->_Values)[i].raw_() << std::endl;
		}
  });
  return sout.str();
};

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
//
// Use safeRun from here on down
//
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

core::T_mv WeakKeyHashTable::gethash(core::T_sp tkey, core::T_sp defaultValue) {
  core::T_mv result_mv;
  safeRun<void()>([&result_mv, this, tkey, defaultValue]() -> void {
      HT_READ_LOCK(this);
      value_type key(tkey);
      size_t pos;
      size_t result = gctools::WeakKeyHashTable::find_no_lock(this->_Keys,key ,pos);
      if (result) { // WeakKeyHashTable::find(this->_Keys,key,false,pos)) { //buckets_find(tbl, this->keys, key, NULL, &b)) {
        value_type& k = (*this->_Keys)[pos];
        GCWEAK_LOG(fmt::format("gethash find successful pos = {}  k= {} k.unboundp()={} k.base_ref().deletedp()={} k.NULLp()={}" , pos , k.raw_() , k.unboundp() , k.deletedp() , (bool)k ));
        if ( k.raw_() && !k.unboundp() && !k.deletedp() ) {
          GCWEAK_LOG(fmt::format("Returning success!"));
          core::T_sp value = smart_ptr<core::T_O>((*this->_Values)[pos]);
          if ( value.same_as_keyP() ) {
            value = smart_ptr<core::T_O>(k);
          }
          result_mv = Values(value,core::lisp_true());
          return;
        }
        GCWEAK_LOG(fmt::format("Falling through"));
      }
      result_mv = Values(defaultValue,nil<core::T_O>());
      return;
    });
  return result_mv;
}

void WeakKeyHashTable::set(core::T_sp key, core::T_sp value) {
  safeRun<void()>([key, value, this]() -> void {
		if (this->fullp_not_safe() || !this->trySet(key,value) ) {
		    int res;
		    value_type dummyKey;
		    size_t dummyPos;
		    this->rehash( dummyKey, dummyPos );
		    res = this->trySet( key, value);
		    GCTOOLS_ASSERT(res);
		}
  });
}

#define HASH_TABLE_ITER(table_type,tablep, key, value) \
  gctools::tagged_pointer<table_type::KeyBucketsType> iter_Keys; \
  gctools::tagged_pointer<table_type::ValueBucketsType> iter_Values; \
  core::T_sp key; \
  core::T_sp value; \
  {\
    HT_READ_LOCK(tablep);\
    iter_Keys = tablep->_Keys; \
    iter_Values = tablep->_Values; \
  }\
  for (size_t it(0), itEnd(iter_Keys->length()); it < itEnd; ++it) {\
  { \
    HT_READ_LOCK(tablep);\
    key = (*iter_Keys)[it]; \
    value = (*iter_Values)[it]; \
  } \
  if (key.raw_()&&!key.unboundp()&&!key.deletedp())

#define HASH_TABLE_ITER_END }

void WeakKeyHashTable::maphash(std::function<void(core::T_sp, core::T_sp)> const &fn) {
  safeRun<void()>(
                  [fn, this]() -> void {
                    HASH_TABLE_ITER(WeakKeyHashTable,this,key,value) {
                      fn(key,value);
                    } HASH_TABLE_ITER_END;
                  }
                  );
}

void WeakKeyHashTable::maphashFn(core::T_sp fn) {
  safeRun<void()>(
                  [fn, this]() -> void {
                    HASH_TABLE_ITER(WeakKeyHashTable,this,key,value) {
                      core::eval::funcall(fn,key,value);
                    } HASH_TABLE_ITER_END;
                  }
                  );
}

bool WeakKeyHashTable::remhash(core::T_sp tkey) {
  bool bresult = false;
  safeRun<void()>([this, tkey, &bresult]() -> void {
      HT_WRITE_LOCK(this);
		size_t b;
		value_type key(tkey);
		size_t result = gctools::WeakKeyHashTable::find_no_lock(this->_Keys, key, b);
		if( ! result ||
                    !((*this->_Keys)[b]).raw_() ||
		    (*this->_Keys)[b].unboundp() ||
		    (*this->_Keys)[b].deletedp() )
		    {
                      if(!this->rehash( key, b)) {
                        bresult = false;
                        return;
                      }
		    }
		if( (*this->_Keys)[b].raw_() &&
                    !(*this->_Keys)[b].unboundp() &&
		    !(*this->_Keys)[b].deletedp() )
		    {
                      auto deleted = value_type(gctools::make_tagged_deleted<core::T_O*>());
                      this->_Keys->set(b, deleted);
                      (*this->_Keys).setDeleted((*this->_Keys).deleted()+1);
                      (*this->_Values)[b] = value_type(gctools::make_tagged_unbound<core::T_O*>());
                      bresult = true;
                      return;
		    }
                bresult = false;
  });
  return bresult;
}

void WeakKeyHashTable::clrhash() {
  safeRun<void()>([this]() -> void {
      HT_WRITE_LOCK(this);
		size_t len = (*this->_Keys).length();
		for ( size_t i(0); i<len; ++i ) {
                  this->_Keys->set(i,value_type(gctools::make_tagged_deleted<core::T_O*>()));
                  (*this->_Values)[i] = value_type(gctools::make_tagged_unbound<core::T_O*>());
		}
		(*this->_Keys).setUsed(0);
		(*this->_Keys).setDeleted(0);
  });
};


DOCGROUP(clasp);
CL_DEFUN core::Vector_sp weak_key_hash_table_pairs(const gctools::WeakKeyHashTable& ht) {
  size_t len = (*ht._Keys).length();
  core::ComplexVector_T_sp keyvalues = core::ComplexVector_T_O::make(len*2,nil<core::T_O>(),core::make_fixnum(0));
  size_t idx(0);
  HT_READ_LOCK(&ht);
  for ( size_t i(0); i<len; ++i ) {
    if ( (*ht._Keys)[i].raw_() &&
         !(*ht._Keys)[i].unboundp() &&
         !(*ht._Keys)[i].deletedp() ) {
      keyvalues->vectorPushExtend((*ht._Keys)[i],16);
      keyvalues->vectorPushExtend((*ht._Values)[i],16);
    }
  }
  return keyvalues;
};
}




#if 0
namespace gctools {

void StrongKeyHashTable::initialize() {
  int length = this->_Length;
  /* round up to next power of 2 */
  if (length == 0)
    length = 2;
  size_t l;
  for (l = 1; l < length; l *= 2)
    ;
  this->_Keys = KeyBucketsAllocatorType::allocate(l);
  this->_Values = ValueBucketsAllocatorType::allocate(l);
  this->_Keys->dependent = this->_Values;
  //  GCTOOLS_ASSERT((reinterpret_cast<uintptr_t>(this->_Keys->dependent) & 0x3) == 0);
  this->_Values->dependent = this->_Keys;
}

uint StrongKeyHashTable::sxhashKey(const value_type &key) {
  uintptr_t hash = lisp_badge(key);
  GCWEAK_LOG(fmt::format("Calling lisp_hash for key: {}" , (void *)hash));
  return core::lisp_hash(reinterpret_cast<uintptr_t>(hash));
}

/*! Return 0 if there is no more room in the sequence of entries for the key
	  Return 1 if the element is found or an unbound or deleted entry is found.
	  Return the entry index in (b)
	*/
size_t StrongKeyHashTable::find(gctools::tagged_pointer<KeyBucketsType> keys, const value_type &key , size_t &b
#ifdef DEBUG_FIND
                        , bool debugFind, stringstream *reportP
#endif
                        ) {
  unsigned long i, h, probe;
  unsigned long l = keys->length() - 1;
  int result = 0;
  h = StrongKeyHashTable::sxhashKey(key);

#ifdef DEBUG_FIND
  if (debugFind) {
    *reportP << __FILE__ << ":" << __LINE__ << " starting find key = " << (void *)(key.raw_()) << " h = " << h << " l = " << l << std::endl;
  }
#endif
  probe = (h >> 8) | 1;
  h &= l;
  i = h;
  do {
    value_type &k = (*keys)[i];
#ifdef DEBUG_FIND
    if (debugFind) {
      *reportP << "  i = " << i << "   k = " << (void *)(k.raw_()) << std::endl;
    }
#endif
    if (k.unboundp() || k == key) {
      b = i;
#ifdef DEBUG_FIND
      if (debugFind && k != key) {
        *reportP << "find  returning 1 b= " << b << " k = " << k.raw_() << std::endl;
      }
#endif
      return 1;
    }
#if defined(USE_BOEHM)
    // Handle splatting
    if (!k.raw_()) {
      auto deleted = value_type(gctools::make_tagged_deleted<core::T_O*>());
      keys->set(i, deleted);
      ValueBucketsType *values = dynamic_cast<ValueBucketsType *>(&*keys->dependent);
      (*values)[i] = value_type(gctools::make_tagged_unbound<core::T_O*>());
    }
#else
    MISSING_GC_SUPPORT();
#endif
    if (result == 0 && (k.deletedp())) {
      b = i;
      result = 1;
    }
    i = (i + probe) & l;
  } while (i != h);
  return result;
}
size_t StrongKeyHashTable::rehash_not_safe(size_t newLength, const value_type &key, size_t &key_bucket) {
  this->_Rehashes++;
  size_t result;
  GCWEAK_LOG(fmt::format("entered rehash newLength = {}" , newLength ));
  size_t i, length;
		// buckets_t new_keys, new_values;
  result = 0;
  length = this->_Keys->length();
  MyType newHashTable(newLength);
  newHashTable.initialize();
		//new_keys = make_buckets(newLength, this->key_ap);
		//new_values = make_buckets(newLength, this->value_ap);
		//new_keys->dependent = new_values;
		//new_values->dependent = new_keys;
  for (i = 0; i < length; ++i) {
    value_type& old_key = (*this->_Keys)[i];
    if (!old_key.unboundp() && !old_key.deletedp() && old_key.raw_() ) {
      size_t found;
      size_t b;
      found = StrongKeyHashTable::find(newHashTable._Keys, old_key, b);
      GCTOOLS_ASSERT(found);// assert(found);            /* new table shouldn't be full */
      if ( !(*newHashTable._Keys)[b].unboundp() ) {
        printf("%s:%d About to copy key: %12p   at index %zu    to newHashTable at index: %zu\n", __FILE__, __LINE__,
               old_key.raw_(), i, b );
        printf("Key = %s\n", core::lisp_rep(old_key).c_str());
        printf("    original value@%p = %s\n", (*this->_Values)[i].raw_(), core::lisp_rep((*this->_Values)[i]).c_str());
        printf("newHashTable value@%p = %s\n", (*newHashTable._Values)[b].raw_(), core::lisp_rep((*newHashTable._Values)[b]).c_str());
        printf("--------- Original table\n");
        printf("%s\n", this->dump("Original").c_str());
        printf("--------- New table\n");
        printf("%s\n", newHashTable.dump("Copy").c_str());
      }
      GCTOOLS_ASSERT((*newHashTable._Keys)[b].unboundp()); /* shouldn't be in new table */
      newHashTable._Keys->set(b,old_key);
      (*newHashTable._Values)[b] = (*this->_Values)[i];
      if (key && old_key == key ) {
        key_bucket = b;
        result = 1;
      }
      (*newHashTable._Keys).setUsed((*newHashTable._Keys).used()+1); // TAG_COUNT(UNTAG_COUNT(new_keys->used) + 1);
    }
  }
  GCTOOLS_ASSERT( (*newHashTable._Keys).used() == (newHashTable.tableSize()) );
		// assert(UNTAG_COUNT(new_keys->used) == table_size(tbl));
  this->swap(newHashTable);
  return result;
}

size_t StrongKeyHashTable::rehash(size_t newLength, const value_type &key, size_t &key_bucket) {
  size_t result;
  result = this->rehash_not_safe(newLength,key,key_bucket);
  return result;
}

/*! trySet returns 0 only if there is no room in the hash-table */
int StrongKeyHashTable::trySet(core::T_sp tkey, core::T_sp value) {
  GCWEAK_LOG(fmt::format("Entered trySet with key {}" , tkey.raw_()));
  size_t b;
  if (tkey == value) {
    value = gctools::make_tagged_same_as_key<core::T_O>();
  }
  value_type key(tkey);
#ifdef DEBUG_TRYSET
  stringstream report;
  report << "About to trySet with the key " << tkey.raw_() << std::endl;
  report << this->dump("trySet-incoming StrongKeyHashTable: ") << std::endl;
  // Count the number of times the key is in the table
  int alreadyThere = 0;
  int firstOtherIndex = 0;
  for (int i(0); i < (*this->_Keys).length(); ++i) {
    if ((*this->_Keys)[i] == tkey) {
      firstOtherIndex = i;
      ++alreadyThere;
    }
  }
#endif
  size_t result = StrongKeyHashTable::find(this->_Keys, key, b);
  if ((!result || (*this->_Keys)[b] != key)) {
    GCWEAK_LOG(fmt::format("then case - Returned from find with result = {}     (*this->_Keys)[b={}] = {}" , result , b , (*this->_Keys)[b].raw_()));
#ifdef DEBUG_TRYSET
    if (alreadyThere) {
      report << "Could not find the key @" << key.raw_() << std::endl;
      if (!result) {
        report << " because find return result = " << result << " - which means that the hash table was searched and the key was not found before the hash list was exhausted" << std::endl;
      }
      if ((*this->_Keys)[b] != key) {
        report << " because find hit an empty (unbound or deleted) slot before it found the key" << std::endl;
      }
      report << " ...  about to check if mps_ld_isstale" << std::endl;
    }
#endif
  } else {
    GCWEAK_LOG(fmt::format("else case - Returned from find with result = {}     (*this->_Keys)[b={}] = {}" , result , b , (*this->_Keys)[b].raw_()));
    GCWEAK_LOG(fmt::format("Calling mps_ld_add for key: {}" , (void *)key.raw_()));
  }
  if ((*this->_Keys)[b].unboundp()) {
    GCWEAK_LOG(fmt::format("Writing key over unbound entry"));
    this->_Keys->set(b, key);
    (*this->_Keys).setUsed((*this->_Keys).used() + 1);
#ifdef DEBUG_GCWEAK
    printf("%s:%d key was unboundp at %zu  used = %d\n", __FILE__, __LINE__, b, this->_Keys->used());
#endif
  } else if ((*this->_Keys)[b].deletedp()) {
    GCWEAK_LOG(fmt::format("Writing key over deleted entry"));
    this->_Keys->set(b, key);
    GCTOOLS_ASSERT((*this->_Keys).deleted() > 0);
    (*this->_Keys).setDeleted((*this->_Keys).deleted() - 1);
#ifdef DEBUG_GCWEAK
    printf("%s:%d key was deletedp at %zu  deleted = %d\n", __FILE__, __LINE__, b, (*this->_Keys).deleted());
#endif // DEBUG_GCWEAK
  }
  GCWEAK_LOG(fmt::format("Setting value at b = {}" , b));
  (*this->_Values).set(b, value_type(value));
#ifdef DEBUG_TRYSET
  // Count the number of times the key is in the table
  int count = 0;
  int otherIndex;
  for (int i(0); i < (*this->_Keys).length(); ++i) {
    if ((*this->_Keys)[i] == tkey) {
      if (i != b) {
        otherIndex = i;
      }
      ++count;
    }
  }
  if (count > 1) {
    printf("Found %d duplicate keys %p in hash table\n", count, tkey.raw_());
    printf("Log of trySet action:\n%s\n", report.str().c_str());
    printf("The new key is at %zu and another instance of the key is at %d\n", b, otherIndex);
    printf("%s\n", this->dump("Final trySet table").c_str());
  }
#endif
  GCWEAK_LOG(fmt::format("Leaving trySet"));
  return 1;
}

string StrongKeyHashTable::dump(const string &prefix) {
  stringstream sout;
  size_t i, length;
  length = this->_Keys->length();
  sout << "===== Dumping StrongKeyHashTable length = " << length << std::endl;
  for (i = 0; i < length; ++i) {
    value_type& old_key = (*this->_Keys)[i];
    sout << prefix << "  [" << i << "]  key= " << old_key.raw_() << "  value = " << (*this->_Values)[i].raw_() << std::endl;
  }
  return sout.str();
};

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
//
// Use safeRun from here on down
//
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

core::T_mv StrongKeyHashTable::gethash(core::T_sp tkey, core::T_sp defaultValue) {
  core::T_mv result_mv;
  value_type key(tkey);
  size_t pos;
  size_t result = gctools::StrongKeyHashTable::find(this->_Keys,key,pos);
  if (result) { // StrongKeyHashTable::find(this->_Keys,key,false,pos)) { //buckets_find(tbl, this->keys, key, NULL, &b)) {
    value_type& k = (*this->_Keys)[pos];
    GCWEAK_LOG(fmt::format("gethash find successful pos = {}  k= {} k.unboundp()={} k.base_ref().deletedp()={} k.NULLp()={}" , pos , k.raw_() , k.unboundp() , k.deletedp() , (bool)k ));
    if ( !k.unboundp() && !k.deletedp() ) {
      GCWEAK_LOG(fmt::format("Returning success!"));
      core::T_sp value = smart_ptr<core::T_O>((*this->_Values)[pos]);
      if ( value.same_as_keyP() ) {
        value = smart_ptr<core::T_O>(k);
      }
      result_mv = Values(value,core::lisp_true());
      return result_mv;
    }
    GCWEAK_LOG(fmt::format("Falling through"));
  }
  result_mv = Values(defaultValue,nil<core::T_O>());
  return result_mv;
}

void StrongKeyHashTable::set(core::T_sp key, core::T_sp value) {
  if (this->fullp_not_safe() || !this->trySet(key,value) ) {
    int res;
    value_type dummyKey;
    size_t dummyPos;
    this->rehash( (*this->_Keys).length() * 2, dummyKey, dummyPos );
    res = this->trySet( key, value);
    GCTOOLS_ASSERT(res);
  }
}

void StrongKeyHashTable::maphash(std::function<void(core::T_sp, core::T_sp)> const &fn) {
  size_t length = this->_Keys->length();
  for (int i = 0; i < length; ++i) {
    value_type& old_key = (*this->_Keys)[i];
    if (!old_key.unboundp() && !old_key.deletedp()) {
      core::T_sp tkey(old_key);
      core::T_sp tval((*this->_Values)[i]);
      fn(tkey,tval);
    }
  }
}

bool StrongKeyHashTable::remhash(core::T_sp tkey) {
  size_t b;
  value_type key(tkey);
  size_t result = gctools::StrongKeyHashTable::find(this->_Keys, key, b);
  if( ! result ||
      (*this->_Keys)[b].unboundp() ||
      (*this->_Keys)[b].deletedp() )
  {
    if(!this->rehash( (*this->_Keys).length(), key, b))
      return false;
  }
  if( !(*this->_Keys)[b].unboundp() &&
      !(*this->_Keys)[b].deletedp() )
  {
    auto deleted = value_type(gctools::make_tagged_deleted<core::T_O*>());
    this->_Keys->set(b, deleted);
    (*this->_Keys).setDeleted((*this->_Keys).deleted()+1);
    (*this->_Values)[b] = value_type(gctools::make_tagged_unbound<core::T_O*>());
    return true;
  }
  return false;
}

void StrongKeyHashTable::clrhash() {
  size_t len = (*this->_Keys).length();
  for ( size_t i(0); i<len; ++i ) {
    this->_Keys->set(i,value_type(gctools::make_tagged_deleted<core::T_O*>()));
    (*this->_Values)[i] = value_type(gctools::make_tagged_unbound<core::T_O*>());
  }
  (*this->_Keys).setUsed(0);
  (*this->_Keys).setDeleted(0);
};
};

#endif

#if defined(USE_MPS)
extern "C" {
using namespace gctools;
mps_res_t weak_obj_scan(mps_ss_t ss, mps_addr_t base, mps_addr_t limit) {
#if !defined(RUNNING_PRECISEPREP)  
  MPS_SCAN_BEGIN(ss) {
    while (base < limit) {
      WeakObject *weakObj = reinterpret_cast<WeakObject *>(base);
      switch (weakObj->kind()) {
      case WeakBucketKind: {
        WeakBucketsObjectType *obj = reinterpret_cast<WeakBucketsObjectType *>(weakObj);
        // Fix the obj->dependent pointer
        core::T_O *p = reinterpret_cast<core::T_O *>(obj->dependent.raw_());
        if (gctools::tagged_objectp(p) && MPS_FIX1(ss, p)) {
          core::T_O *pobj = gctools::untag_object<core::T_O *>(p);
          core::T_O *tag = gctools::ptag<core::T_O *>(p);
          mps_res_t res = MPS_FIX2(ss, reinterpret_cast<mps_addr_t *>(&pobj));
          if (res != MPS_RES_OK)
            return res;
          p = reinterpret_cast<core::T_O *>(reinterpret_cast<uintptr_t>(pobj) | reinterpret_cast<uintptr_t>(tag));
          obj->dependent.rawRef_() = reinterpret_cast<gctools::tagged_pointer<gctools::BucketsBase<gctools::smart_ptr<core::T_O>, gctools::smart_ptr<core::T_O> > >::Type *>(p); //reinterpret_cast<gctools::Header_s*>(p);
        }
        for (int i(0), iEnd(obj->length()); i < iEnd; ++i) {
          core::T_O *p = reinterpret_cast<core::T_O *>(obj->bucket[i].raw_());
          if (gctools::tagged_objectp(p) && MPS_FIX1(ss, p)) {
            core::T_O *pobj = gctools::untag_object<core::T_O *>(p);
            core::T_O *tag = gctools::ptag<core::T_O *>(p);
            mps_res_t res = MPS_FIX2(ss, reinterpret_cast<mps_addr_t *>(&pobj));
            if (res != MPS_RES_OK)
              return res;
            if (pobj == NULL && obj->dependent) {
              obj->dependent->bucket[i] = WeakBucketsObjectType::value_type(gctools::make_tagged_deleted<core::T_O *>());
              obj->bucket[i] = WeakBucketsObjectType::value_type(gctools::make_tagged_deleted<core::T_O *>());
            } else {
              p = reinterpret_cast<core::T_O *>(reinterpret_cast<uintptr_t>(pobj) | reinterpret_cast<uintptr_t>(tag));
              obj->bucket[i].setRaw_(reinterpret_cast<gc::Tagged>(p)); //reinterpret_cast<gctools::Header_s*>(p);
            }
          }
        }
        base = (char *)base + sizeof(WeakBucketsObjectType) + sizeof(typename WeakBucketsObjectType::value_type) * obj->length();
      } break;
      case StrongBucketKind: {
        StrongBucketsObjectType *obj = reinterpret_cast<StrongBucketsObjectType *>(base);
        // Fix the obj->dependent pointer
        core::T_O *p = reinterpret_cast<core::T_O *>(obj->dependent.raw_());
        if (gctools::tagged_objectp(p) && MPS_FIX1(ss, p)) {
          core::T_O *pobj = gctools::untag_object<core::T_O *>(p);
          core::T_O *tag = gctools::ptag<core::T_O *>(p);
          mps_res_t res = MPS_FIX2(ss, reinterpret_cast<mps_addr_t *>(&pobj));
          if (res != MPS_RES_OK)
            return res;
          p = reinterpret_cast<core::T_O *>(reinterpret_cast<uintptr_t>(pobj) | reinterpret_cast<uintptr_t>(tag));
          obj->dependent.rawRef_() = reinterpret_cast<gctools::tagged_pointer<gctools::BucketsBase<gctools::smart_ptr<core::T_O>, gctools::smart_ptr<core::T_O> > >::Type *>(p); //reinterpret_cast<gctools::Header_s*>(p);
        }
        for (int i(0), iEnd(obj->length()); i < iEnd; ++i) {
          // MPS_FIX12(ss,reinterpret_cast<mps_addr_t*>(&(obj->bucket[i].raw_())));
          core::T_O *p = reinterpret_cast<core::T_O *>(obj->bucket[i].raw_());
          if (gctools::tagged_objectp(p) && MPS_FIX1(ss, p)) {
            core::T_O *pobj = gctools::untag_object<core::T_O *>(p);
            core::T_O *tag = gctools::ptag<core::T_O *>(p);
            mps_res_t res = MPS_FIX2(ss, reinterpret_cast<mps_addr_t *>(&pobj));
            if (res != MPS_RES_OK)
              return res;
            p = reinterpret_cast<core::T_O *>(reinterpret_cast<uintptr_t>(pobj) | reinterpret_cast<uintptr_t>(tag));
            obj->bucket[i].setRaw_((gc::Tagged)(p)); //reinterpret_cast<gctools::Header_s*>(p);
          }
        }
        base = (char *)base + sizeof(StrongBucketsObjectType) + sizeof(typename StrongBucketsObjectType::value_type) * obj->length();
      } break;
      case WeakMappingKind: {
        WeakMappingObjectType *obj = reinterpret_cast<WeakMappingObjectType *>(weakObj);
        // Fix the obj->dependent pointer
        {
          core::T_O *p = reinterpret_cast<core::T_O *>(obj->dependent.raw_());
          if (gctools::tagged_objectp(p) && MPS_FIX1(ss, p)) {
            core::T_O *pobj = gctools::untag_object<core::T_O *>(p);
            core::T_O *tag = gctools::ptag<core::T_O *>(p);
            mps_res_t res = MPS_FIX2(ss, reinterpret_cast<mps_addr_t *>(&pobj));
            if (res != MPS_RES_OK)
              return res;
            p = reinterpret_cast<core::T_O *>(reinterpret_cast<uintptr_t>(pobj) | reinterpret_cast<uintptr_t>(tag));
            obj->dependent.rawRef_() = reinterpret_cast<gctools::tagged_pointer<gctools::MappingBase<gctools::smart_ptr<core::T_O>, gctools::smart_ptr<core::T_O> > >::Type *>(p); //reinterpret_cast<gctools::Header_s*>(p);
          }
        }
        core::T_O *p = reinterpret_cast<core::T_O *>(obj->bucket.raw_());
        if (gctools::tagged_objectp(p) && MPS_FIX1(ss, p)) {
          core::T_O *pobj = gctools::untag_object<core::T_O *>(p);
          core::T_O *tag = gctools::ptag<core::T_O *>(p);
          mps_res_t res = MPS_FIX2(ss, reinterpret_cast<mps_addr_t *>(&pobj));
          if (res != MPS_RES_OK)
            return res;
          if (p == NULL && obj->dependent) {
            obj->dependent->bucket = WeakBucketsObjectType::value_type(gctools::make_tagged_deleted<core::T_O *>());
            obj->bucket = WeakBucketsObjectType::value_type(gctools::make_tagged_deleted<core::T_O *>());
          } else {
            p = reinterpret_cast<core::T_O *>(reinterpret_cast<uintptr_t>(pobj) | reinterpret_cast<uintptr_t>(tag));
            obj->bucket.setRaw_((gc::Tagged)(p)); // raw_() = reinterpret_cast<core::T_O*>(p);
          }
        }
        base = (char *)base + sizeof(WeakMappingObjectType);
      } break;
      case StrongMappingKind: {
        StrongMappingObjectType *obj = reinterpret_cast<StrongMappingObjectType *>(base);
        // Fix the obj->dependent pointer
        {
          core::T_O *p = reinterpret_cast<core::T_O *>(obj->dependent.raw_());
          if (gctools::tagged_objectp(p) && MPS_FIX1(ss, p)) {
            core::T_O *pobj = gctools::untag_object<core::T_O *>(p);
            core::T_O *tag = gctools::ptag<core::T_O *>(p);
            mps_res_t res = MPS_FIX2(ss, reinterpret_cast<mps_addr_t *>(&pobj));
            if (res != MPS_RES_OK)
              return res;
            p = reinterpret_cast<core::T_O *>(reinterpret_cast<uintptr_t>(pobj) | reinterpret_cast<uintptr_t>(tag));
            obj->dependent.rawRef_() = reinterpret_cast<gctools::tagged_pointer<gctools::MappingBase<gctools::smart_ptr<core::T_O>, gctools::smart_ptr<core::T_O> > >::Type *>(p); //reinterpret_cast<gctools::Header_s*>(p);
          }
        }
        //                    MPS_FIX12(ss,reinterpret_cast<mps_addr_t*>(&(obj->bucket.raw_())));
        core::T_O *p = reinterpret_cast<core::T_O *>(obj->bucket.raw_());
        if (gctools::tagged_objectp(p) && MPS_FIX1(ss, p)) {
          core::T_O *pobj = gctools::untag_object<core::T_O *>(p);
          core::T_O *tag = gctools::ptag<core::T_O *>(p);
          mps_res_t res = MPS_FIX2(ss, reinterpret_cast<mps_addr_t *>(&pobj));
          if (res != MPS_RES_OK)
            return res;
          p = reinterpret_cast<core::T_O *>(reinterpret_cast<uintptr_t>(pobj) | reinterpret_cast<uintptr_t>(tag));
          obj->bucket.setRaw_((gc::Tagged)(p)); //reinterpret_cast<gctools::Header_s*>(p);
        }
        base = (char *)base + sizeof(StrongMappingObjectType);
      } break;
      case WeakPointerKind: {
        WeakPointer *obj = reinterpret_cast<WeakPointer *>(base);
        // MPS_FIX12(ss,reinterpret_cast<mps_addr_t*>(&(obj->value.raw_())));
        core::T_O *p = reinterpret_cast<core::T_O *>(obj->value.raw_());
        if (gctools::tagged_objectp(p) && MPS_FIX1(ss, p)) {
          core::T_O *pobj = gctools::untag_object<core::T_O *>(p);
          core::T_O *tag = gctools::ptag<core::T_O *>(p);
          mps_res_t res = MPS_FIX2(ss, reinterpret_cast<mps_addr_t *>(&pobj));
          if (res != MPS_RES_OK)
            return res;
          p = reinterpret_cast<core::T_O *>(reinterpret_cast<uintptr_t>(pobj) | reinterpret_cast<uintptr_t>(tag));
          obj->value.setRaw_((gc::Tagged)(p)); //reinterpret_cast<gctools::Header_s*>(p);
        }
        base = (char *)base + sizeof(WeakPointer);
      } break;
      default:
          THROW_HARD_ERROR("Handle other weak kind {}", weakObj->kind());
      }
    };
  }
  MPS_SCAN_END(ss);
#endif
  return MPS_RES_OK;
}

mps_addr_t weak_obj_skip_debug(mps_addr_t base, bool dbg) {
#if !defined(RUNNING_PRECISEPREP)
  GCWEAK_LOG(fmt::format("weak_obj_skip base={}" , ((void *)base)));
  WeakObject *weakObj = reinterpret_cast<WeakObject *>(base);
  switch (weakObj->kind()) {
  case WeakBucketKind: {
    WeakBucketsObjectType *obj = reinterpret_cast<WeakBucketsObjectType *>(weakObj);
    GCWEAK_LOG(fmt::format("WeakBucketKind sizeof(WeakBucketsObjectType)={} + sizeof(typename WeakBucketsObjectType::value_type)={} * obj->length()={}" , sizeof(WeakBucketsObjectType) , sizeof(typename WeakBucketsObjectType::value_type) , obj->length()));
    base = (char *)base + sizeof(WeakBucketsObjectType) + sizeof(typename WeakBucketsObjectType::value_type) * obj->length();
  } break;
  case StrongBucketKind: {
    StrongBucketsObjectType *obj = reinterpret_cast<StrongBucketsObjectType *>(base);
    GCWEAK_LOG(fmt::format("StrongBucketKind sizeof(StrongBucketsObjectType)={} + sizeof(typename StrongBucketsObjectType::value_type)={} * obj->length()={}" , sizeof(StrongBucketsObjectType) , sizeof(typename StrongBucketsObjectType::value_type) , obj->length()));
    base = (char *)base + sizeof(StrongBucketsObjectType) + sizeof(typename StrongBucketsObjectType::value_type) * obj->length();
  } break;
  case WeakMappingKind: {
    GCWEAK_LOG(fmt::format("WeakMappingKind"));
    base = (char *)base + sizeof(WeakMappingObjectType);
  } break;
  case StrongMappingKind: {
    GCWEAK_LOG(fmt::format("StrongMappingKind"));
    base = (char *)base + sizeof(StrongMappingObjectType);
  } break;
  case WeakPointerKind: {
    GCWEAK_LOG(fmt::format("WeakPointerKind"));
    base = (char *)base + sizeof(WeakPointer);
  } break;
  case WeakFwdKind: {
    GCWEAK_LOG(fmt::format("WeakFwdKind"));
    weak_fwd_s *obj = reinterpret_cast<weak_fwd_s *>(base);
    base = (char *)base + Align(obj->size.unsafe_fixnum());
  } break;
  case WeakFwd2Kind: {
    GCWEAK_LOG(fmt::format("WeakFwd2Kind"));
    base = (char *)base + Align(sizeof(weak_fwd2_s));
  } break;
  case WeakPadKind: {
    GCWEAK_LOG(fmt::format("WeakPadKind"));
    weak_pad_s *obj = reinterpret_cast<weak_pad_s *>(base);
    base = (char *)base + Align(obj->size.unsafe_fixnum());
  } break;
  case WeakPad1Kind: {
    GCWEAK_LOG(fmt::format("WeakPad1Kind"));
    base = (char *)base + Align(sizeof(weak_pad1_s));
  }
  default:
      THROW_HARD_ERROR("Handle weak_obj_skip other weak kind {}",  weakObj->kind());
  }
  GCWEAK_LOG(fmt::format("weak_obj_skip returning base={}" , ((void *)base)));
#endif
  return base;
};


mps_addr_t weak_obj_skip(mps_addr_t client) {
  return weak_obj_skip_debug(client,false);
}

__attribute__((noinline))  mps_addr_t weak_obj_skip_debug_wrong_size(mps_addr_t client,
                                                                     size_t allocate_size,
                                                                     size_t skip_size) {
  int64_t delta = (int64_t)allocate_size - (int64_t)skip_size;
  printf("%s:%d weak_obj_skip_debug_wrong_size bad size calc allocate_size -> %lu  obj_skip -> %lu delta -> %d\n         About to recalculate the size - connect a debugger and break on obj_skip_debug_wrong_size to trap\n",
         __FILE__, __LINE__, allocate_size, skip_size, (int)delta );
  return weak_obj_skip_debug(client,true);
}

void weak_obj_fwd(mps_addr_t old, mps_addr_t newv) {
#if !defined(RUNNING_PRECISEPREP)
  WeakObject *weakObj = reinterpret_cast<WeakObject *>(old);
  mps_addr_t limit = weak_obj_skip(old);
  size_t size = (char *)limit - (char *)old;
  assert(size >= Align(sizeof(weak_fwd2_s)));
  if (size == Align(sizeof(weak_fwd2_s))) {
    weak_fwd2_s *weak_fwd2_obj = reinterpret_cast<weak_fwd2_s *>(weakObj);
    weak_fwd2_obj->_setKind(WeakFwd2Kind);
    weak_fwd2_obj->fwd = reinterpret_cast<WeakObject *>(newv);
  } else {
    weak_fwd_s *weak_fwd_obj = reinterpret_cast<weak_fwd_s *>(weakObj);
    weak_fwd_obj->setKind(WeakFwdKind);
    weak_fwd_obj->fwd = reinterpret_cast<WeakObject *>(newv);
    weak_fwd_obj->size = gc::make_tagged_fixnum<core::Fixnum_I *>(size);
  }
#endif
}

mps_addr_t weak_obj_isfwd(mps_addr_t addr) {
#if !defined(RUNNING_PRECISEPREP)
  WeakObject *obj = reinterpret_cast<WeakObject *>(addr);
  switch (obj->kind()) {
  case WeakFwd2Kind: {
    weak_fwd2_s *weak_fwd2_obj = reinterpret_cast<weak_fwd2_s *>(obj);
    return weak_fwd2_obj->fwd;
  } break;
  case WeakFwdKind: {
    weak_fwd_s *weak_fwd_obj = reinterpret_cast<weak_fwd_s *>(obj);
    return weak_fwd_obj->fwd;
  }
  }
#endif
  return NULL;
}

void weak_obj_pad(mps_addr_t addr, size_t size) {
#if !defined(RUNNING_PRECISEPREP)
  WeakObject *weakObj = reinterpret_cast<WeakObject *>(addr);
  assert(size >= Align(sizeof(weak_pad1_s)));
  if (size == Align(sizeof(weak_pad1_s))) {
    weakObj->setKind(WeakPad1Kind);
  } else {
    weakObj->setKind(WeakPadKind);
    weak_pad_s *weak_pad_obj = reinterpret_cast<weak_pad_s *>(addr);
    weak_pad_obj->size = gctools::make_tagged_fixnum<core::Fixnum_I *>(size);
  }
#endif
}
};

#endif
