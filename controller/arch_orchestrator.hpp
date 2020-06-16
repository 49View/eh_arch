//
// Created by Dado on 20/10/2019.
//

#pragma once

#include "../models/house_bsdata.hpp"
#include "../models/room_service_furniture.hpp"
#include <core/resources/resource_utils.hpp>
#include <core/resources/resource_manager.hpp>
#include <eh_arch/render/house_render.hpp>

class SceneGraph;
class RenderOrchestrator;

using PostHouseLoadCallback = std::function<void(std::shared_ptr<HouseBSData> houseJson)>;
using PostHouse3dResolvedCallback = std::function<void(HouseBSData* houseJson)>;

class ArchOrchestrator {
public:
    explicit ArchOrchestrator( SceneGraph& _sg, RenderOrchestrator& _rsg, ArchRenderController& _arc );

    void loadFurnitureMapStorage( const std::string& _name );
    FurnitureMapStorage& FurnitureMap();

    void make3dHouse( const PostHouse3dResolvedCallback& ccf = nullptr );
    void showIMHouse();
    void loadHouse( const std::string& _pid, const PostHouseLoadCallback& ccf );
    void setHouse( const std::shared_ptr<HouseBSData>& _houseJson );
    Matrix4f calcFloorplanNavigationTransform( float screenRatio, float screenPadding );

    void centerCameraMiddleOfHouse( float slack = 0.0f );
    HouseRenderContainer& HRC();
    HouseBSData *H();

protected:
    SceneGraph& sg;
    RenderOrchestrator& rsg;
    ArchRenderController& arc;
    std::shared_ptr<HouseBSData> houseJson;
    HouseRenderContainer hrc;
    FurnitureMapStorage furnitureMap;
};
