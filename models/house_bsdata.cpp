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
    bbox = _floorPlanBBox;
}

void HouseBSData::calcBBox() {
    bbox = Rect2f::INVALID;
    bbox3d = AABB::INVALID;
    for ( auto& floor : mFloors ) {
        floor->calcBBox();
        bbox.merge(floor->BBox());
        bbox3d.merge(floor->BBox3d());
    }

    centre = BBox3d().centre();
    size = BBox3d().size();
}

void HouseBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace, const V3f& _pos, [[maybe_unused]] const Quaternion& _rot ) {
    for ( auto& floor : mFloors ) {
        floor->reRoot(_scale, _scaleSpace, _pos, _rot);
    }
    calcBBox();
    walkableArea = HouseService::area(this);

    // Post rescaling we might need to get feature with accurate values in cm, so this is a chance to do it
    for ( auto& f : mFloors ) {
        for ( auto& r : f->rooms ) {
            RoomService::calcSkirtingSegments(r.get());
        }
    }
}

FloorBSData *HouseBSData::addFloorFromData( const JMATH::Rect2f& _rect ) {

    auto f = std::make_shared<FloorBSData>(_rect, static_cast<int>( mFloors.size() ), defaultCeilingHeight,
                                           defaultGroundHeight, doorHeight, defaultWindowHeight,
                                           defaultWindowBaseOffset);
    mFloors.push_back(f);
    return f.get();
}

// *********************************************************************************************************************
// Floor
// *********************************************************************************************************************

FloorBSData::FloorBSData( const JMATH::Rect2f& _rect, int _floorNumber, float _defaultCeilingHeight,
                          float _defaultGroundHeight, float _doorHeight, float _defaultWindowHeight,
                          float _defaultWindowBaseOffset ) {
    type = ArchType::FloorT;

    bbox = _rect;
    size = V3f{ _rect.width(), _defaultCeilingHeight, _rect.height() };
    centre = XZY::C(_rect.centre(), number * ( _defaultGroundHeight + _defaultCeilingHeight ));

    anchorPoint = JMATH::Rect2fFeature::bottomRight;
    number = _floorNumber;
    ceilingContours.push_back(BBox().points3d(Height()));
    z = number * ( _defaultGroundHeight + _defaultCeilingHeight );
    concreteHeight = _defaultGroundHeight;
    doorHeight = _doorHeight;
    windowHeight = _defaultWindowHeight;
    windowBaseOffset = _defaultWindowBaseOffset;
    hasCoving = true;
}

void FloorBSData::calcBBox() {
    bbox = Rect2f::INVALID;
    for ( const auto& v : FloorService::allFloorePoints(this) ) {
        bbox.expand(v);
    }
    bbox3d.calc(BBox(), z, z+Height(), Matrix4f({ 0.0f, 0.0f, z }));

    //offsetFromFloorAnchor = mHouse->getFirstFloorAnchor();
    //offsetFromFloorAnchor -= ( bbox.topLeft() + bbox.size()*0.5f );
    //offsetFromFloorAnchor3d = Vector3f( offsetFromFloorAnchor, z );
}

void FloorBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace, const V3f& _pos, [[maybe_unused]] const Quaternion& _rot ) {
    ArchSpatial::reRoot(_scale, _scaleSpace, _pos, _rot);

    for ( auto& vv : ceilingContours ) {
        for ( auto& v : vv ) {
            v *= { _scale, _scale, 1.0f };
        }
    }
    for ( auto& v : mPerimeterSegments ) {
        v *= _scale;
    }
    for ( auto& v : perimeterArchSegments ) {
        v.reRoot(_scale, _scaleSpace, _pos, _rot);
    }

    for ( auto& i : windows ) i->reRoot(_scale, _scaleSpace, _pos, _rot);
    for ( auto& i : doors ) i->reRoot(_scale, _scaleSpace, _pos, _rot);
    for ( auto& i : walls ) i->reRoot(_scale, _scaleSpace, _pos, _rot);
    for ( auto& i : rooms ) i->reRoot(_scale, _scaleSpace, _pos, _rot);
    for ( auto& usg : orphanedUShapes ) {
        usg.middle *= _scale;
    }
    for ( auto& usg : orphanedWallSegments ) {
        usg.p1 *= _scale;
        usg.p2 *= _scale;
    }
    for ( auto& i : balconies ) i->reRoot(_scale, _scaleSpace, _pos, _rot);

    //	for ( auto&& i : stairs ) i->reRoot();

    calcBBox();

    area = FloorService::area(this);
}

// *********************************************************************************************************************
// Room
// *********************************************************************************************************************

RoomBSData::RoomBSData( const RoomPreData& _preData, const float _floorHeight, const float _z ) {
    type = ArchType::RoomT;
    for ( const auto& rt: _preData.rtypes ) {
        RoomService::addRoomType(this, rt);
    }
    mHasCoving = !RS::hasRoomType(this, ASType::Kitchen);
    size = V3f{ _preData.bboxInternal.calcWidth(), _floorHeight, _preData.bboxInternal.calcHeight() };
    z = _z;
}

void RoomBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace, const V3f& _pos, [[maybe_unused]] const Quaternion& _rot ) {
    ArchSpatial::reRoot(_scale, _scaleSpace, _pos, _rot);

    Vector3f scale3f = { _scale, _scale, 1.0f };
    for ( auto& covs : mWallSegments ) {
        for ( auto& s : covs ) {
            s.reRoot(_scale, _scaleSpace, _pos, _rot);
        }
    }
    for ( auto& s : mWallSegmentsSorted ) {
        s.reRoot(_scale, _scaleSpace, _pos, _rot);
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

    calcBBox();

    area = RoomService::area(this);
}

void RoomBSData::makeTriangles2d() {
    mTriangles2d.clear();
    Triangulator tri(mPerimeterSegments);
    mTriangles2d = tri.get2dTrianglesTuple();
}

void RoomBSData::calcBBox() {
    bbox = Rect2f::INVALID;

    for ( auto& ws : mWallSegments ) {
        for ( auto& ep : ws ) {
            bbox.expand(ep.p1);
            bbox.expand(ep.p2);
        }
    }
    bbox3d.calc(bbox, z, z + Height(), Matrix4f::IDENTITY);
    centre = bbox.calcCentre();

    makeTriangles2d();
    area = RS::area(this);
}

// *********************************************************************************************************************
// Wall
// *********************************************************************************************************************

WallBSData::WallBSData( const std::vector<Vector2f>& epts,
                        float _height,
                        WallLastPointWrapT wlpw,
                        float _z,
                        uint32_t wf,
                        int64_t _linkedHash,
                        SequencePart _sequencePart ) {
    type = ArchType::WallT;
    wrapLastPoint = wlpw;
    linkedHash = _linkedHash;
    sequencePart = _sequencePart;
    z = _z;
    wallFlags = wf;
    size.setY(_height);
    WallService::update(this, epts);
}

void WallBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace, const V3f& _pos, [[maybe_unused]] const Quaternion& _rot ) {
    ArchSpatial::reRoot(_scale, _scaleSpace, _pos, _rot);

    for ( auto& s : epoints ) {
        s *= _scale;
    }
    for ( auto& s : mUShapes ) {
        s.reRoot(_scale, _scaleSpace, _pos, _rot);
    }

    calcBBox();
}

void WallBSData::calcBBox() {
    if ( epoints.empty() ) return;

    bbox = Rect2f::INVALID;
    for ( auto& ep : epoints ) {
        bbox.expand(ep);
    }
    bbox3d.calc(bbox, z, z + Height(), Matrix4f::IDENTITY);
    w() = bbox.width();
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

// *********************************************************************************************************************
// Fitted Furniture
// *********************************************************************************************************************

[[nodiscard]] bool FittedFurniture::checkIf( FittedFurnitureFlagsT _flag ) const {

    return checkBitWiseFlag(flags, _flag);
}

FittedFurniture::FittedFurniture( const std::tuple<std::string, AABB>& args, std::string _keyTag,
                                  std::string _symbolRef ) :
        name(std::get<0>(args)), keyTag(std::move(_keyTag)), symbolRef(std::move(_symbolRef)) {

    size = std::get<1>(args).size();
    centre = std::get<1>(args).centre();
    calcBBox();
    type = ArchType::FittedFurnitureT;
}

void FittedFurniture::calcBBox() {
    ArchSpatial::calcBBox();
}

// *********************************************************************************************************************
// ArchSpatial
// *********************************************************************************************************************

void ArchSpatial::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace, const V3f& _pos, [[maybe_unused]] const Quaternion& _rot ) {
    //height *= _scale;
    w() *= _scale;
    d() *= _scale;
    if ( _scaleSpace == ArchRescaleSpace::FloorplanScaling ) {
        centre *= V3f{ _scale, 1.0f, _scale};
        position() *= V3f{ _scale, 1.0f, _scale};
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

void ArchSpatial::calcBBox() {
    V3f scaledHalf = half(size * scaling);
    bbox3d = AABB{ (centre + pos) - scaledHalf, (centre + pos) + scaledHalf };
    bbox3d = bbox3d.rotate(rotation);
    bbox = bbox3d.topDown();
}

void ArchSpatial::posBBox() {
    V3f scaledHalf = half(size * scaling);
    bbox3d = AABB{ (centre + pos) - scaledHalf, (centre + pos) + scaledHalf };
    bbox = bbox3d.topDown();
}

void ArchSpatial::rotateBBox( const Quaternion& _rot ) {
    rot() = _rot;
    bbox3d = bbox3d.rotate(rotation);
    bbox = bbox3d.topDown();
}

void ArchSpatial::scaleBBox( const V3f& _scale ) {
    scale() = _scale;
    posBBox();
}

void ArchSpatial::move( const V3f& _off ) {
    position() += _off;
    posBBox();
}

void ArchSpatial::move( const V2f& _off ) {
    position() += XZY::C(_off, 0.0f);
    posBBox();
}

void ArchSpatial::position( const V3f& _pos ) {
    position() = _pos;
    posBBox();
}

void ArchSpatial::position( const V2f& _pos ) {
    position() = V3f{_pos.x(), position().y(), _pos.y()};
    posBBox();
}

void ArchSpatial::center( const V3f& _center ) {
    centre = _center;
    posBBox();
}

void ArchSpatial::rotate( const Quaternion& _rot ) {
    rotateBBox(_rot);
}

void ArchSpatial::scale( const V3f& _scale ) {
    scaleBBox(_scale);
}

[[maybe_unused]] JMATH::AABB& ArchSpatial::BBox3dEmergencyWrite() {
    return bbox3d;
}

float ArchSpatial::Width() const { return size.x(); }
float ArchSpatial::Height() const { return size.y(); }
float ArchSpatial::Depth() const { return size.z(); }
float ArchSpatial::HalfWidth() const { return Width() * 0.5f; }
float ArchSpatial::HalfHeight() const { return Height() * 0.5f; }
float ArchSpatial::HalfDepth() const { return Depth() * 0.5f; }
V3f ArchSpatial::Position() const { return pos; }
[[maybe_unused]] float ArchSpatial::PositionX() const { return Position().x(); }
float ArchSpatial::PositionY() const { return Position().y(); }
float ArchSpatial::PositionZ() const { return Position().z(); }
V2f ArchSpatial::Position2d() const { return pos.xz(); }
V3f ArchSpatial::Center() const { return bbox3d.centre(); }
const V3f& ArchSpatial::Size() const { return size; }
const V3f& ArchSpatial::Scale() const { return scaling; }
const Quaternion& ArchSpatial::Rotation() const { return rotation; }
const JMATH::Rect2f& ArchSpatial::BBox() const { return bbox; }
const JMATH::AABB& ArchSpatial::BBox3d() const { return bbox3d; }
const std::vector<Triangle2d>& ArchSpatial::Triangles2d() const { return mTriangles2d; }
float& ArchSpatial::w() { return size[0]; }
float& ArchSpatial::h() { return size[1]; }
float& ArchSpatial::d() { return size[2]; }
V3f& ArchSpatial::position() { return pos; }
Quaternion& ArchSpatial::rot() { return rotation; }
V3f& ArchSpatial::scale() { return scaling; }

// *********************************************************************************************************************
// TwoUShapeBased
// *********************************************************************************************************************

void TwoUShapesBased::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace, const V3f& _pos, [[maybe_unused]] const Quaternion& _rot ) {
    us1.reRoot( _scale, _scaleSpace, _pos, _rot );
    us2.reRoot( _scale, _scaleSpace, _pos, _rot );
    thickness *= _scale;

    ArchSpatial::reRoot( _scale, _scaleSpace, _pos, _rot );
    TwoUShapesBased::calcBBox();
    calcBBox();
}

void TwoUShapesBased::calcBBox() {
    Vector2f& p1 = us1.middle;
    Vector2f& p2 = us2.middle;

    // Recalculate all data that might have changed
    dirWidth = normalize( p2 - p1 );
    dirDepth = ::rotate( dirWidth, M_PI_2 );
    w() = JMATH::distance( p1, p2 );
    d() = min( us1.width, us2.width );

    centre = XZY::C(V2fc::ZERO, lerp(0.5f, 0.0f, Height()));
    pos = XZY::C(lerp(0.5f, p1, p2), 0.0f);

    bbox = JMATH::Rect2f::INVALID;
    Vector2f negD = -dirDepth * ( HalfDepth() );
    Vector2f posD = dirDepth * ( HalfDepth() );
    Vector2f negW = -dirWidth * ( HalfWidth() );
    Vector2f posW = dirWidth * ( HalfWidth() );
    bbox.expand( pos.xz() + negD + posW );
    bbox.expand( pos.xz() + negD + negW );
    bbox.expand( pos.xz() + posD + posW );
    bbox.expand( pos.xz() + posD + negW );
}

// *********************************************************************************************************************
// Door
// *********************************************************************************************************************

DoorBSData::DoorBSData(float _doorHeight, float _ceilingHeight, const UShape& w1, const UShape& w2, ArchSubTypeT st) {
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

    calcBBox();
}

void DoorBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace, const V3f& _pos, [[maybe_unused]] const Quaternion& _rot ) {
    TwoUShapesBased::reRoot(_scale, _scaleSpace, _pos, _rot);
    DoorService::calculatePivots(this);
}

void DoorBSData::calcBBox() {
    TwoUShapesBased::calcBBox();
    bbox3d.calc( bbox, 0.0f, Height(), Matrix4f::IDENTITY );
}

// *********************************************************************************************************************
// Window
// *********************************************************************************************************************

WindowBSData::WindowBSData( float _windowHeight, float _ceilingHeight, float _defaultWindowBaseOffset, const UShape& w1, const UShape& w2, ArchSubTypeT /*st*/ /*= ArchSubType::NotApplicable */ ) {
    type = ArchType::WindowT;
    wallFlags = WallFlags::WF_HasSkirting | WallFlags::WF_HasCoving;
    us1 = w1;
    us2 = w2;
    us1.type = ArchType::WindowT;
    us2.type = ArchType::WindowT;
    ceilingHeight = _ceilingHeight;
    baseOffset = _defaultWindowBaseOffset;
    h() = _windowHeight;

    calcBBox();

    numPanels = static_cast<int32_t>( Width() / minPanelWidth );
    if ( numPanels <= 0 ) numPanels = 1;
    float totalMainFramesWidths = mainFrameWidth * static_cast<float>( numPanels + 1 );
    if ( totalMainFramesWidths > Width() ) {
        totalMainFramesWidths = Width() / 2;
        mainFrameWidth = Width() / 4.0f;
    }
    minPanelWidth = ( Width() - totalMainFramesWidths ) / numPanels;
}

void WindowBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace, const V3f& _pos, [[maybe_unused]] const Quaternion& _rot ) {
    TwoUShapesBased::reRoot(_scale, _scaleSpace, _pos, _rot);
}

void WindowBSData::calcBBox() {
    TwoUShapesBased::calcBBox();
    // Recalculate center Y, as usually a window does not start from the floor
    centre.setY(lerp(0.5f, baseOffset, baseOffset + Height()));
    bbox3d.calc( bbox, baseOffset, baseOffset + Height(), Matrix4f::IDENTITY );
}

// *********************************************************************************************************************
// Balcony
// *********************************************************************************************************************

BalconyBSData::BalconyBSData( const std::vector<Vector2f>& epts ) {
    epoints = epts;
    calcBBox();
}

void BalconyBSData::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace, const V3f& _pos, [[maybe_unused]] const Quaternion& _rot ) {
    ArchSpatial::reRoot(_scale, _scaleSpace, _pos, _rot);
    for ( auto& s : epoints ) {
        s *= _scale;
    }
    calcBBox();
}

void BalconyBSData::calcBBox() {
    makeTriangles2d();
    for ( const auto& ep : epoints ) {
        bbox3d.expand( XZY::C(ep, z) );
        bbox3d.expand( XZY::C(ep, z+floorHeight) );
    }
    bbox = bbox3d.topDown();
}

void BalconyBSData::makeTriangles2d() {
    mTriangles2d.clear();
    Triangulator tri(epoints);
    mTriangles2d = tri.get2dTrianglesTuple();
}

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

void ArchSegment::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace, const V3f& _pos, [[maybe_unused]] const Quaternion& _rot ) {
    p1 *= _scale;
    p2 *= _scale;
    middle *= _scale;
    for ( auto& quad: quads ) {
        for ( auto& elem : quad ) {
            if ( _scaleSpace == ArchRescaleSpace::FloorplanScaling ) {
                elem *= V3f{_scale, 1.0f, _scale};
            } else {
                elem *= _scale;
            }

        }
    }
}

void UShape::reRoot( float _scale, ArchRescaleSpaceT _scaleSpace, const V3f& _pos, [[maybe_unused]] const Quaternion& _rot ) {
    for ( int64_t t = 0; t < 4; t++ ) points[t] *= _scale;
    middle *= _scale;
    width *= _scale;
}
