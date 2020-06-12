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
    explicit ArchOrchestrator( SceneGraph& _sg, RenderOrchestrator& _rsg );

    void show3dHouse( HouseBSData*, const PostHouse3dResolvedCallback& ccf = nullptr );
    void showIMHouse( HouseBSData*, const ArchRenderController& arc );
    void loadHouse( const std::string& _pid, const PostHouseLoadCallback& ccf );
    Matrix4f
    calcFloorplanNavigationTransform( std::shared_ptr<HouseBSData> houseJson, float screenRatio, float screenPadding );

    void centerCameraMiddleOfHouse( HouseBSData* _houseJson );
    HouseRenderContainer& HRC();
protected:
    SceneGraph& sg;
    RenderOrchestrator& rsg;
    HouseRenderContainer hrc;
};
