//
//  door_service.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "door_service.hpp"
#include "twoushapes_service.hpp"

std::shared_ptr<DoorBSData> DoorService::createDoor( float _doorHeight, float _ceilingHeight, const UShape& w1, const UShape& w2, const float _architraveWidth, ArchSubTypeT st ) {
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
	d1->architraveWidth = _architraveWidth;

	TwoUShapesBasedService::evalData( d1.get() );

	return d1;
//	openingAngle = std::make_shared<AnimType<float>>( 0.0f );
}

void DoorService::toggleOrientations( DoorBSData* d ) {
	switch ( d->orientation ) {
		case DoorOrientation::W1_CW:
		d->orientation = DoorOrientation::W1_CCW;
		break;
		case DoorOrientation::W1_CCW:
		d->orientation = DoorOrientation::W2_CW;
		break;
		case DoorOrientation::W2_CW:
		d->orientation = DoorOrientation::W2_CCW;
		break;
		case DoorOrientation::W2_CCW:
		d->orientation = DoorOrientation::W1_CW;
		break;
		default:
		break;
	}
	setOrientationParameters( d );
}

std::string DoorService::orientationToString( const DoorBSData* d ) {
	switch ( d->orientation ) {
		case DoorOrientation::W1_CW:
		return "DoorOrientation::W1_CW";
		case DoorOrientation::W2_CW:
		return "DoorOrientation::W2_CW";
		case DoorOrientation::W1_CCW:
		return "DoorOrientation::W1_CCW";
		case DoorOrientation::W2_CCW:
		return "DoorOrientation::W2_CCW";
		default:
		break;
	}
	return "";
}

float DoorService::signOfOrientation( const DoorBSData* d ) {
	switch ( d->orientation ) {
		case DoorOrientation::W1_CW:
		case DoorOrientation::W2_CW:
		return 1.0f;
		case DoorOrientation::W1_CCW:
		case DoorOrientation::W2_CCW:
		return -1.0f;
		default:
		break;
	}
	ASSERT( false );
	return 1.0f;
}

float DoorService::signOfOrientationSwizzled( const DoorBSData* d ) {
	switch ( d->orientation ) {
		case DoorOrientation::W1_CW:
		case DoorOrientation::W2_CCW:
		return 1.0f;
		case DoorOrientation::W1_CCW:
		case DoorOrientation::W2_CW:
		return -1.0f;
		default:
		break;
	}
	ASSERT( false );
	return 1.0f;
}

float DoorService::signOfAnchorPoint( const DoorBSData* d ) {
	switch ( d->orientation ) {
		case DoorOrientation::W1_CW:
		case DoorOrientation::W1_CCW:
		return 1.0f;
		case DoorOrientation::W2_CCW:
		case DoorOrientation::W2_CW:
		return -1.0f;
		default:
		break;
	}
	ASSERT( false );
	return 1.0f;
}

void DoorService::rescale( DoorBSData* d, float _scale ) {
	TwoUShapesBasedService::rescale( d, _scale );
}

void DoorService::setOrientationParameters( DoorBSData* d ) {
	switch ( d->orientation ) {
		case DoorOrientation::W1_CCW:
		d->pivotIndex = DoorPivotIndex::W1;
		d->openingAngleMin = 0.0f;
		d->openingAngleMax = -M_PI_2;
		break;
		case DoorOrientation::W2_CW:
		d->pivotIndex = DoorPivotIndex::W2;
		d->openingAngleMin = 0.0f;
		d->openingAngleMax = -M_PI_2;
		break;
		case DoorOrientation::W2_CCW:
		d->pivotIndex = DoorPivotIndex::W2;
		d->openingAngleMin = 0.0f;
		d->openingAngleMax = M_PI_2;
		break;
		case DoorOrientation::W1_CW:
		d->pivotIndex = DoorPivotIndex::W1;
		d->openingAngleMin = 0.0f;
		d->openingAngleMax = M_PI_2;
		break;

		default:
		break;
	}
}

void DoorService::calculatePivots( const DoorBSData* d, const Vector2f& wp1, float realDoorWidth, Vector3f& hingesPivot, Vector3f& frameHingesPivot, Vector3f& doorHandlePivot, float& doorHandleAngle, float& doorGeomPivot, Vector3f& doorHandlePlateDoorSidePivot, Vector3f& doorHandlePlateFrameSidePivot ) {
	float frameGeomPivot = 0.0f;
	float side = sideOfLine( wp1, d->center + d->dirDepth, d->center - d->dirDepth );
	switch ( d->orientation ) {
		case DoorOrientation::W1_CW:
		case DoorOrientation::W2_CW:
		hingesPivot = Vector3f( realDoorWidth*0.5f*side, d->doorGeomThickness*0.5f, 0.0f );
		doorGeomPivot = d->depth*0.5f;
		frameGeomPivot = d->depth*0.5f;
		break;
		case DoorOrientation::W1_CCW:
		case DoorOrientation::W2_CCW:
		doorGeomPivot = -d->depth*0.5f;
		hingesPivot = Vector3f( realDoorWidth*0.5f*side, -d->doorGeomThickness*0.5f, 0.0f );
		frameGeomPivot = -d->depth*0.5f;
		break;
		default:
		ASSERT( 0 );
		break;
	}
	frameHingesPivot = Vector3f( hingesPivot.x(), frameGeomPivot, 0.0f );
	doorHandlePivot = Vector3f( -side * realDoorWidth + ( 0.055f*side ), 0.85f, side * d->doorGeomThickness*0.5f );
	doorHandlePlateDoorSidePivot = Vector3f( -side*realDoorWidth*0.5f, 0.0f, d->height*0.5f );
	doorHandlePlateFrameSidePivot = Vector3f( -side * realDoorWidth*0.5f, doorGeomPivot, d->height*0.5f );
	doorHandleAngle = ( side > 0.0f ? M_PI : 0.0f ) - M_PI_2;
}

void DoorService::getPlasterMiddlePoints( const DoorBSData* d, std::vector<Vector3f>& mpoints ) {
	mpoints.push_back( { d->bbox3d.centre().xy(), d->height + ( d->ceilingHeight - d->height )*0.5f } );
}
