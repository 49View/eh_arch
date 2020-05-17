//
// Created by dado on 17/05/2020.
//

#pragma once

struct RoomBSData;
class FurnitureMapStorage;

namespace KitchenRoomService {
    void createMasterPath( RoomBSData *w, FurnitureMapStorage &furns );
    void createUnits( RoomBSData *w, FurnitureMapStorage &furns );
}



