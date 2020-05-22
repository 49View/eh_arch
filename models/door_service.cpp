//
//  door_service.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "door_service.hpp"
#include "twoushapes_service.hpp"

std::shared_ptr<DoorBSData>
DoorService::createDoor( float _doorHeight, float _ceilingHeight, const UShape& w1, const UShape& w2,
                         const float _architraveWidth, ArchSubTypeT st ) {
    std::shared_ptr<DoorBSData> d1 = std::make_shared<DoorBSData>();

    d1->asType = ASType::Door;
    d1->type = ArchType::DoorT;
    d1->us1 = w1;
    d1->us2 = w2;
    d1->us1.type = ArchType::DoorT;
    d1->us2.type = ArchType::DoorT;
    d1->subType = st;
    d1->thickness = 2.0f; // this is 2 inches
    d1->height = _doorHeight;
    d1->wallFlags = WallFlags::WF_HasCoving;
    d1->ceilingHeight = _ceilingHeight;

    TwoUShapesBasedService::evalData(d1.get());

    return d1;
//	openingAngle = std::make_shared<AnimType<float>>( 0.0f );
}

void DoorService::toggleOrientations( DoorBSData *d ) {
    d->dIndex = (d->dIndex + 1) % 4;
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


void DoorService::rescale( DoorBSData *d, float _scale ) {
    TwoUShapesBasedService::rescale(d, _scale);
}

bool isLeft( int index ) {
    return index >= 2;
}

void DoorService::calculatePivots( DoorBSData *d ) {
    float frameGeomPivot = 0.0f;
    float realDoorWidth = d->width - d->doorTrim * 2.0f;
    float doorGeomDepthPivot = d->depth * 0.5f;

    float side = d->dIndex < 2.0f ? -1.0f : 1.0f;//sideOfLine( wp1, d->center + d->dirDepth, d->center - d->dirDepth );

    d->hingesPivot = Vector3f(realDoorWidth * 0.5f * side, d->doorGeomThickness * 0.5f, 0.0f);

    if ( isOdd(d->dIndex) ) doorGeomDepthPivot *= -1.0f;
    float doorGeomPivotX = isLeft(d->dIndex) ? d->width * 0.5f - d->doorTrim : -d->width * 0.5f + d->doorTrim;
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
        d->doorHandleRot = Quaternion(M_PI, V3f::Z_AXIS) * Quaternion(M_PI, V3f::UP_AXIS) * Quaternion(M_PI, V3f::X_AXIS);
    }


    d->doorHandlePlateDoorSidePivot = Vector3f(-side * realDoorWidth * 0.5f, 0.0f, d->height * 0.5f);
    d->doorHandlePlateFrameSidePivot = Vector3f(-side * realDoorWidth * 0.5f, doorGeomDepthPivot, d->height * 0.5f);
    d->doorHandleAngle = ( side > 0.0f ? M_PI : 0.0f ) - M_PI_2;

    d->openingAngleMax = isOdd(d->dIndex) ? -M_PI_2 : M_PI_2;
    d->openingAngleMax *= isLeft(d->dIndex) ? -1.0f : 1.0f;

    float depthGap = isOdd(d->dIndex) ? d->doorGeomThickness : 0.0f;
    float gapBetweenDoorEdgeAndHinges = 0.003f;
    float totalTrim = ( d->doorTrim * 2.0f + gapBetweenDoorEdgeAndHinges * 2.0f );
    float xPivot = isLeft(d->dIndex) ? -d->width + d->doorTrim * 2.0f + gapBetweenDoorEdgeAndHinges
                                     : gapBetweenDoorEdgeAndHinges;
    // This 2.5f here is to leave space for the frame trim, the inner frame trim, and the 0.5 is a bit of air between it.
    d->doorSize = V2f{ d->width - totalTrim, d->height - d->doorTrim * 2.5f };
    d->doorGeomPivot = V3f{ xPivot, 0.0f, depthGap };

}

void DoorService::getPlasterMiddlePoints( const DoorBSData *d, std::vector<Vector3f>& mpoints ) {
    mpoints.push_back({ d->bbox3d.centre().xy(), d->height + ( d->ceilingHeight - d->height ) * 0.5f });
}
