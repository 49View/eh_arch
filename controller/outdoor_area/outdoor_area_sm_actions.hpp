//
// Created by dado on 04/06/2020.
//

#pragma once

struct ActivateOutdoorAreaUI {
    void operator()( OutdoorAreaUI& oaUI, ArchOrchestrator& asg ) {
        auto oa = EntityFactory::create<OutdoorAreaBSData>();
        FloorService::addOutdoorAreaFromData( asg.H()->mFloors[0].get(), oa );
        oaUI.activate(oa);
    }
};

struct UndoOutdoorArea {
    void operator()( OutdoorAreaBuilder* rb ) noexcept {
//        rb->undo();
    }
};

struct ExitOutdoorArea {
    void operator()( OutdoorAreaUI& oaUI, ArchOrchestrator& asg ) noexcept {
        // On Exit, we might need to save some states, cache or whatever, do it here
        oaUI.activate(nullptr);
        asg.showIMHouse();
    }
};

struct AddPointOutdoorAreaAction {
    void operator()( OutdoorAreaUI& oaUI, const OnSingleTapEvent& mouseEvent ) noexcept {
        oaUI.addPoint(mouseEvent.mousePos);
    }
};

