//
// Created by Dado on 2019-04-25.
//

#pragma once

#include <vector>
#include <core/math/vector4f.h>
#include <core/math/anim_type.hpp>
#include <core/raw_image.h>
#include <core/http/webclient.h>
#include <core/math/matrix_anim.h>
#include <graphics/ghtypes.hpp>
#include <render_scene_graph/ui_view.hpp>
#include "room_builder_segment_points.hpp"

class SceneGraph;
class Renderer;
class RenderOrchestrator;
struct ArchHouseBespokeData;

class RoomBuilder {
public:
    RoomBuilder( SceneGraph& sg, RenderOrchestrator& rsg,
                 std::shared_ptr<HouseBSData> _house );

    void activate();
    void activateDebug();
    bool validateAddPoint( const V2f& _p );
    void addPointToRoom();
    void toggleSegmentType();
    void changeSegmentType( ArchTypeT _st );
    [[nodiscard]] bool isPerimeterClosed() const;
    void onEntry();
    void onExit();

    void clear( const UICallbackHandle& _ch = {} );
    void undo( const UICallbackHandle& _ch = {} );
    void saveSegments( const UICallbackHandle& _ch = {} );
    void loadSegments( const SerializableContainer& _segs );
    void replaceFurniture( const UICallbackHandle& _ch = {} );

    void setSegmentTypeFromIndex( const UICallbackHandle& _ch = {} );
    void setCurrentPointerPos( const V2f& _p );
    void setInputPoint( const V2f& _p );

    std::shared_ptr<HouseBSData> finalise();
    std::shared_ptr<HouseBSData> finaliseWithClose();
    [[nodiscard]] V3f   bestStartingPoint() const;
    [[nodiscard]] float WallWidth() const;

    V2fVectorOfVector bespokeriseWalls();

private:
    void setUIStatusAfterChange( bool wasFinalised = false );
    void fade2dScene( float _time, float value );
    void setSegmentType( ArchTypeT _st );
    void setBestFittingSegmentTypeAfterSegmentCompletion();

    [[nodiscard]] bool checkSegmentLongEnough() const;
    [[nodiscard]] bool checkPointWillClosePerimeter() const;
    [[nodiscard]] bool checkPointIntersect( const V3f& _p ) const;

    ArchHouseBespokeData bespokerise();
    void roomPreBakedFurnitureSetup( FurnitureRuleScript& ruleScript );

    V3f convert2dPosTo3d( const V2f& _p );
    V3f snapper( const V3f& _p );
    void refresh();
    void saveCachedSegments();
    void drawWindow( int _bucketList, const V3f& _p1, const V3f& _p2, float _lineWidth, const C4f& _color );
    void drawSingleDoor2d( int _bucketList, const V3f& _p1, const V3f& _p2, float _lineWidth, const C4f& _color );
    void drawFloorplanSegments();

private:
    SceneGraph& sg;
    RenderOrchestrator& rsg;

    std::shared_ptr<HouseBSData> house;
    RoomBuilderSegmentPoints segments;
    SerializableContainer cachedSegments;
    V3f currentPoint = V3f::ZERO;
    V3f inputPoint = V3f::ZERO;
    V3f mBestStartingPoint = V3f::ZERO;
    float snapThreshold = 0.10f;
    float wallWidth = 0.15f;
    bool hasBeenSnapped = false;
    bool currentPointValid = false;
    bool finalised = false;
    int furnitureBucket = 0;
    int roomEditBucket = 0;

    ArchTypeT activeSegmentType = ArchType::WallT;

    std::vector<VPListSP> gripVPs;
};
