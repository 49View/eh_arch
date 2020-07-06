//
// Created by dado on 06/07/2020.
//

#pragma once

#include <core/state_machine_helper.hpp>
#include <eh_arch/state_machine/arch_sm_events__fsm.hpp>
#include <eh_arch/state_machine/arch_sm_actions__fsm.hpp>

struct EditStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / []{} = state<class EditState>

#ifdef _49VIEW_EDITOR_MODE_
            ,state<class EditState> + event<OnTouchMoveViewportSpaceEvent>[TouchMoveFeatureManipulation{}] / UpdateFeatureManipulationIm{}
            ,state<class EditState> + event<OnKeyToggleEvent>[KeyToggleFeatureManipulation{}] / UpdateFeatureManipulationIm{}
            ,state<class EditState> + event<OnSpecialSpaceEvent>[SpecialSpaceToggleFeatureManipulation{}] / UpdateFeatureManipulationIm{}
#endif
            ,state<class EditState> + event<OnUndoEvent> / UndoFeatureManipulation{}
            ,state<class EditState> + event<OnRedoEvent> / RedoFeatureManipulation{}
            ,state<class EditState> + event<OnIncrementalScaleEvent> / IncrementalScaleFeatureManipulation{}
            ,state<class EditState> + event<OnSpaceEvent>[SpaceToggleFeatureManipulation{}] / UpdateFeatureManipulationIm{}
            ,state<class EditState> + event<OnDeleteEvent>[DeleteFeatureManipulation{}] / ExitFeatureManipulation{}

            ,state<class EditState> + event<OnFirstTimeTouchDownViewportSpaceEvent>[TouchedDownFirstTimeFittedFurnitureGuard{}] / UpdateFeatureManipulationIm{}
            ,state<class EditState> + event<OnTouchUpViewportSpaceEvent>[TouchUpEventFeatureManipulation{}] / UpdateFeatureManipulationIm{}
            ,state<class EditState> + event<OnSingleTapViewportSpaceEvent>[SingleTapViewportSpaceFeatureManipulationGuard{}] / ExitFeatureManipulation{}

            ,state<class EditState> + event<OnMakeHouse3dEvent> / MakeHouse3d{}
            ,state<class EditState> + event<OnRecalculateFurnitureEvent> / FurnishHouse{}
        );
    }
};
