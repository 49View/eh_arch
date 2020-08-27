//
// Created by dado on 04/06/2020.
//

#pragma once

struct ActivateOutdoorAreaUI {
    void operator()( OutdoorAreaUI& oaUI, ArchOrchestrator& asg ) {
        oaUI.activate();
    }
};

struct UndoOutdoorArea {
    void operator()( OutdoorAreaUI& oaUI ) noexcept {
        oaUI.undo();
    }
};

struct RedoOutdoorArea {
    void operator()( OutdoorAreaUI& oaUI ) noexcept {
        oaUI.redo();
    }
};

struct ExitOutdoorArea {
    void operator()( OutdoorAreaUI& oaUI, ArchOrchestrator& asg ) noexcept {
        // On Exit, we might need to save some states, cache or whatever, do it here
        FloorService::addOutdoorAreaFromData( asg.H()->mFloors[0].get(), oaUI.OutdoorAreaData() );
        oaUI.deactivate();
        asg.showIMHouse();
    }
};

struct AddPointOutdoorAreaAction {
    void operator()( OutdoorAreaUI& oaUI, const OnSingleTapEvent& mouseEvent ) noexcept {
        oaUI.addPoint(mouseEvent.mousePos);
    }
};
