// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "parallel_for.h"

namespace embree
{
  template<typename Value>
    struct ParallelPrefixSumState 
  {
    enum { MAX_TASKS = 32 };
    Value counts[MAX_TASKS];
    Value sums  [MAX_TASKS];
  };

#if 0 && USE_TBB

  template<typename Index, typename Value, typename Func, typename Reduction>
  class ParallelScanBody 
  {
  public:
    const Value& identity;
    const Func& func;
    const Reduction& reduction;
    Value sum;

  public:
    ParallelScanBody ( const Value& identity, const Func& func, const Reduction& reduction ) 
      : identity(identity), func(func), reduction(reduction), sum(identity) {}

    ParallelScanBody ( ParallelScanBody& b, tbb::split ) 
      : identity(b.identity), func(b.func), reduction(b.reduction), sum(identity) {}

    void reverse_join( ParallelScanBody& a ) { 
      sum = reduction(a.sum,sum);
    }

    void assign( ParallelScanBody& b ) { 
      sum = b.sum; 
    }

    template<typename Tag>
      void operator()( const tbb::blocked_range<Index>& r, Tag ) 
      {
	Value temp = sum;
	sum = func(range<Index>(r.begin(),r.end()),temp);
      }
  };

  template<typename Index, typename Value, typename Func, typename Reduction>
    __forceinline Value parallel_prefix_sum( ParallelPrefixSumState<Value>& state, Index first, Index last, Index minStepSize, const Value& identity, const Func& func, const Reduction& reduction)
  {
    ParallelScanBody<Index,Value,Func,Reduction> body(identity,func,reduction);
    tbb::parallel_scan( tbb::blocked_range<Index>(first,last), body );
    return body.sum;
  }

#else

  template<typename Index, typename Value, typename Func, typename Reduction>
    __forceinline Value parallel_prefix_sum( ParallelPrefixSumState<Value>& state, Index first, Index last, Index minStepSize, const Value& identity, const Func& func, const Reduction& reduction)
  {
    /* calculate number of tasks to use */
#if USE_TBB
    const size_t numThreads = tbb::task_scheduler_init::default_num_threads();
#else
    LockStepTaskScheduler* scheduler = LockStepTaskScheduler::instance();
    const size_t numThreads = scheduler->getNumThreads();
#endif
    const size_t numBlocks  = (last-first+minStepSize-1)/minStepSize;
    const size_t taskCount  = min(numThreads,numBlocks,size_t(ParallelPrefixSumState<Value>::MAX_TASKS));

    /* perform parallel prefix sum */
    parallel_for(taskCount, [&](const size_t taskIndex)
    {
      const size_t i0 = first+(taskIndex+0)*(last-first)/taskCount;
      const size_t i1 = first+(taskIndex+1)*(last-first)/taskCount;
      state.counts[taskIndex] = func(range<size_t>(i0,i1),state.sums[taskIndex]);
    });

    /* calculate prefix sum */
    Value sum=identity; // FIXME: optimize, initialize with first element
    for (size_t i=0; i<taskCount; i++) 
    {
      const Value c = state.counts[i];
      state.sums[i] = sum;
      sum=reduction(sum,c);
    }

    return sum;
  }
#endif
}
