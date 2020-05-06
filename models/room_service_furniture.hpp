//
// Created by Dado on 2019-05-26.
//

#pragma once

#include "room_service.hpp"

using FurnitureMultiMap = std::unordered_multimap<FT, FittedFurniture>;
using FurnitureMap = std::unordered_map<FT, FittedFurniture>;
class SceneGraph;

class FurnitureMapStorage {
public:
    explicit FurnitureMapStorage( SceneGraph& _sg ) : sg( _sg ) {}

    void addIndex( FT _ft, const std::string& _name, const std::string& _symbolRef = "" );
    void addIndex( FT _ft, FittedFurniture& _ff );

    FittedFurniture& spawn( FT ft );
private:
    FurnitureMultiMap storage{};
    FurnitureMap index{};
    SceneGraph& sg;
};
