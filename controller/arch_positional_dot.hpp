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
struct InputMods;

class ArchPositionalDot {
public:
    ArchPositionalDot();
    void tick( const HouseBSData* _house, const V3f& _dir, const InputMods& _mods, RenderOrchestrator& rsg );

    // Events
    void updateRender( RenderOrchestrator& rsg );
    void touchMoveWithModKeyCtrl(const HouseBSData *_house, RenderOrchestrator& rsg);
    void firstTimeTouchDown();
    void touchUp();
    void singleTap(RenderOrchestrator& rsg);

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
    bool bRoomBboxCheck = false;
    FeatureIntersectionFlagsT featureIntersectionFlags = 0;
    V3f  prevFurnitureMovePosition{};
    V3f dir{V3f::ZERO};
    V3f hitPosition{V3f::ZERO};
    FeatureIntersection fd{};
    FeatureIntersection fdFurniture{};
};



