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

    if ( !bb.isActive() ) return;

    ImGui::Begin("Outdoor Area");
    if ( !bb.empty() ) {
        if ( ImGui::Button("Garden") ) {
            bb.makeGarden();
        }
        ImGui::SameLine();
        if ( ImGui::Button("Balcony") ) {
            bb.makeBalcony();
        }
        ImGui::SameLine();
        if ( ImGui::Button("Terrace") ) {
            bb.makeTerrace();
        }
        ImGui::Separator();
        if ( ImGui::Button("Add Boundary") ) {
            boundaryIndex++;
            bb.addBoundary(OutdoorBoundary{});
        }
    }
    int i = 0;
    int bc = 0;
    for ( auto& boundary : bb.Boundaries() ) {
        ImGui::PushID(i++);
        ImGui::SameLine();
        if ( ImGui::RadioButton("Flat", &boundary.extrusionType, 0) ) {
            bb.refresh();
        }
        ImGui::SameLine();
        if ( ImGui::RadioButton("Contour", &boundary.extrusionType, 1) ) {
            bb.refresh();
        }
        ImGui::PopID();
        ImGui::PushID(i++);
        if ( ImGui::InputFloat("Elevation", &boundary.elevation) ) {
            bb.refresh();
        }
        ImGui::PopID();
        ImGui::PushID(i++);
        if ( ImGui::InputFloat("Height", &boundary.zPull) ) {
            bb.refresh();
        }
        ImGui::PopID();
        ImGui::PushID(i++);
        if ( ImGui::InputFloat("Contour Width", &boundary.followerWidth) ) {
            bb.refresh();
        }
        ImGui::PopID();
        ImGui::PushID(i++);
        if ( ImGui::Button("Material") ) {
            bb.refresh();
        }
        ImGui::PopID();
        ImGui::PushID(i++);
        if ( ImGui::Button("Clone") ) {
            bb.cloneBoundary(bc);
        }
        ImGui::PopID();
        ImGui::Separator();
        ImGui::Separator();
        ++bc;
    }
    ImGui::End();
}

void OutdoorAreaUI::activate() {
    bb.OutdoorAreaData(EntityFactory::create<OutdoorAreaBSData>());
    bb.clear();
}

void OutdoorAreaUI::deactivate() {
    bb.reset();
}

void OutdoorAreaUI::addPoint( const V2f& _p ) {
    if ( bb.empty() ) {
        activate();
        bb.addBoundary(OutdoorBoundary{});
    }
    bb.addPoint( _p, boundaryIndex );
}

const std::shared_ptr<OutdoorAreaBSData>& OutdoorAreaUI::OutdoorAreaData() const {
    return bb.OutdoorAreaData();
}

void OutdoorAreaUI::undo() {
    bb.undo();
}

void OutdoorAreaUI::redo() {
    bb.redo();
}

