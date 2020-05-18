//
// Created by dado on 17/05/2020.
//

#pragma once

struct RoomBSData;
struct FloorBSData;
class FurnitureMapStorage;

namespace KitchenRoomService {
    void createMasterPath( FloorBSData *f, RoomBSData *w, FurnitureMapStorage &furns );
    void createUnits( FloorBSData *f, RoomBSData *w, FurnitureMapStorage &furns );
}



