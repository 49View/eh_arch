//
//  door_service.hpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#pragma once

#include "house_bsdata.hpp"

namespace DoorService {
    // Create
    std::shared_ptr<DoorBSData>
    createDoor( float _doorHeight, float _ceilingHeight, const UShape& w1, const UShape& w2, float _architraveWidth,
                ArchSubTypeT st = ArchSubType::NotApplicable );

    // Query
    std::string orientationToString( const DoorBSData *d );
    void getPlasterMiddlePoints( const DoorBSData *d, std::vector<Vector3f>& mpoints );

    // Update
    void calculatePivots( DoorBSData *d );
    void setPivotPoint( DoorBSData*d, int pivotPointIndex );
    void toggleOrientations( DoorBSData *d );
    void rescale( DoorBSData *d, float _scale );

    // Delete
};
