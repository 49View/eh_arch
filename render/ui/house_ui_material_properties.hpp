//
// Created by dado on 26/07/2020.
//

#pragma once

#include <core/resources/material_and_color_property.hpp>
#include <graphics/imgui/imgui.h>
#include <graphics/imgui/im_gui_utils.h>

static inline void slimMaterialAndColorPropertyMemo( const V2f& pos, const MaterialAndColorProperty& mcp, unsigned int guiTexture ) {

    static bool no_titlebar = true;
    static bool no_scrollbar = true;
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

    static constexpr float colThumbSize = 128.f;
    auto viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{ pos.x(), pos.y() });
    ImGui::SetNextWindowSize(ImVec2{ viewport->Size.x * 0.5f, 200.0f });
    ImGui::Begin("slimMaterialAndColorPropertyMemo", nullptr, window_flags);
    ImGui::Text("%s", mcp.materialName.c_str());

    ImGui::ImageButton( ImGuiRenderTexture(guiTexture), ImVec2(colThumbSize, colThumbSize));
    ImGui::SameLine();
    ImGui::ImageButton( ImGuiRenderTexture(guiTexture), ImVec2(colThumbSize, colThumbSize), ImVec2(0,0), ImVec2(1,1), -1, ImVec4(0,0,0,0), ImVec4(mcp.color.x(), mcp.color.y(), mcp.color.z(), 1.0f) );

    ImGui::End();
}
