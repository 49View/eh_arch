//
// Created by dado on 06/07/2020.
//

#pragma once

#include <core/state_machine_helper.hpp>
#include <eh_arch/state_machine/arch_sm_events__fsm.hpp>
#include <eh_arch/makers/arch_sm_actions_bespoke_builder.hpp>
#include <eh_arch/makers/arch_sm_actions_outdoor_area_builder.hpp>
#include <eh_arch/makers/arch_sm_actions_maker_builder.hpp>

struct MakerStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / [] {} = state<class HouseMaker>
            ,state<class HouseMaker> + event<OnGlobalRescaleEvent> / GlobalRescale{}
            ,state<class HouseMaker> + event<OnClearEvent> / ClearEverthing{}
            ,state<class HouseMaker> + event<OnHouseMakerToggleEvent> / ActivateFloorplanView{FloorPlanRenderMode::Debug3d}
            ,state<class HouseMaker> + event<OnLoadFloorPlanEvent> / LoadFloorPlan{}
            ,state<class HouseMaker> + event<OnCreateNewPropertyFromFloorplanImageEvent> / CreateNewPropertyFromFloorplanImage{}
            ,state<class HouseMaker> + event<OnUpdateHMBEvent> / UpdateHMB{}
            ,state<class HouseMaker> + event<OnHouseChangeElevationEvent> / ChangeElevation{}
            ,state<class HouseMaker> + event<OnMakeHouse3dEvent> / MakeHouse3d{}
            ,state<class HouseMaker> + event<OnImportExcaliburLinkEvent> / ImportExcaliburLink{}
            ,state<class HouseMaker> + event<OnCreateHouseTexturesEvent> / CreateHouseTextures{}
            ,state<class HouseMaker> + event<OnElaborateHouseBitmapEvent> / ElaborateHouseBitmap{}
            ,state<class HouseMaker> + event<OnRecalculateFurnitureEvent> / FurnishHouse{}
            ,state<class HouseMaker> + event<OnTopDownToggleEvent> / ActivateTopDownView{}
            ,state<class HouseMaker> + event<OnUndoEvent> / UndoFeatureManipulation{}
            ,state<class HouseMaker> + event<OnRedoEvent> / RedoFeatureManipulation{}
        );
    }
};

struct BespokeStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / []{} = state<class BespokeState>
            ,state<class BespokeState> + event<OnUndoEvent> / UndoBespoke{}
            ,state<class BespokeState> + event<OnClearEvent> / ClearBespoke{}
            ,state<class BespokeState> + event<OnTouchMoveEvent> / TouchMoveBespoke{}
            ,state<class BespokeState> + event<OnTouchUpEvent> / TouchUpEventBespoke{}
            ,state<class BespokeState> + event<OnKeyToggleEvent> / KeyToggleBespoke{}
            ,state<class BespokeState> + event<OnLoadFloorPlanEvent> / LoadFloorPlan{}
            ,state<class BespokeState> + event<OnMakeHouse3dEvent> / MakeHouse3d{}
            ,state<class BespokeState> + event<OnElaborateHouseBitmapEvent> / ElaborateHouseBitmap{}
            ,state<class BespokeState> + event<OnRecalculateFurnitureEvent> / FurnishHouse{}
        );
    }
};

struct OutdoorAreaStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
                *state<class Initial> / []{LOGRS("OutdoorArea state")} = state<class OutdoorAreaState>
                ,state<class OutdoorAreaState> + event<OnSingleTapEvent> / AddPointOutdoorAreaAction{}
        );
    }
};