//
//  door_service.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "door_service.hpp"
#include <core/util_follower.hpp>
#include "twoushapes_service.hpp"
#include "room_service.hpp"
#include "floor_service.hpp"

void DoorService::toggleOrientations( DoorBSData *d ) {
    d->dIndex = ( d->dIndex + 1 ) % 4;
    calculatePivots(d);
}

void DoorService::setPivotPoint( DoorBSData *d, int pivotPointIndex ) {
    d->dIndex = pivotPointIndex;
    calculatePivots(d);
}

std::string DoorService::orientationToString( const DoorBSData *d ) {
    switch ( d->dIndex ) {
        case 0:
            return "Right Top";
        case 1:
            return "Right Bottom";
        case 2:
            return "Left Top";
        case 3:
            return "left Bottom";
        default:
            break;
    }
    return "";
}

bool isLeft( int index ) {
    return index >= 2;
}

void DoorService::calculatePivots( DoorBSData *d ) {
    float frameGeomPivot = 0.0f;
    float realDoorWidth = d->Width() - d->doorTrim * 2.0f;
    float doorGeomDepthPivot = d->HalfDepth();

    float side = d->dIndex < 2.0f ? -1.0f : 1.0f;//sideOfLine( wp1, d->center + d->dirDepth, d->center - d->dirDepth );

    d->hingesPivot = Vector3f(realDoorWidth * 0.5f * side, d->doorGeomThickness * 0.5f, 0.0f);

    if ( isOdd(d->dIndex) ) doorGeomDepthPivot *= -1.0f;
    float doorGeomPivotX = isLeft(d->dIndex) ? d->Width() * 0.5f - d->doorTrim : -d->Width() * 0.5f + d->doorTrim;
    d->doorPivot = { doorGeomPivotX, d->doorTrim, doorGeomDepthPivot };

    d->frameHingesPivot = Vector3f(d->hingesPivot.x(), frameGeomPivot, 0.0f);
    d->doorHandlePivotLeft = Vector3f(-side * realDoorWidth + ( 0.075f * side ), 0.85f, -d->doorGeomThickness);
    d->doorHandlePivotRight = Vector3f(-side * realDoorWidth + ( 0.075f * side ), 0.85f, 0.0f);

    if ( isOdd(d->dIndex) ) {
        d->doorHandlePivotLeft += V3f::Z_AXIS * d->doorGeomThickness;
        d->doorHandlePivotRight += V3f::Z_AXIS * d->doorGeomThickness;
    }
    d->doorHandleRot = Quaternion(M_PI, V3f::UP_AXIS);
    if ( isLeft(d->dIndex) ) {
        d->doorHandlePivotLeft += V3f::Z_AXIS * d->doorGeomThickness;
        d->doorHandlePivotRight -= V3f::Z_AXIS * d->doorGeomThickness;
        d->doorHandleRot =
                Quaternion(M_PI, V3f::Z_AXIS) * Quaternion(M_PI, V3f::UP_AXIS) * Quaternion(M_PI, V3f::X_AXIS);
    }

    d->doorHandlePlateDoorSidePivot = Vector3f(-side * realDoorWidth * 0.5f, 0.0f, d->HalfHeight());
    d->doorHandlePlateFrameSidePivot = Vector3f(-side * realDoorWidth * 0.5f, doorGeomDepthPivot, d->HalfHeight());
    d->doorHandleAngle = ( side > 0.0f ? M_PI : 0.0f ) - M_PI_2;

    d->openingAngleMax = isOdd(d->dIndex) ? -M_PI_2 : M_PI_2;
    d->openingAngleMax *= isLeft(d->dIndex) ? -1.0f : 1.0f;

    float depthGap = isOdd(d->dIndex) ? d->doorGeomThickness : 0.0f;
    float gapBetweenDoorEdgeAndHinges = 0.003f;
    float totalTrim = ( d->doorTrim * 2.0f + gapBetweenDoorEdgeAndHinges * 2.0f );
    float xPivot = isLeft(d->dIndex) ? -d->Width() + d->doorTrim * 2.0f + gapBetweenDoorEdgeAndHinges
                                     : gapBetweenDoorEdgeAndHinges;
    // This 2.5f here is to leave space for the frame trim, the inner frame trim, and the 0.5 is a bit of air between it.
    d->doorSize = V2f{ d->Width() - totalTrim, d->Height() - d->doorTrim * 2.5f };
    d->doorGeomPivot = V3f{ xPivot, 0.0f, depthGap };
}

void DoorService::reevaluateInRoom( DoorBSData *d, const RoomBSData* room ) {
    if ( RS::hasRoomType(room, ASType::Hallway) ) {
        float vwangle = -atan2(-d->dirWidth.y(), d->dirWidth.x());
        V2f dn = V2fc::X_AXIS * d->Width();
        dn.rotate(vwangle + M_PI * 0.8f);
        V2f checkPoint = d->Position2d() + dn;

        bool isInsideRoom = RS::isPointInsideRoom(room, checkPoint);
        if ( isInsideRoom || ( d->isMainDoor && !isInsideRoom ) ) {
            DoorService::setPivotPoint(d, 0);
        }
    }
    if ( RoomService::hasRoomType(room, ASType::Storage) |
         RoomService::hasRoomType(room, ASType::Cupboard) ) {
        d->isDoorTypicallyShut = true;
    }
}

void DoorService::reevaluate( DoorBSData *d, FloorBSData* f ) {
    DoorService::calculatePivots(d);
    for ( auto& roomHash : d->rooms ) {
        auto room = FloorService::findRoomWithHash(f, roomHash);
        if ( room ) {
            DoorService::reevaluateInRoom( d, room );
        }
    }
}

void DoorService::getPlasterMiddlePoints( const DoorBSData *d, std::vector<Vector3f>& mpoints ) {
    mpoints.push_back({ d->BBox3d().centre().xy(), d->Height() + ( d->ceilingHeight - d->Height() ) * 0.5f });
}
