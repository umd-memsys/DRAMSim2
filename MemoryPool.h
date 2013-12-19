/*********************************************************************************
*  Copyright (c) 2010-2013,  Paul Rosenfeld
*                             Elliott Cooper-Balis
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] gmail [dot] com
*  All rights reserved.
*  
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  
*     * Redistributions of source code must retain the above copyright notice,
*        this list of conditions and the following disclaimer.
*  
*     * Redistributions in binary form must reproduce the above copyright notice,
*        this list of conditions and the following disclaimer in the documentation
*        and/or other materials provided with the distribution.
*  
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/

#ifndef _MEMORY_POOL_
#define _MEMORY_POOL_

#include <iostream>
#include <list> 
#include <vector> 
#include <assert.h>

using std::vector; 
using std::list; 

/**
 * @brief A ridiculously simple memory pool implementation that can grow dynamically
 *
 * This implementation requires the objects to have a default constructor and
 * puts the onus on the user to initialize the objects once they are returned
 * from the pool. 
 */
template <class T>
class MemoryPool {

	vector<T> pool; 
	list<T *> free_list; 
	// on a dealloc, check to make sure the item isn't already on the free list somehow
	bool check_uniqueness; 
	bool debug; 

	public:
	/* 
	 * @param n_elem the initial number of elements in this pool 
	 */
	MemoryPool(unsigned n_elem) 
		: pool(n_elem)
		  , check_uniqueness(true)
		  , debug(true) 
	{
		// Initially, everyone is on the free list 
		for (size_t i=0; i<n_elem; i++) {
			free_list.push_back(&pool[i]);
		}
	}

	/**
	 * Returns a pointer to an UNINITIALIZED object from the pool; automatically
	 * resizes the storage if necessary
	 */
	T *alloc() {
		// resize the pool if the there aren't any more free entries 
		if (free_list.empty()) {
			unsigned old_size = pool.size(); 
			unsigned grow_by = old_size; 
			unsigned new_size = old_size + grow_by; 
			if (debug) {
				std::cerr << "Resizing from "<<old_size<<" entries to "<<new_size<<" entries."<<std::endl;
			}

			// reallocate the storage
			pool.resize(new_size); 

			// add all the new entries to the freelist 
			for (size_t i=old_size; i<new_size; i++) {
				free_list.push_front(&pool[i]);
			}
		}
		T *ret = free_list.front(); 
		free_list.pop_front(); 
		return ret; 
	}
	
	/**
	 * 'free' an object back into the memory pool 
	 */
	void dealloc(T *elem) {
		if (check_uniqueness) {
			for (typename list<T *>::iterator it=free_list.begin(); it != free_list.end(); it++) {
				assert(*it != elem); 
			}
		}
		free_list.push_front(elem); 
	}

};

#endif


