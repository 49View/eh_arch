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
    static std::shared_ptr<DoorBSData>
    createDoor( float _doorHeight, float _ceilingHeight, const UShape& w1, const UShape& w2, float _architraveWidth,
                ArchSubTypeT st = ArchSubType::NotApplicable );

    // Query
    static std::string orientationToString( const DoorBSData *d );
    static void getPlasterMiddlePoints( const DoorBSData *d, std::vector<Vector3f>& mpoints );

    // Update
    static void calculatePivots( DoorBSData *d );
    static void toggleOrientations( DoorBSData *d );
    static void rescale( DoorBSData *d, float _scale );

    // Delete
};
