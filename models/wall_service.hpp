//
//  wall_service.hpp
//  sixthview
//
//  Created by Dado on 05/10/2015.
//
//

#pragma once

#include "house_bsdata.hpp"

struct ArchStructuralFeatureDescriptor;

namespace WallService {
    std::shared_ptr<WallBSData> createWall( const std::vector<Vector2f>& epts, float _height,
                                            WallLastPointWrapT wlpw = WallLastPointWrap::Yes,
                                            float _z = 0.0f,
                                            uint32_t wf = WallFlags::WF_HasSkirting | WallFlags::WF_HasCoving,
                                            const HouseHints::WallHints& uShapesHints = {} );
    std::shared_ptr<WallBSData> createWall2( const std::vector<Vector2f>& epts, float _height,
                                             WallLastPointWrapT wlpw = WallLastPointWrap::Yes,
                                             float _z = 0.0f,
                                             uint32_t wf = WallFlags::WF_HasSkirting | WallFlags::WF_HasCoving,
                                             int64_t _linkedHash = 0,
                                             SequencePart sequencePart = 0 );

    // Query
    bool contains( const WallBSData *w, const Vector2f& pos );
    void getPlasterMiddlePoints( const WallBSData *w, std::vector<Vector3f>& mpoints );
    void getSegmentUShapePoint( const WallBSData *w, const int index, Vector2f& us1, Vector2f& us2, Vector2f& usm,
                                Vector2f& usn, const float off = 0.0001f );
    bool checkIndexAreInMiddleAnyUSHape( const WallBSData *w, int i1, int i2 );
    Vector3f middlePointAt( const WallBSData *w, size_t index );
    bool findElementAt( const WallBSData *w, const Vector2f& pos, Vector2f& w1 );
    float segmentLenght( const WallBSData *w, size_t index );
    void getArchSegments( const WallBSData *w, const int32_t floorNumber, const int32_t wallNumber,
                          std::vector<ArchSegment>& ws );
    bool intersectLine2d( const WallBSData *w, Vector2f const& p0, Vector2f const& p1, Vector2f& i );
    void intersectLine2dMin( WallBSData *w, Vector2f const& p0, Vector2f const& p1, Vector2f& i, float& minDist,
                             ArchIntersection& ret, uint32_t filterFlags = 0 );
    QuadVector3fList vertsForWallAt( const WallBSData *w, int t, const std::vector<std::vector<Vector3f>>& cd );
    bool checkUShapeIndexStartIsDoorOrWindow( const WallBSData *w, size_t index );

    bool isWindowOrDoorPart( const WallBSData* w );
    void addToArchSegment( const WallBSData *w, int32_t floorNumber, int32_t wallNumber, std::vector<ArchSegment>& ws );
    void addToArchSegmentInternal( const WallBSData *w, int32_t floorNumber, int32_t wallNumber,
                                   std::vector<ArchSegment>& ws );

    ArchStructuralFeatureDescriptor getNearestFeatureToPoint( const HouseBSData *w, const V2f& point, float nearFactor );

    // update
    void update( WallBSData *w );
    void update( WallBSData *w, const std::vector<Vector2f>& epts );
    void rescale( WallBSData *w, float _scale );
    void calcBBox( WallBSData *w );
    void updateUShapes( WallBSData *w );
    void ushapesReconciliation( WallBSData *w );
    void removeUnPairedUShapes( WallBSData *w );
    void addPointAfterIndex( WallBSData *w, uint64_t pointIndex, const V2f& point );
    void addTwoShapeAfterIndex( WallBSData *w, uint64_t pointIndex, const V2f& point );
    void movePoint( WallBSData *w, uint64_t pointIndex, const V2f& offset, bool incremental );
    void deletePoint( WallBSData *w, uint64_t pointIndex );
    void deleteEdge( WallBSData *w, uint64_t pointIndex );
    void moveFeature( HouseBSData *houseJson, const ArchStructuralFeatureDescriptor& asf, const V2f& offset, bool incremental );
    void deleteFeature( HouseBSData *houseJson, const ArchStructuralFeatureDescriptor& asf );
    void splitEdgeAndAddPointInTheMiddle( HouseBSData *houseJson, const ArchStructuralFeatureDescriptor& asf, const V2f& newPoint );
    void createTwoShapeOnSelectedEdge( HouseBSData *houseJson, const ArchStructuralFeatureDescriptor& asf, const V2f& newPoint );
    void makeTriangles2d( WallBSData *w );

    // Calcs
    void perimeterFromSegments( const std::vector<std::vector<ArchSegment>>& segments,
                                std::vector<Vector2f>& perimeterSegments, float& perimeterLength );
};
