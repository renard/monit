/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU Affero General Public License in all respects
 * for all of the code used other than OpenSSL.  
 */


#ifndef ATOMIC_INCLUDED
#define ATOMIC_INCLUDED

#include <unistd.h>
#if DARWIN
#include <libkern/OSAtomic.h>
#elif FREEBSD
#include <sys/types.h>
#include <machine/atomic.h>
#elif SOLARIS
#include <atomic.h>
#elif (! ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 1))))
#error Atomic require GCC version > 4.1
#endif


/**
 * Atomic memory operations and spinlock. Spin locks are a simple, fast, 
 * thread-safe synchronization primitive that is suitable in situations
 * where contention is expected to be low. The spinlock operations use 
 * memory barriers to synchronize access to shared memory protected by
 * the lock. Preemption is possible while the lock is held.
 *
 * @author Tildeslash Ltd. - www.tildeslash.com
 * @see www.mmonit.com
 * @file
 */


typedef struct { volatile int counter; } Atomic_T;


/**
 * Initialize an Atomic lock variable
 * @param A An Atomic object
 * @hideinitializer
 */
#define Atomic_init(A) ((&(A))->counter = 0)


/**
 * Read Atomic value
 * @param A An Atomic object
 * @return The integer value of <code>A</code>
 * @hideinitializer
 */
#define Atomic_read(A) ((&(A))->counter)


/**
 * Set Atomic value
 * @param A An Atomic object
 * @param i The new integer value of <code>A</code>
 * @hideinitializer
 */
#define Atomic_set(A, i) ((&(A))->counter = (i))


/**
 * Atomically increment <code>A</code> (by one) and return 
 * the new value of <code>A</code>
 * @param A An Atomic object
 * @return The new integer value of <code>A</code>
 * @hideinitializer
 */
#if DARWIN
#define Atomic_inc(A) (OSAtomicIncrement32Barrier(&(&(A))->counter))
#elif FREEBSD
#define Atomic_inc(A) (atomic_fetchadd_32(&(&(A))->counter, 1) + 1)
#elif SOLARIS
#define Atomic_inc(A) (atomic_inc_32_nv(&(&(A))->counter))
#elif __GNUC__
#define Atomic_inc(A)  (__sync_add_and_fetch(&(&(A))->counter, 1))
#endif


/**
 * Atomically decrement <code>A</code> (by one) and return the new
 * value of <code>A</code>
 * @param A An Atomic object
 * @return The new integer value of <code>A</code>
 * @hideinitializer
 */
#if DARWIN
#define Atomic_dec(A) (OSAtomicDecrement32Barrier(&(&(A))->counter))
#elif FREEBSD
#define Atomic_dec(A) (atomic_fetchadd_32(&(&(A))->counter, -1) - 1)
#elif SOLARIS
#define Atomic_dec(A) (atomic_dec_32_nv(&(&(A))->counter))
#elif __GNUC__
#define Atomic_dec(A) (__sync_sub_and_fetch(&(&(A))->counter, 1))
#endif


/**
 * Atomic compare and swap. If the current value of A is 
 * <code>o</code>, then write <code>n</code> into <code>A</code>. 
 * Returns true if the comparison is successful and <code>n</code>
 * was written, otherwise false
 * @param A An Atomic object
 * @param o The integer value to test against <code>A</code>
 * @param n The new integer value of <code>A</code> if the comparison 
 * was successful
 * @return True if compare-and-swap succeeded otherwise false
 * @hideinitializer
 */
#if DARWIN
#define Atomic_cas(A, o, n) (OSAtomicCompareAndSwap32Barrier((o), (n), &(&(A))->counter))
#elif FREEBSD
#define Atomic_cas(A, o, n) (atomic_cmpset_acq_32(&(&(A))->counter, (o), (n)))
#elif SOLARIS
#define Atomic_cas(A, o, n) (atomic_cas_32(&(&(A))->counter, (o), (n)))
#elif __GNUC__
#define Atomic_cas(A, o, n) (__sync_bool_compare_and_swap(&(&(A))->counter, (o), (n)))
#endif


/**
 * Spinlock on <code>A</code>. This method will spin if the lock is already
 * held. Because it can spin, this lock may be inefficient in some situations.
 * @param A An Atomic object to lock
 * @hideinitializer
 */
#define Atomic_lock(A) do { while (Atomic_cas(A, 0, 1) == 0) usleep(1); } while(0)


/**
 * Unconditionally unlocks <code>A</code> by zeroing it
 * @param A An Atomic object to unlock
 * @hideinitializer
 */
#define Atomic_unlock(A) ((&(A))->counter = 0)


/**
 * Defines a block of code to execute after the given spinlock is acquired
 * @param A The Atomic to lock
 * @hideinitializer
 */
#define SPINLOCK(A) do { Atomic_T *_yyatomic= (&(A)); Atomic_lock(*_yyatomic);


/**
 * Ends a SPINLOCK block
 * @hideinitializer
 */
#define END_SPINLOCK Atomic_unlock(*_yyatomic); } while (0)


#endif
