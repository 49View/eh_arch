//
// Created by dado on 02/07/2020.
//

#include "arch_tour.hpp"
#include <core/math/anim.h>
#include <core/camera.h>

void TourPlayback::addKeyFrame( const CameraSpatialsKeyFrame& path ) {
    positions.emplace_back(path.timestamp + currTimeLineStamp, path.pos);
    quats.emplace_back(path.timestamp + currTimeLineStamp, path.qangle);
    fovs.emplace_back(path.timestamp + currTimeLineStamp, path.fov);
}

void TourPlayback::playBack( std::shared_ptr <Camera> cam ) {
    animUUIDs.clear();
    animUUIDs.emplace_back(Timeline::play(cam->PosAnim(), 0, positions, AnimLoopType::Loop));
    animUUIDs.emplace_back(Timeline::play(cam->QAngleAnim(), 0, quats, AnimLoopType::Loop));
    animUUIDs.emplace_back(Timeline::play(cam->FoVAnim(), 0, fovs, AnimLoopType::Loop));
}

void TourPlayback::stopPlayBack( std::shared_ptr <Camera> cam ) {
    if ( animUUIDs.size() == 3 ) {
        Timeline::stop(cam->PosAnim(), animUUIDs[0], cam->PosAnim()->value);
        Timeline::stop(cam->QAngleAnim(), animUUIDs[1], cam->QAngleAnim()->value);
        Timeline::stop(cam->FoVAnim(), animUUIDs[2], cam->FoVAnim()->value);
        animUUIDs.clear();
    }
    positions.clear();
    quats.clear();
    fovs.clear();
    currTimeLineStamp = 0.0f;
}

void TourPlayback::beginPath( const std::vector <CameraSpatialsKeyFrame>& path ) {
    addKeyFrame(path.front());
    currTimeLineStamp += 0.001f;
    addKeyFrame(path.front());
    currTimeLineStamp += 0.001f;
}

void TourPlayback::endPath( const std::vector <CameraSpatialsKeyFrame>& path ) {
    currTimeLineStamp += 0.001f;
    addKeyFrame(path.back());
    currTimeLineStamp += 0.001f;
    addKeyFrame(path.back());
    currTimeLineStamp += path.back().timestamp;
}
