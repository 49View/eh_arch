//
// Created by dado on 10/08/2020.
//

#include "house_bsdata.hpp"

#include <core/math/triangulator.hpp>
#include <core/util.h>

#include "floor_service.hpp"
#include "room_service.hpp"
#include "wall_service.hpp"
#include "ushape_service.hpp"
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

void HouseBSData::rescale( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    for ( auto& floor : mFloors ) {
        floor->rescale(_scale, _scaleSpace);
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
    bbox3d.calc(BBox(), Height(), Matrix4f({ 0.0f, 0.0f, z }));

    //offsetFromFloorAnchor = mHouse->getFirstFloorAnchor();
    //offsetFromFloorAnchor -= ( bbox.topLeft() + bbox.size()*0.5f );
    //offsetFromFloorAnchor3d = Vector3f( offsetFromFloorAnchor, z );
}

void FloorBSData::rescale( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    ArchSpatial::rescale(_scale, _scaleSpace);

    for ( auto& vv : ceilingContours ) {
        for ( auto& v : vv ) {
            v *= { _scale, _scale, 1.0f };
        }
    }
    for ( auto& v : mPerimeterSegments ) {
        v *= _scale;
    }
    for ( auto& v : perimeterArchSegments ) {
        ArchSegmentService::rescale(v, _scale);
    }

    for ( auto& i : windows ) i->rescale(_scale, _scaleSpace);
    for ( auto& i : doors ) i->rescale(_scale, _scaleSpace);
    for ( auto& i : walls ) i->rescale(_scale, _scaleSpace);
    for ( auto& i : rooms ) i->rescale(_scale, _scaleSpace);
    for ( auto& usg : orphanedUShapes ) {
        usg.middle *= _scale;
    }
    for ( auto& usg : orphanedWallSegments ) {
        usg.p1 *= _scale;
        usg.p2 *= _scale;
    }

    //	for ( auto&& i : stairs ) i->rescale();

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

void RoomBSData::rescale( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    ArchSpatial::rescale(_scale, _scaleSpace);

    Vector3f scale3f = { _scale, _scale, 1.0f };
    for ( auto& covs : mWallSegments ) {
        for ( auto& s : covs ) {
            ArchSegmentService::rescale(s, _scale);
        }
    }
    for ( auto& s : mWallSegmentsSorted ) {
        ArchSegmentService::rescale(s, _scale);
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
    bbox3d.calc(bbox, Height(), Matrix4f::IDENTITY);
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

void WallBSData::rescale( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    ArchSpatial::rescale(_scale, _scaleSpace);

    for ( auto& s : epoints ) {
        s *= _scale;
    }
    for ( auto& s : mUShapes ) {
        UShapeService::rescale(s, _scale);
    }

    calcBBox();
}

void WallBSData::calcBBox() {
    if ( epoints.empty() ) return;

    bbox = Rect2f::INVALID;
    for ( auto& ep : epoints ) {
        bbox.expand(ep);
    }
    bbox3d.calc(bbox, Height(), Matrix4f::IDENTITY);
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

FittedFurniture::FittedFurniture( const std::tuple<std::string, V3f>& args, std::string _keyTag,
                                  std::string _symbolRef ) :
        name(std::get<0>(args)), keyTag(std::move(_keyTag)), symbolRef(std::move(_symbolRef)) {
    size = std::get<1>(args);
    type = ArchType::FittedFurnitureT;
}

void FittedFurniture::calcBBox() {
    V3f scaledHalf = half(size * scale);
    bbox3d = AABB{ position3d - scaledHalf, position3d + scaledHalf };
    bbox3d = bbox3d.rotate(rotation);
    bbox = bbox3d.topDown();
    centre = bbox3d.centre();
}

// *********************************************************************************************************************
// ArchSpatial
// *********************************************************************************************************************

void ArchSpatial::rescale( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    //height *= _scale;
    w() *= _scale;
    d() *= _scale;
    if ( _scaleSpace == ArchRescaleSpace::FloorplanScaling ) {
        center() *= V3f{ _scale, 1.0f, _scale};
    } else {
        center() *= _scale;
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

void TwoUShapesBased::rescale( float _scale, ArchRescaleSpaceT _scaleSpace ) {

    UShapeService::rescale( us1, _scale );
    UShapeService::rescale( us2, _scale );
    thickness *= _scale;

    ArchSpatial::rescale( _scale, _scaleSpace );
    calcBBox();
}

void TwoUShapesBased::calcBBox() {
    Vector2f& p1 = us1.middle;
    Vector2f& p2 = us2.middle;

    // Recalculate center
    centre = XZY::C(lerp(0.5f, p1, p2), 0.0f);

    // Recalculate all data that might have changed
    dirWidth = normalize( p2 - p1 );
    dirDepth = rotate( dirWidth, M_PI_2 );
    w() = JMATH::distance( p1, p2 );
    d() = min( us1.width, us2.width );

    bbox = JMATH::Rect2f::INVALID;
    Vector2f negD = -dirDepth * ( HalfDepth() );
    Vector2f posD = dirDepth * ( HalfDepth() );
    Vector2f negW = -dirWidth * ( HalfWidth() );
    Vector2f posW = dirWidth * ( HalfWidth() );
    bbox.expand( Center() + negD + posW );
    bbox.expand( Center() + negD + negW );
    bbox.expand( Center() + posD + posW );
    bbox.expand( Center() + posD + negW );

    bbox3d.calc( bbox, ceilingHeight, Matrix4f::IDENTITY );
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

void DoorBSData::rescale( float _scale, ArchRescaleSpaceT _scaleSpace ) {
    TwoUShapesBased::rescale(_scale, _scaleSpace);
    DoorService::calculatePivots(this);
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
