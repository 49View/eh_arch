//
// Created by dado on 10/08/2020.
//

#include "house_bsdata.hpp"

#include <core/math/triangulator.hpp>
#include <core/util.h>

#include "floor_service.hpp"
#include "room_service.hpp"
#include "wall_service.hpp"
#include "door_service.hpp"
#include "arch_segment_service.hpp"
#include "house_service.hpp"

// *********************************************************************************************************************
// House
// *********************************************************************************************************************

HouseBSData::HouseBSData( const Rect2f& _floorPlanBBox ) {
    initialiseVolume(AABB{ XZY::C(_floorPlanBBox.bottomRight(), 0.0f), XZY::C( _floorPlanBBox.topLeft(), 0.0f) });
}

void HouseBSData::updateVolumeInternal() {
    for ( auto& floor : mFloors ) {
        mergeVolume(floor->updateVolume());
    }
}

float HouseBSData::Elevation() const {
    return elevation;
}

V2f HouseBSData::PlaneOffset() const {
    return planeOffset;
}

void HouseBSData::recalcSkirtingSegments() {
    for ( auto& f : mFloors ) {
        for ( auto& r : f->rooms ) {
            RoomService::calcSkirtingSegments(r.get());
        }
    }
}

void HouseBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    for ( auto& floor : mFloors ) {
        floor->reRoot(_scale, _scaleSpace);
    }
    updateVolume();
    walkableArea = HouseService::area(this);
    // Post rescaling we might need to get feature with accurate values in cm, so this is a chance to do it
    recalcSkirtingSegments();
}

V3f HouseBSData::GeoOffset() const {
    return XZY::C(planeOffset, elevation);
}

void HouseBSData::elevate( float _elevation ) {
    elevation = _elevation;
//    for ( auto& floor : mFloors ) {
//        floor->elevation = floor->number * ( floor->concreteHeight + floor->Height() );
//    }
//    updateVolume();
}

FloorBSData *HouseBSData::addFloorFromData( const JMATH::Rect2f& _rect ) {

    auto f = std::make_shared<FloorBSData>(_rect, static_cast<int>( mFloors.size() ), defaultCeilingHeight,
                                           defaultGroundHeight, doorHeight, defaultWindowHeight,
                                           defaultWindowBaseOffset);
    mFloors.push_back(f);
    return f.get();
}

std::string HouseBSData::getLightmapID() const {
    return propertyId + "_lightmap";
}

// *********************************************************************************************************************
// Floor
// *********************************************************************************************************************

FloorBSData::FloorBSData( const JMATH::Rect2f& _rect, int _floorNumber, float _defaultCeilingHeight,
                          float _defaultGroundHeight, float _doorHeight, float _defaultWindowHeight,
                          float _defaultWindowBaseOffset ) {
    type = ArchType::FloorT;

    AABB lBBox3d{};
    auto lSize = V3f{ _rect.width(), _defaultCeilingHeight, _rect.height() };
    auto lCentre = XZY::C(_rect.centre(), number * ( _defaultGroundHeight + _defaultCeilingHeight ));
    lBBox3d.setCenterAndSize(lCentre, lSize);
    initialiseVolume(lBBox3d);

    anchorPoint = JMATH::Rect2fFeature::bottomRight;
    number = _floorNumber;
    ceilingContours.push_back(BBox().points3d(Height()));
    concreteHeight = _defaultGroundHeight;
    doorHeight = _doorHeight;
    windowHeight = _defaultWindowHeight;
    windowBaseOffset = _defaultWindowBaseOffset;
    hasCoving = true;
}

void FloorBSData::updateVolumeInternal() {
    for ( auto& i : windows ) {
        mergeVolume(i->updateVolume());
    }
    for ( auto& i : doors ) {
        mergeVolume(i->updateVolume());
    }
    for ( auto& i : walls ) {
        mergeVolume(i->updateVolume());
    }
    for ( auto& i : rooms ) {
        mergeVolume(i->updateVolume());
    }
    for ( auto& i : outdoorAreas ) {
        mergeVolume(i->updateVolume());
    }
}

void FloorBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    ArchSpatial::reRoot(_scale, _scaleSpace);

    for ( auto& vv : ceilingContours ) {
        for ( auto& v : vv ) {
            v *= { _scale, _scale, 1.0f };
        }
    }
    for ( auto& v : mPerimeterSegments ) {
        v *= _scale;
    }
    for ( auto& v : perimeterArchSegments ) {
        v.reRoot(_scale, _scaleSpace);
    }

    for ( auto& i : windows ) {
        i->reRoot(_scale, _scaleSpace);
    }
    for ( auto& i : doors ) {
        i->reRoot(_scale, _scaleSpace);
    }
    for ( auto& i : walls ) {
        i->reRoot(_scale, _scaleSpace);
    }
    for ( auto& i : rooms ) {
        i->reRoot(_scale, _scaleSpace);
    }
    for ( auto& usg : orphanedUShapes ) {
        usg.middle *= _scale;
    }
    for ( auto& usg : orphanedWallSegments ) {
        usg.p1 *= _scale;
        usg.p2 *= _scale;
    }
    for ( auto& i : outdoorAreas ) {
        i->reRoot(_scale, _scaleSpace);
    }

    for ( auto&& i : stairs ) {
        i->reRoot(_scale, _scaleSpace);
    }

    area = FloorService::area(this);
}

// *********************************************************************************************************************
// Room
// *********************************************************************************************************************

RoomBSData::RoomBSData( const RoomPreData& _preData, const float _floorHeight, const float _elevation ) {
    type = ArchType::RoomT;
    for ( const auto& rt: _preData.rtypes ) {
        RoomService::addRoomType(this, rt);
    }
    mHasCoving = !RS::hasRoomType(this, ASType::Kitchen);
    size = V3f{ _preData.bboxInternal.calcWidth(), _floorHeight, _preData.bboxInternal.calcHeight() };
    floorElevation = _elevation;
}

void RoomBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    ArchSpatial::reRoot(_scale, _scaleSpace);

    Vector3f scale3f = { _scale, _scale, 1.0f };
    for ( auto& covs : mWallSegments ) {
        for ( auto& s : covs ) {
            s.reRoot(_scale, _scaleSpace);
        }
    }
    for ( auto& s : mWallSegmentsSorted ) {
        s.reRoot(_scale, _scaleSpace);
    }
    for ( auto& s : mPerimeterSegments ) {
        s *= _scale;
    }
    mPerimeter *= _scale;
    for ( auto& s : mMaxEnclosingBoundingBox ) {
        s *= _scale;
    }
    for ( auto& s : mLightFittings ) {
        s.lightPosition *= scale3f;
    }
    for ( auto& covs : mvCovingSegments ) {
        for ( auto& s : covs ) {
            s *= _scale;
        }
    }
    for ( auto& covs : mvSkirtingSegments ) {
        for ( auto& s : covs ) {
            s *= _scale;
        }
    }
    mBBoxCoving *= _scale;

    mLongestWallOppositePoint *= _scale;

    maxSizeEnclosedHP1 *= _scale;
    maxSizeEnclosedHP2 *= _scale;
    maxSizeEnclosedWP1 *= _scale;
    maxSizeEnclosedWP2 *= _scale;

    area = RoomService::area(this);
}

void RoomBSData::makeTriangles2d() {
    mTriangles2d.clear();
    Triangulator tri(mPerimeterSegments);
    mTriangles2d = tri.get2dTrianglesTuple();
}

void RoomBSData::updateVolumeInternal() {

    for ( auto& ws : mWallSegments ) {
        for ( auto& ep : ws ) {
            bbox.expand(ep.p1);
            bbox.expand(ep.p2);
        }
    }
    bbox3d.calc(bbox, floorElevation, floorElevation + Height(), Matrix4f::IDENTITY());

    for ( auto& i : mFittedFurniture ) {
        mergeVolume(i->updateVolume());
    }

    makeTriangles2d();
    area = RS::area(this);
}

// *********************************************************************************************************************
// Wall
// *********************************************************************************************************************

WallBSData::WallBSData( const std::vector<Vector2f>& epts,
                        float _height,
                        WallLastPointWrapT wlpw,
                        float _elevation,
                        uint32_t wf,
                        int64_t _linkedHash,
                        SequencePart _sequencePart ) {
    type = ArchType::WallT;
    wrapLastPoint = wlpw;
    linkedHash = _linkedHash;
    sequencePart = _sequencePart;
    elevation = _elevation;
    wallFlags = wf;
    size.setY(_height);
    WallService::update(this, epts);
}

void WallBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    ArchSpatial::reRoot(_scale, _scaleSpace);

    for ( auto& s : epoints ) {
        s *= _scale;
    }
    for ( auto& s : mUShapes ) {
        s.reRoot(_scale, _scaleSpace);
    }
}

void WallBSData::updateVolumeInternal() {
    for ( auto& ep : epoints ) {
        bbox.expand(ep);
    }
    bbox3d.calc(bbox, elevation, elevation + Height(), Matrix4f::IDENTITY());
    w() = bbox.width();
    d() = bbox.height();
    makeTriangles2d();
}

void WallBSData::makeTriangles2d() {
    mTriangles2d.clear();
    // Check they are not self-intersecting
    size_t cSize = epoints.size();
    Vector2f pi = V2fc::ZERO;
    bool bNonPolyLine = false;
    for ( size_t t = 0; t < cSize; t++ ) {
        Vector2f p1 = epoints[getCircularArrayIndexUnsigned(t, cSize)];
        Vector2f p2 = epoints[getCircularArrayIndexUnsigned(t + 1, cSize)];
        for ( size_t m = t + 2; m < cSize + t - 2; m++ ) {
            if ( t != m ) {
                Vector2f p3 = epoints[getCircularArrayIndexUnsigned(m, cSize)];
                Vector2f p4 = epoints[getCircularArrayIndexUnsigned(m + 1, cSize)];
                if ( intersection(p1, p2, p3, p4, pi) ) {
                    bNonPolyLine = true;
                    break;
                }
            }
        }
        if ( bNonPolyLine ) break;
    }
    if ( !bNonPolyLine ) {
        Triangulator tri(epoints, 0.000001f);
        mTriangles2d = tri.get2dTrianglesTuple();
    }
}

float WallBSData::Elevation() const {
    return elevation;
}

// *********************************************************************************************************************
// Fitted Furniture
// *********************************************************************************************************************

[[nodiscard]] bool FittedFurniture::checkIf( FittedFurnitureFlagsT _flag ) const {

    return checkBitWiseFlag(flags, _flag);
}

FittedFurniture::FittedFurniture( const std::tuple<std::string, AABB>& args, std::string _keyTag,
                                  std::string _symbolRef ) :
        name(std::get<0>(args)), keyTag(std::move(_keyTag)), symbolRef(std::move(_symbolRef)) {

    auto lBBox = std::get<1>(args);
    initialiseVolume(lBBox);
    type = ArchType::FittedFurnitureT;
}

void FittedFurniture::updateVolumeInternal() {
    V3f scaledHalf = half(size * scaling);
    bbox3d = AABB{ ( centre + pos ) - scaledHalf, ( centre + pos ) + scaledHalf };
    bbox3d = bbox3d.rotate(rotation);
    bbox = bbox3d.topDown();
}

// *********************************************************************************************************************
// ArchSpatial
// *********************************************************************************************************************

void ArchSpatial::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    //height *= _scale;
    w() *= _scale;
    d() *= _scale;
    if ( _scaleSpace == ArchRescaleSpace::FloorplanScaling ) {
        centre *= V3f{ _scale, 1.0f, _scale };
        position() *= V3f{ _scale, 1.0f, _scale };
    } else {
        centre *= _scale;
        position() *= _scale;
        h() *= _scale;
    }

    for ( auto& vts : mTriangles2d ) {
        std::get<0>(vts) *= _scale;
        std::get<1>(vts) *= _scale;
        std::get<2>(vts) *= _scale;
    }
}

// *********************************************************************************************************************
// TwoUShapeBased
// *********************************************************************************************************************

void TwoUShapesBased::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    us1.reRoot(_scale, _scaleSpace);
    us2.reRoot(_scale, _scaleSpace);
    thickness *= _scale;

    ArchSpatial::reRoot(_scale, _scaleSpace);
}

void TwoUShapesBased::calcVolume() {
    Vector2f& p1 = us1.middle;
    Vector2f& p2 = us2.middle;

    // Recalculate all data that might have changed
    dirWidth = normalize(p2 - p1);
    dirDepth = ::rotate(dirWidth, M_PI_2);
    w() = JMATH::distance(p1, p2);
    d() = min(us1.width, us2.width);

    centre = XZY::C(V2fc::ZERO, lerp(0.5f, 0.0f, Height()));
    pos = XZY::C(lerp(0.5f, p1, p2), 0.0f);

    bbox = JMATH::Rect2f::INVALID;
    Vector2f negD = -dirDepth * ( HalfDepth() );
    Vector2f posD = dirDepth * ( HalfDepth() );
    Vector2f negW = -dirWidth * ( HalfWidth() );
    Vector2f posW = dirWidth * ( HalfWidth() );
    bbox.expand(pos.xz() + negD + posW);
    bbox.expand(pos.xz() + negD + negW);
    bbox.expand(pos.xz() + posD + posW);
    bbox.expand(pos.xz() + posD + negW);
}

// *********************************************************************************************************************
// Door
// *********************************************************************************************************************

DoorBSData::DoorBSData( float _doorHeight, float _ceilingHeight, const UShape& w1, const UShape& w2, ArchSubTypeT st ) {
    type = ArchType::DoorT;
    us1 = w1;
    us2 = w2;
    us1.type = ArchType::DoorT;
    us2.type = ArchType::DoorT;
    subType = st;
    thickness = 2.0f; // this is 2 inches
    wallFlags = WallFlags::WF_HasCoving;
    ceilingHeight = _ceilingHeight;
    h() = _doorHeight;

    calcVolume();
    bbox3d.calc(bbox, 0.0f, Height(), Matrix4f::IDENTITY());
}

void DoorBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    TwoUShapesBased::reRoot(_scale, _scaleSpace);
    DoorService::calculatePivots(this);
}

void DoorBSData::updateVolumeInternal() {
    calcVolume();
    bbox3d.calc(bbox, 0.0f, Height(), Matrix4f::IDENTITY());
}

// *********************************************************************************************************************
// Window
// *********************************************************************************************************************

WindowBSData::WindowBSData( float _windowHeight, float _ceilingHeight, float _defaultWindowBaseOffset, const UShape& w1,
                            const UShape& w2, ArchSubTypeT /*st*/ /*= ArchSubType::NotApplicable */ ) {
    type = ArchType::WindowT;
    wallFlags = WallFlags::WF_HasSkirting | WallFlags::WF_HasCoving;
    us1 = w1;
    us2 = w2;
    us1.type = ArchType::WindowT;
    us2.type = ArchType::WindowT;
    ceilingHeight = _ceilingHeight;
    baseOffset = _defaultWindowBaseOffset;
    h() = _windowHeight;

    calcVolume();
    centre.setY(lerp(0.5f, baseOffset, baseOffset + Height()));
    bbox3d.calc(bbox, baseOffset, baseOffset + Height(), Matrix4f::IDENTITY());

    numPanels = static_cast<int32_t>( Width() / minPanelWidth );
    if ( numPanels <= 0 ) numPanels = 1;
    float totalMainFramesWidths = mainFrameWidth * static_cast<float>( numPanels + 1 );
    if ( totalMainFramesWidths > Width() ) {
        totalMainFramesWidths = Width() / 2;
        mainFrameWidth = Width() / 4.0f;
    }
    minPanelWidth = ( Width() - totalMainFramesWidths ) / numPanels;
}

void WindowBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    TwoUShapesBased::reRoot(_scale, _scaleSpace);
}

void WindowBSData::updateVolumeInternal() {
    calcVolume();
    // Recalculate center Y, as usually a window does not start from the floor
    centre.setY(lerp(0.5f, baseOffset, baseOffset + Height()));
    bbox3d.calc(bbox, baseOffset, baseOffset + Height(), Matrix4f::IDENTITY());
}

// *********************************************************************************************************************
// OutdoorArea
// *********************************************************************************************************************

OutdoorBoundary::OutdoorBoundary( const std::vector<Vector2f>& epts ) {
    bPoints = epts;
}

OutdoorAreaBSData::OutdoorAreaBSData( const std::vector<Vector2f>& epts ) {
    outdoorBoundaries.emplace_back(OutdoorBoundary{epts});
//    calcBBox();
}

void OutdoorAreaBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    ArchSpatial::reRoot(_scale, _scaleSpace);
    for ( auto& boundary : outdoorBoundaries ) {
        for ( auto& ep : boundary.bPoints ) {
            ep *= _scale;
        }
    }
}

void OutdoorAreaBSData::updateVolumeInternal() {
    makeTriangles2d();
    for ( const auto& boundary : outdoorBoundaries ) {
        for ( const auto& ep : boundary.bPoints ) {
            bbox3d.expand(XZY::C(ep, boundary.elevation));
            bbox3d.expand(XZY::C(ep, boundary.elevation + boundary.zPull));
        }
    }
    bbox = bbox3d.topDown();
}

void OutdoorAreaBSData::addBoundary( const OutdoorBoundary& _ob ) {
    outdoorBoundaries.emplace_back(_ob);
//    calcBBox();
}

void OutdoorAreaBSData::makeTriangles2d() {
    mTriangles2d.clear();
    if ( !outdoorBoundaries.empty() && !outdoorBoundaries[0].bPoints.empty() ) {
        Triangulator tri(outdoorBoundaries[0].bPoints);
        mTriangles2d = tri.get2dTrianglesTuple();
    }
}

const std::vector<OutdoorBoundary>& OutdoorAreaBSData::Boundaries() const {
    return outdoorBoundaries;
}

std::vector<OutdoorBoundary>& OutdoorAreaBSData::Boundaries() {
    return outdoorBoundaries;
}

OutdoorBoundary& OutdoorAreaBSData::Boundary( std::size_t _index ) {
    return outdoorBoundaries[_index];
}

// *********************************************************************************************************************
// ArchSegment
// *********************************************************************************************************************

std::ostream& operator<<( std::ostream& os, const ArchSegment& segment ) {
    os << "iFloor: " << segment.iFloor << " iWall: " << segment.iWall << " iIndex: " << segment.iIndex
       << " wallHash: " << segment.wallHash << " p1: " << segment.p1 << " p2: " << segment.p2 << " middle: "
       << segment.middle << " normal: " << segment.normal << " crossNormal: " << segment.crossNormal << " tag: "
       << segment.tag << " sequencePart: " << segment.sequencePart << " zHeights: " << segment.quads.size();
    return os;
}

bool ArchSegment::operator==( const ArchSegment& rhs ) const {
    return iFloor == rhs.iFloor &&
           iWall == rhs.iWall &&
           iIndex == rhs.iIndex &&
           wallHash == rhs.wallHash &&
           p1 == rhs.p1 &&
           p2 == rhs.p2 &&
           middle == rhs.middle &&
           normal == rhs.normal &&
           crossNormal == rhs.crossNormal &&
           tag == rhs.tag &&
           quads == rhs.quads &&
           sequencePart == rhs.sequencePart;
}

bool ArchSegment::operator!=( const ArchSegment& rhs ) const {
    return !( rhs == *this );
}

float ArchSegment::length() const {
    return distance(p1, p2);
}

void ArchSegment::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    p1 *= _scale;
    p2 *= _scale;
    middle *= _scale;
    for ( auto& quad: quads ) {
        for ( auto& elem : quad ) {
            if ( _scaleSpace == ArchRescaleSpace::FloorplanScaling ) {
                elem *= V3f{ _scale, 1.0f, _scale };
            } else {
                elem *= _scale;
            }

        }
    }
}

// *********************************************************************************************************************
// UShape
// *********************************************************************************************************************

void UShape::reRoot( float _scale, [[maybe_unused]] ArchRescaleSpaceT _scaleSpace ) {
    for ( int64_t t = 0; t < 4; t++ ) points[t] *= _scale;
    middle *= _scale;
    width *= _scale;
}

// *********************************************************************************************************************
// Stairs
// *********************************************************************************************************************

void StairsBSData::updateVolumeInternal() {
    V3f scaledHalf = half(size * scaling);
    bbox3d = AABB{ ( centre + pos ) - scaledHalf, ( centre + pos ) + scaledHalf };
    bbox3d = bbox3d.rotate(rotation);
    bbox = bbox3d.topDown();
}
