//
//  room_service.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "room_service.hpp"

#include <poly/scene_graph.h>
#include <core/math/triangulator.hpp>
#include <core/service_factory.h>

#include "floor_service.hpp"
#include "arch_segment_service.hpp"
#include "room_service_furniture.hpp"

bool FurnitureRuleScript::execute( RoomBSData* r, FurnitureMapStorage& furns, const FurnitureRuleFunctionContainer& funcRules ) const {
    bool completed = true;
    for ( const auto& rule : rules ) {
        if ( rule.hasSomethingToDo() ) {
            completed &= funcRules[rule.getRuleFunctionIndex()]( r, furns, rule );
        } else {
            completed = false;
        }
    }
    return completed;
}

namespace RoomService {

    PivotPointPosition pivotPointPositionMirrorY( PivotPointPosition where ) {
        switch ( where ) {
            case PivotPointPosition::TopLeft:
                return PivotPointPosition::TopRight;
            case PivotPointPosition::TopRight:
                return PivotPointPosition::TopLeft;
            case PivotPointPosition::LeftCenter:
                return PivotPointPosition::RightCenter;
            case PivotPointPosition::RightCenter:
                return PivotPointPosition::LeftCenter;
            default:
                return PivotPointPosition::Invalid;
        }
    }

float furnitureAngleFromWall( const ArchSegment *ls ) {
        return atan2( ls->normal.y(), ls->normal.x()) - M_PI_2;
    }

    float furnitureAngleFromNormal( const Vector2f& normal ) {
        return atan2( normal.y(), normal.x()) - M_PI_2;
    }

    Vector2f furnitureNormalFromAngle( const float angle ) {
        return { cosf( angle + M_PI_2 ), sinf( angle + M_PI_2 ) };
    }

    const ArchSegment *longestSegment( const RoomBSData* r ) {
        return &r->mWallSegmentsSorted[0];
    }

    const ArchSegment *secondLongestSegment( const RoomBSData* r ) {
        if ( r->mWallSegmentsSorted.size() > 1 )
            return &r->mWallSegmentsSorted[1];
        return nullptr;
    }

    const ArchSegment *shortestSegment( const RoomBSData* r ) {
        return &r->mWallSegmentsSorted[r->mWallSegmentsSorted.size()-1];
    }

    const ArchSegment *secondShortestSegment( const RoomBSData* r ) {
        if( r->mWallSegmentsSorted.size() > 1 )
            return &r->mWallSegmentsSorted[r->mWallSegmentsSorted.size()-2];
        return nullptr;
    }

    const ArchSegment *thirdShortestSegment( const RoomBSData* r ) {
        if ( r->mWallSegmentsSorted.size() > 2 )
            return &r->mWallSegmentsSorted[r->mWallSegmentsSorted.size()-3];
        return nullptr;
    }

    const ArchSegment *segmentAtIndex( const RoomBSData* r, uint32_t _index) {
        if ( r->mWallSegmentsSorted.size() <= _index ) return nullptr;
        return &r->mWallSegmentsSorted[_index];
    }

    void calcLongestWall( RoomBSData* r ) {
        float maxLength = 0.0f;

        r->mWallSegmentsSorted.clear();
        for ( auto& mw : r->mWallSegments ) {
            r->mWallSegmentsSorted.insert( std::end( r->mWallSegmentsSorted ), std::begin( mw ), std::end( mw ));
        }

        std::sort( r->mWallSegmentsSorted.begin(), r->mWallSegmentsSorted.end(),
                   []( const ArchSegment& a, const ArchSegment& b ) -> bool {
                       return ArchSegmentService::length( a ) > ArchSegmentService::length( b );
                   } );

        for ( auto t = 0u; t < r->mWallSegmentsSorted.size(); t++ ) {
            float wlength = ArchSegmentService::length( r->mWallSegmentsSorted[t] );
            if ( wlength > maxLength ) {
                r->mLongestWall = t;
                maxLength = wlength;
            }
        }

        const ArchSegment *ls = longestSegment( r );
        Vector2f i{V2f::ZERO};
        r->mLongestWallOpposite = -1;
        // Find wall opposite
        for ( auto t = 0u; t < r->mWallSegmentsSorted.size(); t++ ) {
            if (( r->mWallSegmentsSorted[t].tag & WallFlags::WF_IsDoorPart ) > 0 ||
                ( r->mWallSegmentsSorted[t].tag & WallFlags::WF_IsWindowPart ) > 0 ) {
                continue;
            }
            Vector2f normalOpposite = r->mWallSegmentsSorted[t].normal;
            if ( isScalarEqual( dot( normalOpposite, ls->normal ), -1.0f )) {
                if ( intersection( r->mWallSegmentsSorted[t].p1, r->mWallSegmentsSorted[t].p2, ls->middle,
                                   ls->middle + ( ls->normal * 1000.0f ), i )) {
                    r->mLongestWallOpposite = t;
                    r->mLongestWallOppositePoint = i;
                    break;
                }
            }
        }
    }

    ArchSegment *longestSegmentCornerP1( RoomBSData* r ) {
        roomTypeIndex rti = sortedSegmentToPairIndex( r, r->mLongestWall );
        return &r->mWallSegments[rti.first][getCircularArrayIndex( rti.second - 1,
                                                                   static_cast<int32_t>( r->mWallSegments[rti.first].size()))];
    }

    ArchSegment *longestSegmentCornerP2( RoomBSData* r ) {
        roomTypeIndex rti = sortedSegmentToPairIndex( r, r->mLongestWall );
        return &r->mWallSegments[rti.first][getCircularArrayIndex( rti.second + 1,
                                                                   static_cast<int32_t>( r->mWallSegments[rti.first].size()))];
    }

    ArchSegment *longestSegmentOpposite( RoomBSData* r ) {
        if ( r->mLongestWallOpposite < 0 ) {
            return nullptr;
        }
        return &r->mWallSegmentsSorted[r->mLongestWallOpposite];
    }

    float middleHeightFromObject( RoomBSData* r, FittedFurniture& base, FittedFurniture& dec ) {
        return ((r->height - r->mBBoxCoving.height()*0.01f - base.size.y() - dec.size.y()) / 2.0f);
    }

    const ArchSegment *getWallSegmentFor( RoomBSData* r, const WSLO wslo, uint32_t _exactIndex ) {
        const ArchSegment *ls = nullptr;

        switch ( wslo ) {
            case WSLOH::Longest():
                ls = RoomService::longestSegment( r );
                break;
            case WSLOH::LongestOpposite():
                ls = RoomService::longestSegmentOpposite( r );
                break;
            case WSLOH::SecondLongest():
                ls = RoomService::secondLongestSegment( r );
                break;
            case WSLOH::Shortest():
                ls = RoomService::shortestSegment( r );
                break;
            case WSLOH::SecondShortest():
                ls = RoomService::secondShortestSegment( r );
                break;
            case WSLOH::ExactIndex():
                ls = RoomService::segmentAtIndex( r, _exactIndex );
                break;
            default:
                ls = nullptr;
        }
        return ls;
    }

    void placeAroundInternal(  FittedFurniture& dest, const FittedFurniture& source, PivotPointPosition where, const V2f& slack, const float _height ) {
        switch ( where ) {
            case PivotPointPosition::TopLeft:
                dest.xyLocation = source.widthNormal * ( source.size.width() * 0.5f + dest.size.width()*0.5f + slack.x()) +
                                  source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth()*0.5f + slack.y());
                break;
            case PivotPointPosition::TopRight:
                dest.xyLocation = source.widthNormal * ( -source.size.width() * 0.5f - dest.size.width()*0.5f + slack.x()) +
                                  source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth()*0.5f + slack.y());
                break;
            case PivotPointPosition::TopCenter:
                dest.xyLocation = source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth()*0.5f + slack.y());
                break;
            case PivotPointPosition::LeftCenter:
                dest.xyLocation = source.widthNormal * -( source.size.width() * 0.5f + dest.size.width()*0.5f + slack.x());
                break;
            case PivotPointPosition::RightCenter:
                dest.xyLocation = source.widthNormal * ( source.size.width() * 0.5f + dest.size.width()*0.5f + slack.x());
                break;
            default:
                break;
        }
        dest.xyLocation += source.xyLocation;
        dest.heightOffset = _height;
        dest.rotation = source.rotation;
        dest.depthNormal = source.depthNormal;
        dest.widthNormal = source.widthNormal;
    }

    void placeAlongWallInternal(  FittedFurniture& dest, const FittedFurniture& source,
            const ArchSegment *ls, WallSegmentCorner preferredCorner, const V2f& slack, const float _height ) {

        auto ln = preferredCorner == WSC_P2 ? (ls->p1 - ls->p2) : (ls->p2 -ls->p1);
        ln = normalize(ln);
        dest.xyLocation = ln * ( source.size.width() * 0.5f + dest.size.width()*0.5f + slack.x());
        dest.xyLocation += source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth()*0.5f + slack.y());
        dest.xyLocation += source.xyLocation;
        dest.heightOffset = _height;
        dest.rotation = source.rotation;
        dest.depthNormal = source.depthNormal;
        dest.widthNormal = source.widthNormal;
    }

//    void placeAroundInBetweenInternal(  FittedFurniture& dest, const FittedFurniture& source, PivotPointPosition where, const V2f& slack, const float _height ) {
//        switch ( where ) {
//            case PivotPointPosition::TopLeft:
//                dest.xyLocation = source.widthNormal * ( source.size.width() * 0.5f + dest.size.width()*0.5f + slack.x()) +
//                                  source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth()*0.5f + slack.y());
//                break;
//            case PivotPointPosition::TopRight:
//                dest.xyLocation = source.widthNormal * ( -source.size.width() * 0.5f - dest.size.width()*0.5f + slack.x()) +
//                                  source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth()*0.5f + slack.y());
//                break;
//            case PivotPointPosition::TopCenter:
//                dest.xyLocation = source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth()*0.5f + slack.y());
//                break;
//            case PivotPointPosition::LeftCenter:
//                dest.xyLocation = source.widthNormal * -( source.size.width() * 0.5f + dest.size.width()*0.5f + slack.x());
//                break;
//            case PivotPointPosition::RightCenter:
//                dest.xyLocation = source.widthNormal * ( source.size.width() * 0.5f + dest.size.width()*0.5f + slack.x());
//                break;
//            default:
//                break;
//        }
//        dest.xyLocation += source.xyLocation;
//        dest.heightOffset = _height;
//        dest.rotation = source.rotation;
//        dest.depthNormal = source.depthNormal;
//        dest.widthNormal = source.widthNormal;
//    }

    bool placeAround( RoomBSData* r, FittedFurniture& dest, const FittedFurniture& source, PivotPointPosition where,
                      const V2f& slack, const float _height ) {
        placeAroundInternal( dest, source, where, slack, _height );
        bool completed = RS::addFurniture( r, dest );

        // If the around area is not available, try the opposite direction if there's space
        if ( !completed ) {
            if ( auto newwhere = pivotPointPositionMirrorY( where ); newwhere != PivotPointPosition::Invalid ) {
                placeAroundInternal( dest, source, newwhere, slack, _height );
                completed = RS::addFurniture( r, dest );
            }
        }
        return completed;
    }

    bool placeWallAlong( RoomBSData* r, FittedFurniture& dest, const FittedFurniture& source,
                         const ArchSegment *ls, WallSegmentCorner preferredCorner, const V2f& slack, const float _height ) {
        placeAlongWallInternal( dest, source, ls, preferredCorner, slack, _height );
        return RS::addFurniture( r, dest );
    }
//    bool placeInBetween( RoomBSData* r, const std::vector<FittedFurniture>& fset, PivotPointPosition where,
//                      const V2f& slack, const float _height ) {
//        placeAroundInternal( dest, source, where, slack, _height );
//        bool completed = RS::addFurniture( r, dest );
//
//        // If the around area is not available, try the opposite direction if there's space
//        if ( !completed ) {
//            if ( auto newwhere = pivotPointPositionMirrorY( where ); newwhere != PivotPointPosition::Invalid ) {
//                placeAroundInternal( dest, source, newwhere, slack, _height );
//                completed = RS::addFurniture( r, dest );
//            }
//        }
//        return completed;
//    }

    bool placeWallCorner( RoomBSData* r, FittedFurniture& _ff, const ArchSegment *ls,
                          const V2f& slack, WallSegmentCorner wsc, float _height ) {
        if ( !ls ) return false;
        if ( checkBitWiseFlag(ls->tag, WF_IsDoorPart) || checkBitWiseFlag(ls->tag, WF_IsWindowPart) ) return false;

        auto cornerPoint = wsc == WSC_P1 ? ls->p1 : ls->p2;
        Vector2f lpn = normalize( ls->middle - cornerPoint );
        Vector2f offset = ls->normal * (( _ff.size.depth() * 0.5f ) + skirtingDepth(r) + slack.y()) +
                ( lpn * (_ff.size.width() * 0.5f + skirtingDepth(r) + slack.x()) );
        _ff.xyLocation = cornerPoint + offset;
        _ff.heightOffset = _height;
        _ff.rotation = Vector4f{ V3f::UP_AXIS, RoomService::furnitureAngleFromWall( ls ) };
        _ff.widthNormal = ls->crossNormal;
        _ff.depthNormal = ls->normal;

        return RS::addFurniture( r, _ff );
    }

    bool placeWallAligned( RoomBSData* r, FittedFurniture& _ff,
                           const WSLO wslo, uint32_t _exactIndex ) {

        auto ls = getWallSegmentFor( r, wslo, _exactIndex );
        if ( !ls ) return false;
        if ( checkBitWiseFlag(ls->tag, WF_IsDoorPart) || checkBitWiseFlag(ls->tag, WF_IsWindowPart) ) return false;

        Vector2f offset = ls->normal * (( _ff.size.depth() * 0.5f ) + skirtingDepth(r));
        _ff.xyLocation = ls->middle + offset;
        _ff.rotation = Vector4f{ V3f::UP_AXIS, RoomService::furnitureAngleFromWall( ls ) };
        _ff.widthNormal = ls->crossNormal;
        _ff.depthNormal = ls->normal;

        return addFurniture( r, _ff );
    }

    bool placeMiddle( RoomBSData* r, FittedFurniture& _ff, const V2f& _center ) {

        _ff.xyLocation = _center;
        _ff.rotation = V4f{ V3f::UP_AXIS, 0.0f };
        _ff.widthNormal = V2f::X_AXIS;
        _ff.depthNormal = V2f::Y_AXIS;

        return addFurniture( r, _ff );
    }

    bool cplaceMainWith2Sides( RoomBSData* r, FurnitureMapStorage& furns, const FurniturePlacementRule& fpd ) {
        FT main = fpd.getBase(0);
        WSLO refWall = fpd.getWallSegmentId().type;
        bool completed = true;

        auto mainF = furns.spawn( main );
        completed = RS::placeWallAligned( r, mainF, refWall );
        if ( completed ) {
            if ( fpd.hasBase(1) ) {
                completed &= RS::placeAround( r, furns.spawn( fpd.getBase(1)), mainF, PPP::TopLeft );
            }
            if ( fpd.hasBase(2) ) {
                completed &= RS::placeAround( r, furns.spawn( fpd.getBase(2) ), mainF, PPP::TopRight );
            }
        }
        for ( const auto& dec : fpd.getDecorations() ) {
            auto decF = furns.spawn( dec );
            float h1 = middleHeightFromObject( r, mainF, decF );
            completed &= h1 >= 0.0f;
            if ( h1 >= 0.0f ) {
                float h =  mainF.size.y() + h1;
                completed &= RS::placeAround( r,decF, mainF, PPP::TopCenter, V2f::ZERO, h);
            }
        }
        return completed;
    }

    bool cplaceSetAlignedAtCorner( RoomBSData* r, FurnitureMapStorage& furns, const FurniturePlacementRule& fpd ) {
        const std::vector<FT>& fset = fpd.getBases();
        WSLO refWall = fpd.getWallSegmentId().type;

        bool completed = true;

        auto ls = getWallSegmentFor( r, refWall, fpd.getWallSegmentId().index );
        if (!ls) return false;
        if ( checkBitWiseFlag(ls->tag, WF_IsDoorPart) || checkBitWiseFlag(ls->tag, WF_IsWindowPart) ) return false;
        auto prevFurn = furns.spawn(fset.front());
        completed = RS::placeWallCorner( r, prevFurn, ls, fpd.getSlack().xy(), fpd.getPreferredCorner() );
        if ( !completed ) return false;
        for ( size_t t = 1; t < fset.size(); t++ ) {
            auto currFurn = furns.spawn( fset[t] );
            completed &= RS::placeWallAlong( r, currFurn, prevFurn, ls, fpd.getPreferredCorner(), fpd.getSlack(t).xy() );
            if ( !completed ) break;
            prevFurn = currFurn;
        }
        return completed;
    }

    bool cplaceCornerWithDec( RoomBSData* r, FurnitureMapStorage& furns, const FurniturePlacementRule& fpd ) {
        FT main = fpd.getBase(0);
        WSLO refWall = fpd.getWallSegmentId().type;

        bool completed = true;
        auto mainF = furns.spawn( main );
        auto ls = getWallSegmentFor( r, refWall, fpd.getWallSegmentId().index );
        completed &= RS::placeWallCorner( r,mainF, ls, fpd.getSlack().xy(), WSC_P2 );
        if ( !completed ) return false;

        if ( fpd.hasDecorations() ) {
            const std::vector<FT>& decorations = fpd.getDecorations();
            auto dec = furns.spawn( decorations.front() );
            float h1 = middleHeightFromObject( r, mainF, dec );
            completed &= h1 >= 0.0f;
            if ( h1 >= 0.0f ) {
                float h = mainF.size.y() + h1;
                completed &= RS::placeWallCorner( r, dec, ls, fpd.getSlack().xy(), WSC_P2, h );
            }
        } else {
            completed = false;
        }
        return completed;
    }

    bool cplaceMiddleOfRoom( RoomBSData *r, FurnitureMapStorage& furns, const FurniturePlacementRule& fpd ) {
        auto center = RS::maxEnclsingBoundingBoxCenter( r );

        return placeMiddle( r, furns.spawn(fpd.getBase(0)), center );
    }

    bool runRuleScript( RoomBSData* r, FurnitureMapStorage& furns, const FurnitureRuleScript& fs ) {
        return fs.execute( r, furns, RS::functionRules );
    }

    ClipperLib::Path V2fToPath( const std::vector<Vector2f>& _values ) {
        ClipperLib::Path ret;
        for ( auto& p : _values ) {
            ret << p;
        }
        return ret;
    }

    bool addFurniture( RoomBSData* r, FittedFurniture& _ff ) {
        _ff.position3d = XZY::C( _ff.xyLocation, _ff.heightOffset );
        _ff.bbox3d = AABB{ _ff.position3d - (_ff.size * 0.5f), _ff.position3d + (_ff.size * 0.5f) };
        _ff.bbox3d = _ff.bbox3d.rotate( _ff.rotation );

        ClipperLib::Clipper c;
        ClipperLib::Paths solution;
        c.AddPath( V2fToPath(_ff.bbox3d.topDown().points()), ClipperLib::ptClip, true );
        c.AddPath( V2fToPath(r->mPerimeterSegments), ClipperLib::ptSubject, true );
        c.Execute( ClipperLib::ctDifference, solution, ClipperLib::pftNonZero, ClipperLib::pftNonZero );

        if ( solution.size() == 2 ) {
            if ( !_ff.checkIf(FF_CanOverlap) ) {
                for ( const auto& furn : r->mFittedFurniture ) {
                    if ( _ff.heightOffset != furn.heightOffset ) {
                        if ( _ff.heightOffset > furn.heightOffset + furn.bbox3d.calcHeight() ||
                             _ff.heightOffset + _ff.bbox3d.calcHeight() < furn.heightOffset ) {
                            continue;
                        }
                    }
                    if ( _ff.bbox3d.topDown().intersect( furn.bbox3d.topDown(), 0.001f )) {
                        return false;
                    }
                }
            }
            r->mFittedFurniture.push_back( _ff );
            return true;
        }
        return false;
    }

    void clearFurniture( RoomBSData* r ) {
        r->mFittedFurniture.clear();
    }

    void furnishBedroom( RoomBSData* r ) {
//        FittedFurniture bed{ std::string("bed"), { 1.52f, 1.98f, 0.5f } };
//        placeWallAligned( r, bed, WallSegmentLenghtOrder::SecondLongest );
//        addFurniture( r, bed );
    }

    void furnishLiving( RoomBSData* r ) {
//        FittedFurniture sofa{ std::string("sofa"), { 2.50f, 1.0f, 0.5f } };
//        placeWallAligned( r, sofa, WallSegmentLenghtOrder::Longest );
//
//        FittedFurniture tvStand{ std::string("sideboard"), { 1.50f, 0.55f, 0.5f } };
//        placeWallAligned( r, tvStand, WallSegmentLenghtOrder::SecondLongest );
//
//        r->mFittedFurniture.push_back( sofa );
//        r->mFittedFurniture.push_back( tvStand );
    }

    void furnish( RoomBSData* r ) {
        switch ( r->asType ) {
            case ASType::GenericRoom:
                break;
            case ASType::BedroomDouble:
            case ASType::BedroomMaster:
            case ASType::BedroomSingle:
                furnishBedroom(r);
                break;
            case ASType::LivingRoom:
            case ASType::Studio:
                furnishLiving(r);
                break;
        }
    }

}

FittedFurniture& FurnitureMapStorage::spawn( FT _ft ) {
    auto f = index[_ft];
    storage.emplace( _ft, f );

    auto range = storage.equal_range(_ft );
    auto i = range.first;
    auto ret = range.first;
    for (; i != range.second; ++i) {
        ret = i;
    }
    return ret->second;
}

void initializeDefaultFurnituresFlags( FT _ft, FittedFurniture& _ff ) {
    if ( _ft == FTH::Carpet() ) {
        orBitWiseFlag( _ff.flags, FF_CanOverlap );
    }
    if ( _ft == FTH::Picture() ) {
        orBitWiseFlag( _ff.flags, FF_CanBeHanged | FF_isDecoration );
    }
}

void FurnitureMapStorage::addIndex( FT _ft, FittedFurniture& _ff ) {
    initializeDefaultFurnituresFlags( _ft, _ff );
    index.emplace( _ft, _ff );
}


void FurnitureMapStorage::addIndex( FT _ft, const std::string& _name, const std::string& _symbolRef ) {
    auto fpair = sg.getGeomNameSize( _name );
    auto ff = FittedFurniture{fpair, _symbolRef};
    this->addIndex( _ft, ff );
}

FurniturePlacementRule FurniturePlacementRule::randomGeneration() {
    FurniturePlacementRule ret{ unitRandI( static_cast<int>(RS::functionRules.size()) - 1) };

    ret.addBase( FTH::at(unitRandI(FTH::count()-1)) );
    ret.addBase( FTH::at(unitRandI(FTH::count()-1)) );
    ret.addBase( FTH::at(unitRandI(FTH::count()-1)) );
    ret.setWallSegmentId( {WSLOH::at(unitRandI(WSLOH::count()-1)), 0} );
    ret.setPreferredCorner( WallSegmentCorner(unitRandI(2)) );

    return ret;
}
