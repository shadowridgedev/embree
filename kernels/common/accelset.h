// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "default.h"
#include "builder.h"
#include "geometry.h"

namespace embree
{
  /*! Base class for set of acceleration structures. */
  class AccelSet : public Geometry
  {
    ALIGNED_CLASS;
  public:

    /*! type of this geometry */
    static const Geometry::Type geom_type = Geometry::USER_GEOMETRY;

    typedef RTCIntersectFunc IntersectFunc;
    typedef RTCIntersectFunc4 IntersectFunc4;
    typedef RTCIntersectFunc8 IntersectFunc8;
    typedef RTCIntersectFunc16 IntersectFunc16;
    typedef RTCIntersectFuncN IntersectFuncN;
    
    typedef RTCOccludedFunc OccludedFunc;
    typedef RTCOccludedFunc4 OccludedFunc4;
    typedef RTCOccludedFunc8 OccludedFunc8;
    typedef RTCOccludedFunc16 OccludedFunc16;
    typedef RTCOccludedFuncN OccludedFuncN;

#if defined(__SSE__)
    typedef void (*ISPCIntersectFunc4)(void* ptr, RTCRay4& ray, size_t item, __m128 valid);
    typedef void (*ISPCOccludedFunc4 )(void* ptr, RTCRay4& ray, size_t item, __m128 valid);
#endif

#if defined(__AVX__)
    typedef void (*ISPCIntersectFunc8)(void* ptr, RTCRay8& ray, size_t item, __m256 valid);
    typedef void (*ISPCOccludedFunc8 )(void* ptr, RTCRay8& ray, size_t item, __m256 valid);
#endif

#if defined(__AVX512F__)
    typedef void (*ISPCIntersectFunc16)(void* ptr, RTCRay16& ray, size_t item, __m128i valid); // mask gets passed as 16 bytes
    typedef void (*ISPCOccludedFunc16 )(void* ptr, RTCRay16& ray, size_t item, __m128i valid); // mask gets passed as 16 bytes
#endif

#if defined(__MIC__)
    typedef void (*ISPCIntersectFunc16)(void* ptr, RTCRay16& ray, size_t item, __mmask16 valid);
    typedef void (*ISPCOccludedFunc16 )(void* ptr, RTCRay16& ray, size_t item, __mmask16 valid);
#endif

    typedef void (*ErrorFunc) ();

    struct Intersector1
    {
      Intersector1 (ErrorFunc error = nullptr) 
      : intersect((IntersectFunc)error), occluded((OccludedFunc)error), name(nullptr) {}

      Intersector1 (IntersectFunc intersect, OccludedFunc occluded, const char* name)
      : intersect(intersect), occluded(occluded), name(name) {}
      
      operator bool() const { return name; }
        
      public:
        static const char* type;
        const char* name;
        IntersectFunc intersect;
        OccludedFunc occluded;  
      };
      
      struct Intersector4 
      {
        Intersector4 (ErrorFunc error = nullptr) 
        : intersect((void*)error), occluded((void*)error), name(nullptr), ispc(false) {}

        Intersector4 (void* intersect, void* occluded, const char* name, bool ispc)
        : intersect(intersect), occluded(occluded), name(name), ispc(ispc) {}
	
        operator bool() const { return name; }
        
      public:
        static const char* type;
        const char* name;
        void* intersect;
        void* occluded;
	bool ispc;
      };
      
      struct Intersector8 
      {
        Intersector8 (ErrorFunc error = nullptr) 
        : intersect((void*)error), occluded((void*)error), name(nullptr), ispc(false) {}

        Intersector8 (void* intersect, void* occluded, const char* name, bool ispc)
        : intersect(intersect), occluded(occluded), name(name), ispc(ispc) {}
        
        operator bool() const { return name; }
        
      public:
        static const char* type;
        const char* name;
        void* intersect;
        void* occluded;
	bool ispc;
      };
      
      struct Intersector16 
      {
        Intersector16 (ErrorFunc error = nullptr) 
        : intersect((void*)error), occluded((void*)error), name(nullptr), ispc(false) {}

        Intersector16 (void* intersect, void* occluded, const char* name, bool ispc)
        : intersect(intersect), occluded(occluded), name(name), ispc(ispc) {}
        
        operator bool() const { return name; }
        
      public:
        static const char* type;
        const char* name;
        void* intersect;
        void* occluded;
	bool ispc;
      };

      struct IntersectorN
      {
        IntersectorN (ErrorFunc error = nullptr) 
        : intersect((IntersectFuncN)error), occluded((OccludedFuncN)error), name(nullptr) {}
        
        IntersectorN (IntersectFuncN intersect, OccludedFuncN occluded, const char* name)
        : intersect(intersect), occluded(occluded), name(name) {}
        
        operator bool() const { return name; }
        
      public:
        static const char* type;
        const char* name;
        IntersectFuncN intersect;
        OccludedFuncN occluded;  
      };
      
    public:
      
      /*! construction */
      AccelSet (Scene* parent, size_t items, size_t numTimeSteps);
      
      /*! makes the acceleration structure immutable */
      virtual void immutable () {}
      
      /*! build accel */
      virtual void build (size_t threadIndex, size_t threadCount) = 0;

      /*! Calculates the bounds of an item */
      __forceinline BBox3fa bounds (size_t item) const
      {
        BBox3fa box[2]; // have to always use 2 boxes as the geometry might have motion blur
        assert(item < size());
        if (boundsFunc2) boundsFunc2(boundsFunc2UserPtr,intersectors.ptr,item,(RTCBounds*)box);
        else             boundsFunc(intersectors.ptr,item,(RTCBounds&)box[0]);
        return box[0];
      }

      /*! Calculates the bounds of an item */
      __forceinline std::pair<BBox3fa,BBox3fa> bounds_mblur (size_t item) const
      {
        BBox3fa box[2]; 
        assert(item < size());
        if (boundsFunc2) boundsFunc2(boundsFunc2UserPtr,intersectors.ptr,item,(RTCBounds*)box);
        else             boundsFunc(intersectors.ptr,item,(RTCBounds&)box[0]);
        return std::make_pair(box[0],box[1]);
      }

      /*! check if the i'th primitive is valid */
      __forceinline bool valid(size_t i, BBox3fa* bbox = nullptr) const 
      {
        const BBox3fa b = bounds(i);
        if (bbox) *bbox = b;
        return isvalid(b);
      }
      
      void enabling ();
      void disabling();

  public:

      /*! Intersects a single ray with the scene. */
      __forceinline void intersect (RTCRay& ray, size_t item) 
      {
        assert(item < size());
        assert(intersectors.intersector1.intersect);
        intersectors.intersector1.intersect(intersectors.ptr,ray,item);
      }
   
      /*! Intersects a packet of 4 rays with the scene. */
#if defined(__SSE__)   
      __forceinline void intersect4 (const vbool4& valid, RTCRay4& ray, size_t item) 
      {
        assert(item < size());
        assert(intersectors.intersector4.intersect);
	if (intersectors.intersector4.ispc) ((ISPCIntersectFunc4)intersectors.intersector4.intersect)(intersectors.ptr,ray,item,valid);
        else                                ((    IntersectFunc4)intersectors.intersector4.intersect)(&valid,intersectors.ptr,ray,item);
      }
#endif
      
#if defined(__AVX__)
      /*! Intersects a packet of 8 rays with the scene. */
      __forceinline void intersect8 (const vbool8& valid, RTCRay8& ray, size_t item) 
      {
        assert(item < size());
        assert(intersectors.intersector8.intersect);
	if (intersectors.intersector8.ispc) ((ISPCIntersectFunc8)intersectors.intersector8.intersect)(intersectors.ptr,ray,item,valid);
        else                                ((    IntersectFunc8)intersectors.intersector8.intersect)(&valid,intersectors.ptr,ray,item);
      }
#endif

      /*! Intersects a packet of 16 rays with the scene. */
#if defined(__AVX512F__)
      __forceinline void intersect16 (const vbool16& valid, RTCRay16& ray, size_t item) 
      {
        assert(item < size());
        assert(intersectors.intersector16.occluded);
	if (intersectors.intersector16.ispc) {
    	  ((ISPCIntersectFunc16)intersectors.intersector16.intersect)(intersectors.ptr,ray,item,valid.mask8());
	}
        else {
          vint16 mask = valid.mask32();
	  ((IntersectFunc16)intersectors.intersector16.intersect)(&mask,intersectors.ptr,ray,item);
        }
      }
#endif

      /*! Intersects a packet of 16 rays with the scene. */
#if defined(__MIC__) 
      __forceinline void intersect16 (const vbool16& valid, RTCRay16& ray, size_t item) 
      {
        assert(item < size());
        assert(intersectors.intersector16.occluded);
	if (intersectors.intersector16.ispc) {
         ((ISPCIntersectFunc16)intersectors.intersector16.intersect)(intersectors.ptr,ray,item,valid);
	}
        else {
          vint16 mask = valid.mask32();
	  ((IntersectFunc16)intersectors.intersector16.intersect)(&mask,intersectors.ptr,ray,item);
        }
      }
#endif

      /*! Intersects a stream of rays with the scene. */
      __forceinline void intersectN (RTCRay** rays, size_t N, size_t item) 
      {
        assert(item < size());
        assert(intersectors.intersectorN.intersect);
        intersectors.intersectorN.intersect(intersectors.ptr,rays,N,item);
      }
      
      /*! Tests if single ray is occluded by the scene. */
      __forceinline void occluded (RTCRay& ray, size_t item) {
        assert(intersectors.intersector1.occluded);
        intersectors.intersector1.occluded(intersectors.ptr,ray,item);
      }
      
      /*! Tests if a packet of 4 rays is occluded by the scene. */
#if defined(__SSE__)
      __forceinline void occluded4 (const vbool4& valid, RTCRay4& ray, size_t item) 
      {
        assert(item < size());
	assert(intersectors.intersector4.occluded);
	if (intersectors.intersector4.ispc) ((ISPCOccludedFunc4)intersectors.intersector4.occluded)(intersectors.ptr,ray,item,valid);
        else                                ((    OccludedFunc4)intersectors.intersector4.occluded)(&valid,intersectors.ptr,ray,item);
      }
#endif
      
      /*! Tests if a packet of 8 rays is occluded by the scene. */
#if defined(__AVX__)
      __forceinline void occluded8 (const vbool8& valid, RTCRay8& ray, size_t item) 
      {
        assert(item < size());
	assert(intersectors.intersector8.occluded);
	if (intersectors.intersector8.ispc) ((ISPCOccludedFunc8)intersectors.intersector8.occluded)(intersectors.ptr,ray,item,valid);
        else                                ((    OccludedFunc8)intersectors.intersector8.occluded)(&valid,intersectors.ptr,ray,item);
      }
#endif
      
      /*! Tests if a packet of 16 rays is occluded by the scene. */
#if defined(__AVX512F__)
      __forceinline void occluded16 (const vbool16& valid, RTCRay16& ray, size_t item) 
      {
        assert(item < size());
        assert(intersectors.intersector16.occluded);
	if (intersectors.intersector16.ispc) {
	  ((ISPCOccludedFunc16)intersectors.intersector16.occluded)(intersectors.ptr,ray,item,valid.mask8());
	}
	else {
          vint16 mask = valid.mask32();
	  ((OccludedFunc16)intersectors.intersector16.occluded)(&mask,intersectors.ptr,ray,item);
        }
      }
#endif

      /*! Tests if a packet of 16 rays is occluded by the scene. */
#if defined(__MIC__)
      __forceinline void occluded16 (const vbool16& valid, RTCRay16& ray, size_t item) 
      {
        assert(item < size());
        assert(intersectors.intersector16.occluded);
	if (intersectors.intersector16.ispc) {
	  ((ISPCOccludedFunc16)intersectors.intersector16.occluded)(intersectors.ptr,ray,item,valid);
	}
	else {
          vint16 mask = valid.mask32();
	  ((OccludedFunc16)intersectors.intersector16.occluded)(&mask,intersectors.ptr,ray,item);
        }
      }
#endif

      /*! Tests if a stream of rays is occluded by the scene. */
      __forceinline void occludedN (RTCRay** rays, size_t N, size_t item) {
        assert(intersectors.intersectorN.occluded);
        intersectors.intersectorN.occluded(intersectors.ptr,rays,N,item);
      }
      
    public:
      RTCBoundsFunc  boundsFunc;
      RTCBoundsFunc2 boundsFunc2;
      void* boundsFunc2UserPtr;

      struct Intersectors 
      {
        Intersectors() : ptr(nullptr) {}
      public:
        void* ptr;
        Intersector1 intersector1;
        Intersector4 intersector4;
        Intersector8 intersector8;
        Intersector16 intersector16;
        IntersectorN intersectorN;
      } intersectors;
  };

#define DEFINE_SET_INTERSECTOR1(symbol,intersector)                     \
  AccelSet::Intersector1 symbol((AccelSet::IntersectFunc)intersector::intersect, \
                                (AccelSet::OccludedFunc )intersector::occluded, \
                                TOSTRING(isa) "::" TOSTRING(symbol));

#define DEFINE_SET_INTERSECTOR4(symbol,intersector)                         \
  AccelSet::Intersector4 symbol((void*)intersector::intersect, \
                                (void*)intersector::occluded, \
                                TOSTRING(isa) "::" TOSTRING(symbol),	\
				false);

#define DEFINE_SET_INTERSECTOR8(symbol,intersector)                         \
  AccelSet::Intersector8 symbol((void*)intersector::intersect, \
                                (void*)intersector::occluded, \
                                TOSTRING(isa) "::" TOSTRING(symbol),	\
				false);

#define DEFINE_SET_INTERSECTOR16(symbol,intersector)                         \
  AccelSet::Intersector16 symbol((void*)intersector::intersect, \
                                 (void*)intersector::occluded, \
                                 TOSTRING(isa) "::" TOSTRING(symbol),\
				 false);  
}
