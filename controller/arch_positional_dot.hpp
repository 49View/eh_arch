//
// Created by dado on 05/07/2020.
//

#pragma once

#include <string>
#include <core/math/vector3f.h>
#include <core/math/vector4f.h>
#include <core/math/anim_type.hpp>
#include <core/math/plane3f.h>
#include <core/math/aabb.h>
#include <eh_arch/models/htypes.hpp>
#include <eh_arch/models/arch_structural_service.hpp>

class RenderOrchestrator;

struct HouseBSData;
struct AggregatedInputData;
struct InputMods;
struct FittedFurniture;

enum class FadeInternalPhase {
    In,
    Out
};

struct FadeInOutContainer {
    FadeInOutContainer();

    [[nodiscard]] float value() const;
    void setValue( float _value);
    floata& anim();

    void fade(FadeInternalPhase phase);
    void fadeIn();
    void fadeOut();

    std::string inPhaseId;
    std::string outPhaseId;
private:
    floata floatAnim;
};

class ArchPositionalDot {
public:
    ArchPositionalDot() = default;
    void tick( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg, bool isComingFromInternal = false,
               const C4f& _dotColor = C4f::SKY_BLUE );
    void tickControlKey( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg );

    // Events
    void touchMoveWithModKeyCtrl( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg );
    void firstTimeTouchDown();
    void firstTimeTouchDownCtrlKey( const V3f& _dir, RenderOrchestrator& rsg );
    void touchUp();
    void touchUpWithModKeyCtrl();
    void singleTap( RenderOrchestrator& rsg );
    void spaceToggle( RenderOrchestrator& rsg );
    void deleteSelected( RenderOrchestrator& rsg );
private:
    void updateDot( RenderOrchestrator& rsg, const C4f& _dotColor );
    void updateFurnitureSelection( RenderOrchestrator& rsg, const V3f& centerBottomPos, const C4f& _dotColor );

    [[nodiscard]] bool isMouseOverFurnitureInnerSelector( const V3f& _origin, const V3f& _dir ) const;

private:
    FadeInOutContainer positionalDotAlphaAnim;
    bool isFlying = false;
    bool antiWallRotation = false;
    bool bRoomBboxCheck = false;
    V3f hitPosition{ V3f::ZERO };
    FeatureIntersection fd;

    FadeInOutContainer furnitureSelectionAlphaAnim;
    bool bFurnitureTargetLocked = false;
    bool bFillFullFurnitureOutline = false;
    Plane3f furniturePlane;
    FittedFurniture *furnitureSelected = nullptr;
    V3f centerBottomFurnitureSelected{ V3f::ZERO };
    std::vector<V3f> furnitureSelectionOutline;
    JMATH::AABB centerBottomBBox;
    V3f prevFurnitureMovePosition{};
    FeatureIntersection fdFurniture;
};



