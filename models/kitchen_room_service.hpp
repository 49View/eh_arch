//
// Created by dado on 17/05/2020.
//

#pragma once

#include <eh_arch/models/htypes.hpp>

struct RoomBSData;
struct FloorBSData;
class FurnitureMapStorage;

namespace KitchenRoomService {
    // Create/Update
    void createKitchen( FloorBSData *f, RoomBSData *w, FurnitureMapStorage &furns );
    void clear( RoomBSData *w );
    void setNextMainWorktopIndexCandidate( RoomBSData* w, GenericCallback ccf = nullptr );
    //Query
    bool hasKitchen( const RoomBSData *w );
}



