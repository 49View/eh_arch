//
// Created by dado on 17/05/2020.
//

#pragma once

struct RoomBSData;
struct FloorBSData;
class FurnitureMapStorage;

namespace KitchenRoomService {
    // Create/Update
    void createKitchen( FloorBSData *f, RoomBSData *w, FurnitureMapStorage &furns );
    void clear( RoomBSData *w );

    //Query
    bool hasKitchen( const RoomBSData *w );
}



