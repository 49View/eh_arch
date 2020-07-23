//
// Created by dado on 06/07/2020.
//

#pragma once

#include <core/state_machine_helper.hpp>
#include <eh_arch/state_machine/arch_sm_events__fsm.hpp>
#include <eh_arch/state_machine/arch_sm_actions__fsm.hpp>

struct TourStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / []{} = state<class TourState>
            ,state<class TourState> + event<OnTakeScreenShotEvent> / TakeScreenShot{}
        );
    }
};

struct FloorPlanViewStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / []{} = state<class FloorPlanViewState>
        );
    }
};

struct ExploreEditStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / []{} = state<class ExploreEditState>

            ,state<class ExploreEditState> + event<OnUndoEvent> / UndoExploreAction{}
            ,state<class ExploreEditState> + event<OnRedoEvent> / RedoExploreAction{}

            ,state<class ExploreEditState> + event<OnTickControlKeyEvent> / TickControlKey{}
            ,state<class ExploreEditState> + event<OnFirstTimeTouchDownWithModKeyCtrlEvent> / FirstTimeTouchDownWithModKeyCtrl{}
            ,state<class ExploreEditState> + event<OnTouchMoveWithModKeyCtrlEvent> / TouchMoveWithModKeyCtrl{}
            ,state<class ExploreEditState> + event<OnTouchUpWithModKeyCtrlEvent> / TouchUpWithModKeyCtrl{}

            ,state<class ExploreEditState> + event<OnSpaceEvent> / SpaceToggle{}
            ,state<class ExploreEditState> + event<OnDeleteEvent> / DeleteSelected{}
        );
    }
};

struct ExploreStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / []{} = state<class ExploreState>
            ,state<class ExploreState> + event<OnWhichRoomAmIEvent> / WhichRoomAmI{}
            ,state<class ExploreState> + event<OnPushTourPathEvent> / PushTourPath{}
            ,state<class ExploreState> + event<OnPushKeyFrameTourPathEvent> / PushKeyFrameTourPath{}
            ,state<class ExploreState> + event<OnPopTourPathEvent> / PopTourPath{}
            ,state<class ExploreState> + event<OnMakeHouse3dEvent> / MakeHouse3d{}
            ,state<class ExploreState> + event<OnAddFurnitureSingleEvent> / AddFurnitureSingle{}
            ,state<class ExploreState> + event<OnTakeScreenShotEvent> / TakeScreenShot{}

            // Mouse/Camera events
            ,state<class ExploreState> + event<OnTickEvent> / Tick{}
            ,state<class ExploreState> + event<OnFirstTimeTouchDownEvent> / FirstTimeTouchDown{}
            ,state<class ExploreState> + event<OnTouchUpEvent> / TouchUp{}
            ,state<class ExploreState> + event<OnSingleTapEvent> / SingleTap{}

            // Sub state machines
            ,state<class ExploreState> + event<OnExploreEditEnterEvent> / []{} = state<ExploreEditStateMachine>
        );
    }
};

struct DollyHouseStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / []{} = state<class DollyHouseState>
        );
    }
};

struct TopDownStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / []{} = state<class TopDownState>
        );
    }
};

struct OrbitModeStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
                *state<class Initial> / []{} = state<class OrbitModeState>
        );
    }
};
