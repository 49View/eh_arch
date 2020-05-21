//
// Created by Dado on 20/10/2019.
//

#pragma once

#include "../models/house_bsdata.hpp"
#include "../models/room_service_furniture.hpp"
#include <core/resources/resource_utils.hpp>
#include <core/resources/resource_manager.hpp>
#include <eh_arch/scene/arch_service.hpp>

class SceneGraph;
class RenderOrchestrator;

class ArchSceneGraph {
public:
    explicit ArchSceneGraph( SceneGraph& _sg, RenderOrchestrator& _rsg, FurnitureMapStorage& _furns );

    void showHouse();
    void loadHouse( const std::string& _pid );
    void calcFloorplanNavigationTransform( std::shared_ptr<HouseBSData> houseJson );

    FurnitureMapStorage&    Furns() { return furns; }

    void update();

protected:

    void publishAndAddCallback();
    void realTimeCallbacks();
    void consumeCallbacks();

protected:
    SceneGraph& sg;
    RenderOrchestrator& rsg;
    FurnitureMapStorage& furns;

    std::shared_ptr<Matrix4f> floorplanNavigationMatrix;
    std::shared_ptr<HouseBSData> houseJson;
    std::pair<std::shared_ptr<HouseBSData>, bool> callbackStream;
};
