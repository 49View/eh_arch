//
// Created by Dado on 2019-05-04.
//

#include "room_builder_segment_points.hpp"
#include <core/math/poly_utils.hpp>
#include <core/math/triangulator.hpp>

void RoomBuilderSegmentPoints::add( const RoomBuilderSegmentPoint& _p ) {
    plist.emplace_back( _p.point );
    ptypes.emplace_back( _p.type );
    bbox.expand( _p.point.xz() );
}

V3f RoomBuilderSegmentPoints::back() const {
    return plist.back();
}

V3f RoomBuilderSegmentPoints::front() const {
    return plist.front();
}

void RoomBuilderSegmentPoints::pop_back() {
    if ( ptypes.size() > 3 && ptypes[ptypes.size()-3] != ArchType::WallT ) {
        for ( auto t = 0; t < 3; t++ ) {
            plist.pop_back();
            ptypes.pop_back();
        }
    } else {
        plist.pop_back();
        ptypes.pop_back();
    }
    bboxUpdate();
}

void RoomBuilderSegmentPoints::pop_back_single() {
    plist.pop_back();
    ptypes.pop_back();
    bboxUpdate();
}

void RoomBuilderSegmentPoints::pop_front() {
    plist.erase(plist.begin());
    ptypes.erase(ptypes.begin());
    bboxUpdate();
}

V3f RoomBuilderSegmentPoints::operator[]( size_t _index ) const {
    return plist[_index];
}

const V3fVector& RoomBuilderSegmentPoints::points() const {
    return plist;
}

SegmentStripVector3d RoomBuilderSegmentPoints::pointsOf( ArchTypeT pt ) const {
    SegmentStripVector3d ret{};
    SegmentStrip3d currStrip{};

    for ( size_t t = 0; t < plist.size(); t++ ) {
        if ( checkBitWiseFlag( ptypes[t], pt ) ) {
            currStrip.strip.emplace_back( plist[t] );
            if ( t == plist.size() - 1 ) {
                ret.push_back(currStrip);
            }
        } else {
            if ( !currStrip.strip.empty() ) {
                currStrip.strip.emplace_back( plist[t] );
                ret.push_back(currStrip);
                currStrip.strip.clear();
            }
            currStrip.type = ptypes[t];
        }
    }
    return ret;
}

std::vector<RoomBuilderSegmentPoint> RoomBuilderSegmentPoints::pointsPair() const {
    std::vector<RoomBuilderSegmentPoint> ret{};

    for ( size_t t = 0; t < plist.size(); t++ ) {
        ret.emplace_back( RoomBuilderSegmentPoint{ plist[t], ptypes[t] } );
    }
    return ret;
}

void RoomBuilderSegmentPoints::changePointTypeBack( ArchTypeT _st ) {
    if ( !plist.empty() ) {
        ptypes.back() = _st;
    }
}

bool RoomBuilderSegmentPoints::hasSelfClosingWallsOnly( const V3f& _p ) const {
    if ( plist.size() < 3 ) return false;
    for ( const auto& pt : ptypes ) {
        if ( pt != ArchType::WallT ) return false;
    }
    return _p == plist.front();
}

void RoomBuilderSegmentPoints::finalise() {
//    if ( front() == back() ) {
//        pop_back_single();
//    }

    V3fVector shiftedStartPlist{};
    std::vector<ArchTypeT> shiftedStartPtypes{};
    size_t numPointsToShift = 0;
    for ( ; numPointsToShift < plist.size(); numPointsToShift++ ) {
        shiftedStartPlist.emplace_back( plist[numPointsToShift] );
        shiftedStartPtypes.emplace_back( ArchType::WallT );
        if ( ptypes[numPointsToShift] != ArchType::WallT ) {
            break;
        }
    }
    plist.erase( plist.begin(), plist.begin() + numPointsToShift );
    ptypes.erase( ptypes.begin(), ptypes.begin() + numPointsToShift );
    std::copy (shiftedStartPlist.begin(), shiftedStartPlist.end(), std::back_inserter(plist));
    std::copy (shiftedStartPtypes.begin(), shiftedStartPtypes.end(), std::back_inserter(ptypes));

    std::ostringstream ss;
    for ( auto t = 0u; t < plist.size(); t++ ) {
        ss << plist[t];
        ss << ptypes[t];
    }
    Hash( ss.str() );

    // Winding Order
//    forceWindingOrder( plist, WindingOrder::CCW, ptypes );
}

void RoomBuilderSegmentPoints::clear() {
    plist.clear();
    ptypes.clear();
    bbox = Rect2f::INVALID;
}

void RoomBuilderSegmentPoints::bboxUpdate() {
    bbox = Rect2f::INVALID;
    for ( const auto& p : plist ) {
        bbox.expand( p.xz() );
    }
}

V3f RoomBuilderSegmentPoints::centerForCamera() const {
    return V3f{ BBox().centre().x()+1.95f, 6.5f, BBox().centre().y()-0.5f };
}

void RoomBuilderSegmentPoints::optimize() {
    if ( plist.size() < 3 ) return;
    int wrappedIndexOffset = 2;
    for ( size_t t = 0; t < plist.size()-wrappedIndexOffset; t++ ) {
        size_t tp1 = cai(t+1, plist.size());
        if ( ptypes[t] == ptypes[tp1] ) {
            size_t tp2 = cai(t+2, plist.size());
            if ( isCollinear( plist[t], plist[tp1], plist[tp2]) ) {
                ptypes.erase( ptypes.begin() + tp1 );
                plist.erase( plist.begin() + tp1 );
                t = -1;
            }
        }
    }
}

float RoomBuilderSegmentPoints::area() const {

    if ( plist.size() < 3 ) return 0.0f;

    V2fVector vtri{};
    for ( const auto& v : plist ) {
        vtri.emplace_back( v.xz() );
    }
    Triangulator tri( vtri );

    return getAreaOf(tri.get2dTrianglesTuple());
}

void RoomBuilderSegmentPoints::scale( float scaleFactor ) {
    for ( auto& v : plist ) {
        v*=scaleFactor;
    }
}
