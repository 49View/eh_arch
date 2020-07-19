//
// Created by dado on 05/07/2020.
//

#pragma once

#include <string>
#include <core/math/anim_type.hpp>
#include <eh_arch/models/arch_structural_service.hpp>

class RenderOrchestrator;
struct HouseBSData;
struct AggregatedInputData;

class ArchPositionalDot {
public:
    ArchPositionalDot();
    void update( const HouseBSData* _house, const AggregatedInputData& _aid, RenderOrchestrator& rsg );

private:
    floata positionalDotAlphaAnim;
    std::string positionalDotAlphaFadeInAnimKey;
    std::string positionalDotAlphaFadeOutAnimKey;
    bool positionChangedIn = false;
    bool positionChangedOut = false;
    bool currentHit = false;
    bool isFlying = false;
    bool antiWallRotation = false;
    bool bHitFurniture = false;
    V3f  prevFurnitureMovePosition{};
    FeatureIntersection fd{};
    FeatureIntersection fdFurniture{};
};



