//
// Created by dado on 04/06/2020.
//

#pragma once

struct OnWhichRoomAmIEvent{};
struct OnPushTourPathEvent{};
struct OnPushKeyFrameTourPathEvent {
    float timestamp = 5.0f;
};
struct OnPopTourPathEvent {
    int popIndex = -1;
};
