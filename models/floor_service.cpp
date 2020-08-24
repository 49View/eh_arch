//
//  floor_service.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "floor_service.hpp"
#include <set>
#include <ostream>
#include <core/math/rect2f.h>
#include <core/util.h>
#include <core/util_follower.hpp>
#include <core/math/triangulator.hpp>
#include <poly/polyclipping/clipper.hpp>
#include <core/math/plane3f.h>

#include "room_service.hpp"
#include "wall_service.hpp"
#include "twoushapes_service.hpp"
#include "arch_segment_service.hpp"
#include "door_service.hpp"
#include "arch_structural_service.hpp"

std::vector<DebugUShapeIntersection> FloorServiceIntermediateData::usi;
std::vector<ArchSegment> FloorServiceIntermediateData::wsg;
std::vector<ArchSegment> FloorServiceIntermediateData::wsge;
std::vector<std::pair<V2f, V2f>> FloorServiceIntermediateData::rcus;

void FloorService::addWallsFromData( FloorBSData *f, const V2fVectorOfVector& floorWalls, WallLastPointWrapT wpw ) {
    for ( const auto& fw : floorWalls ) {
        auto w = std::make_shared<WallBSData>(fw, f->Height(), wpw, 0.0f,
                                              WallFlags::WF_HasSkirting | WallFlags::WF_HasCoving);
        f->walls.push_back(w);
    }
    f->calcBBox();
}

void FloorService::addRoomsFromData( FloorBSData *f, const HouseBSData *house, const std::vector<RoomPreData>& rds ) {
    // finally insert in floor
    for ( auto& r : rds ) {
        auto newRoom = std::make_shared<RoomBSData>(r, f->Height(), 0.0f);
        RoomService::updateFromArchSegments(newRoom.get(), r.wallSegmentsInternal);
        f->rooms.push_back(newRoom);
    }
}

void FloorService::addOutdoorAreaFromData( FloorBSData *f, const std::shared_ptr<OutdoorAreaBSData>& _outdoorArea ) {
    f->outdoorAreas.emplace_back(_outdoorArea);
    f->calcBBox();
}

void FloorService::updateFromNewDoorOrWindow( FloorBSData *f ) {

    std::pair<int, std::shared_ptr<RoomBSData>> rwsMapping;
    FloorService::roomRecognition(f);
    //	f->rooms.clear();
    //	FloorService::addRoomsFromData( f, rws );
    for ( auto& room : f->rooms ) {
        float minDistanceError = std::numeric_limits<float>::max();
        int ri = 0;
        for ( auto& rw : f->rooms ) {
            float distTL = distance(rw->BBox().topLeft(), room->BBox().topLeft());
            float distBR = distance(rw->BBox().bottomRight(), room->BBox().bottomRight());
            float td = distTL + distBR;
            if ( td < minDistanceError ) {
                rwsMapping = { ri, room };
                minDistanceError = td;
            }
            ++ri;
        }

        float lPerimeter = 0.0;
        for ( auto& rwsp : f->rooms[rwsMapping.first]->mWallSegments ) {
            int csize = static_cast<int>( rwsp.size());
            for ( int q = 0; q < csize; q++ ) {
                lPerimeter += ArchSegmentService::length(rwsp[q]);
            }
        }

        //		if ( rwsMapping.second->WallSegments().size() != rws[rwsMapping.first].wallSegments.size() ) {
        if ( !isScalarEqual(rwsMapping.second->mPerimeter, lPerimeter, 0.01f) ) {
            RoomService::updateFromArchSegments(rwsMapping.second.get(),
                                                f->rooms[rwsMapping.first]->mWallSegments);
        }
    }
}

void FloorService::addDoorFromData( FloorBSData *f, float _doorHeight, const UShape& w1, const UShape& w2,
                                    ArchSubTypeT /*st*/ /*= ArchSubType::NotApplicable */ ) {

    auto d1 = std::make_shared<DoorBSData>(_doorHeight, f->Height(), w1, w2);
    f->doors.push_back(d1);

//	auto wd = WallService::createWall( TwoUShapesBasedService::createWallVertices( d1.get() ), f->Height() - _doorHeight,
//									   WallLastPointWrap::Yes, _doorHeight, WallFlags::WF_HasCoving |
//											   WallFlags::WF_IsDoorPart );

    f->walls.push_back(std::make_shared<WallBSData>(TwoUShapesBasedService::createFrontWallVertices2(d1.get()),
                                                f->Height() - _doorHeight,
                                                WallLastPointWrap::No, _doorHeight, WallFlags::WF_HasCoving |
                                                                                    WallFlags::WF_IsDoorPart,
                                                d1->hash));

    f->walls.push_back(std::make_shared<WallBSData>(TwoUShapesBasedService::createFrontWallVertices3(d1.get()),
                                                f->Height() - _doorHeight,
                                                WallLastPointWrap::No, _doorHeight, WallFlags::WF_HasCoving |
                                                                                    WallFlags::WF_IsDoorPart,
                                                d1->hash));
}

void
FloorService::addWindowFromData( FloorBSData *f, float _windowHeight, float _defaultWindowBaseOffset, const UShape& w1,
                                 const UShape& w2 ) {

    auto d1 = std::make_shared<WindowBSData>(_windowHeight, f->Height(), _defaultWindowBaseOffset, w1, w2);
    f->windows.push_back(d1);

    auto wd1 = std::make_shared<WallBSData>(TwoUShapesBasedService::createFrontWallVertices2(d1.get()), d1->baseOffset,
                                        WallLastPointWrap::No, 0.0f,
                                        WallFlags::WF_HasSkirting | WallFlags::WF_IsWindowPart, d1->hash, 0);
    auto wd2 = std::make_shared<WallBSData>(TwoUShapesBasedService::createFrontWallVertices2(
            d1.get()), f->Height() - ( d1->baseOffset + _windowHeight ), WallLastPointWrap::No,
                                        d1->baseOffset + _windowHeight,
                                        WallFlags::WF_HasCoving | WallFlags::WF_IsWindowPart, d1->hash, 1);

    auto wd3 = std::make_shared<WallBSData>(TwoUShapesBasedService::createFrontWallVertices3(d1.get()), d1->baseOffset,
                                        WallLastPointWrap::No, 0.0f,
                                        WallFlags::WF_HasSkirting | WallFlags::WF_IsWindowPart, d1->hash, 0);
    auto wd4 = std::make_shared<WallBSData>(TwoUShapesBasedService::createFrontWallVertices3(d1.get()),
                                        f->Height() - ( d1->baseOffset + _windowHeight ), WallLastPointWrap::No,
                                        d1->baseOffset + _windowHeight,
                                        WallFlags::WF_HasCoving | WallFlags::WF_IsWindowPart, d1->hash, 1);
    f->walls.push_back(wd1);
    f->walls.push_back(wd2);
    f->walls.push_back(wd3);
    f->walls.push_back(wd4);
}

std::vector<UShape *> FloorService::allUShapes( FloorBSData *f ) {
    std::vector<UShape *> ret{};
    for ( auto& w : f->walls ) {
        for ( auto& us : w->mUShapes ) ret.push_back(&us);
    }
    return ret;
}

bool FloorService::checkTwoUShapesDoNotIntersectAnything( FloorBSData *f, UShape *s1, UShape *s2 ) {
    typedef std::pair<Vector2f, Vector2f> line;

    std::vector<line> allLines;
    for ( const auto& w : f->walls ) {
        auto p1 = lerp(0.5f, s1->middle, s2->middle);
        if ( ArchStructuralService::isPointInside(w.get(), p1) ) {
            FloorServiceIntermediateData::USI().emplace_back(s1, s2, p1, 1);
            return false;
        }
        auto p2 = s1->middle + s1->crossNormals[0] * 0.01f;
        if ( ArchStructuralService::isPointInside(w.get(), p2) ) {
            FloorServiceIntermediateData::USI().emplace_back(s1, s2, p2, 2);
            return false;
        }
        auto p3 = s2->middle + s2->crossNormals[0] * 0.01f;
        if ( ArchStructuralService::isPointInside(w.get(), p3) ) {
            FloorServiceIntermediateData::USI().emplace_back(s1, s2, p3, 3);
            return false;
        }
        int wepSize = static_cast<int>( w->epoints.size());
        for ( auto i = 0; i < wepSize; i++ ) {
            allLines.emplace_back(w->epoints[cai(i, wepSize)], w->epoints[cai(i + 1, wepSize)]);
        }
    }
    Vector2f r{ V3f::ZERO };
    Vector2f start = s1->middle + ( normalize(s2->middle - s1->middle) * 0.01f );
    Vector2f end = s2->middle + ( normalize(s1->middle - s2->middle) * 0.01f );
    for ( auto& l : allLines ) {
        if ( intersection(start, end, l.first, l.second, r) ) {
            return false;
        }
    }

    return true;
}

using VectorOfUShapePairs = std::vector<std::pair<UShape *, UShape *>>;

VectorOfUShapePairs UShapesAlignmentInternal( FloorBSData *f, VectorOfUShapePairs& shapePairs ) {
    Vector2f r{};
    std::vector<UShape *> allUShapes = FloorService::allUShapes(f);
    VectorOfUShapePairs mergeList{};
    shapePairs.clear();

    for ( size_t t = 0; t < allUShapes.size(); t++ ) {
        UShape *s1 = allUShapes[t];
        UShape *s3 = nullptr;
        if ( s1->type == ArchType::WallT ) {
            bool bFound = false;
            float minDist = 1e10;
            int64_t foundIndex = -1;
            for ( size_t m = 0; m < allUShapes.size(); m++ ) {
                UShape *s2 = allUShapes[m];
                if ( t != m && ( s2->type == ArchType::WallT ) ) {
                    float d = dot(s1->crossNormals[0], s2->crossNormals[0]);
                    if ( isScalarEqual(fabs(d), 1.0f, 0.03f) ) {
                        if ( intersection(s1->middle - ( s1->crossNormals[0] * 1000.0f ),
                                          s1->middle + ( s1->crossNormals[0] * 1000.0f ), s2->points[1], s2->points[2],
                                          r) ) {
                            // check that the two uShapes are aligned decently, otherwise is a hit with ushapes that are too disjointed
                            bFound = true;
                            float dist = JMATH::distance(s1->middle, r);
                            // We also check that the distance between the ushapes is grater than at least a quarter of
                            // their width, to avoid havign very narrow doors or windows which 99% is a result of an artifact
                            bool hasMinGap = dist > quarter(s1->width);
                            // if the two twoshapes are too closed together then add them to the merge list
                            if ( !hasMinGap ) {
                                mergeList.emplace_back(s1, s2);
                                break;
                            }
                            if ( minDist > dist ) {
                                if ( FloorService::checkTwoUShapesDoNotIntersectAnything(f, s1, s2) ) {
                                    // Align uShapes
                                    s3 = s2;
                                    UShape *sourceUSRay = s1->width < s3->width ? s1 : s3;
                                    UShape *destUSRay = s1->width < s3->width ? s3 : s1;

                                    Vector2f ri1 = V2fc::ZERO;
                                    Vector2f ri2 = V2fc::ZERO;
                                    intersection(sourceUSRay->points[1] + sourceUSRay->crossNormals[0] * 1000.0f,
                                                 sourceUSRay->points[1] + sourceUSRay->crossNormals[1] * 1000.0f,
                                                 destUSRay->points[1] + destUSRay->inwardNormals[0] * 1000.0f,
                                                 destUSRay->points[1] + destUSRay->inwardNormals[1] * 1000.0f, ri1);
                                    intersection(sourceUSRay->points[2] + sourceUSRay->crossNormals[0] * 1000.0f,
                                                 sourceUSRay->points[2] + sourceUSRay->crossNormals[1] * 1000.0f,
                                                 destUSRay->points[1] + destUSRay->inwardNormals[0] * 1000.0f,
                                                 destUSRay->points[1] + destUSRay->inwardNormals[1] * 1000.0f, ri2);

                                    if ( distance(ri1, destUSRay->points[2]) < destUSRay->width * 0.25f ||
                                         distance(ri2, destUSRay->points[1]) < destUSRay->width * 0.25f ) {
                                        if ( isbetween(s1->width / s3->width, 0.8f, 1.25f) ) {
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
                shapePairs.emplace_back(s1, s3);
            }
        }
    }

    return mergeList;
}

void
checkUShapesProblemsAfterAlignment( FloorBSData *f, VectorOfUShapePairs& shapePairs, VectorOfUShapePairs& mergeList ) {
    // Right, now collect the merge list and merge the two walls together
    struct WM1 {
        WallBSData *w1;
        WallBSData *w2;
        WM1() = default;
        WM1( WallBSData *w1, WallBSData *w2 ) : w1(w1), w2(w2) {}
        bool operator==( const WM1& rhs ) const {
            return w1 == rhs.w1 &&
                   w2 == rhs.w2;
        }
        bool operator!=( const WM1& rhs ) const {
            return !( rhs == *this );
        }
    };

    class WM1HashFunctor {
    public:
        size_t operator()( const WM1& _elem ) const {
            return std::hash<std::string>{}(std::to_string(_elem.w1->hash) + std::to_string(_elem.w2->hash));
        }
    };

    if ( !mergeList.empty() ) {
        std::unordered_set<UShape *> alreadyElaborated{};
        std::unordered_set<WM1, WM1HashFunctor> wallsToMerge;
        for ( const auto& mergePair : mergeList ) {
            auto *s1 = mergePair.first;
            auto *s2 = mergePair.second;

            if ( alreadyElaborated.find(s1) != alreadyElaborated.end() ) continue;
            if ( alreadyElaborated.find(s2) != alreadyElaborated.end() ) continue;

            float dist = half(distance(s1->middle, s2->middle));
            WallBSData *w1 = nullptr;
            WallBSData *w2 = nullptr;
            for ( auto& w: f->walls ) {
                if ( WallService::hasUShape(w.get(), s1) ) {
                    w1 = w.get();
                    if ( w1 && w2 ) break;
                }
                if ( WallService::hasUShape(w.get(), s2) ) {
                    w2 = w.get();
                    if ( w1 && w2 ) break;
                }
            }
            ASSERT(w1 && w2);
            WallService::movePoint(w1, s1->indices[1], w1->epoints[s1->indices[1]] + s1->crossNormals[0] * dist * 1.01f,
                                   false);
            WallService::movePoint(w1, s1->indices[2], w1->epoints[s1->indices[2]] + s1->crossNormals[0] * dist * 1.01f,
                                   false);

            WallService::movePoint(w2, s2->indices[1], w2->epoints[s2->indices[1]] + s2->crossNormals[0] * dist * 1.01f,
                                   false);
            WallService::movePoint(w2, s2->indices[2], w2->epoints[s2->indices[2]] + s2->crossNormals[0] * dist * 1.01f,
                                   false);

            alreadyElaborated.emplace(s1);
            alreadyElaborated.emplace(s2);
            wallsToMerge.emplace(w1, w2);
            LOGRS("I have merged these two " << s1->middle << " " << s2->middle)
        }

        // Now merge those walls together
        std::unordered_set<WallBSData *> wallsToBeDeleted{};
        for ( auto& wm : wallsToMerge ) {
            bool w1deleted = wallsToBeDeleted.find(wm.w1) != wallsToBeDeleted.end();
            bool w2deleted = wallsToBeDeleted.find(wm.w2) != wallsToBeDeleted.end();
            if ( !( w1deleted && w2deleted ) ) {
                auto wMaster = wm.w1;
                auto wSlave = wm.w2;
                if ( w1deleted ) {
                    wMaster = wm.w2;
                    wSlave = wm.w1;
                }

                WallService::mergePoints(wMaster, wSlave->epoints);
                WallService::update(wMaster);
                wallsToBeDeleted.emplace(wSlave);
            }
        }

        for ( const auto& wd : wallsToBeDeleted ) {
            FloorService::removeArch(f, wd->hash);
        }

        // Rinse and repeat ushapes alignments
        UShapesAlignmentInternal(f, shapePairs);
    }
}

std::vector<std::pair<UShape *, UShape *>> FloorService::alignSuitableUShapesFromWalls( FloorBSData *f ) {

    VectorOfUShapePairs shapePairs{};
    VectorOfUShapePairs mergeList = UShapesAlignmentInternal(f, shapePairs);
    checkUShapesProblemsAfterAlignment(f, shapePairs, mergeList);

    return shapePairs;
}

bool FloorService::isIndexInUShape( size_t t, WallBSData *w ) {
    int csize = static_cast<int>( w->epoints.size());
    auto p1 = w->epoints[t];
    auto p2 = w->epoints[cai(t + 1, csize)];
    for ( const auto& us : w->mUShapes ) {
        if ( UShapeService::isMaineEdgeEspsilon(p1, p2, us, 0.005f) ) {
            return true;
        }
    }
    return false;
}

void
FloorService::externalRaysIntoWalls( FloorBSData *f, std::vector<ArchSegment>& ws, std::vector<ArchSegment>& wse ) {
    float floorRadius = JMATH::max(f->BBox().width(), f->BBox().height());

    std::vector<std::pair<V2f, V2f>> wp{};
    for ( const auto& w : f->walls ) {
        int csize = static_cast<int>( w->epoints.size());
        int wrapAmount = w->wrapLastPoint != 0 ? 0 : 1;
        for ( auto t = 0; t < csize - wrapAmount; t++ ) {
            if ( !isIndexInUShape(t, w.get()) && w->sequencePart == 0 ) {
                auto p1 = w->epoints[t];
                auto p2 = w->epoints[cai(t + 1, csize)];
                wp.emplace_back(p1, p2);
            }
        }
    }
    FloorServiceIntermediateData::RCUnconnectedSegments() = wp;

    Vector2f i = V2fc::ZERO;
    int vn = 0;
    for ( const auto& w : f->walls ) {
        for ( size_t t = 0; t < w->epoints.size() - ( w->wrapLastPoint != 0 ? 0 : 1 ); t++ ) {
            if ( isIndexInUShape(t, w.get()) || w->sequencePart > 0 ) continue;
            int t1 = cai(t + 1, w->epoints.size());
            V2f middlePoint = WallService::middlePointAt(w.get(), t).xy();
            Vector2f l1 = middlePoint + ( w->enormals[t] * 0.01f );
            bool bFound = false;
            for ( size_t q = 0; q < 30; q++ ) {
                float a1 = ( (float) ( q / 30.0 ) * TWO_PI );
                Vector2f l2 = l1 + ( V2f{ sin(a1), cos(a1) } * floorRadius * PI<float> );
                for ( const auto& vp : wp ) {
                    bFound = intersection(vp.first, vp.second, l1, l2, i);
                    if ( bFound ) break;
                }
                if ( !bFound ) break;
            }
            auto plasterType = bFound ? GHType::WallPlasterInternal : GHType::WallPlasterExternal;
            orBitWiseFlag(w->slinesGHType[t], plasterType);
            auto as = ArchSegmentService::createArchSegment(f->walls, f->number, vn, t, w->hash, w->epoints[t],
                                                            w->epoints[t1],
                                                            middlePoint, w->enormals[t], w->wallFlags,
                                                            w->sequencePart, w->Elevation(), w->Height());
            if ( bFound ) ws.push_back(as); else wse.push_back(as);
            vn++;
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
    output.push_back(first);
    Vector2f endPoint = first.p2;
    rbbox.expand(endPoint);
    auto tooLong = 0u;
    auto wssize = wsIntern.size();
    while ( !isVerySimilar(output.back().p2, output.front().p1) && tooLong < wssize ) {
        for ( auto it = wsIntern.begin(); it != wsIntern.end(); ++it ) {
            auto w = *it;
            if ( isVerySimilar(w.p1, endPoint, 0.01f) ) {
                output.emplace_back(w);
                endPoint = w.p2;
                rbbox.expand(endPoint);
                wsIntern.erase(it);
                break;
            }
        }
        tooLong++;
    }
    erase_if(wsIntern, [&]( const auto& elem ) -> bool {
        return elem == first;
    });

    // NDDado: not sure it's same to get the minimum size here check because sizes have not been calculated yet.
    //&& rbbox.hasMimimunSize( V2f{ .1f })
    bool isValidRoom = ArchSegmentService::doSegmentsEndsMeet(output) && output.size() > 2 && !rbbox.isDegenerated();

    if ( isValidRoom ) {
        ws = wsIntern;
    }

    return isValidRoom;
}

void addRoomSegments( FloorBSData *f, std::vector<ArchSegment>& ws, RoomPreDataResult& rdsc, size_t firstIndex = 0 ) {

    JMATH::Rect2f rbbox{ JMATH::Rect2f::INVALID };
    std::vector<ArchSegment> output{};
    bool isValidPreRoom = addRoomSegmentsCore(output, ws, rbbox, firstIndex);

    if ( isValidPreRoom ) {
        std::vector<std::vector<ArchSegment>> allRoomSegments;
        allRoomSegments.push_back(output);
        rdsc.rds.emplace_back(allRoomSegments, rbbox, std::vector<ASTypeT>{ ASType::GenericRoom });
    }

//    LOGRS( "Room size: " << output.size() << " Original Segments # " << ws.size());

}

RoomPreDataResult addRDSRooms( FloorBSData *f, std::vector<ArchSegment>& ws ) {

    RoomPreDataResult ret;

    if ( ws.empty() ) {
        return RoomPreDataResult{ false };
    }

    // Add rooms
    size_t incCounter = 0;
    size_t firstIndex = 0;
    size_t tooLong = ws.size();
    while ( incCounter < tooLong && !ws.empty() ) {
        addRoomSegments(f, ws, ret, firstIndex);
        firstIndex = tooLong == ws.size() ? firstIndex + 1 : 0;
        tooLong = ws.size();
        ++incCounter;
    }

    f->orphanedWallSegments = ws;

    ret.isValidPreRoom = ws.empty();
    return ret;
}

void FloorService::removeUnPairedUShapes( FloorBSData *f ) {
    std::vector<float> uwidths;
    for ( auto& w : f->walls ) {
        for ( const auto& us : w->mUShapes ) {
            if ( !us.mIsPaired ) {
                f->orphanedUShapes.emplace_back(us);
            }
            uwidths.emplace_back(us.width);
        }
        WallService::removeUnPairedUShapes(w.get());
    }

    if ( !f->orphanedUShapes.empty() ) {
        auto umedian = medianSortSpread(uwidths, 1);
        erase_if(f->orphanedUShapes, [umedian]( const auto& us ) -> bool {
            return us.width > umedian.second * 1.15f;
        });
    }
}

void FloorService::mergePoints( FloorBSData *f, const V2fVectorOfVector& points, const Rect2f& pointsBBox ) {

    if ( f->BBox().contains(pointsBBox) || f->BBox().intersect(pointsBBox) ) {
        for ( const auto& wallPoints : points ) {
            bool bHasMerged = false;
            for ( auto& w : f->walls ) {
                bHasMerged |= WallService::mergePoints(w.get(), wallPoints);
            }
            if ( !bHasMerged ) {
                addWallsFromData(f, points, WallLastPointWrap::Yes);
            }
        }
    }
}

float FloorService::updatePerimeter( FloorBSData *f, const std::vector<ArchSegment>& singleRoomSegmentsExternal ) {
    f->mPerimeterSegments.clear();
    float lPerimeter = 0.0;
    if ( !isVerySimilar(singleRoomSegmentsExternal.front().p1, singleRoomSegmentsExternal.back().p2) )
        f->mPerimeterSegments.push_back(singleRoomSegmentsExternal.front().p1);
    for ( auto& q : singleRoomSegmentsExternal ) {
        f->mPerimeterSegments.push_back(q.p2);
        lPerimeter += ArchSegmentService::length(q);
    }
    removeCollinear(f->mPerimeterSegments, 0.001f);
    f->perimeterArchSegments = singleRoomSegmentsExternal;

//    for ( auto& ps : f->perimeterArchSegments ) {
//        ps.quads.emplace_back( makeQuadV3f(XZY::C(ps.p1, f->Elevation()), XZY::C(ps.p2, f->Elevation()), f->Height()) );
//    }

    return lPerimeter;
}

RoomPreDataResult FloorService::roomRecognition( FloorBSData *f ) {

    RoomPreDataResult rds{};

    std::vector<ArchSegment> ws{};
    std::vector<ArchSegment> wse{};

    externalRaysIntoWalls(f, ws, wse);

    if ( ws.empty() ) {
        LOGRS("[ERROR] externalRaysIntoWalls returned zero array")
        rds.isValidPreRoom = false;
        return rds;
    }

    FloorServiceIntermediateData::WSGE() = wse;
    FloorServiceIntermediateData::WSG() = ws;

    rds = addRDSRooms(f, ws);

    if ( rds.isValidPreRoom ) {
        // External floor Perimeter
        std::vector<ArchSegment> externalFloorSegments{};
        JMATH::Rect2f rbboxExternal = JMATH::Rect2f::INVALID;
        addRoomSegmentsCore(externalFloorSegments, wse, rbboxExternal);
        FloorService::updatePerimeter(f, externalFloorSegments);
        if ( !ArchSegmentService::doSegmentsEndsMeet(externalFloorSegments) ) {
            LOGRS("[ERROR] Cannot close external perimeter of the house")
            rds.isValidPreRoom = false;
            return rds;
        }
    }

    if ( !rds.isValidPreRoom ) {
        LOGRS("[ERROR] Rooms have not been detected")
    }

    return rds;
}

std::vector<RoomBSData *> FloorService::roomsIntersectingBBox( FloorBSData *f, const Rect2f& bbox, bool earlyOut ) {
    std::vector<RoomBSData *> ret{};
    auto v2p = ClipperLib::V2fToPath(bbox.points());
    for ( auto& r : f->rooms ) {
        ClipperLib::Clipper c;
        ClipperLib::Paths solution;
        c.AddPath(v2p, ClipperLib::ptSubject, true);
        c.AddPath(ClipperLib::V2fToPath(r->mPerimeterSegments), ClipperLib::ptClip, true);
        c.Execute(ClipperLib::ctIntersection, solution, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
        if ( !solution.empty() ) {
            ret.emplace_back(r.get());
            if ( earlyOut ) break;
        }
    }
    return ret;
}

void FloorService::reevaluateDoorsAndWindowsAfterRoomChange( FloorBSData *f ) {
    for ( auto& d : f->doors ) {
        DoorService::reevaluate(d.get(), f);
    }
}

void FloorService::assignRoomTypeFromBeingClever( FloorBSData *f, HouseBSData *house ) {
    int numberOfGenericRoom = 0;
    int numberOfKitchenOrLivingRooms = 0;
    int numberOfBathrooms = 0;

    for ( auto& room : f->rooms ) {
        auto *r = room.get();

        // Bathrooms
        // Count number of doors and windows a room has, if it has 1 door and no windows... Probably it's a bathroom!
        if ( RoomService::hasRoomType(r, ASType::GenericRoom) && r->doors.size() == 1 && r->windows.empty() ) {
            RoomService::setRoomType(r, ASType::Bathroom);
        }

        // Hallways
        // Guess hallways by checking if a room has no windows and more than 2 doors.
        if ( RoomService::hasRoomType(r, ASType::GenericRoom) && r->doors.size() > 2 && r->windows.empty() ) {
            RoomService::setRoomType(r, ASType::Hallway);
        }

        if ( RoomService::hasRoomType(r, ASType::GenericRoom) ) numberOfGenericRoom++;
        if ( RoomService::hasRoomType(r, ASType::LivingRoom) || RoomService::hasRoomType(r, ASType::Kitchen) ) numberOfKitchenOrLivingRooms++;
        if ( RoomService::hasRoomType(r, ASType::Bathroom) || RoomService::hasRoomType(r, ASType::ToiletRoom) || RoomService::hasRoomType(r, ASType::ShowerRoom) ) numberOfBathrooms++;
    }

    if ( house->mFloors.size() == 1 && numberOfGenericRoom > 0 && numberOfBathrooms == 0 ) {
        for ( auto& room : f->rooms ) {
            auto *r = room.get();
            if ( RoomService::hasRoomType(r, ASType::GenericRoom) && r->doors.size() == 1 ) {
                RoomService::setRoomType(r, ASType::Bathroom);
                numberOfBathrooms++;
            }
        }

    }

    // Guess Bedrooms and Kitchen/living rooms (with open plan) (Only for flats/apartments) (floors == 1)
    // **** Now this is complete wild guess but we are going to try it nevertheless ****
    // Basically the idea is that if we have between 2 and 4 unassigned rooms (a 1 to 3 bedroom flat)
    // We will guess that the biggest room is the Living/Kitchen area, the rest are the bedrooms
    // We can do this because we've already detected bathrooms and Hallways above. It's still wild, but might work.
    if ( house->mFloors.size() == 1 && numberOfGenericRoom >= 2 && numberOfGenericRoom <= 4 && numberOfKitchenOrLivingRooms == 0 ) {
        std::vector<std::pair<float, RoomBSData *>> areaPairs{};
        for ( auto& room : f->rooms ) {
            auto *r = room.get();
            if ( RoomService::hasRoomType(r, ASType::GenericRoom) ) {
                areaPairs.emplace_back(RoomService::area(r), r);
            }
        }
        std::sort(areaPairs.begin(), areaPairs.end(), []( const auto& a, const auto& b ) -> bool {
            return a.first > b.first;
        });
        bool goForIt = areaPairs[0].first > areaPairs[1].first * 1.5f;
        if ( goForIt ) {
            // Add the Kitchen/Living
            RoomService::setRoomType(areaPairs[0].second, ASType::Kitchen);
            RoomService::addRoomType(areaPairs[0].second, ASType::LivingRoom);
            // Add the biggest bedroom first, so this is the master bedroom
            RoomService::setRoomType(areaPairs[1].second, ASType::BedroomMaster);
            // Add the rest, if any
            for ( auto t = 2u; t < areaPairs.size(); t++ ) {
                RoomService::setRoomType(areaPairs[t].second, ASType::BedroomDouble);
            }
        }
    }
    // Another super guess, for 1 bed flats with open-space kitchen we might guess if it has detected a bathroom and a
    // bedroom, then the undetected room should be the living/kitchen.
    if ( house->mFloors.size() == 1 && numberOfGenericRoom == 1 ) {
        for ( auto& room : f->rooms ) {
            auto *r = room.get();
            if ( RoomService::hasRoomType(r, ASType::GenericRoom) ) {
                RoomService::setRoomType(r, ASType::Kitchen);
                RoomService::addRoomType(r, ASType::LivingRoom);
            }
        }
    }

}

void FloorService::calcWhichRoomDoorsAndWindowsBelong( FloorBSData *f, HouseBSData *house ) {
    for ( auto& w : f->windows ) {
        auto rooms = FloorService::roomsIntersectingBBox(f, w->BBox().squared(), true);
        if ( !rooms.empty() ) {
            auto r = rooms[0];
            w->rooms.emplace_back(r->hash);
            r->windows.emplace_back(w->hash);
            if ( RoomService::hasRoomType(r, ASType::Kitchen) ) {
                w->hasCurtains = false;
                w->hasBlinds = true;
            }
            V2f pointCheckInsideRoom = w->Position2d() + ( w->dirDepth * ( w->Depth() * 0.51f ) );
            w->rotOrientation = ArchStructuralService::isPointInside(r, pointCheckInsideRoom) ? M_PI : 0.0f;
        }
    }

    for ( auto& w : f->doors ) {
        auto rooms = FloorService::roomsIntersectingBBox(f, w->BBox().squared(), false);
        for ( const auto& r : rooms ) {
            r->doors.emplace_back(w->hash);
            w->rooms.emplace_back(r->hash);
        }
        // If it has only 1 room collided with it means that it's an external door
        if ( rooms.size() == 1 ) {
            w->isMainDoor = true;
        }
    }

    // Now we have all doors and windows connected, try to use some more guessing to help us out.
    FloorService::assignRoomTypeFromBeingClever(f, house);

    // Go back to re-evaluate every door after all possible discoveries/guesses have been made
    FloorService::reevaluateDoorsAndWindowsAfterRoomChange(f);
}

void FloorService::guessFittings( FloorBSData *f, FurnitureMapStorage& furns ) {
    for ( auto& r : f->rooms ) {
        RoomService::calcOptimalLightingFittingPositions(r.get());
        RoomService::addSocketsAndSwitches(r.get());
        RoomService::furnish(f, r.get(), furns);
    }
}

const RoomBSData *FloorService::findRoomWithHash( FloorBSData *f, int64_t hash ) {

    for ( const auto& r : f->rooms ) {
        if ( r->hash == hash ) {
            return r.get();
        }
    }

    return nullptr;
}

void FloorService::removeArch( FloorBSData *f, int64_t hashToRemove ) {
    // First check what type it is, in case we need to do cross-checking with other elements
    ArchStructural *elem = findElementWithHash(f, hashToRemove);
    if ( elem == nullptr ) return;

    if ( ArchStructuralService::typeIsiPoint(elem) ) {
    } else if ( ArchStructuralService::typeIsDOW(elem) ) {
        erase_if(f->windows, hashToRemove);
        erase_if(f->doors, hashToRemove);
    } else if ( ArchStructuralService::typeIsWall(elem) ) {
        erase_if(f->walls, hashToRemove);
    } else if ( ArchStructuralService::typeIsFittedFurniture(elem) ) {
        for ( auto& room : f->rooms ) {
            erase_if(room->mFittedFurniture, hashToRemove);
        }
    } else if ( ArchStructuralService::typeIsOutdoorArea(elem) ) {
        erase_if(f->outdoorAreas, hashToRemove);
    } else {
        erase_if(f->stairs, hashToRemove);
    }
}

void FloorService::moveArch( FloorBSData *f, ArchStructural *elem, const V2f& offset2d ) {
    // First check what type it is, in case we need to do cross-checking with other elements
    if ( elem == nullptr ) return;

    if ( ArchStructuralService::typeIsiPoint(elem) ) {
    } else if ( ArchStructuralService::typeIsDOW(elem) ) {
    } else if ( ArchStructuralService::typeIsWall(elem) ) {
    } else if ( ArchStructuralService::typeIsFittedFurniture(elem) ) {
        for ( auto& room : f->rooms ) {
            for ( auto& ff : room->mFittedFurniture ) {
                if ( ff.get() == elem ) {
                    ff->move(XZY::C(offset2d, 0.0f));
                }
            }
        }
    }
}

void FloorService::swapWindowOrDoor( FloorBSData *f, HouseBSData *h, int64_t hashOfTwoShape ) {
    auto elem = dynamic_cast<TwoUShapesBased *>(FloorService::findElementWithHash(f, hashOfTwoShape));
    if ( elem ) {
        FloorService::removeLinkedArch(f, hashOfTwoShape);
        FloorService::removeArch(f, hashOfTwoShape);
        if ( checkBitWiseFlag(elem->type, ArchType::WindowT) ) {
            FloorService::addDoorFromData(f, h->doorHeight, elem->us1, elem->us2);
        } else {
            FloorService::addWindowFromData(f, h->defaultWindowHeight, h->defaultWindowBaseOffset, elem->us1,
                                            elem->us2);
        }
    }
}

void FloorService::changeUShapeType( FloorBSData *f, const UShape& sourceUShape1, const UShape& sourceUShape2,
                                     ArchType _type ) {
    for ( auto& wall : f->walls ) {
        for ( auto& us : wall->mUShapes ) {
            if ( UShapeService::doesShareMaineEdge(us, sourceUShape1) ||
                 UShapeService::doesShareMaineEdge(us, sourceUShape2) ) {
                us.type = _type;
            }
        }
    }
}

void
FloorService::changeTypeOfSelectedElementTo( FloorBSData *f, ArchStructural *source, ArchType t, ArchSubTypeT st ) {
    ArchStructural *s = findElementWithHash(f, source->hash);

    if ( s ) {
        std::vector<UShape *> allUShapes = FloorService::allUShapes(f);
        auto *w = dynamic_cast<TwoUShapesBased *>( source );
        switch ( t ) {
            case ArchType::WindowT: {
                addWindowFromData(f, f->windowHeight, f->windowBaseOffset, w->us1, w->us2);
                break;
            }
            case ArchType::DoorT: {
                addDoorFromData(f, f->doorHeight, w->us1, w->us2, st);
                break;
            }
            default:
                break;
        }

        changeUShapeType(f, w->us1, w->us2, t);
        removeLinkedArch(f, source->hash);
        removeArch(f, source->hash);

        updateFromNewDoorOrWindow(f);
    }
}

float FloorService::area( const FloorBSData *f ) {
    float ret = 0.0f;

    for ( const auto& w : f->rooms ) ret += RoomService::area(w.get());
    for ( const auto& w : f->doors ) ret += w->Width() * w->Depth();
    for ( const auto& w : f->windows ) ret += w->Width() * w->Depth();

    return ret;
}

void FloorService::setCoving( FloorBSData *f, bool _state ) {
    f->hasCoving = _state;
    for ( auto& r : f->rooms ) {
        RoomService::setCoving(r.get(), _state);
    }
}

void FloorService::addCeilingContour( FloorBSData *f, const std::vector<Vector3f>& cc ) {
    f->ceilingContours.push_back(cc);
}

std::string FloorService::naturalLanguageFloorNumber( int numFloor ) {
    if ( numFloor < 5 ) {
        if ( numFloor == 0 ) return "Ground Floor";
        if ( numFloor == 1 ) return "First Floor";
        if ( numFloor == 2 ) return "Second Floor";
        if ( numFloor == 3 ) return "Third Floor";
        if ( numFloor == 4 ) return "Fourth Floor";
    }

    return std::to_string(numFloor) + "th Floor";
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
        for ( auto& v : vv ) r.expand(v.xy());
        if ( r.contains(v1) ) {
            topZ1 = vv[0].z();
            hitLevel1 = static_cast<int>(level) + 1;
            bFoundAtLeastOneContour = true;
        }
    }

    return bFoundAtLeastOneContour;
}

std::vector<Vector2f> FloorService::allFloorePoints( const FloorBSData *f ) {
    std::vector<Vector2f> ret;
    for ( auto& w : f->walls ) {
        for ( const auto& data : w->epoints ) {
            ret.push_back(data);
        }
    }
    return ret;
}

bool FloorService::intersectLine2d( const FloorBSData *f, Vector2f const& p0, Vector2f const& p1, Vector2f& i ) {
    bool ret = false;
    if ( f->BBox().lineIntersection(p0, p1) ) {
        for ( const auto& w : f->walls ) {
            if ( WallService::intersectLine2d(w.get(), p0, p1, i) ) return true;
        }
        for ( const auto& w : f->windows ) {
            if ( ArchStructuralService::intersectLine2d(w.get(), p0, p1, i) ) return true;
        }
        for ( const auto& w : f->doors ) {
            if ( ArchStructuralService::intersectLine2d(w.get(), p0, p1, i) ) return true;
        }
        for ( const auto& w : f->outdoorAreas ) {
            if ( ArchStructuralService::intersectLine2d(w.get(), p0, p1, i) ) return true;
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
    if ( f->BBox().lineIntersection(p0, p1) ) {
        for ( const auto& w : f->walls ) {
            WallService::intersectLine2dMin(w.get(), p0, p1, i, minDist, ret, filterFlags);
        }
    }
    if ( ret.hit ) {
        V2f i1 = lerp(0.5f, i, p0);
        for ( const auto& w : f->rooms ) {
            ret.hit &= !ArchStructuralService::isPointInside(w.get(), i1);
        }
    }
    return ret;
}

bool FloorService::isInsideRoomRDS( const V2f& i, const std::vector<RoomPreData>& rds ) {

    V2fVector vlist;
    for ( const auto& rd : rds ) {
        for ( const auto& as : rd.wallSegmentsInternal ) {
            V2fVector rpoints;
            rpoints.emplace_back(as.front().p1);
            for ( const auto& s : as ) {
                rpoints.push_back(s.p2);
            }
            Triangulator tri(rpoints, 0.000001f);
            auto triangles = tri.get2dTrianglesTuple();

            for ( auto& t : triangles ) {
                if ( isInsideTriangle(i, std::get<0>(t), std::get<1>(t), std::get<2>(t)) ) {
                    return true;
                }
            }
        }
    }
    return false;
}

void FloorService::removeLinkedArch( FloorBSData *f, int64_t hashToRemove ) {
    if ( hashToRemove == 0 ) return;
    // First check what type it is, in case we need to do cross-checking with other elements
    auto elems = findElementWithLinkedHash(f, hashToRemove);
    if ( elems.empty() ) return;

    for ( auto& e : elems ) {
        erase_if(f->walls, e->hash);
    }
}

void FloorService::removeWalls( FloorBSData *f ) {
    f->walls.clear();
}

void FloorService::removeWalls( FloorBSData *f, float wwidth ) {
    f->walls.erase(remove_if(f->walls.begin(), f->walls.end(),
                             [wwidth]( auto const& us ) -> bool { return us->Width() == wwidth; }), f->walls.end());
}

ArchStructural *FloorService::findElementWithHash( const FloorBSData *f, int64_t _hash ) {
    for ( auto& i : f->windows ) if ( i->hash == _hash ) return i.get();
    for ( auto& i : f->doors ) if ( i->hash == _hash ) return i.get();
    for ( auto& i : f->walls ) if ( i->hash == _hash ) return i.get();
    for ( auto& i : f->rooms ) {
        if ( i->hash == _hash ) return i.get();
        for ( auto& i2 : i->mFittedFurniture ) {
            if ( i2->hash == _hash ) return i2.get();
        }
    }
    for ( auto&& i : f->stairs ) if ( i->hash == _hash ) return i.get();
    for ( const auto& i : f->outdoorAreas ) if ( i->hash == _hash ) return i.get();

    return nullptr;
}

std::vector<ArchStructural *> FloorService::findElementWithLinkedHash( const FloorBSData *f, int64_t _hash ) {
    std::vector<ArchStructural *> ret;

    for ( auto& i : f->windows ) {
        if ( i->linkedHash == _hash ) ret.push_back(i.get());
    }
    for ( auto& i : f->doors ) {
        if ( i->linkedHash == _hash ) ret.push_back(i.get());
    }
    for ( auto& i : f->walls ) {
        if ( i->linkedHash == _hash ) ret.push_back(i.get());
    }
    for ( auto&& i : f->stairs ) {
        if ( i->linkedHash == _hash ) ret.push_back(i.get());
    }
    for ( auto&& i : f->outdoorAreas ) {
        if ( i->linkedHash == _hash ) ret.push_back(i.get());
    }

    return ret;
}

bool FloorService::findWallAt( const FloorBSData *f, const Vector2f& matPos, std::vector<ArchStructural *>& ret ) {
    bool hasBeenFound = false;

    for ( const auto& w : f->walls ) {
        if ( WallService::contains(w.get(), matPos) ) {
            ret.push_back(w.get());
            hasBeenFound = true;
        }
    }
    return hasBeenFound;
}

bool FloorService::findRoomAt( const FloorBSData *f, const Vector2f& matPos, std::vector<ArchStructural *>& ret ) {
    bool hasBeenFound = false;

    for ( const auto& w : f->rooms ) {
        if ( ArchStructuralService::isPointInside(w.get(), matPos) ) {
            ret.push_back(w.get());
            hasBeenFound = true;
        }
    }
    return hasBeenFound;
}

std::optional<RoomBSData *> FloorService::whichRoomAmI( const FloorBSData *f, const Vector2f& _pos ) {
    for ( const auto& r : f->rooms ) {
        bool isInHere = ArchStructuralService::isPointInside(r.get(), _pos);
        if ( isInHere ) {
            return r.get();
        }
    }
    return std::nullopt;
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
        float roomArea = RoomService::area(r.get());
        if ( roomArea > _currMaxArea ) {
            _currMaxArea = roomArea;
            _currCenter = RoomService::maxEnclsingBoundingBoxCenter(r.get());
        }
    }

}

ClipperLib::Paths FloorService::calcPlainPath( const FloorBSData *f ) {
    ClipperLib::Paths ret;

    ClipperLib::Path bboxPointsTop;
    std::vector<Vector2f> bp = f->BBox().points();
    bboxPointsTop << bp[0] << bp[1] << bp[2] << bp[3];
    ret.push_back(bboxPointsTop);
    ClipperLib::Path bboxPointsTop2;
    bboxPointsTop2 << bp[0] << bp[1] << bp[2] << bp[3];
    ret.push_back(bboxPointsTop2);

    return ret;
}

void FloorService::clearFurniture( FloorBSData *f ) {
    for ( const auto& w : f->rooms ) {
        RS::clearFurniture(w.get());
    }
}

// Rollbacks

void FloorService::rollbackToCalculatedWalls( FloorBSData *f ) {
    // So we need to update the walls because we need to clear all possible UShape flags
    for ( auto& w : f->walls ) {
        WallService::update(w.get());
    }
    erase_if(f->walls, []( const auto& w ) -> bool {
        return WallService::isWindowOrDoorPart(w.get());
    });
    f->rooms.clear();
    f->windows.clear();
    f->doors.clear();
    f->stairs.clear();
    f->orphanedUShapes.clear();
    f->orphanedWallSegments.clear();
}

bool FloorService::hasAnyWall( const FloorBSData *f ) {
    return !f->walls.empty();
}

bool FloorService::isFloorUShapeValid( const FloorUShapesPair& fus ) {
    return fus.f && fus.us1 && fus.us2;
}

std::optional<uint64_t>
FloorService::findRoomArchSegmentWithWallHash( FloorBSData *f, HashEH hashToFind, int64_t index ) {

    for ( const auto& room : f->rooms ) {
        auto ret = RoomService::findArchSegmentWithWallHash(room.get(), hashToFind, index);
        if ( ret ) return ret;
    }

    return std::nullopt;
}

void FloorService::rayFeatureIntersect( const FloorBSData *f, const RayPair3& rayPair, FeatureIntersection& fd,
                                        FeatureIntersectionFlagsT fif ) {

    if ( ArchStructuralService::intersectRay(f, rayPair) ) {
        for ( const auto& room : f->rooms ) {
            if ( ArchStructuralService::intersectRay(room.get(), rayPair) ) {

                // Floors
                if ( checkBitWiseFlag(fif, FeatureIntersectionFlags::FIF_Floors) ) {
                    V3f a = XZY::C(std::get<0>(room->Triangles2d()[0]), f->BBox3d().centreBottom().y());
                    V3f b = XZY::C(std::get<2>(room->Triangles2d()[0]), f->BBox3d().centreBottom().y());
                    V3f c = XZY::C(std::get<1>(room->Triangles2d()[0]), f->BBox3d().centreBottom().y());
                    Plane3f planeFloor{ a, b, c };
                    if ( planeFloor.intersectRayOnTriangles2dMin(rayPair, room->Triangles2d(), f->BBox3d().centreBottom().y(), fd.nearV) ) {
                        fd.normal = planeFloor.n;
                        fd.arch = room.get();
                        fd.room = room.get();
                        fd.intersectedType = GHType::Floor;
                    }
                }

                // Walls
                if ( checkBitWiseFlag(fif, FeatureIntersectionFlags::FIF_Walls) ) {
                    for ( auto& wd : room->mWallSegmentsSorted ) {
                        for ( const auto& quad : wd.quads ) {
                            Plane3f plane{ quad[0], quad[2], quad[1] };
                            if ( plane.intersectRayOnQuadMin(rayPair, quad, fd.nearV) ) {
                                V3f iPoint = rayPair.origin + ( rayPair.dir * fd.nearV );
                                fd.normal = plane.n;
                                fd.archSegment = &wd;
                                fd.room = room.get();
                                if ( distance(iPoint.y(), f->BBox3d().centreBottom().y()) < 0.15f ) {
                                    fd.intersectedType = GHType::Skirting;
                                } else if ( room->mHasCoving && distance(iPoint.y(), f->BBox3d().centreBottom().y() + f->Height()) < 0.15f ) {
                                    fd.intersectedType = GHType::Coving;
                                } else {
                                    fd.intersectedType = GHType::Wall;
                                }

                            }
                        }
                    }
                }

                // Ceilings
                if ( checkBitWiseFlag(fif, FeatureIntersectionFlags::FIF_Ceilings) ) {
                    V3f a = XZY::C(std::get<0>(room->Triangles2d()[0]), f->BBox3d().centreBottom().y() + f->Height());
                    V3f b = XZY::C(std::get<1>(room->Triangles2d()[0]), f->BBox3d().centreBottom().y() + f->Height());
                    V3f c = XZY::C(std::get<2>(room->Triangles2d()[0]), f->BBox3d().centreBottom().y() + f->Height());
                    Plane3f planeFloor{ a, b, c };
                    if ( planeFloor.intersectRayOnTriangles2dMin(rayPair, room->Triangles2d(), f->BBox3d().centreBottom().y() + f->Height(),
                                                                 fd.nearV, WindingOrder::CW) ) {
                        fd.normal = -planeFloor.n;
                        fd.arch = room.get();
                        fd.room = room.get();
                        fd.intersectedType = GHType::Ceiling;
                    }
                }

                // Furniture
                if ( checkBitWiseFlag(fif, FeatureIntersectionFlags::FIF_Furnitures) ) {
                    for ( const auto& ff : room->mFittedFurniture ) {
                        if ( ArchStructuralService::intersectRayMin(ff.get(), rayPair, fd.nearV) ) {
                            fd.arch = ff.get();
                            fd.room = room.get();
                            fd.intersectedType = GHType::Furniture;
                        }
                    }
                }

                // Windows
                if ( checkBitWiseFlag(fif, FeatureIntersectionFlags::FIF_Windows) ) {
                    for ( const auto& w : f->windows ) {
                        if ( ArchStructuralService::intersectRayMin(w.get(), rayPair, fd.nearV) ) {
                            fd.arch = w.get();
                            fd.room = room.get();
                            fd.intersectedType = GHType::Window;
                        }
                    }
                }

                // Balconies
                if ( checkBitWiseFlag(fif, FeatureIntersectionFlags::FIF_Balconies) ) {
                    for ( const auto& w : f->outdoorAreas ) {
                        if ( ArchStructuralService::intersectRayMin(w.get(), rayPair, fd.nearV) ) {
                            fd.arch = w.get();
                            fd.room = room.get();
                            fd.intersectedType = GHType::OutdoorArea;
                        }
                    }
                }

            }
        }
    }
}
