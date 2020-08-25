//
// Created by dado on 25/08/2020.
//

#include "outdoor_area_ui.hpp"

#include <core/util.h>
#include <core/resources/resource_metadata.hpp>
#include <graphics/imgui/imgui.h>
#include <graphics/imgui/im_gui_utils.h>
#include <poly/scene_graph.h>
#include <render_scene_graph/render_orchestrator.h>

#include <eh_arch/models/house_service.hpp>
#include <eh_arch/render/house_render.hpp>
#include <eh_arch/controller/arch_orchestrator.hpp>
#include <eh_arch/controller/arch_render_controller.hpp>
#include <eh_arch/models/htypes_functions.hpp>

#include "eh_arch/state_machine/arch_sm_events__fsm.hpp"

void OutdoorAreaUI::update( ArchOrchestrator& asg, RenderOrchestrator& rsg ) {
    ImGui::Begin("Outdoor Area");
    ImGui::Text("%s", "lalala");
    ImGui::End();
}
