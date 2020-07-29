//
// Created by dado on 27/07/2020.
//

#include "remote_entity_selector.hpp"
#include <graphics/render_light_manager.h>
#include <eh_arch/render/ui/house_ui_material_properties.hpp>

int RemoteEntitySelector::groupIndex() const {
    return originalTabIndex;
}

ResourceMetadataListCallback RemoteEntitySelector::resListCallback() {
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
}

void RemoteEntitySelector::prepare( const FeatureIntersection& _fd, const FeatureIntersection& _fdfurniture,
                                    const std::string& _presets, const std::string& _resourceGroup, int _defaultTab ) {
    fd = _fd;
    fdfurniture = _fdfurniture;
    label = fd.intersectedType;
    resourceGroup = _resourceGroup;
    originalTabIndex = _defaultTab;
    defaultTabIndex = _defaultTab;
    auto presets =
            resourceGroup != ResourceGroup::Geom ? defaultMaterialAndColorPropertyPresetsForGHType(label)
                                                 : _presets;
    if ( !presets.empty() ) {
        ResourceMetaData::getListOf(resourceGroup, presets, resListCallback());
    }

    materialAndColorTarget = getCommonMaterialChangeMapping(label, fd.archSegment, fd.room, bKichenElementSelected );
}

std::vector<std::string> RemoteEntitySelector::tagsSanitisedFor( const std::string& query, const std::string& group,
                                                                 const std::vector<std::string>& tags ) {
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

void RemoteEntitySelector::applyInjection( ArchOrchestrator& asg ) {

    if ( !bKichenElementSelected ) {
        if ( changeScope == MaterialAndColorPropertyChangeScope::ScopeRoom ) {
            if ( fd.intersectedType == GHType::Floor ) {
                RoomService::changeFloorsMaterial(fd.room, *materialAndColorTarget);
            } else if ( fd.intersectedType == GHType::Wall ) {
                RoomService::changeWallsMaterial(fd.room, *materialAndColorTarget);
            } else if ( fd.intersectedType == GHType::Ceiling ) {
                RoomService::changeCeilingsMaterial(fd.room, *materialAndColorTarget);
            } else if ( fd.intersectedType == GHType::Skirting ) {
                RoomService::changeSkirtingsMaterial(fd.room, *materialAndColorTarget);
            } else if ( fd.intersectedType == GHType::Coving ) {
                RoomService::changeCovingsMaterial(fd.room, *materialAndColorTarget);
            }
        }
        if ( changeScope == MaterialAndColorPropertyChangeScope::ScopeHouse ) {
            if ( fd.intersectedType == GHType::Floor ) {
                HouseService::changeFloorsMaterial(asg.H(), *materialAndColorTarget);
            } else if ( fd.intersectedType == GHType::Wall ) {
                HouseService::changeWallsMaterial(asg.H(), *materialAndColorTarget);
            } else if ( fd.intersectedType == GHType::Ceiling ) {
                HouseService::changeCeilingsMaterial(asg.H(), *materialAndColorTarget);
            } else if ( fd.intersectedType == GHType::Skirting ) {
                HouseService::changeSkirtingsMaterial(asg.H(), *materialAndColorTarget);
            } else if ( fd.intersectedType == GHType::Coving ) {
                HouseService::changeCovingsMaterial(asg.H(), *materialAndColorTarget);
            }
        }
    }

    asg.make3dHouse([&]() { LOGRS("Spawn an house changing a material color") });
    asg.pushHouseChange();
}

void RemoteEntitySelector::injectMaterial( ArchOrchestrator& asg, const EntityMetaData& meta ) {
    materialAndColorTarget->materialHash = meta.hash;
    materialAndColorTarget->materialName = meta.name;
    if ( materialAndColorTarget->colorHash.empty() ) {
        materialAndColorTarget->color = fd.room->wallsMaterial.color;
        materialAndColorTarget->colorHash = fd.room->wallsMaterial.colorHash;
        materialAndColorTarget->colorName = fd.room->wallsMaterial.colorName;
    }
    applyInjection(asg);
}

void RemoteEntitySelector::injectColor( ArchOrchestrator& asg, const EntityMetaData& meta ) {
    if ( materialAndColorTarget->materialHash.empty() ) {
        materialAndColorTarget->materialHash = fd.room->wallsMaterial.materialHash;
        materialAndColorTarget->materialName = fd.room->wallsMaterial.materialName;
    }
    materialAndColorTarget->color = meta.color;
    materialAndColorTarget->colorHash = meta.hash;
    materialAndColorTarget->colorName = meta.name;
    applyInjection(asg);
}

void RemoteEntitySelector::addNewFurniture( ArchOrchestrator& asg, EntityMetaData meta ) {
    auto ff = FittedFurniture{ { meta.hash, meta.bboxSize },"sofa", S::SQUARE };
    auto clonedFurniture = EntityFactory::clone(ff);

    auto hitBestPoint = fd.nearV < fdfurniture.nearV ? fd.hitPosition : fdfurniture.hitPosition;
    float heightOffset = fd.nearV < fdfurniture.nearV ? 0.0f : fdfurniture.arch->bbox3d.calcHeight();
    V2f pos = XZY::C2(hitBestPoint);// + depthOffset;
    auto f = HouseService::findFloorOf(asg.H(), fd.room->hash);
    RS::placeManually(FurnitureRuleParams{ f, fd.room, clonedFurniture, pos, heightOffset, FRPSource{reinterpret_cast<FittedFurniture*>(fdfurniture.arch)},
                                           FRPFurnitureRuleFlags{ forceManualFurnitureFlags } });
    asg.make3dHouse([]() {});
    asg.pushHouseChange();
}

void RemoteEntitySelector::update( ArchOrchestrator& asg, const std::string& mediaFolder, RenderOrchestrator& rsg ) {

//    ImGui::ShowDemoWindow();

    static bool no_titlebar = true;
    static bool no_scrollbar = false;
    static bool no_menu = true;
    static bool no_move = true;
    static bool no_resize = true;
    static bool no_collapse = true;
    static bool no_nav = true;
    static bool no_background = false;
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
    auto wSize = max(viewport->Size.y * 0.2f, 300.0f);
    ImGui::SetNextWindowPos(ImVec2{ 0.0f, viewport->Size.y - wSize });
    ImGui::SetNextWindowSize(ImVec2{ viewport->Size.x, wSize });
    ImGui::Begin("Entity", nullptr, window_flags);
    auto wWidth = ImGui::GetWindowWidth();
    auto windowPadding = ImGui::GetStyle().WindowPadding.x;
    auto framePadding = ImGui::GetStyle().FramePadding.x;

    if ( resourceGroup == ResourceGroup::Geom ) {

        float columnCurrentMcp = max(200.0f, wWidth * 0.15f);
        float columnOptionsMcp = wWidth - columnCurrentMcp;
        ImGui::Columns(2, "mcpCols");
        ImGui::SetColumnWidth(0, columnCurrentMcp);
        ImGui::SetColumnWidth(1, columnOptionsMcp);

        ImGui::NextColumn();

        static char query[256] = { '\0' };
        ImGui::SetNextItemWidth(-1.0f);
        if ( ImGui::InputTextWithHint("", "Search for...(IE: \"coffee,table\")", query, 256,
                                      ImGuiInputTextFlags_EnterReturnsTrue) ) {
            ResourceMetaData::getListOf(resourceGroup, query, resListCallback());
        }
        if ( !metadataGeomList.empty() ) {
            int grouping =
                    static_cast<int>(( columnOptionsMcp - windowPadding * 2 ) / ( thumbSize + framePadding )) -
                    1;
            for ( auto m = 0u; m < metadataGeomList.size(); m += grouping ) {
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
                            addNewFurniture(asg, meta);
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

        ImGui::TextColored(ImVec4(0.7, 0.7, 0.4, 1.0), "%s", RoomService::roomName(fd.room).c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.9, 0.7, 0.2, 1.0), "%s", GHTypeToString(label).c_str());
        ImGui::SameLine();
        ImGui::RadioButton("Single", &changeScope, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Room", &changeScope, 1);
        ImGui::SameLine();
        ImGui::RadioButton("House", &changeScope, 2);

        float columnCurrentMcp = max(200.0f, wWidth * 0.15f);
        float columnOptionsMcp = wWidth - columnCurrentMcp;
        ImGui::Columns(2, "mcpCols");
        ImGui::SetColumnWidth(0, columnCurrentMcp);
        ImGui::SetColumnWidth(1, columnOptionsMcp);
        slimMaterialAndColorPropertyMemo( rsg, materialAndColorTarget );

        if ( RS::hasRoomType( fd.room, ASType::Kitchen ) ) {
            if (ImGui::Button("Kitchen worktop")) {
                bKichenElementSelected = true;
                materialAndColorTarget = &fd.room->kitchenData.worktopMaterial;
            }
            if (ImGui::Button("Kitchen units")) {
                bKichenElementSelected = true;
                materialAndColorTarget = &fd.room->kitchenData.unitsMaterial;
            }
            if (ImGui::Button("Kitchen back splash")) {
                bKichenElementSelected = true;
                materialAndColorTarget = &fd.room->kitchenData.backSplashMaterial;
            }
        }
        int lightIndex = 1;
        for ( const auto& roomLight : fd.room->mLightFittings ) {
            std::string lightLabel = "Light " + std::to_string(lightIndex++);
            auto renderLight = rsg.RR().LM()->findPointLight( roomLight.key );
            if ( renderLight ) {
                if ( ImGui::Checkbox(lightLabel.c_str(), &renderLight->GoingUp()) ) {
                    renderLight->updateLightIntensityAfterToggle();
                }
            }
        }

        ImGui::NextColumn();

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

                if ( !metadataMaterialList.empty() ) {
                    int grouping =
                            static_cast<int>(( columnOptionsMcp - windowPadding * 2 ) / ( thumbSize + framePadding )) -
                            1;
                    float matThumbSize = thumbSize - framePadding;
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
                                if ( ImGui::ImageButton(ImGuiRenderTexture(im), ImVec2(matThumbSize, matThumbSize)) ) {
                                    injectMaterial(asg, meta);
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

                if ( metadataColorList.empty() ) {
                    ResourceMetaData::getListOf(ResourceGroup::Color, "yellow",
                                                [&]( CRefResourceMetadataList el ) { metadataColorList = el; });
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
                for ( auto& color : colors ) {
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
                    constexpr float thumbSizeMedium = 64.0f;
                    int grouping = static_cast<int>(( columnOptionsMcp - windowPadding * 2 ) /
                                                    ( thumbSizeMedium + framePadding )) - 1;
                    float colThumbSize = thumbSizeMedium - framePadding;

                    for ( auto m = 0u; m < metadataColorList.size(); m += grouping ) {
                        ImGui::NewLine();
                        for ( int t = 0; t < grouping; t++ ) {
                            if ( m + t >= metadataColorList.size() ) break;
                            const auto& meta = metadataColorList[m + t];
                            if ( ImGui::ColorButton(meta.color.toString().c_str(),
                                                    ImVec4(meta.color.x(), meta.color.y(), meta.color.z(), 1.0f), 0,
                                                    ImVec2(colThumbSize, colThumbSize)) ) {
                                injectColor(asg, meta);
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
