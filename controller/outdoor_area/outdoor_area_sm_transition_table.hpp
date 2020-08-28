//
// Created by dado on 06/07/2020.
//

#pragma once

#include <core/state_machine_helper.hpp>
#include <eh_arch/state_machine/arch_sm_events__fsm.hpp>
#include <eh_arch/makers/arch_sm_actions_bespoke_builder.hpp>
#include <eh_arch/makers/arch_sm_actions_maker_builder.hpp>
#include <eh_arch/controller/outdoor_area/outdoor_area_sm_actions.hpp>

struct OutdoorAreaStateMachine {
    auto operator()() const noexcept {
        return make_transition_table(
            *state<class Initial> / []{} = state<class OutdoorAreaState>
            ,state<OutdoorAreaState> + event<OnSingleTapEvent> / AddPointOutdoorAreaAction{}
            ,state<OutdoorAreaState> + event<OnUndoEvent> / UndoOutdoorArea{}
            ,state<OutdoorAreaState> + event<OnRedoEvent> / RedoOutdoorArea{}
        );
    }
};