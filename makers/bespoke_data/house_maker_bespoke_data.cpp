//
//  House_maker_bitmap.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "house_maker_bespoke_data.hpp"
#include <core/profiler.h>

std::shared_ptr<HouseBSData> HouseMakerBespokeData::make( ArchHouseBespokeData&& _data ) {
    PROFILE_BLOCK( "House service elaborate" );

    mHouse = std::make_shared<HouseBSData>();

    for ( const auto& wallPoints : _data.floorPoints ) {
        auto f = mHouse->addFloorFromData( Rect2f( wallPoints ) );
        WallLastPointWrapT wpw = _data.twoShapePoints.empty();
        FloorService::addWallsFromData( f, wallPoints, wpw );
        for ( const auto& dp : _data.twoShapePoints ) {
            switch ( dp.type ) {
                case ArchType::WindowT:
                    FloorService::addWindowFromData( f, mHouse->defaultWindowHeight, mHouse->defaultWindowBaseOffset, dp.us1, dp.us2 );
                    break;
                case ArchType::DoorT:
                    FloorService::addDoorFromData( f, mHouse->doorHeight, dp.us1, dp.us2 );
                    break;
            }
        }
    }
    guessRooms();

    return mHouse;
}

void HouseMakerBespokeData::guessRooms() {

//    for ( auto& f : mHouse->mFloors ) {
//        FloorService::roomRecognition( f.get() );
//        FloorService::addRoomsFromData( f.get() );
//    }
}
