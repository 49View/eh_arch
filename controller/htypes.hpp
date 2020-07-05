//
// Created by dado on 29/06/2020.
//

#pragma once

using PostHouseLoadCallback = std::function<void()>;
using PostHouse3dResolvedCallback = std::function<void()>;

enum class ArchIOEvents {
    AIOE_None,
    AIOE_OnLoad,
    AIOE_OnLoadComplete,
};

enum ArchViewingMode {
    AVM_Hidden = -1,
    AVM_Tour = 0,
    AVM_Walk,
    AVM_FloorPlan,
    AVM_TopDown,
    AVM_DollHouse
};

enum class FloorPlanRenderMode {
    Normal2d,
    Normal3d,
    Debug2d,
    Debug3d,
    Debug3dSelection
};
