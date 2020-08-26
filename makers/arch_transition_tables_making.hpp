//
// Created by dado on 06/07/2020.
//

#pragma once

#include <core/state_machine_helper.hpp>
#include <eh_arch/state_machine/arch_sm_events__fsm.hpp>
#include <eh_arch/makers/arch_sm_actions_bespoke_builder.hpp>
#include <eh_arch/controller/outdoorArea/arch_sm_actions_outdoor_area_builder.hpp>
#include <eh_arch/makers/arch_sm_actions_maker_builder.hpp>

struct MakerStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / [] {} = state<class HouseMaker>
            ,state<HouseMaker> + event<OnGlobalRescaleEvent> / GlobalRescale{}
            ,state<HouseMaker> + event<OnClearEvent> / ClearEverthing{}
            ,state<HouseMaker> + event<OnHouseMakerToggleEvent> / ActivateFloorplanView{FloorPlanRenderMode::Debug3d}
            ,state<HouseMaker> + event<OnLoadFloorPlanEvent> / LoadFloorPlan{}
            ,state<HouseMaker> + event<OnCreateNewPropertyFromFloorplanImageEvent> / CreateNewPropertyFromFloorplanImage{}
            ,state<HouseMaker> + event<OnUpdateHMBEvent> / UpdateHMB{}
            ,state<HouseMaker> + event<OnHouseChangeElevationEvent> / ChangeElevation{}
            ,state<HouseMaker> + event<OnMakeHouse3dEvent> / MakeHouse3d{}
            ,state<HouseMaker> + event<OnImportExcaliburLinkEvent> / ImportExcaliburLink{}
            ,state<HouseMaker> + event<OnCreateHouseTexturesEvent> / CreateHouseTextures{}
            ,state<HouseMaker> + event<OnElaborateHouseBitmapEvent> / ElaborateHouseBitmap{}
            ,state<HouseMaker> + event<OnRecalculateFurnitureEvent> / FurnishHouse{}
            ,state<HouseMaker> + event<OnTopDownToggleEvent> / ActivateTopDownView{}
            ,state<HouseMaker> + event<OnUndoEvent> / UndoFeatureManipulation{}
            ,state<HouseMaker> + event<OnRedoEvent> / RedoFeatureManipulation{}
        );
    }
};

struct BespokeStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / []{} = state<class BespokeState>
            ,state<BespokeState> + event<OnUndoEvent> / UndoBespoke{}
            ,state<BespokeState> + event<OnClearEvent> / ClearBespoke{}
            ,state<BespokeState> + event<OnTouchMoveEvent> / TouchMoveBespoke{}
            ,state<BespokeState> + event<OnTouchUpEvent> / TouchUpEventBespoke{}
            ,state<BespokeState> + event<OnKeyToggleEvent> / KeyToggleBespoke{}
            ,state<BespokeState> + event<OnLoadFloorPlanEvent> / LoadFloorPlan{}
            ,state<BespokeState> + event<OnMakeHouse3dEvent> / MakeHouse3d{}
            ,state<BespokeState> + event<OnElaborateHouseBitmapEvent> / ElaborateHouseBitmap{}
            ,state<BespokeState> + event<OnRecalculateFurnitureEvent> / FurnishHouse{}
        );
    }
};

struct OutdoorAreaStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / []{} = state<class OutdoorAreaState>
            ,state<OutdoorAreaState> + event<OnFinaliseEvent> / FinaliseOutdoorArea{}
            ,state<OutdoorAreaState> + event<OnSingleTapEvent> / AddPointOutdoorAreaAction{}
        );
    }
};