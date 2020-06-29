//
// Created by Dado on 20/10/2019.
//

#pragma once

#include <core/memento.hpp>
#include <core/resources/resource_utils.hpp>
#include <core/resources/resource_manager.hpp>

#include <eh_arch/render/house_render.hpp>
#include <eh_arch/models/house_bsdata.hpp>
#include <eh_arch/models/room_service_furniture.hpp>
#include "htypes.hpp"

class SceneGraph;
class RenderOrchestrator;

class ArchOrchestrator {
public:
    explicit ArchOrchestrator( SceneGraph& _sg, RenderOrchestrator& _rsg, ArchRenderController& _arc );

    void loadFurnitureMapStorage( const std::string& _name );
    FurnitureMapStorage& FurnitureMap();

    void make3dHouse( const PostHouse3dResolvedCallback& ccf = nullptr );
    void showIMHouse();
    void loadHouse( const std::string& _pid, const PostHouseLoadCallback& ccf,
                    const PostHouseLoadCallback& ccfailure = nullptr );
    void saveHouse();
    void setHouse( const std::shared_ptr<HouseBSData>& _houseJson );
    void pushHouseChange();
    void undoHouseChange();
    void redoHouseChange();

    void setViewingMode( ArchViewingMode _wm );

    void onEvent(ArchIOEvents event);
    bool hasEvent(ArchIOEvents event) const;
    Matrix4f calcFloorplanNavigationTransform( float screenRatio, float screenPadding );

    void centerCameraMiddleOfHouse( float slack = 0.0f );
    HouseRenderContainer& HRC();
    HouseBSData *H();

    void setTourView();
    void setAssistedView();
    void setWalkView();
    void setFloorPlanView();
    void setTopDownView();
    void setDollHouseView();

protected:
    SceneGraph& sg;
    RenderOrchestrator& rsg;
    ArchRenderController& arc;
    Memento<HouseBSData> houseJson;
    ArchIOEvents currIOEvent = ArchIOEvents::AIOE_None;
    HouseRenderContainer hrc;
    FurnitureMapStorage furnitureMap;
    bool loadingMutex = false;
};
