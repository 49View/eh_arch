//
// Created by dado on 04/06/2020.
//

#pragma once

#include <core/math/vector2f.h>
#include <eh_arch/state_machine/arch_sm_events.hpp>

struct OnActivateEvent {
    std::function<void()> ccf = nullptr;
};
struct OnAltPressedEvent {};
struct OnClearEvent {};
struct OnDoubleTapEvent {};
struct OnUndoEvent {};
struct OnRedoEvent {};

struct OnLoadFloorPlanEvent {
    PropertyListing property;
};
struct OnCreateNewPropertyFromFloorplanImageEvent {
    std::string floorplanFileName;
};
struct OnImportExcaliburLinkEvent {
    std::string excaliburLink;
};

struct OnCreateHouseTexturesEvent {};
struct OnUpdateHMBEvent {};
struct OnMakeHouse3dEvent {};
struct OnElaborateHouseBitmapEvent {};
struct OnRecalculateFurnitureEvent {};

struct OnHouseMakerToggleEvent{};
struct OnTourToggleEvent{};
struct OnExploreToggleEvent{};
struct OnTopDownToggleEvent{};
struct OnDollyHouseToggleEvent{};

struct OnFirstTimeTouchDownEvent {
    V2f mousePos{V2fc::HUGE_VALUE_NEG};
};
struct OnFirstTimeTouchDownViewportSpaceEvent {
    V2f viewportPos{V2fc::HUGE_VALUE_NEG};
};

struct OnTouchMoveEvent{
    V2f mousePos{V2fc::HUGE_VALUE_NEG};
};
struct OnTouchMoveViewportSpaceEvent {
    V2f viewportPos{V2fc::HUGE_VALUE_NEG};
};

struct OnSingleTapEvent {
    V2f mousePos{V2fc::HUGE_VALUE_NEG};
};

struct OnTouchUpEvent {
    V2f mousePos{V2fc::HUGE_VALUE_NEG};
};

struct OnSingleTapViewportSpaceEvent {
    V2f viewportPos{V2fc::HUGE_VALUE_NEG};
};

struct OnTouchUpViewportSpaceEvent {
    V2f viewportPos{V2fc::HUGE_VALUE_NEG};
};

struct OnKeyToggleEvent{
    int keyCode = 0;
    V2f viewportPos{V2fc::HUGE_VALUE_NEG};
};

struct OnIncrementalScaleEvent {
    float incrementalScaleFactor = 0.0f;
};

struct OnFinaliseEvent {};
struct OnEscapeEvent {};
struct OnSpaceEvent {};
struct OnSpecialSpaceEvent {};
struct OnDeleteEvent {};
struct OnGlobalRescaleEvent {
    float oldScaleFactor = 1.0f;
    float currentScaleFactorMeters = 1.0f;
};

struct OnEnterFittedFurnitureManipulationEvent{};
