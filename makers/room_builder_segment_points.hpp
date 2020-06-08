//
// Created by Dado on 2019-05-04.
//

#pragma once

#include <vector>
#include <core/math/vector4f.h>
#include <core/util.h>
#include <eh_arch/models/htypes.hpp>
#include <core/hashable.hpp>
#include <eh_arch/makers/bespoke_data/house_maker_bespoke_data.hpp>

struct RoomBuilderSegmentPoint {
    V3f point = Vector3f::ZERO;
    ArchTypeT type = ArchType::WallT;
};

class RoomBuilderSegmentPoints : public Hashable<> {
public:
    RoomBuilderSegmentPoints() = default;
    JSONSERIAL(RoomBuilderSegmentPoints, plist, ptypes)
    void add( const RoomBuilderSegmentPoint& _p );
    void clear();
    V3f back( ) const;
    V3f front( ) const;
    void pop_back( );
    void pop_back_single( );
    void pop_front( );
    size_t size() const { return plist.size(); }
    bool empty() const { return plist.empty(); }
    Rect2f BBox() const { return bbox; }
    void bboxUpdate();
    [[nodiscard]] V3f centerForCamera() const;
    void optimize();
    float area() const;
    void scale( float scaleFactor );

    V3f operator[]( size_t _index ) const;

    const V3fVector& points() const;
    SegmentStripVector3d pointsOf( ArchTypeT pt ) const;
    V2fVectorOfVector wallSegments() const;
    std::vector<RoomBuilderSegmentPoint> pointsPair() const;

    template <typename T>
    TwoShapeVector<T> pairOf( ArchTypeT pt ) const {
        TwoShapeVector<T> ret{};

        for ( size_t t = 0; t < plist.size()-1; t++ ) {
            if ( checkBitWiseFlag( ptypes[t], pt ) ) {
                ret.emplace_back( plist[t], plist[t+1] );
            }
        }
        return ret;
    }

    void changePointTypeBack( ArchTypeT _st );
    bool hasSelfClosingWallsOnly( const V3f& _p ) const;
    void finalise();

private:
    // SOA for faster retrival of plists to draw
    V3fVector plist{};
    std::vector<ArchTypeT> ptypes{};
    Rect2f bbox{Rect2f::INVALID};
};

