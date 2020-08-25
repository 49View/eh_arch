//
// Created by dado on 04/06/2020.
//

#pragma once

struct UndoOutdoorArea {
    void operator()( OutdoorAreaBuilder* rb ) noexcept {
//        rb->undo();
    }
};

struct ExitOutdoorArea {
    void operator()( OutdoorAreaUI& oaUI, OutdoorAreaBuilder* bb ) noexcept {
        // On Exit, we might need to save some states, cache or whatever, do it here
        oaUI.activate(false);
        bb->clear();
    }
};

struct AddPointOutdoorAreaAction {
    void operator()( OutdoorAreaBuilder* bb, const OnSingleTapEvent& mouseEvent ) noexcept {
        bb->addPoint(mouseEvent.mousePos, 0);
    }
};

struct FinaliseOutdoorArea {
    void operator()( OutdoorAreaBuilder* bb, ArchOrchestrator& asg ) noexcept {
        if ( asg.H() ) {
            FloorService::addOutdoorAreaFromData( asg.H()->mFloors[0].get(), bb->OutdoorAreaData() );
            asg.showIMHouse();
        }
    }
};
