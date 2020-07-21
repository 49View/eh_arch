//
// Created by dado on 05/07/2020.
//

#pragma once

#include <string>
#include <core/math/vector3f.h>
#include <core/math/vector4f.h>
#include <core/math/anim_type.hpp>
#include <core/math/plane3f.h>
#include <eh_arch/models/htypes.hpp>
#include <eh_arch/models/arch_structural_service.hpp>

class RenderOrchestrator;
struct HouseBSData;
struct AggregatedInputData;
struct InputMods;
struct FittedFurniture;

class ArchPositionalDot {
public:
    ArchPositionalDot();
    void tick( const HouseBSData* _house, const V3f& _dir, RenderOrchestrator& rsg, const C4f& _dotColor = C4f::SKY_BLUE );
    void tickControlKey( const HouseBSData* _house, const V3f& _dir, RenderOrchestrator& rsg );

    // Events
    void touchMoveWithModKeyCtrl(const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg);
    void firstTimeTouchDown();
    void firstTimeTouchDownCtrlKey(const V3f& _dir, RenderOrchestrator& rsg);
    void touchUp();
    void touchUpWithModKeyCtrl();
    void singleTap(RenderOrchestrator& rsg);

private:
    void fadeInDot();
    void fadeOutDot();
    void fadeInFurntiture();
    void fadeOutFurniture();
    void updateDot( RenderOrchestrator& rsg, const C4f& _dotColor );
    void updateFurnitureSelection( RenderOrchestrator& rsg, const V3f& centerBottomPos );

private:
    floata positionalDotAlphaAnim;
    std::string positionalDotAlphaFadeInAnimKey;
    std::string positionalDotAlphaFadeOutAnimKey;
    bool isFlying = false;
    bool antiWallRotation = false;
    bool bRoomBboxCheck = false;
    V3f hitPosition{V3f::ZERO};
    FeatureIntersection fd;

    floata furniturePositionalDotAlphaAnim;
    std::string furniturePositionalDotAlphaFadeInAnimKey;
    std::string furniturePositionalDotAlphaFadeOutAnimKey;
    bool bBullsEye = false;
    bool bFurnitureTargetLocked = false;
    Plane3f furniturePlane;
    FittedFurniture* furnitureSelected = nullptr;
    V3f centerBottomFurnitureSelected{V3f::ZERO};
    V3f  prevFurnitureMovePosition{};
    FeatureIntersection fdFurniture;
};



