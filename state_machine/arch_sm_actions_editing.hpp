//
// Created by dado on 04/06/2020.
//

#pragma once

#include <render_scene_graph/backend_io_services.hpp>
#include <eh_arch/models/house_service.hpp>
#include <eh_arch/models/wall_service.hpp>
#include <eh_arch/models/door_service.hpp>

struct MakeHouse3d {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        asg.make3dHouse([&]() {
            LOGRS("Spawn an house")
        });
    }
};

struct EnterFeatureManipulation {
    void operator()( ArchOrchestrator& asg ) noexcept {
        asg.showIMHouse();
    }
};

struct UpdateFeatureManipulationIm {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        asg.showIMHouse();
    }
};

struct UpdateFeatureManipulationFull {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        MakeHouse3d{}(asg, rsg, arc);
        asg.showIMHouse();
    }
};

struct ExitFeatureManipulation {
    void operator()( ArchOrchestrator& asg ) noexcept {
        asg.showIMHouse();
    }
};

struct UndoFeatureManipulation {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        asg.undoHouseChange();
        arc.resetSelection();
        UpdateFeatureManipulationFull{}(asg, rsg, arc);
    }
};

struct RedoFeatureManipulation {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        asg.redoHouseChange();
        arc.resetSelection();
        UpdateFeatureManipulationFull{}(asg, rsg, arc);
    }
};

enum class TouchedSelectionFlag {
    Keep,
    Toggle
};

static inline bool
touchSelection( const V2f& is, TouchedSelectionFlag tsf, ArchOrchestrator& asg, ArchRenderController& arc ) {
    if ( !asg.H() ) return false;
    float aroundDistance = 0.05f;
    auto afs = WallService::getNearestFeatureToPoint(asg.H(), is, aroundDistance);
    if ( afs.feature != ArchStructuralFeature::ASF_None ) {
        if ( tsf == TouchedSelectionFlag::Toggle ) arc.singleSelectionToggle(afs, is, SelectionFlags::RemoveAtTouchUp);
        if ( tsf == TouchedSelectionFlag::Keep ) arc.singleSelectionKeep(afs, is, SelectionFlags::RemoveAtTouchUp);
        return arc.selectionCount() > 0;
    } else {
        if ( auto door = HouseService::point2d<DoorBSData, IsInside>(asg.H(), is); door ) {
            afs.feature = ArchStructuralFeature::ASF_Poly;
            afs.elem = door.get();
            if ( tsf == TouchedSelectionFlag::Toggle ) arc.singleSelectionToggle(afs, is);
            if ( tsf == TouchedSelectionFlag::Keep ) arc.singleSelectionKeep(afs, is);
            return arc.selectionCount() > 0;
        }
        if ( auto window = HouseService::point2d<WindowBSData, IsInside>(asg.H(), is); window ) {
            afs.feature = ArchStructuralFeature::ASF_Poly;
            afs.elem = window.get();
            if ( tsf == TouchedSelectionFlag::Toggle ) arc.singleSelectionToggle(afs, is);
            if ( tsf == TouchedSelectionFlag::Keep ) arc.singleSelectionKeep(afs, is);
            return arc.selectionCount() > 0;
        }
        if ( auto ff = HouseService::point2d<FittedFurniture, IsInside>(asg.H(), is); ff ) {
            afs.feature = ArchStructuralFeature::ASF_Poly;
            afs.elem = ff.get();
            if ( tsf == TouchedSelectionFlag::Toggle ) arc.singleSelectionToggle(afs, is);
            if ( tsf == TouchedSelectionFlag::Keep ) arc.singleSelectionKeep(afs, is);
            return arc.selectionCount() > 0;
        }
        if ( auto room = HouseService::point2d<RoomBSData, IsInside>(asg.H(), is); room ) {
            afs.feature = ArchStructuralFeature::ASF_Poly;
            afs.elem = room.get();
            if ( tsf == TouchedSelectionFlag::Toggle ) arc.singleSelectionToggle(afs, is);
            if ( tsf == TouchedSelectionFlag::Keep ) arc.singleSelectionKeep(afs, is);
            return arc.selectionCount() > 0;
        }
    }
    // If it gets here and didn't hit anything it means that it points nothing, so clear the selections
    arc.resetSelection();
    return false;//arc.selectionCount() > 0;
}

struct SingleTapViewportHouseMakerManipulationGuard {
    bool operator()( const OnSingleTapViewportSpaceEvent& mouseEvent, ArchOrchestrator& asg,
                     ArchRenderController& arc ) noexcept {
        return touchSelection(mouseEvent.viewportPos, TouchedSelectionFlag::Toggle, asg, arc);
    }
};

struct SingleTapViewportSpaceFeatureManipulationGuard {
    bool operator()( const OnSingleTapViewportSpaceEvent& mouseEvent, ArchOrchestrator& asg,
                     ArchRenderController& arc ) noexcept {
        return !touchSelection(mouseEvent.viewportPos, TouchedSelectionFlag::Toggle, asg, arc);
    }
};

struct TouchedDownFirstTimeFeatureManipulationGuard {
    bool operator()( const OnFirstTimeTouchDownViewportSpaceEvent& mouseEvent, ArchOrchestrator& asg,
                     RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        bool res = touchSelection(mouseEvent.viewportPos, TouchedSelectionFlag::Keep, asg, arc);
        UpdateFeatureManipulationIm()(asg, rsg, arc);
        return res;
    }
};

struct TouchedDownFirstTimeFittedFurnitureGuard {
    bool operator()( const OnFirstTimeTouchDownViewportSpaceEvent& mouseEvent, ArchOrchestrator& asg,
                     RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        bool res = touchSelection(mouseEvent.viewportPos, TouchedSelectionFlag::Keep, asg, arc);
        UpdateFeatureManipulationIm()(asg, rsg, arc);
        return !res;
    }
};

struct TouchUpEventFeatureManipulation {
    bool operator()( ArchRenderController& arc, ArchOrchestrator& asg, RenderOrchestrator& rsg ) noexcept {
        asg.pushHouseChange();
        MakeHouse3d{}(asg, rsg, arc);
        asg.showIMHouse();
        return true;
    }
};

struct DeleteFeatureManipulation {
    bool operator()( ArchRenderController& arc, ArchOrchestrator& asg, RenderOrchestrator& rsg ) noexcept {
        arc.deleteElementsOnSelectionList([&]( const ArchStructuralFeatureDescriptor& asf ) {
            if ( asf.feature == ArchStructuralFeature::ASF_Poly ) {
                HouseService::removeArch(asg.H(), asf.elem->hash);
            } else {
                WallService::deleteFeature(asf);
            }
            asg.H()->updateVolume();
            MakeHouse3d{}(asg, rsg, arc);
            asg.showIMHouse();
            asg.pushHouseChange();
        });
        return true;
    }
};

struct SpaceToggleFeatureManipulation {
    bool operator()( ArchRenderController& arc, ArchOrchestrator& asg ) noexcept {
        arc.toggleElementsOnSelectionList([&]( const ArchStructuralFeatureDescriptor& asf ) {
            switch ( asf.elem->type ) {
                case DoorT:
                    DoorService::toggleOrientations(dynamic_cast<DoorBSData *>(asf.elem));
                    break;
                case FittedFurnitureT:
                    RoomServiceFurniture::rotateFurniture(dynamic_cast<FittedFurniture *>(asf.elem), QuaternionC::QuarterYawRotation );
                    break;
                default:
                    break;
            }
            asg.pushHouseChange();
        });
        return true;
    }
};

struct IncrementalScaleFeatureManipulation {
    bool operator()( ArchRenderController& arc, ArchOrchestrator& asg, OnIncrementalScaleEvent event ) noexcept {
        arc.toggleElementsOnSelectionList([&]( const ArchStructuralFeatureDescriptor& asf ) {
            switch ( asf.elem->type ) {
                case FittedFurnitureT:
                    (dynamic_cast<FittedFurniture *>(asf.elem))->scale(V3f{event.incrementalScaleFactor});
                    break;
                default:
                    break;
            }
            asg.pushHouseChange();
        });
        return true;
    }
};

struct FurnishHouse {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        HouseService::guessFittings(asg.H(), asg.FurnitureMap());
        MakeHouse3d{}(asg, rsg, arc);
        asg.showIMHouse();
        asg.pushHouseChange();
    }
};

struct AddFurnitureSingle {
    void operator()( const OnAddFurnitureSingleEvent& event, ArchOrchestrator& asg, RenderOrchestrator& rsg,
                     ArchRenderController& arc ) {
        auto ff = FittedFurniture{ { event.furnitureSet.name, event.furnitureSet.bbox3d }, FurnitureTypeHandler::name(FT(event.furnitureSet.ftype)),
                                   event.furnitureSet.symbol };
        auto ffs = EntityFactory::cloneHashed(ff);
        V2f pos = RS::maxEnclsingBoundingBoxCenter(event.room);
        auto f = HouseService::findFloorOf(asg.H(), event.room->hash);
        RS::placeManually(
                FurnitureRuleParams{ f, event.room, ffs, pos, FRPFurnitureRuleFlags{ forceManualFurnitureFlags } });

        MakeHouse3d{}(asg, rsg, arc);
        asg.showIMHouse();
        asg.pushHouseChange();
    }
};

struct TakeScreenShot {
    void operator()( const OnTakeScreenShotEvent& event, RenderOrchestrator& rsg ) {
        rsg.takeScreenShot(event.screenShotCallback);
    }
};


struct [[maybe_unused]] TouchMovePolyFeatureManipulation {
    bool operator()( const OnTouchMoveViewportSpaceEvent& mouseEvent, ArchOrchestrator& asg,
                     ArchRenderController& arc ) noexcept {
        auto is = mouseEvent.viewportPos;
        arc.moveSelectionList(is, [&]( const ArchStructuralFeatureDescriptor& asf, const V2f& offset ) {
            if ( asf.feature == ArchStructuralFeature::ASF_Poly ) {
                HouseService::moveArch(asg.H(), dynamic_cast<ArchStructural *>(asf.elem), offset);
            }
        });
        return true;
    }
};
