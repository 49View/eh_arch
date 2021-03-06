//
// Created by dado on 04/06/2020.
//

#pragma once

#include <core/math/vector2f.h>
#include <eh_arch/models/property_list.hpp>
#include <utility>

struct OnWhichRoomAmIEvent {
};
struct OnPushTourPathEvent {
};
struct OnPushKeyFrameTourPathEvent {
    float timestamp = 5.0f;
};
struct OnPopTourPathEvent {
    int popIndex = -1;
};

struct OnActivateEvent {
    OnActivateEvent() = default;
    explicit OnActivateEvent( std::function<void()>  ccf ) : ccf(std::move(ccf)) {}
    explicit OnActivateEvent( FloorPlanRenderMode _floorPlanRenderMode ) : floorPlanRenderMode(_floorPlanRenderMode) {}

    std::function<void()> ccf = nullptr;
    FloorPlanRenderMode floorPlanRenderMode = FloorPlanRenderMode::Normal3d;
};

struct OnLoadFloorPlanEvent {
    PropertyListing property;
};
struct OnCreateNewPropertyFromFloorplanImageEvent {
    std::string floorplanFileName;
};
struct OnImportExcaliburLinkEvent {
    std::string excaliburLink;
};

struct OnCreateHouseTexturesEvent {
};
struct OnUpdateHMBEvent {
};
struct OnHouseChangeElevationEvent {
    float elevation = 0.0f;
};
struct OnMakeHouse3dEvent {
};
struct OnElaborateHouseBitmapEvent {
};
struct OnRecalculateFurnitureEvent {
};
struct OnAddFurnitureSingleEvent {
    RoomBSData *room = nullptr;
    FurnitureSet furnitureSet{};
};

struct OnHouseMakerToggleEvent {
};
struct OnTourToggleEvent {
};
struct OnOrbitModeEvent {
};
struct OnExploreToggleEvent {
};
struct OnTopDownToggleEvent {
};
struct OnFlorPlanViewToggleEvent {
};
struct OnDollyHouseToggleEvent {
};
struct OnBakeLightmapsEvent {
};


struct OnIncrementalScaleEvent {
    float incrementalScaleFactor = 0.0f;
};

struct OnGlobalRescaleEvent {
    float oldScaleFactor = 1.0f;
    float currentScaleFactorMeters = 1.0f;
};


struct OnActivateOutdoorAreaBuilderEvent {};