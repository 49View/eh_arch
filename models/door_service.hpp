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
    // Query
    std::string orientationToString( const DoorBSData *d );
    void getPlasterMiddlePoints( const DoorBSData *d, std::vector<Vector3f>& mpoints );

    // Update
    void calculatePivots( DoorBSData *d );
    void setPivotPoint( DoorBSData*d, int pivotPointIndex );
    void toggleOrientations( DoorBSData *d );
    void resize( DoorBSData *d, float _scale );
    void reevaluate( DoorBSData *d, FloorBSData* f );
    void reevaluateInRoom( DoorBSData *d, const RoomBSData* room );

    // Delete
};
