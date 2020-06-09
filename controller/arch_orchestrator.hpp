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

using PostHouseLoadCallback = std::function<void(HouseBSData* houseJson)>;

class ArchOrchestrator {
public:
    explicit ArchOrchestrator( SceneGraph& _sg, RenderOrchestrator& _rsg );

    void show3dHouse( HouseBSData*, const PostHouseLoadCallback& ccf = nullptr );
    void showIMHouse( HouseBSData*, const ArchRenderController& ims );
    void loadHouse( const std::string& _pid, const PostHouseLoadCallback& ccf = nullptr );
    Matrix4f
    calcFloorplanNavigationTransform( std::shared_ptr<HouseBSData> houseJson, float screenRatio, float screenPadding );

    void centerCameraMiddleOfHouse( HouseBSData* _houseJson );

protected:
    SceneGraph& sg;
    RenderOrchestrator& rsg;
};
