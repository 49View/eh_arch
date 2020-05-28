//
// Created by Dado on 20/10/2019.
//

#pragma once

#include "../models/house_bsdata.hpp"
#include "../models/room_service_furniture.hpp"
#include <core/resources/resource_utils.hpp>
#include <core/resources/resource_manager.hpp>

class SceneGraph;

class RenderOrchestrator;

namespace ShowHouseMatrixFlags {
    static constexpr uint64_t None = 0;
    static constexpr uint64_t Show2dFloorPlan = 1u << 1u;
    static constexpr uint64_t Show3dFloorPlan = 1u << 2u;
    static constexpr uint64_t Show3dHouse = 1u << 3u;
    static constexpr uint64_t UseDebugMode = 1u << 4u;
};

using ShowHouseMatrixFlagsT = uint64_t;

class ShowHouseMatrix {
public:
    ShowHouseMatrix() = default;
    ShowHouseMatrix( ShowHouseMatrixFlagsT flags ) : flags(flags) {}
    ShowHouseMatrix( ShowHouseMatrixFlagsT flags, float fp2DScreenRatio, float fp2DScreenPadding ) : flags(flags),
                                                                                                     fp2dScreenRatio(
                                                                                                             fp2DScreenRatio),
                                                                                                     fp2dScreenPadding(
                                                                                                             fp2DScreenPadding) {}
    [[nodiscard]] float getFp2DScreenRatio() const {
        return fp2dScreenRatio;
    }
    void setFp2DScreenRatio( float fp2DScreenRatio ) {
        fp2dScreenRatio = fp2DScreenRatio;
    }
    [[nodiscard]] float getFp2DScreenPadding() const {
        return fp2dScreenPadding;
    }
    void setFp2DScreenPadding( float fp2DScreenPadding ) {
        fp2dScreenPadding = fp2DScreenPadding;
    }
    [[nodiscard]] ShowHouseMatrixFlagsT getFlags() const {
        return flags;
    }
    void setFlags( ShowHouseMatrixFlagsT _flags ) {
        flags = _flags;
    }
private:
    ShowHouseMatrixFlagsT flags = ShowHouseMatrixFlags::None;
    float fp2dScreenRatio = 4.0f;
    float fp2dScreenPadding = 0.02f;
};


class ArchSceneGraph {
public:
    explicit ArchSceneGraph( SceneGraph& _sg, RenderOrchestrator& _rsg, FurnitureMapStorage& _furns );

    void showHouse( std::shared_ptr<HouseBSData>, ShowHouseMatrix );
    void loadHouse( const std::string& _pid, ShowHouseMatrix );
    void
    calcFloorplanNavigationTransform( std::shared_ptr<HouseBSData> houseJson, float screenRatio, float screenPadding );

    FurnitureMapStorage& Furns() { return furns; }

    void update();

protected:

    void consumeCallbacks();

protected:
    SceneGraph& sg;
    RenderOrchestrator& rsg;
    FurnitureMapStorage& furns;

    std::shared_ptr<Matrix4f> floorplanNavigationMatrix;
    std::shared_ptr<HouseBSData> houseJson;
    std::pair<std::shared_ptr<HouseBSData>, ShowHouseMatrix> callbackStream;
};
