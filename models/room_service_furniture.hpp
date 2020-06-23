//
// Created by Dado on 2019-05-26.
//

#pragma once

#include "room_service.hpp"

using FurnitureMultiMap = std::unordered_multimap<FT, FittedFurniture>;
using FurnitureMap = std::unordered_map<FT, FittedFurniture>;
class SceneGraph;

JSONDATA( FurnitureSet, ftype, name, bboxSize, symbol )
    uint64_t ftype;
    std::string name;
    V3f bboxSize;
    std::string symbol;
    FurnitureSet( uint64_t ftype, const std::string &name, const V3f &bboxSize, const std::string &symbol ) : ftype(
            ftype ), name( name ), bboxSize( bboxSize ), symbol( symbol ) {}
};

JSONDATA( FurnitureSetContainer, name, set )
    std::string name;
    std::vector<FurnitureSet> set;
};

class FurnitureMapStorage {
public:
    FurnitureMapStorage() = default;

    void addIndex( const FurnitureSet& _fs );
    void addIndex( FT _ft, FittedFurniture& _ff );

    std::shared_ptr<FittedFurniture> spawn( FT ft );
private:
    FurnitureMultiMap storage{};
    FurnitureMap index{};
};

namespace RoomServiceFurniture {
    void addDefaultFurnitureSet( const std::string& _name );
    void addFurnitureSingle( FloorBSData* f, RoomBSData* room, FurnitureMapStorage& furn, FT ftype );
    void rotateFurniture( FittedFurniture* ff );
    void scaleIncrementalFurniture( FittedFurniture* ff, float scale );
}