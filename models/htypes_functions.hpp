//
// Created by dado on 20/06/2020.
//

#pragma once

#include "htypes.hpp"
#include "house_bsdata.hpp"
#include "kitchen_room_service.hpp"
#include <core/resources/material_and_color_property.hpp>

template<typename A, typename R>
inline static MaterialAndColorProperty *getCommonMaterialChangeMapping( GHTypeT key, A *arch, R *resource ) {
    if ( key == GHType::Wall ) return &arch->wallMaterial;
    if ( key == GHType::Floor ) return &resource->floorMaterial;
    if ( key == GHType::Skirting ) return &resource->skirtingMaterial;
    if ( key == GHType::Coving ) return &resource->covingMaterial;
    if ( key == GHType::Ceiling ) return &resource->ceilingMaterial;

    // This can be optional
    if ( KitchenRoomService::hasKitchen( resource ) ) {
        if ( key == GHType::KitchenWorktop ) return &resource->kitchenData.worktopMaterial;
        if ( key == GHType::KitchenCabinet ) return &resource->kitchenData.unitsMaterial;
        if ( key == GHType::KitchenBackSplash ) return &resource->kitchenData.backSplashMaterial;
    }
    return nullptr;
}

std::string GHTypeToString( uint64_t elem );
std::string defaultMaterialAndColorPropertyPresetsForGHType( GHTypeT key );
