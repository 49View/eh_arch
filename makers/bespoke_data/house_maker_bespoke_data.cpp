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
        mHouse->bbox = Rect2f( wallPoints );
        auto f = HouseService::addFloorFromData( mHouse.get(), mHouse->bbox );
        WallLastPointWrapT wpw = _data.twoShapePoints.empty();
        FloorService::addWallsFromData( f.get(), wallPoints, wpw );
        for ( const auto& dp : _data.twoShapePoints ) {
            switch ( dp.type ) {
                case ArchType::WindowT:
                    FloorService::addWindowFromData( f.get(), mHouse->defaultWindowHeight, mHouse->defaultWindowBaseOffset, dp.us1, dp.us2 );
                    break;
                case ArchType::DoorT:
                    FloorService::addDoorFromData( f.get(), mHouse->doorHeight, dp.us1, dp.us2 );
                    break;
            }
        }
    }
    guessRooms();
    guessFittings();

    return mHouse;
}

void HouseMakerBespokeData::guessRooms() {

    for ( auto& f : mHouse->mFloors ) {
        FloorService::roomRecognition( f.get() );
        FloorService::addRoomsFromData( f.get() );
    }
}

void HouseMakerBespokeData::guessFittings() {
    for ( auto& f : mHouse->mFloors ) {
        FloorService::guessFittings( f.get() );
    }
}
