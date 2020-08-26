//
// Created by dado on 25/08/2020.
//

#include "outdoor_area_ui.hpp"

#include <graphics/imgui/imgui.h>
#include <poly/scene_graph.h>
#include <render_scene_graph/render_orchestrator.h>

#include <eh_arch/render/house_render.hpp>
#include <eh_arch/controller/arch_orchestrator.hpp>
#include <eh_arch/controller/arch_render_controller.hpp>

#include <eh_arch/controller/outdoor_area/outdoor_area_builder.hpp>

[[maybe_unused]] OutdoorAreaUI::OutdoorAreaUI( OutdoorAreaBuilder& bb ) : bb(bb) {
}

void OutdoorAreaUI::update( ArchOrchestrator& asg, RenderOrchestrator& rsg ) {

    if ( !activated ) return;

    ImGui::Begin("Outdoor Area");
    if ( ImGui::Button("Add Boundary") ) {
        boundaryIndex++;
        bb.OutdoorAreaData()->Boundaries().emplace_back(OutdoorBoundary{});
    }
    int i = 0;
    for ( auto& boundary : bb.OutdoorAreaData()->Boundaries() ) {
        ImGui::PushID(i++);
        ImGui::SameLine();
        ImGui::RadioButton("Flat", &boundary.extrusionType, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Contour", &boundary.extrusionType, 1);
        ImGui::PopID();
        ImGui::PushID(i++);
        ImGui::InputFloat("Elevation", &boundary.elevation);
        ImGui::PopID();
        ImGui::PushID(i++);
        ImGui::InputFloat("Height", &boundary.zPull);
        ImGui::PopID();
        ImGui::PushID(i++);
        ImGui::InputFloat("Contour Width", &boundary.followerWidth);
        ImGui::PopID();
        ImGui::PushID(i++);
        ImGui::Button("Material");
        ImGui::PopID();
    }
    ImGui::End();
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
