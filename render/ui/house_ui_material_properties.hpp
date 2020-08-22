//
// Created by dado on 26/07/2020.
//

#pragma once

#include <core/resources/material_and_color_property.hpp>
#include <graphics/imgui/imgui.h>
#include <graphics/imgui/im_gui_utils.h>

inline void slimMaterialAndColorPropertyMemo( RenderOrchestrator& rsg, MaterialAndColorProperty* mcp, GenericCallback ccf = nullptr ) {

    auto ims = S::WHITE;
    auto mat = rsg.SG().get<Material>(mcp->materialHash);
    if ( mat ) {
        ims = mat->getDiffuseTexture();
    }

    static constexpr float colThumbSize = 128.f;
    ImGui::Text("%s", mcp->materialName.c_str());
    ImGui::Text("%s", mcp->colorName.c_str());

    if ( ImGui::ImageButton(ImGuiRenderTexture(rsg.TH(ims)), ImVec2(colThumbSize, colThumbSize), ImVec2(0, 0),
                            ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0),
                            ImVec4(mcp->color.x(), mcp->color.y(), mcp->color.z(), 1.0f)) ) {
        if ( ccf ) ccf();
    }
}
