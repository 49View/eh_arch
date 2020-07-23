//
// Created by dado on 04/06/2020.
//

#pragma once

struct [[maybe_unused]] ActivateOrbitMode {
    void
    operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        rsg.setRigCameraController(CameraControlType::Orbit);
        rsg.useSkybox(true);
        rsg.RR().createGrid(CommandBufferLimits::GridStart, 1.0f, ( Color4f::PASTEL_GRAYLIGHT ),
                            ( Color4f::DARK_GRAY ), V2f{ 15.0f }, 0.015f);
    }
};

struct InitializeHouseMaker {
    void operator()( SceneGraph& sg, ArchRenderController& arc, RenderOrchestrator& rsg, ArchOrchestrator& asg,
                     const OnActivateEvent& ev ) noexcept {
        rsg.DC()->setQuat(quatCompose(V3f{ M_PI_2, 0.0f, 0.0f }));
        rsg.DC()->setPosition(V3f::UP_AXIS * 15.0f);
        rsg.RR().setIndoorSceneCoeff(1.0f);
        asg.setFloorPlanView(ev.floorPlanRenderMode);
        if ( ev.ccf ) ev.ccf();
    }
};

static inline void show3dViewInternal( ArchOrchestrator& asg, const std::function<void()>& callback ) {
    if ( !asg.H() ) return;
    if ( asg.HRC().houseId != asg.H()->propertyId ) {
        asg.make3dHouse(callback);
    } else {
        callback();
    }
}

struct [[maybe_unused]] ActivateFloorplanView {
    void
    operator()( SceneGraph& sg, ArchRenderController& arc, RenderOrchestrator& rsg, ArchOrchestrator& asg ) const noexcept {
        asg.setFloorPlanView(floorPlanRenderMode);
    }

    FloorPlanRenderMode floorPlanRenderMode = FloorPlanRenderMode::Normal3d;
};

struct ActivateTourView {
    void
    operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        show3dViewInternal(asg, [&]() {
            asg.setTourView();
        });
    }
};

struct ActivateTopDownView {
    void
    operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        show3dViewInternal(asg, [&]() {
            asg.setTopDownView();
        });
    }
};

struct ActivateWalkView {
    void
    operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        LOGRS("Switching to Walk View")
        show3dViewInternal(asg, [&]() {
            asg.setWalkView();
        });
    }
};

struct ActivateDollyHouseView {
    void
    operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        show3dViewInternal(asg, [&]() {
            asg.setDollHouseView();
        });
    }
};

struct WhichRoomAmI {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        if ( asg.H() ) {
            V2f pos2d = XZY::C2(rsg.DC()->getPosition());
            if ( auto ret = HouseService::whichRoomAmI(asg.H(), pos2d); ret ) {
                arc.setSelectionList(*ret);
            }
        }
    }
};

static void renderCameraLocator(ArchOrchestrator& asg, RenderOrchestrator& rsg) {
    // Update camera locator on map
    auto camPos = rsg.DC()->getPosition() * V3f::MASK_Y_OUT;
    auto camDir = -rsg.DC()->getDirection() * 0.7f;
    auto sm = DShaderMatrix{ DShaderMatrixValue2dColor };
    rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraLocatorIM, camPos, V4f::DARK_RED, 0.4f,
                                 RDSPreMult(asg.FloorplanNavigationMatrix()),
                                 sm, std::string{ "CameraOminoKey" });
    rsg.RR().draw<DArrow>(CommandBufferLimits::CameraLocatorIM, V3fVector{ camPos, camPos + camDir },
                          RDSArrowAngle(0.45f),
                          RDSArrowLength(0.6f), V4f::DARK_RED, 0.004f, sm, RDSPreMult(asg.FloorplanNavigationMatrix()),
                          std::string{ "CameraOminoKeyDirection1" });
}

struct PushTourPath {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        HouseService::pushTourPath(asg.H(), CameraSpatialsKeyFrame{ rsg.DC()->getSpatials(), 0.0f });
    }
};

struct PushKeyFrameTourPath {
    void operator()( OnPushKeyFrameTourPathEvent event, ArchOrchestrator& asg, RenderOrchestrator& rsg ) {
        HouseService::pushKeyFrameTourPath(asg.H(), CameraSpatialsKeyFrame{ rsg.DC()->getSpatials(), event.timestamp });
    }
};

struct PopTourPath {
    void
    operator()( OnPopTourPathEvent event, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        HouseService::popTourPath(asg.H(), event.popIndex);
    }
};


struct TickExploreEdit {
    void operator()( const OnTickEvent& event, ArchOrchestrator& asg, RenderOrchestrator& rsg ) {
        if ( asg.H() ) {
            renderCameraLocator( asg, rsg );
            asg.Explorer().tickControlKey( asg.H(), event.aid.mouseViewportDir(TouchIndex::TOUCH_ZERO, rsg.DC().get()), rsg );
        }
    }
};

struct Tick {
    void operator()( const OnTickEvent& event, ArchOrchestrator& asg, RenderOrchestrator& rsg ) {
        if ( asg.H() ) {
            renderCameraLocator( asg, rsg );
            asg.PositionalDot().tick( asg.H(), event.aid.mouseViewportDir(TouchIndex::TOUCH_ZERO, rsg.DC().get()), rsg );
        }
    }
};

struct UndoExploreAction {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        asg.undoHouseChange();
        arc.resetSelection();
        MakeHouse3d{}(asg, rsg, arc);
    }
};

struct RedoExploreAction {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        asg.redoHouseChange();
        arc.resetSelection();
        MakeHouse3d{}(asg, rsg, arc);
    }
};

struct FirstTimeTouchDown {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        if ( asg.H() ) {
            asg.PositionalDot().firstTimeTouchDown();
        }
    }
};

struct FirstTimeTouchDownExploreEdit {
    void operator()( const OnFirstTimeTouchDownEvent& event,  ArchOrchestrator& asg, RenderOrchestrator& rsg ) {
        if ( asg.H() ) {
            asg.Explorer().firstTimeTouchDownCtrlKey(event.aid.mouseViewportDir(TouchIndex::TOUCH_ZERO, rsg.DC().get()), rsg);
        }
    }
};

struct SpaceToggle {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg ) {
        if ( asg.H() ) {
            asg.Explorer().spaceToggle(rsg);
            asg.pushHouseChange();
        }
    }
};

struct DeleteSelected {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg ) {
        if ( asg.H() ) {
            asg.Explorer().deleteSelected(rsg);
            asg.pushHouseChange();
        }
    }
};

struct TouchMoveExploreEdit {
    void operator()( const OnTouchMoveEvent& event, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        if ( asg.H() ) {
            asg.Explorer().touchMoveWithModKeyCtrl( asg.H(), event.aid.mouseViewportDir(TouchIndex::TOUCH_ZERO, rsg.DC().get()), rsg );
        }
    }
};

struct TouchUpExploreEdit {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        if ( asg.H() ) {
            bool isDirty = asg.Explorer().touchUpWithModKeyCtrl( rsg );
            if ( isDirty ) {
                asg.pushHouseChange();
            }
        }
    }
};

struct TouchUp {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        if ( asg.H() ) {
            asg.PositionalDot().touchUp();
        }
    }
};

struct SingleTap {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        if ( asg.H() ) {
            asg.PositionalDot().singleTap( rsg );
        }
    }
};

struct TimeIncrementFast {
    void operator()( const OnIncreaseValueFastAltEvent& event, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        if ( asg.H() ) {
            rsg.addSecondsToTime(60*event.increment);
        }
    }
};

struct TimeDecrementFast {
    void operator()( const OnDecreaseValueFastAltEvent& event, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        if ( asg.H() ) {
            rsg.addSecondsToTime(60*event.increment);
        }
    }
};

struct TimeIncrement {
    void operator()( const OnIncreaseValueAltEvent& event, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        if ( asg.H() ) {
            rsg.addSecondsToTime(60*event.increment);
        }
    }
};

struct TimeDecrement {
    void operator()( const OnDecreaseValueAltEvent& event, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        if ( asg.H() ) {
            rsg.addSecondsToTime(60*event.increment);
        }
    }
};
