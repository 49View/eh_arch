//
// Created by dado on 10/08/2020.
//

#include "house_bsdata.hpp"

#include <core/math/triangulator.hpp>

#include "floor_service.hpp"
#include "room_service.hpp"
#include "wall_service.hpp"

// *********************************************************************************************************************
// House
// *********************************************************************************************************************

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

FloorBSData *HouseBSData::addFloorFromData( const JMATH::Rect2f& _rect ) {

    auto f = std::make_shared<FloorBSData>(_rect, static_cast<int>( mFloors.size() ), defaultCeilingHeight,
                                           defaultGroundHeight, doorHeight, defaultWindowHeight,
                                           defaultWindowBaseOffset);
    mFloors.push_back(f);
    return f.get();
}

HouseBSData::HouseBSData( const Rect2f& _floorPlanBBox ) {
    bbox = _floorPlanBBox;
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

void WallBSData::calcBBox() {
    if ( epoints.empty() ) return;

    bbox = Rect2f::INVALID;
    for ( auto& ep : epoints ) {
        bbox.expand(ep);
    }
    bbox3d.calc(bbox, Height(), Matrix4f::IDENTITY);
    w() = bbox.width();
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

void FittedFurniture::calcBBox() {
    V3f scaledHalf = half(size * scale);
    bbox3d = AABB{ position3d - scaledHalf, position3d + scaledHalf };
    bbox3d = bbox3d.rotate(rotation);
    bbox = bbox3d.topDown();
    centre = bbox3d.centre();
}
