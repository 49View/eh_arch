//
// Created by Dado on 2019-05-26.
//

#pragma once

#include "room_service.hpp"

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
    FurnitureMultiMap index{};
};

namespace RoomServiceFurniture {
    void addDefaultFurnitureSet( const std::string& _name );
    void rotateFurniture( RoomBSData* r, FittedFurniture *ff, const Quaternion& rot, SceneGraph& sg );
    void rotateFurniture( FittedFurniture* ff, const Quaternion& rot );
    void moveFurniture( FittedFurniture* ff, const V3f& off );
    void moveFurniture( RoomBSData* r, FittedFurniture* ff, const V3f& off, SceneGraph& sg );
    void scaleIncrementalFurniture( FittedFurniture* ff, float scale );
}