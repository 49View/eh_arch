//
// Created by dado on 05/07/2020.
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

static constexpr float fullDotOpacityValue = 0.75f;
static constexpr float dotFadeTime = 0.15f;

class ArchPositionalDot {
public:
    ArchPositionalDot() = default;
    void tick( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg );

    // Events
    void firstTimeTouchDown();
    void touchUp();
    void singleTap( RenderOrchestrator& rsg );
private:
    void updateDot( RenderOrchestrator& rsg );

private:
    FadeInOutSwitch positionalDotAlphaAnim{ fullDotOpacityValue, dotFadeTime };
    bool isFlying = false;
    bool antiWallRotation = false;
    V3f hitPosition{ V3f::ZERO };
    FeatureIntersection fd;
};



