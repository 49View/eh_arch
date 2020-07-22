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
            ,state<class ExploreState> + event<OnTickControlKeyEvent> / TickControlKey{}
            ,state<class ExploreState> + event<OnFirstTimeTouchDownEvent> / FirstTimeTouchDown{}
            ,state<class ExploreState> + event<OnFirstTimeTouchDownWithModKeyCtrlEvent> / FirstTimeTouchDownWithModKeyCtrl{}
            ,state<class ExploreState> + event<OnTouchMoveWithModKeyCtrlEvent> / TouchMoveWithModKeyCtrl{}
            ,state<class ExploreState> + event<OnTouchUpEvent> / TouchUp{}
            ,state<class ExploreState> + event<OnTouchUpWithModKeyCtrlEvent> / TouchUpWithModKeyCtrl{}
            ,state<class ExploreState> + event<OnSingleTapEvent> / SingleTap{}
            // Basic House customisation
            ,state<class ExploreState> + event<OnSpaceEvent> / SpaceToggle{}
            ,state<class ExploreState> + event<OnDeleteEvent> / DeleteSelected{}
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
