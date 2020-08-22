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
#include <eh_arch/controller/arch_tour.hpp>
#include <eh_arch/controller/arch_positional_dot.hpp>
#include "htypes.hpp"

class SceneGraph;
class RenderOrchestrator;
class ArchExplorer;
struct AggregatedInputData;

class ArchOrchestrator {
public:
    explicit ArchOrchestrator( SceneGraph& _sg, RenderOrchestrator& _rsg, ArchRenderController& _arc, ArchExplorer& _ae );

    void loadFurnitureMapStorage( const std::string& _name );
    FurnitureMapStorage& FurnitureMap();

    void make3dHouse( const PostHouse3dResolvedCallback& ccf = nullptr );
    void showIMHouse();
    void loadHouse( const std::string& _pid, const PostHouseLoadCallback& ccf,
                    const PostHouseLoadCallback& ccFailure = nullptr );
    void saveHouse();
    void setHouse( const std::shared_ptr<HouseBSData>& _houseJson );
    void pushHouseChange();
    void undoHouseChange();
    void redoHouseChange();

    void onEvent(ArchIOEvents event);
    bool hasEvent(ArchIOEvents event) const;
    Matrix4f calcFloorplanNavigationTransform( float screenRatio, float screenPadding );

    void centerCameraMiddleOfHouse( float slack = 0.0f );
    void centerCameraMiddleOfHouseWithFloorplanInfoOffset( float floorplanOffset, float slack = 0.0f);
    const Matrix4f& FloorplanNavigationMatrix() const;
    HouseRenderContainer& HRC();
    HouseBSData *H();

    void setViewingMode( ArchViewingMode _wm );
    void setTourView();
    void setWalkView( float animationSpeed = 1.0f );
    void setFloorPlanView( FloorPlanRenderMode );
    void setTopDownView();
    void setDollHouseView();
    void toggleCollisions();

    ArchPositionalDot& PositionalDot();
    ArchExplorer& Explorer();

protected:
    SceneGraph& sg;
    RenderOrchestrator& rsg;
    ArchRenderController& arc;
    ArchExplorer& archExplorer;
    Memento<HouseBSData> houseJson;
    ArchIOEvents currIOEvent = ArchIOEvents::AIOE_None;
    HouseRenderContainer hrc;
    FurnitureMapStorage furnitureMap;
    bool loadingMutex = false;
    FloorPlanRenderMode lastKnownGoodFloorPlanRenderMode = FloorPlanRenderMode::Normal3d;
    Matrix4f floorplanNavigationMatrix{Matrix4f::MIDENTITY()};
    ArchPositionalDot positionalDot;
    TourPlayback tourPlayback;
};
