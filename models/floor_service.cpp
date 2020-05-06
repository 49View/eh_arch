//
//  floor_service.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "floor_service.hpp"
#include <set>
#include <core/math/poly_utils.hpp>
#include <ostream>
#include <core/math/triangulator.hpp>

#include "room_service.hpp"
#include "wall_service.hpp"
#include "twoushapes_service.hpp"
#include "arch_segment_service.hpp"
#include "window_service.hpp"
#include "door_service.hpp"
#include "arch_structural_service.hpp"

std::vector<DebugUShapeIntersection> FloorServiceIntermediateData::usi;
std::vector<ArchSegment> FloorServiceIntermediateData::wsg;
std::vector<ArchSegment> FloorServiceIntermediateData::wsge;
std::vector<std::pair<V2f, V2f>> FloorServiceIntermediateData::rcus;

void FloorService::addWallsFromData( FloorBSData *f, const V2fVectorOfVector& floorWalls, WallLastPointWrapT wpw ) {
    for ( const auto& fw : floorWalls ) {
        auto w = WallService::createWall2( fw,
                                           f->height,
                                           wpw,
                                           0.0f,
                                           WallFlags::WF_HasSkirting | WallFlags::WF_HasCoving );

        f->walls.push_back( w );
    }
    calcBBox( f );
}

void FloorService::addRoomsFromData( FloorBSData *f ) {
    // finally insert in floor
    for ( auto& r : f->rds ) {
        auto newRoom = RoomService::createRoom( r, f->height, f->z );
        RoomService::updateFromArchSegments( newRoom.get(), r.wallSegmentsInternal );
        f->rooms.push_back( newRoom );
    }
}

void FloorService::updateFromNewDoorOrWindow( FloorBSData *f ) {

    std::pair<int, std::shared_ptr<RoomBSData>> rwsMapping;
    FloorService::roomRecognition( f );
    //	f->rooms.clear();
    //	FloorService::addRoomsFromData( f, rws );
    for ( auto& room : f->rooms ) {
        float minDistanceError = std::numeric_limits<float>::max();
        int ri = 0;
        for ( auto& rw : f->rds ) {
            float distTL = distance( rw.bboxInternal.topLeft(), room->bbox.topLeft());
            float distBR = distance( rw.bboxInternal.bottomRight(), room->bbox.bottomRight());
            float td = distTL + distBR;
            if ( td < minDistanceError ) {
                rwsMapping = { ri, room };
                minDistanceError = td;
            }
            ++ri;
        }

        float lPerimeter = 0.0;
        for ( auto& rwsp : f->rds[rwsMapping.first].wallSegmentsInternal ) {
            int csize = static_cast<int>( rwsp.size());
            for ( int q = 0; q < csize; q++ ) {
                lPerimeter += ArchSegmentService::length( rwsp[q] );
            }
        }

        //		if ( rwsMapping.second->WallSegments().size() != rws[rwsMapping.first].wallSegments.size() ) {
        if ( !isScalarEqual( rwsMapping.second->mPerimeter, lPerimeter, 0.01f )) {
            RoomService::updateFromArchSegments( rwsMapping.second.get(),
                                                 f->rds[rwsMapping.first].wallSegmentsInternal );
        }
    }
}

void FloorService::addDoorFromData( FloorBSData *f, float _doorHeight, const UShape& w1, const UShape& w2,
                                    ArchSubTypeT /*st*/ /*= ArchSubType::NotApplicable */ ) {

    std::shared_ptr<DoorBSData> d1 = DoorService::createDoor( _doorHeight, f->height, w1, w2, 0.1f );
    f->doors.push_back( d1 );

//	auto wd = WallService::createWall( TwoUShapesBasedService::createWallVertices( d1.get() ), f->height - _doorHeight,
//									   WallLastPointWrap::Yes, _doorHeight, WallFlags::WF_HasCoving |
//											   WallFlags::WF_IsDoorPart );

    f->walls.push_back( WallService::createWall2( TwoUShapesBasedService::createFrontWallVertices2( d1.get()),
                                                  f->height - _doorHeight,
                                                  WallLastPointWrap::No, _doorHeight, WallFlags::WF_HasCoving |
                                                                                      WallFlags::WF_IsDoorPart,
                                                  d1->hash ));

    f->walls.push_back( WallService::createWall2( TwoUShapesBasedService::createFrontWallVertices3( d1.get()),
                                                  f->height - _doorHeight,
                                                  WallLastPointWrap::No, _doorHeight, WallFlags::WF_HasCoving |
                                                                                      WallFlags::WF_IsDoorPart,
                                                  d1->hash ));

}

void
FloorService::addWindowFromData( FloorBSData *f, float _windowHeight, float _defaultWindowBaseOffset, const UShape& w1,
                                 const UShape& w2 ) {

    std::shared_ptr<WindowBSData> d1 = WindowService::createWindow( _windowHeight, f->height, _defaultWindowBaseOffset,
                                                                    w1, w2 );
    f->windows.push_back( d1 );

    auto wd1 = WallService::createWall2( TwoUShapesBasedService::createFrontWallVertices2( d1.get()), d1->baseOffset,
                                         WallLastPointWrap::No, 0.0f,
                                         WallFlags::WF_HasSkirting | WallFlags::WF_IsWindowPart, d1->hash, 0 );
    auto wd2 = WallService::createWall2( TwoUShapesBasedService::createFrontWallVertices2(
            d1.get()), f->height - ( d1->baseOffset + _windowHeight ), WallLastPointWrap::No,
                                         d1->baseOffset + _windowHeight,
                                         WallFlags::WF_HasCoving | WallFlags::WF_IsWindowPart, d1->hash, 1 );

    auto wd3 = WallService::createWall2( TwoUShapesBasedService::createFrontWallVertices3( d1.get()), d1->baseOffset,
                                         WallLastPointWrap::No, 0.0f,
                                         WallFlags::WF_HasSkirting | WallFlags::WF_IsWindowPart, d1->hash, 0 );
    auto wd4 = WallService::createWall2( TwoUShapesBasedService::createFrontWallVertices3( d1.get()),
                                         f->height - ( d1->baseOffset + _windowHeight ), WallLastPointWrap::No,
                                         d1->baseOffset + _windowHeight,
                                         WallFlags::WF_HasCoving | WallFlags::WF_IsWindowPart, d1->hash, 1 );
    f->walls.push_back( wd1 );
    f->walls.push_back( wd2 );
    f->walls.push_back( wd3 );
    f->walls.push_back( wd4 );
}

std::vector<UShape *> FloorService::allUShapes( FloorBSData *f ) {
    std::vector<UShape *> ret;
    for ( auto& w : f->walls ) {
        for ( auto& us : w->mUShapes ) ret.push_back( &us );
    }
    return ret;
}

bool FloorService::checkTwoUShapesDoNotIntersectAnything( FloorBSData *f, UShape *s1, UShape *s2 ) {
    typedef std::pair<Vector2f, Vector2f> line;

    std::vector<line> allLines;
    for ( const auto& w : f->walls ) {
        auto p1 = lerp( 0.5f, s1->middle, s2->middle );
        if ( ArchStructuralService::isPointInside( w.get(), p1 )) {
            FloorServiceIntermediateData::USI().emplace_back( s1, s2, p1, 1 );
            return false;
        }
        auto p2 = s1->middle + s1->crossNormals[0] * 0.01f;
        if ( ArchStructuralService::isPointInside( w.get(), p2 )) {
            LOGRS( "Beccato 3" );
            FloorServiceIntermediateData::USI().emplace_back( s1, s2, p2, 2 );
            return false;
        }
        auto p3 = s2->middle + s2->crossNormals[0] * 0.01f;
        if ( ArchStructuralService::isPointInside( w.get(), p3 )) {
            LOGRS( "Beccato 2" );
            FloorServiceIntermediateData::USI().emplace_back( s1, s2, p3, 3 );
            return false;
        }
        int wepSize = static_cast<int>( w->epoints.size());
        for ( auto i = 0; i < wepSize; i++ ) {
            allLines.emplace_back( w->epoints[cai( i, wepSize )], w->epoints[cai( i + 1, wepSize )] );
        }
    }
    Vector2f r;
    Vector2f start = s1->middle + ( normalize( s2->middle - s1->middle ) * 0.01f );
    Vector2f end = s2->middle + ( normalize( s1->middle - s2->middle ) * 0.01f );
    for ( auto& l : allLines ) {
        if ( intersection( start, end, l.first, l.second, r )) {
            return false;
        }
    }

    return true;
}

std::vector<std::pair<UShape *, UShape *>> FloorService::alignSuitableUShapesFromWalls( FloorBSData *f ) {
    Vector2f r{};
    std::vector<UShape *> allUShapes = FloorService::allUShapes( f );
    std::vector<std::pair<UShape *, UShape *>> shapePairs;

    for ( size_t t = 0; t < allUShapes.size(); t++ ) {
        UShape *s1 = allUShapes[t];
        UShape *s3 = nullptr;
        if ( s1->type == ArchType::WallT ) {
            bool bFound = false;
            float minDist = 1e10;
            int64_t foundIndex = -1;
            for ( size_t m = 0; m < allUShapes.size(); m++ ) {
                UShape *s2 = allUShapes[m];
                if ( t != m && ( s2->type == ArchType::WallT )) {
                    float d = dot( s1->crossNormals[0], s2->crossNormals[0] );
                    if ( isScalarEqual( fabs( d ), 1.0f, 0.03f )) {
                        if ( intersection( s1->middle - ( s1->crossNormals[0] * 1000.0f ),
                                           s1->middle + ( s1->crossNormals[0] * 1000.0f ), s2->points[1], s2->points[2],
                                           r )) {
                            // check that the two uShapes are aligned decently, otherwise is a hit with ushapes that are too disjointed
                            bFound = true;
                            float dist = JMATH::distance( s1->middle, r );
                            if ( minDist > dist ) {
                                if ( checkTwoUShapesDoNotIntersectAnything( f, s1, s2 )) {
                                    // Align uShapes
                                    s3 = s2;
                                    UShape *sourceUSRay = s1->width < s3->width ? s1 : s3;
                                    UShape *destUSRay = s1->width < s3->width ? s3 : s1;

                                    Vector2f ri1 = Vector2f::ZERO;
                                    Vector2f ri2 = Vector2f::ZERO;
                                    intersection( sourceUSRay->points[1] + sourceUSRay->crossNormals[0] * 1000.0f,
                                                  sourceUSRay->points[1] + sourceUSRay->crossNormals[1] * 1000.0f,
                                                  destUSRay->points[1] + destUSRay->inwardNormals[0] * 1000.0f,
                                                  destUSRay->points[1] + destUSRay->inwardNormals[1] * 1000.0f, ri1 );
                                    intersection( sourceUSRay->points[2] + sourceUSRay->crossNormals[0] * 1000.0f,
                                                  sourceUSRay->points[2] + sourceUSRay->crossNormals[1] * 1000.0f,
                                                  destUSRay->points[1] + destUSRay->inwardNormals[0] * 1000.0f,
                                                  destUSRay->points[1] + destUSRay->inwardNormals[1] * 1000.0f, ri2 );

                                    if ( distance( ri1, destUSRay->points[2] ) < destUSRay->width * 0.25f ||
                                         distance( ri2, destUSRay->points[1] ) < destUSRay->width * 0.25f ) {
                                        if ( isbetween( s1->width / s3->width, 0.8f, 1.25f )) {
                                            minDist = dist;
                                            foundIndex = m;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if ( bFound && foundIndex >= 0 ) {
                s1->mIsPaired = true;
                s3->mIsPaired = true;
                shapePairs.emplace_back( s1, s3 );
            }
        }
    }

    return shapePairs;
}

bool isIndexInUShape( size_t t, WallBSData *w ) {
    int csize = static_cast<int>( w->epoints.size());
    auto p1 = w->epoints[t];
    auto p2 = w->epoints[cai( t + 1, csize )];
    for ( const auto& us : w->mUShapes ) {
        if ( UShapeService::isMaineEdgeEspsilon( p1, p2, us, 0.07f )) {
            return true;
        }
    }
    return false;
}

void
FloorService::externalRaysIntoWalls( FloorBSData *f, std::vector<ArchSegment>& ws, std::vector<ArchSegment>& wse ) {
    float floorRadius = JMATH::max( f->bbox.width(), f->bbox.height());

    std::vector<std::pair<V2f, V2f>> wp{};
    for ( auto& w : f->walls ) {
        int csize = static_cast<int>( w->epoints.size());
        int wrapAmount = w->wrapLastPoint != 0 ? 0 : 1;
        for ( auto t = 0; t < csize - wrapAmount; t++ ) {
            if ( !isIndexInUShape( t, w.get()) && w->sequencePart == 0 ) {
                auto p1 = w->epoints[t];
                auto p2 = w->epoints[cai( t + 1, csize )];
                wp.emplace_back( p1, p2 );
            }
        }
    }
    FloorServiceIntermediateData::RCUnconnectedSegments() = wp;

    Vector2f i = V2f::ZERO;
    int vn = 0;
    for ( auto& w : f->walls ) {
        for ( size_t t = 0; t < w->epoints.size() - ( w->wrapLastPoint != 0 ? 0 : 1 ); t++ ) {
            if ( isIndexInUShape( t, w.get()) || w->sequencePart > 0 ) continue;
            int t1 = cai( t + 1, w->epoints.size());
            V2f middlePoint = WallService::middlePointAt( w.get(), t ).xy();
            Vector2f l1 = middlePoint + ( w->enormals[t] * 0.01f );
            bool bFound = false;
            for ( size_t q = 0; q < 30; q++ ) {
                float a1 = ((float) ( q / 30.0 ) * TWO_PI );
                Vector2f l2 = l1 + ( V2f{ sin( a1 ), cos( a1 ) } * floorRadius * PI<float> );
                for ( const auto& vp : wp ) {
                    bFound = intersection( vp.first, vp.second, l1, l2, i );
                    if ( bFound ) break;
                }
                if ( !bFound ) break;
            }
            auto plasterType = bFound ? GHType::WallPlasterInternal : GHType::WallPlasterExternal;
            orBitWiseFlag( w->slinesGHType[t], plasterType );
            auto as = ArchSegmentService::createArchSegment( f->number, vn, t, w->hash, w->epoints[t], w->epoints[t1],
                                                             middlePoint, w->enormals[t], w->wallFlags,
                                                             w->sequencePart );
            if ( bFound ) ws.push_back( as ); else wse.push_back( as );
            vn++;
        }
        if ( !checkBitWiseFlag( w->slinesGHType[0], GHType::WallPlasterExternal ) && w->linkedHash != 0 ) {
            for ( auto& lh : f->windows ) {
                if ( lh->hash == w->linkedHash ) {
                    lh->insideRoomPointingNormal = w->enormals[0];
                }
            }
        }
    }
}

size_t numTotalSegments( const std::vector<std::vector<ArchSegment>>& _segments ) {
    size_t n = 0;
    for ( auto& rws : _segments ) {
        n += rws.size();
    }
    return n;
}

bool addRoomSegmentsCore( std::vector<ArchSegment>& output, std::vector<ArchSegment>& ws, JMATH::Rect2f& rbbox,
                          size_t firstIndex = 0 ) {
    std::vector<ArchSegment> wsIntern = ws;
    rbbox = { JMATH::Rect2f::INVALID };
    ArchSegment first = *( wsIntern.begin() + firstIndex );
    output.push_back( first );
    Vector2f endPoint = first.p2;
    rbbox.expand( endPoint );
    auto tooLong = 0u;
    auto wssize = wsIntern.size();
    while ( !isVerySimilar( output.back().p2, output.front().p1 ) && tooLong < wssize ) {
        for ( auto it = wsIntern.begin(); it != wsIntern.end(); ++it ) {
            auto w = *it;
            if ( isVerySimilar( w.p1, endPoint, 0.01f )) {
                output.emplace_back( w );
                endPoint = w.p2;
                rbbox.expand( endPoint );
                wsIntern.erase( it );
                break;
            }
        }
        tooLong++;
    }
    erase_if( wsIntern, [&]( const auto& elem ) -> bool {
        return elem == first;
    } );

    // NDDado: not sure it's same to get the minimum size here check because sizes have not been calculated yet.
    //&& rbbox.hasMimimunSize( V2f{ .1f })
    bool isValidRoom = ArchSegmentService::doSegmentsEndsMeet( output ) && output.size() > 2 && !rbbox.isDegenerated();

    if ( isValidRoom ) {
        ws = wsIntern;
    }

    return isValidRoom;
}

bool addRoomSegments( FloorBSData *f, std::vector<ArchSegment>& ws, size_t firstIndex = 0 ) {

    JMATH::Rect2f rbbox{ JMATH::Rect2f::INVALID };
    std::vector<ArchSegment> output{};
    bool isValidRoom = addRoomSegmentsCore( output, ws, rbbox, firstIndex );

    if ( isValidRoom ) {
        std::vector<std::vector<ArchSegment>> allRoomSegments;
        allRoomSegments.push_back( output );
        f->rds.emplace_back( allRoomSegments, rbbox, ASType::GenericRoom );
    }

    return isValidRoom;
//    LOGRS( "Room size: " << output.size() << " Original Segments # " << ws.size());

}

bool addRDSRooms( FloorBSData *f, std::vector<ArchSegment>& ws ) {

    if ( ws.empty()) return false;

    // Add rooms
    size_t incCounter = 0;
    size_t firstIndex = 0;
    size_t tooLong = ws.size();
    while ( incCounter < tooLong && !ws.empty()) {
        addRoomSegments( f, ws, firstIndex );
        firstIndex = tooLong == ws.size() ? firstIndex + 1 : 0;
        tooLong = ws.size();
        ++incCounter;
    }
    return ws.empty();
}

void FloorService::removeUnPairedUShapes( FloorBSData *f ) {
    std::vector<float> uwidths;
    for ( auto& w : f->walls ) {
        for ( const auto& us : w->mUShapes ) {
            if ( !us.mIsPaired ) {
                f->orphanedUShapes.emplace_back( us );
            } uwidths.emplace_back( us.width );
        }
        WallService::removeUnPairedUShapes( w.get());
    }

    if ( !f->orphanedUShapes.empty() ) {
        auto umedian = medianSortSpread( uwidths, 1 );
        erase_if( f->orphanedUShapes, [umedian]( const auto& us ) -> bool {
            return us.width > umedian.second * 1.15f;
        } );
    }
}

float FloorService::updatePerimeter( FloorBSData *f, const std::vector<ArchSegment>& singleRoomSegmentsExternal ) {
    f->mPerimeterSegments.clear();
    float lPerimeter = 0.0;
    if ( !isVerySimilar( singleRoomSegmentsExternal.front().p1, singleRoomSegmentsExternal.back().p2 ))
        f->mPerimeterSegments.push_back( singleRoomSegmentsExternal.front().p1 );
    for ( auto& q : singleRoomSegmentsExternal ) {
        f->mPerimeterSegments.push_back( q.p2 );
        lPerimeter += ArchSegmentService::length( q );
    }
    removeCollinear( f->mPerimeterSegments, 0.001f );

    return lPerimeter;
}

bool FloorService::roomRecognition( FloorBSData *f ) {

    f->rds.clear();

    std::vector<ArchSegment> ws{};
    std::vector<ArchSegment> wse{};

    externalRaysIntoWalls( f, ws, wse );

    if ( ws.empty()) {
        LOGRS( "[ERROR] externalRaysIntoWalls returned zero array" );
        return false;
    }

    FloorServiceIntermediateData::WSGE() = wse;
    FloorServiceIntermediateData::WSG() = ws;

    bool roomsHaveBeenCalculatedCorrectly = addRDSRooms( f, ws );

    if ( roomsHaveBeenCalculatedCorrectly ) {
        // External floor Perimeter
        std::vector<ArchSegment> externalFloorSegments{};
        JMATH::Rect2f rbboxExternal = JMATH::Rect2f::INVALID;
        addRoomSegmentsCore( externalFloorSegments, wse, rbboxExternal );
        FloorService::updatePerimeter( f, externalFloorSegments );
        if ( !ArchSegmentService::doSegmentsEndsMeet( externalFloorSegments )) {
            LOGRS( "[ERROR] Cannot close external perimeter of the house" );
            return false;
        }
    }

    if ( !roomsHaveBeenCalculatedCorrectly ) {
        LOGRS( "[ERROR] Rooms have not been detected" );
    }

    return roomsHaveBeenCalculatedCorrectly;
}

void FloorService::guessFittings( FloorBSData *f ) {
    for ( auto& r : f->rooms ) {
        RoomService::calcOptimalLightingFittingPositions( r.get());
//		RoomService::calcSkirtingSegments( r.get() );
        RoomService::addSocketsAndSwitches( r.get());
        RoomService::furnish( r.get());
    }
}

bool FloorService::findRoom( FloorBSData *f, int _floorNumber, const ASTypeT _roomASTypeToFind,
                             std::shared_ptr<RoomBSData>& ret ) {

    if ( f->number != _floorNumber ) return false;

    for ( const auto& r : f->rooms ) {
        if ( r->asType == _roomASTypeToFind ) {
            ret = r;
            return true;
        }
    }

    return false;
}

void FloorService::removeArch( FloorBSData *f, int64_t hashToRemove ) {
    // First check what type it is, in case we need to do cross-checking with other elements
    ArchStructural *elem = findElementWithHash( f, hashToRemove );
    if ( elem == nullptr ) return;

    if ( ArchStructuralService::typeIsiPoint( elem )) {
    } else if ( ArchStructuralService::typeIsDOW( elem )) {
        erase_if( f->windows, hashToRemove );
        erase_if( f->doors, hashToRemove );
    } else if ( ArchStructuralService::typeIsWall( elem )) {
        erase_if( f->walls, hashToRemove );
    } else {
        erase_if( f->stairs, hashToRemove );
    }
}

void FloorService::swapWindowOrDoor( FloorBSData *f, HouseBSData *h, int64_t hashOfTwoShape ) {
    auto elem = dynamic_cast<TwoUShapesBased *>(FloorService::findElementWithHash( f, hashOfTwoShape ));
    if ( elem ) {
        FloorService::removeLinkedArch( f, hashOfTwoShape );
        FloorService::removeArch( f, hashOfTwoShape );
        if ( elem->asType == ArchType::WindowT ) {
            FloorService::addWindowFromData( f, h->defaultWindowHeight, h->defaultWindowBaseOffset, elem->us1,
                                             elem->us2 );
        } else {
            FloorService::addWindowFromData( f, h->defaultWindowHeight, h->defaultWindowBaseOffset, elem->us1,
                                             elem->us2 );
        }
    }
}

void FloorService::changeUShapeType( FloorBSData *f, const UShape& sourceUShape1, const UShape& sourceUShape2,
                                     ArchType _type ) {
    for ( auto& wall : f->walls ) {
        for ( auto& us : wall->mUShapes ) {
            if ( UShapeService::doesShareMaineEdge( us, sourceUShape1 ) ||
                 UShapeService::doesShareMaineEdge( us, sourceUShape2 )) {
                us.type = _type;
            }
        }
    }
}

void
FloorService::changeTypeOfSelectedElementTo( FloorBSData *f, ArchStructural *source, ArchType t, ArchSubTypeT st ) {
    ArchStructural *s = findElementWithHash( f, source->hash );

    if ( s ) {
        std::vector<UShape *> allUShapes = FloorService::allUShapes( f );
        TwoUShapesBased *w = dynamic_cast<TwoUShapesBased *>( source );
        switch ( t ) {
            case ArchType::WindowT: {
                addWindowFromData( f, f->windowHeight, f->windowBaseOffset, w->us1, w->us2 );
                break;
            }
            case ArchType::DoorT: {
                addDoorFromData( f, f->doorHeight, w->us1, w->us2, st );
                break;
            }
            default:
                break;
        }

        changeUShapeType( f, w->us1, w->us2, t );
        removeLinkedArch( f, source->hash );
        removeArch( f, source->hash );

        updateFromNewDoorOrWindow( f );
    }
}

void FloorService::rescale( FloorBSData *f, float _scale ) {
    ArchStructuralService::rescale( f, _scale );

    for ( auto& vv : f->ceilingContours ) {
        for ( auto& v : vv ) {
            v *= { _scale, _scale, 1.0f };
        }
    }
    for ( auto& v : f->mPerimeterSegments ) {
        v *= _scale;
    }

    for ( auto& i : f->windows ) WindowService::rescale( i.get(), _scale );
    for ( auto& i : f->doors ) DoorService::rescale( i.get(), _scale );
    for ( auto& i : f->walls ) WallService::rescale( i.get(), _scale );
    for ( auto& i : f->rooms ) RoomService::rescale( i.get(), _scale );
    for ( auto& usg : f->orphanedUShapes ) {
        usg.middle *= _scale;
    }

    //	for ( auto&& i : stairs ) i->rescale();

    calcBBox( f );
}

void FloorService::setCoving( FloorBSData *f, bool _state ) {
    f->hasCoving = _state;
    for ( auto& r : f->rooms ) {
        RoomService::setCoving( r.get(), _state );
    }
}

void FloorService::addCeilingContour( FloorBSData *f, const std::vector<Vector3f>& cc ) {
    f->ceilingContours.push_back( cc );
}

std::string FloorService::naturalLanguageFloorNumber( int numFloor ) {
    if ( numFloor < 5 ) {
        if ( numFloor == 0 ) return "Ground Floor";
        if ( numFloor == 1 ) return "First Floor";
        if ( numFloor == 2 ) return "Second Floor";
        if ( numFloor == 3 ) return "Third Floor";
        if ( numFloor == 4 ) return "Fourth Floor";
    }

    return std::to_string( numFloor ) + "th Floor";
}

//void assignStairsToFloor( const JMATH::Rect2f& stairsRect );
//void FloorService::assignStairsToFloor( const JMATH::Rect2f& stairsRect ) {
//	if ( bbox.contains( stairsRect ) ) {
//		// Get the bbox for the stairs
//		std::shared_ptr<StairsData> stair = std::make_shared<StairsData>( stairsRect, Vector3f( 90.0f, height, 20.0f ), number );
//		stairs.push_back( stair );
//	}
//}

// Ceilings

bool FloorService::isInsideCeilingContour( const FloorBSData *f, const Vector2f& v1, float& topZ1, int& hitLevel1 ) {

    bool bFoundAtLeastOneContour = false;
    for ( auto level = 0u; level < f->ceilingContours.size(); level++ ) {
        auto& vv = f->ceilingContours[level];
        Rect2f r = Rect2f::INVALID;
        for ( auto& v : vv ) r.expand( v.xy());
        if ( r.contains( v1 )) {
            topZ1 = vv[0].z();
            hitLevel1 = level + 1;
            bFoundAtLeastOneContour = true;
        }
    }

    return bFoundAtLeastOneContour;
}

std::vector<Vector2f> FloorService::allFloorePoints( const FloorBSData *f ) {
    std::vector<Vector2f> ret;
    for ( auto& w : f->walls ) {
        for ( const auto& data : w->epoints ) {
            ret.push_back( data );
        }
    }
    return ret;
}

bool FloorService::intersectLine2d( const FloorBSData *f, Vector2f const& p0, Vector2f const& p1, Vector2f& i ) {
    bool ret = false;
    if ( f->bbox.lineIntersection( p0, p1 )) {
        for ( const auto& w : f->walls ) {
            if ( WallService::intersectLine2d( w.get(), p0, p1, i )) return true;
        }
        for ( const auto& w : f->windows ) {
            if ( TwoUShapesBasedService::intersectLine2d( w.get(), p0, p1, i )) return true;
        }
        for ( const auto& w : f->doors ) {
            if ( TwoUShapesBasedService::intersectLine2d( w.get(), p0, p1, i )) return true;
        }
        //for ( auto& w : f->stairs ) {
        //	if ( StairsService::intersectLine2d( w, p0, p1, i ) ) return true;
        //}
    }
    return ret;
}

ArchIntersection
FloorService::intersectLine2dMin( const FloorBSData *f, Vector2f const& p0, Vector2f const& p1, Vector2f& i,
                                  uint32_t filterFlags ) {
    ArchIntersection ret{};
    float minDist = std::numeric_limits<float>::max();
    if ( f->bbox.lineIntersection( p0, p1 )) {
        for ( const auto& w : f->walls ) {
            WallService::intersectLine2dMin( w.get(), p0, p1, i, minDist, ret, filterFlags );
        }
    }
    if ( ret.hit ) {
        V2f i1 = lerp( 0.5f, i, p0 );
        for ( const auto& w : f->rooms ) {
            ret.hit &= !ArchStructuralService::isPointInside( w.get(), i1 );
        }
    }
    return ret;
}

bool FloorService::isInsideRoomRDS( const V2f& i, const std::vector<RoomPreData>& rds ) {

    V2fVector vlist;
    for ( const auto& rd : rds ) {
        for ( const auto& as : rd.wallSegmentsInternal ) {
            V2fVector rpoints;
            rpoints.emplace_back( as.front().p1 );
            for ( const auto& s : as ) {
                rpoints.push_back(s.p2);
            }
            Triangulator tri( rpoints, 0.000001f );
            auto triangles = tri.get2dTrianglesTuple();

            for ( auto& t : triangles ) {
                if ( isInsideTriangle( i, std::get<0>( t ), std::get<1>( t ), std::get<2>( t ) ) ) {
                    return true;
                }
            }
        }
    }
    return false;
}

void FloorService::calcBBox( FloorBSData *f ) {
    f->bbox = Rect2f::INVALID;
    std::vector<Vector2f> ret = allFloorePoints( f );
    for ( auto& v : ret ) {
        f->bbox.expand( v );
    }
    f->bbox3d.calc( f->bbox, f->height, Matrix4f( { 0.0f, 0.0f, f->z } ));

    //offsetFromFloorAnchor = mHouse->getFirstFloorAnchor();
    //offsetFromFloorAnchor -= ( bbox.topLeft() + bbox.size()*0.5f );
    //offsetFromFloorAnchor3d = Vector3f( offsetFromFloorAnchor, z );
}

void FloorService::removeLinkedArch( FloorBSData *f, int64_t hashToRemove ) {
    if ( hashToRemove == 0 ) return;
    // First check what type it is, in case we need to do cross-checking with other elements
    auto elems = findElementWithLinkedHash( f, hashToRemove );
    if ( elems.empty()) return;

    for ( auto& e : elems ) {
        erase_if( f->walls, e->hash );
    }
}

void FloorService::removeWalls( FloorBSData *f ) {
    f->walls.clear();
}

void FloorService::removeWalls( FloorBSData *f, float wwidth ) {
    f->walls.erase( remove_if( f->walls.begin(), f->walls.end(),
                               [wwidth]( auto const& us ) -> bool { return us->width == wwidth; } ), f->walls.end());
}

float FloorService::area( const FloorBSData *f ) {
    float ret = 0.0f;

    for ( const auto& w : f->rooms ) {
        ret += RoomService::area( w.get());
    }

    return ret;
}

ArchStructural *FloorService::findElementWithHash( const FloorBSData *f, int64_t _hash ) {
    for ( auto& i : f->windows ) if ( i->hash == _hash ) return i.get();
    for ( auto& i : f->doors ) if ( i->hash == _hash ) return i.get();
    for ( auto& i : f->walls ) if ( i->hash == _hash ) return i.get();
    for ( auto&& i : f->stairs ) if ( i->hash == _hash ) return i.get();

    return nullptr;
}

std::vector<ArchStructural *> FloorService::findElementWithLinkedHash( const FloorBSData *f, int64_t _hash ) {
    std::vector<ArchStructural *> ret;

    for ( auto& i : f->windows ) {
        if ( i->linkedHash == _hash ) ret.push_back( i.get());
    }
    for ( auto& i : f->doors ) {
        if ( i->linkedHash == _hash ) ret.push_back( i.get());
    }
    for ( auto& i : f->walls ) {
        if ( i->linkedHash == _hash ) ret.push_back( i.get());
    }
    for ( auto&& i : f->stairs ) {
        if ( i->linkedHash == _hash ) ret.push_back( i.get());
    }

    return ret;
}

bool FloorService::findWallAt( const FloorBSData *f, const Vector2f& matPos, std::vector<ArchStructural *>& ret ) {
    bool hasBeenFound = false;

    for ( const auto& w : f->walls ) {
        if ( WallService::contains( w.get(), matPos )) {
            ret.push_back( w.get());
            hasBeenFound = true;
        }
    }
    return hasBeenFound;
}

bool FloorService::findRoomAt( const FloorBSData *f, const Vector2f& matPos, std::vector<ArchStructural *>& ret ) {
    bool hasBeenFound = false;

    for ( const auto& w : f->rooms ) {
        if ( ArchStructuralService::isPointInside( w.get(), matPos )) {
            ret.push_back( w.get());
            hasBeenFound = true;
        }
    }
    return hasBeenFound;
}

bool FloorService::whichRoomAmI( const FloorBSData *f, const Vector2f& _pos, std::shared_ptr<RoomBSData>& outRoom ) {
    for ( const auto& r : f->rooms ) {
        bool isInHere = ArchStructuralService::isPointInside( r.get(), _pos );
        if ( isInHere ) {
            outRoom = r;
            return true;
        }
    }
    return false;
}

int64_t FloorService::findWallIndex( const FloorBSData *f, int64_t _hash ) {
    int64_t c = 0;
    for ( auto& w : f->walls ) {
        if ( w->hash == _hash ) return c;
        c++;
    }
    return -1;
}

void FloorService::centrePointOfBiggestRoom( const FloorBSData *f, float& _currMaxArea,
                                             Vector2f& _currCenter ) {
    for ( const auto& r : f->rooms ) {
        float roomArea = RoomService::area( r.get());
        if ( roomArea > _currMaxArea ) {
            _currMaxArea = roomArea;
            _currCenter = RoomService::maxEnclsingBoundingBoxCenter( r.get());
        }
    }

}

ClipperLib::Paths FloorService::calcPlainPath( const FloorBSData *f ) {
    ClipperLib::Paths ret;

    ClipperLib::Path bboxPointsTop;
    std::vector<Vector2f> bp = f->bbox.points();
    bboxPointsTop << bp[0] << bp[1] << bp[2] << bp[3];
    ret.push_back( bboxPointsTop );
    ClipperLib::Path bboxPointsTop2;
    bboxPointsTop2 << bp[0] << bp[1] << bp[2] << bp[3];
    ret.push_back( bboxPointsTop2 );

    return ret;
}

void FloorService::clearFurniture( FloorBSData *f ) {
    for ( const auto& w : f->rooms ) {
        RS::clearFurniture( w.get());
    }
}

// Rollbacks

void FloorService::rollbackToCalculatedWalls( FloorBSData *f ) {
    for ( auto& w : f->walls ) {
        WallService::update( w.get() );
    }
    f->rooms.clear();
    f->rds.clear();
    f->windows.clear();
    f->doors.clear();
    f->stairs.clear();
    f->orphanedUShapes.clear();
}
