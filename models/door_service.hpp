//
//  door_service.hpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#pragma once

#include "house_bsdata.hpp"

class DoorService {
public:
	// Create
	static std::shared_ptr<DoorBSData> createDoor( float _doorHeight, float _ceilingHeight, const UShape& w1, const UShape& w2, float _architraveWidth, ArchSubTypeT st = ArchSubType::NotApplicable );

	// Query
	static std::string orientationToString( const DoorBSData* d );
	static float signOfOrientation( const DoorBSData* d );
	static float signOfOrientationSwizzled( const DoorBSData* d );
	static float signOfAnchorPoint( const DoorBSData* d );
	static void getPlasterMiddlePoints( const DoorBSData* d, std::vector<Vector3f>& mpoints );
	static void calculatePivots( const DoorBSData* d, const Vector2f& wp1, float realDoorWidth, Vector3f& hingesPivot, Vector3f& frameHingesPivot, Vector3f& doorHandlePivot, float& doorHandleAngle, float& doorGeomPivot, Vector3f& doorHandlePlateDoorSidePivot, Vector3f& doorHandlePlateFrameSidePivot );
	
	// Update
	static void toggleOrientations( DoorBSData* d );
	static void rescale( DoorBSData* d, float _scale );
	static void setOrientationParameters( DoorBSData* d );

	// Delete
};
