//
// Created by dado on 02/07/2020.
//

#pragma once

#include <vector>
#include <string>
#include <memory>

class Camera;
class Vector3f;
class Quaternion;
struct CameraSpatialsKeyFrame;
template <typename T>
struct KeyFramePair;

class TourPlayback {
public:
    void addKeyFrame( const CameraSpatialsKeyFrame& path );
    void playBack( std::shared_ptr<Camera> cam );
    void stopPlayBack( std::shared_ptr<Camera> cam );
    void beginPath( const std::vector<CameraSpatialsKeyFrame>& path );
    void endPath( const std::vector<CameraSpatialsKeyFrame>& path );

private:
    std::vector<KeyFramePair<Vector3f>> positions{};
    std::vector<KeyFramePair<Quaternion>> quats{};
    std::vector<KeyFramePair<float>> fovs{};
    std::vector<std::string> animUUIDs;
    float currTimeLineStamp = 0.0f;
};



