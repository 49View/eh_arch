//
// Created by dado on 23/07/2020.
//

#pragma once

#include <string>
#include <core/math/vector3f.h>
#include <core/math/vector4f.h>
#include <core/math/anim.h>
#include <core/math/plane3f.h>
#include <core/math/aabb.h>
#include <eh_arch/models/htypes.hpp>
#include <eh_arch/models/arch_structural_service.hpp>

class RenderOrchestrator;

struct HouseBSData;
struct AggregatedInputData;
struct InputMods;
struct FittedFurniture;

static constexpr float explorerFullDotOpacityValue = 0.75f;
static constexpr float explorerDotFadeTime = 0.15f;

class ArchExplorer {
public:
    ArchExplorer() = default;
    void tickControlKey( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg );

    // Events
    void touchMoveWithModKeyCtrl( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg );
    void firstTimeTouchDownCtrlKey( const V3f& _dir, RenderOrchestrator& rsg );
    bool touchUpWithModKeyCtrl( RenderOrchestrator& rsg );
    void spaceToggle( RenderOrchestrator& rsg );
    void deleteSelected( RenderOrchestrator& rsg );
    void cloneSelected( HouseBSData *_house, RenderOrchestrator& rsg );
    void replaceFurnitureWithOneOfItsKind( RenderOrchestrator& rsg );

private:
    void updateFurnitureSelection( RenderOrchestrator& rsg, const V3f& centerBottomPos, const C4f& _dotColor );
    [[nodiscard]] bool isMouseOverFurnitureInnerSelector( const V3f& _origin, const V3f& _dir ) const;

private:
    FeatureIntersection fd;

    FadeInOutSwitch furnitureSelectionAlphaAnim{ explorerFullDotOpacityValue, explorerDotFadeTime };
    FadeInOutSwitch positionalDotAlphaAnim{ explorerFullDotOpacityValue, explorerDotFadeTime };
    bool bRoomBboxCheck = false;
    bool bFurnitureTargetLocked = false;
    bool bFillFullFurnitureOutline = false;
    bool bFurnitureDirty = false;
    Plane3f furniturePlane;
    FittedFurniture *furnitureSelected = nullptr;
    V3f centerBottomFurnitureSelected{ V3f::ZERO };
    std::vector<V3f> furnitureSelectionOutline;
    JMATH::AABB centerBottomBBox;
    V3f prevFurnitureMovePosition{};
    FeatureIntersection fdFurniture;
};
