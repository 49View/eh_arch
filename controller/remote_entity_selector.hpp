//
// Created by dado on 20/06/2020.
//

#pragma once

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

namespace MaterialAndColorPropertyChangeScope {
    static constexpr int ScopeSingle = 0;
    static constexpr int ScopeRoom = 1;
    static constexpr int ScopeHouse = 2;
};

using MaterialAndColorPropertyChangeScopeT = int;

class RemoteEntitySelector {
public:

    ResourceMetadataListCallback resListCallback();
    void prepare( const FeatureIntersection& _fd, const std::string& _presets, const std::string& _resourceGroup,
                  int _defaultTab );
    static std::vector<std::string>
    tagsSanitisedFor( const std::string& query, const std::string& group, const std::vector<std::string>& tags );
    void update( ArchOrchestrator& asg, const std::string& mediaFolder, RenderOrchestrator& rsg );
    [[nodiscard]] int groupIndex() const;

private:
    void injectColor( ArchOrchestrator& asg, const EntityMetaData& meta );
    void injectMaterial( ArchOrchestrator& asg, const EntityMetaData& meta );
    void applyInjection( ArchOrchestrator& asg );
private:
    FeatureIntersection fd;
    GHTypeT label{ GHType::None };
    std::string resourceGroup{};
    int originalTabIndex = 0;
    int defaultTabIndex = 0;
    MaterialAndColorPropertyChangeScopeT changeScope = MaterialAndColorPropertyChangeScope::ScopeRoom;
    MaterialAndColorProperty *materialAndColorTarget = nullptr;
    ResourceMetadataList metadataGeomList{};
    ResourceMetadataList metadataMaterialList{};
    ResourceMetadataList metadataColorList{};
};
