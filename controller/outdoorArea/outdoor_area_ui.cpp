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

#include <eh_arch/models/htypes_functions.hpp>
#include <eh_arch/models/house_service.hpp>
#include <eh_arch/render/house_render.hpp>
#include <eh_arch/controller/arch_orchestrator.hpp>
#include <eh_arch/controller/arch_render_controller.hpp>

#include <eh_arch/state_machine/arch_sm_events__fsm.hpp>
#include <eh_arch/controller/outdoorArea/outdoor_area_builder.hpp>

OutdoorAreaUI::OutdoorAreaUI( OutdoorAreaBuilder& bb ) : bb(bb) {

}

void OutdoorAreaUI::update( ArchOrchestrator& asg, RenderOrchestrator& rsg ) {

    if ( !activated ) return;

    ImGui::Begin("Outdoor Area");
    if ( ImGui::Button("Add Boundary") ) {
        boundaryIndex++;
        bb.OutdoorAreaData()->Boundaries().emplace_back(OutdoorBoundary{});
    }
    for ( auto& boundary : bb.OutdoorAreaData()->Boundaries() ) {
        ImGui::InputFloat("Elevation", &boundary.elevation);
        ImGui::InputFloat("Height", &boundary.zPull);
        ImGui::Button("Material");
    }
    ImGui::End();
//    float elevation = 0.0f;
//    float zPull = 0.1f;
//    int extrusionType = 0; // We'll have at least "Extrude" (flat poly) and "Follower" (like a skirting board)
//    MaterialAndColorProperty outdoorBoundaryMaterial{"wood,beech"};
}

void OutdoorAreaUI::activate( bool _flag ) {
    activated = _flag;
    bb.clear();
}

void OutdoorAreaUI::addPoint( const V2f& _p ) {
    bb.addPoint( _p, boundaryIndex );
}

const std::shared_ptr<OutdoorAreaBSData>& OutdoorAreaUI::OutdoorAreaData() const {
    return bb.OutdoorAreaData();
}
