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


class RemoteEntitySelector {
public:

    ResourceMetadataListCallback resListCallback() {
        if ( resourceGroup == ResourceGroup::Material ) {
            return [this]( CRefResourceMetadataList el ) {
                metadataMaterialList = el;
            };
        }
        if ( resourceGroup == ResourceGroup::Geom ) {
            return [this]( CRefResourceMetadataList el ) {
                metadataGeomList = el;
            };
        }
        return nullptr;
    };

    void prepare( GHTypeT _label, const std::string& _presets, const std::string& _resourceGroup, int _defaultTab ) {
        label = _label;
        resourceGroup = _resourceGroup;
        originalTabIndex = _defaultTab;
        defaultTabIndex = _defaultTab;
        auto presets =
                resourceGroup != ResourceGroup::Geom ? defaultMaterialAndColorPropertyPresetsForGHType(_label)
                                                         : _presets;
        if ( !presets.empty() ) {
            ResourceMetaData::getListOf(resourceGroup, presets, resListCallback());
        }
    }

    static std::vector<std::string>
    tagsSanitisedFor( const std::string& query, const std::string& group, const std::vector<std::string>& tags ) {
        auto ret = tags;
        erase_if(ret, [query]( const auto& elem ) -> bool {
            return elem == query;
        });
        if ( group == ResourceGroup::Material ) {
            erase_if(ret, []( const auto& elem ) -> bool {
                return elem == "sbsar";
            });
        }
        return ret;
    }

    template<typename R>
    void update( ArchOrchestrator& asg, bool isWidgetOpen, const std::string& mediaFolder, RenderOrchestrator& rsg,
                 R *_resource ) {

//        ImGui::ShowDemoWindow();

        if ( !_resource || resourceGroup.empty() || !isWidgetOpen ) return;

        static bool no_titlebar = true;
        static bool no_scrollbar = false;
        static bool no_menu = true;
        static bool no_move = true;
        static bool no_resize = true;
        static bool no_collapse = true;
        static bool no_nav = true;
        static bool no_background = true;
        static bool no_bring_to_front = false;
        static bool no_docking = true;

        ImGuiWindowFlags window_flags = 0u;
        if ( no_titlebar ) window_flags |= ImGuiWindowFlags_NoTitleBar;
        if ( no_scrollbar ) window_flags |= ImGuiWindowFlags_NoScrollbar;
        if ( !no_menu ) window_flags |= ImGuiWindowFlags_MenuBar;
        if ( no_move ) window_flags |= ImGuiWindowFlags_NoMove;
        if ( no_resize ) window_flags |= ImGuiWindowFlags_NoResize;
        if ( no_collapse ) window_flags |= ImGuiWindowFlags_NoCollapse;
        if ( no_nav ) window_flags |= ImGuiWindowFlags_NoNav;
        if ( no_background ) window_flags |= ImGuiWindowFlags_NoBackground;
        if ( no_bring_to_front ) window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        if ( no_docking ) window_flags |= ImGuiWindowFlags_NoDocking;

        auto viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2{ viewport->Size.x * 0.48f, viewport->Size.y * 0.25f });
        ImGui::SetNextWindowSize(ImVec2{ viewport->Size.x * 0.5f, viewport->Size.y * 0.5f });
        ImGui::Begin("Entity", nullptr, window_flags);
        auto wWidth = ImGui::GetWindowWidth();
        auto windowPadding = ImGui::GetStyle().WindowPadding.x;
        auto framePadding = ImGui::GetStyle().FramePadding.x;

        if ( resourceGroup == ResourceGroup::Geom ) {
            static char query[256] = { '\0' };
            ImGui::SetNextItemWidth(-1.0f);
            if ( ImGui::InputTextWithHint("", "Search for...(IE: \"coffee,table\")", query, 256,
                                          ImGuiInputTextFlags_EnterReturnsTrue) ) {
                ResourceMetaData::getListOf(resourceGroup, query, resListCallback());
            }
            if ( !metadataGeomList.empty() ) {
                int grouping = 3;
                for ( auto m = 0u; m < metadataGeomList.size(); m += 3 ) {
                    ImGui::NewLine();
                    for ( int t = 0; t < grouping; t++ ) {
                        if ( t > 0 ) ImGui::SameLine();
                        if ( m + t >= metadataGeomList.size() ) break;
                        const auto& meta = metadataGeomList[m + t];
                        if ( !meta.thumb.empty() ) {
                            auto imr = rsg.SG().get<RawImage>(meta.thumb);
                            if ( !imr ) {
                                auto fileData = FM::readLocalFileC(
                                        mediaFolder + "entities/" + meta.group + "/" + meta.thumb);
                                if ( !fileData.empty() ) {
                                    rsg.SG().addRawImageIM(meta.thumb, RawImage{ fileData });
                                }
                            }
                        }
                        auto im = rsg.TH(meta.thumb.empty() ? S::WHITE : meta.thumb);
                        if ( im ) {
                            if ( ImGui::ImageButton(ImGuiRenderTexture(im), ImVec2(thumbSize, thumbSize)) ) {
//                                backEnd->process_event(OnAddFurnitureSingleEvent{_resource, FurnitureSet{FT::FT_Sofa, meta.hash, meta.bboxSize, S::SQUARE}});
                            }
                            auto sanitizedTags = tagsSanitisedFor(query, meta.group, meta.tags);
                            if ( ImGui::IsItemHovered() ) {
                                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                                ImGui::BeginTooltip();
                                ImGui::Text("%s", arrayToStringCompact(sanitizedTags).c_str());
                                ImGui::EndTooltip();
                            }
                        }
                    }
                }
            }
        }

        if ( resourceGroup == ResourceGroup::Material || resourceGroup == ResourceGroup::Color ) {

            ImGui::TextColored(ImVec4(0.9, 0.7, 0.2, 1.0), "%s", GHTypeToString(label).c_str());

            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if ( ImGui::BeginTabBar("MyTabBar", tab_bar_flags) ) {

                ImGuiTabItemFlags tMFlags = defaultTabIndex == 1 ? ImGuiTabItemFlags_SetSelected : 0;
                if ( ImGui::BeginTabItem("Material", nullptr, tMFlags) ) {
                    static char query[256] = { '\0' };
                    ImGui::SetNextItemWidth(-1.0f);
                    if ( ImGui::InputTextWithHint("", R"(Filter by...(IE: "wood", "carpet"))", query, 256,
                                                  ImGuiInputTextFlags_EnterReturnsTrue) ) {
                        ResourceMetaData::getListOf(ResourceGroup::Material, query, resListCallback());
                    }

                    materialAndColorTarget = getCommonMaterialChangeMapping(label, _resource);
                    if ( !materialAndColorTarget ) return;

                    if ( !metadataMaterialList.empty() ) {
                        int grouping = ( wWidth - windowPadding * 2 ) / ( thumbSize + framePadding );
                        int matThumbSize = thumbSize - framePadding;
                        for ( auto m = 0u; m < metadataMaterialList.size(); m += grouping ) {
                            for ( int t = 0; t < grouping; t++ ) {
                                if ( t > 0 ) ImGui::SameLine();
                                if ( m + t >= metadataMaterialList.size() ) break;
                                const auto& meta = metadataMaterialList[m + t];
                                auto imr = rsg.SG().get<RawImage>(meta.thumb);
                                if ( !imr ) {
                                    auto fileData = FM::readLocalFileC(
                                            mediaFolder + "entities/" + meta.group + "/" + meta.thumb);
                                    if ( !fileData.empty() ) {
                                        rsg.SG().addRawImageIM(meta.thumb, RawImage{ fileData });
                                    }
                                }
                                auto im = rsg.TH(meta.thumb);
                                if ( im ) {
                                    if ( ImGui::ImageButton(ImGuiRenderTexture(im),
                                                            ImVec2(matThumbSize, matThumbSize)) ) {
                                        materialAndColorTarget->materialHash = meta.hash;
                                        materialAndColorTarget->materialName = meta.name;
                                        asg.make3dHouse([&]() { LOGRS("Spawn an house changing a material") });
                                        //                                backEnd->process_event(OnMakeHouse3dEvent{});
                                    }
                                    auto sanitizedTags = tagsSanitisedFor(query, meta.group, meta.tags);
                                    if ( ImGui::IsItemHovered() ) {
                                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                                        ImGui::BeginTooltip();
                                        ImGui::Text("%s", arrayToStringCompact(sanitizedTags).c_str());
                                        ImGui::EndTooltip();
                                    }
                                }
                            }
                        }
                    }

                    ImGui::EndTabItem();
                }

                ImGuiTabItemFlags tCFlags = defaultTabIndex == 2 ? ImGuiTabItemFlags_SetSelected : 0;
                if ( ImGui::BeginTabItem("Color", nullptr, tCFlags) ) {
                    materialAndColorTarget = getCommonMaterialChangeMapping(label, _resource);

                    if ( metadataColorList.empty() ) {
                        ResourceMetaData::getListOf(ResourceGroup::Color, "yellow",
                                                    [&]( CRefResourceMetadataList el ) {
                                                        metadataColorList = el;
                                                    });
                    }

                    std::vector<std::pair<std::string, C4f>> colors;
                    colors.emplace_back("red", C4f::INDIAN_RED);
                    colors.emplace_back("green", C4f::FOREST_GREEN);
                    colors.emplace_back("black", C4f::DARK_GRAY);

                    colors.emplace_back("blue", C4f::SKY_BLUE);
                    colors.emplace_back("cream", C4f::SAND);
                    colors.emplace_back("grey", C4f::PASTEL_GRAY);

                    colors.emplace_back("orange", C4f::PASTEL_ORANGE);
                    colors.emplace_back("pink", C4f::HOT_PINK);
                    colors.emplace_back("purple", C4f::DARK_PURPLE);

                    colors.emplace_back("teal", C4f::PASTEL_CYAN);
                    colors.emplace_back("white", C4f::LIGHT_GREY);
                    colors.emplace_back("yellow", C4f::PASTEL_YELLOW);

                    constexpr int colorFamilyThumbSize = 32;
                    for ( auto m = 0u; m < colors.size(); m++ ) {
                        const auto& color = colors[m];
                        if ( ImGui::ColorButton(color.first.c_str(),
                                                ImVec4(color.second.x(), color.second.y(), color.second.z(), 1.0f), 0,
                                                ImVec2(colorFamilyThumbSize, colorFamilyThumbSize)) ) {
                            ResourceMetaData::getListOf(ResourceGroup::Color, color.first,
                                                        [&]( CRefResourceMetadataList el ) {
                                                            metadataColorList = el;
                                                        });
                        }
                        if ( ImGui::IsItemHovered() ) {
                            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                            ImGui::SetTooltip("%s", color.first.c_str());
                        }
                        ImGui::SameLine();
                    }

                    if ( !metadataColorList.empty() ) {
                        constexpr int thumbSizeMedium = 64;
                        int grouping = ( wWidth - windowPadding * 2 ) / ( thumbSizeMedium + framePadding );
                        int colThumbSize = thumbSizeMedium - framePadding;

                        for ( auto m = 0u; m < metadataColorList.size(); m += grouping ) {
                            ImGui::NewLine();
                            for ( int t = 0; t < grouping; t++ ) {
                                if ( m + t >= metadataColorList.size() ) break;
                                const auto& meta = metadataColorList[m + t];
                                if ( ImGui::ColorButton(meta.color.toString().c_str(),
                                                        ImVec4(meta.color.x(), meta.color.y(), meta.color.z(), 1.0f), 0,
                                                        ImVec2(colThumbSize, colThumbSize)) ) {
                                    materialAndColorTarget->color = meta.color;
                                    materialAndColorTarget->colorHash = meta.hash;
                                    materialAndColorTarget->colorName = meta.name;
                                    asg.make3dHouse([&]() { LOGRS("Spawn an house changing a material color") });
                                }
                                if ( ImGui::IsItemHovered() ) {
                                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                                    ImGui::SetTooltip("%s", meta.name.c_str());
                                    // NDDado: this will make sense when we can just swap the material color, not re-making the whole house
//                                    if ( meta.color != materialAndColorTarget->color ) {
//                                        materialAndColorTarget->color = meta.color;
//                                        materialAndColorTarget->colorHash = meta.hash;
//                                        materialAndColorTarget->colorName = meta.name;
//                                        asg.make3dHouse([&]() { LOGRS("Spawn an house changing a material color") });
//                                    }
                                }
                                ImGui::SameLine();
                            }
                        }
                    }
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }

        defaultTabIndex = 0;
        ImGui::End();
    }

    [[nodiscard]] int groupIndex() const {
        return originalTabIndex;
    }

private:
    GHTypeT label{ GHType::None };
    std::string resourceGroup{};
    int originalTabIndex = 0;
    int defaultTabIndex = 0;
    MaterialAndColorProperty *materialAndColorTarget = nullptr;
    ResourceMetadataList metadataGeomList{};
    ResourceMetadataList metadataMaterialList{};
    ResourceMetadataList metadataColorList{};
};
