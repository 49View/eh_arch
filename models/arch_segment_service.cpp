//
// Created by dado on 04/07/2020.
//

#include "arch_segment_service.hpp"
#include <core/math/vector_util.hpp>
#include <core/util.h>

#include "house_bsdata.hpp"
#include "floor_service.hpp"

ArchSegment ArchSegmentService::createArchSegment( const std::vector <std::shared_ptr<WallBSData>>& floorWalls,
                                                   const int32_t _iFloor, const int32_t _iWall, const int32_t _iIndex,
                                                   const int64_t _wallHash, const Vector2f& _p1, const Vector2f& _p2,
                                                   const Vector2f& _middle, const Vector2f& _normal,
                                                   const uint64_t& _tag, const uint64_t& _sequencePart, float _z,
                                                   float _height ) {
    ArchSegment ret;
    ret.iFloor = _iFloor;
    ret.iWall = _iWall;
    ret.iIndex = _iIndex;
    ret.wallHash = _wallHash;
    ret.p1 = _p1;
    ret.p2 = _p2;
    ret.middle = _middle;
    ret.normal = _normal;
    ret.tag = _tag;
    ret.crossNormal = normalize( ret.p2 - ret.p1 );
    ret.sequencePart = _sequencePart;
    ret.quads.emplace_back( makeQuadV3f(XZY::C(_p1, _z), XZY::C(_p2, _z), _height) );
    for ( const auto& w : floorWalls ) {
        int csize = static_cast<int>( w->epoints.size());
        int wrapAmount = w->wrapLastPoint != 0 ? 0 : 1;
        for ( auto t = 0; t < csize - wrapAmount; t++ ) {
            if ( !FloorService::isIndexInUShape( t, w.get()) && w->sequencePart != 0 ) {
                auto p1 = w->epoints[t];
                auto p2 = w->epoints[cai( t + 1, csize )];
                if ( p1 == _p1 && p2 == _p2 ) {
                    ret.quads.emplace_back( makeQuadV3f(XZY::C(_p1, w->z), XZY::C(_p2, w->z),  w->Height() ) );
                }
            }
        }
    }

    return ret;
}

float ArchSegmentService::length( const ArchSegment& _as ) {
    return distance( _as.p1, _as.p2 );
}

bool ArchSegmentService::doSegmentsEndsMeet( const ArchSegment& s1, const ArchSegment& s2 ) {
    return isVerySimilar( s1.p1, s2.p2 ) || isVerySimilar( s1.p2, s2.p1 );
}

bool ArchSegmentService::doSegmentsEndsMeet( const std::vector <ArchSegment>& segs ) {
    if ( segs.size() < 2 ) return false;
    return doSegmentsEndsMeet( segs.front(), segs.back());
}

bool ArchSegmentService::isSegmentPartOfWindowOrDoor( const ArchSegment& ls ) {
    return checkBitWiseFlag(ls.tag, WallFlags::WF_IsDoorPart) | checkBitWiseFlag(ls.tag, WallFlags::WF_IsWindowPart);
}
