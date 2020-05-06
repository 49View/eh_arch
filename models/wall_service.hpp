//
//  wall_service.hpp
//  sixthview
//
//  Created by Dado on 05/10/2015.
//
//

#pragma once

#include "house_bsdata.hpp"

class WallService {
public:
	static std::shared_ptr<WallBSData> createWall( const std::vector<Vector2f>& epts, float _height,
	        WallLastPointWrapT wlpw = WallLastPointWrap::Yes,
	        float _z = 0.0f,
	        uint32_t wf = WallFlags::WF_HasSkirting | WallFlags::WF_HasCoving,
            const HouseHints::WallHints& uShapesHints = {} );
    static std::shared_ptr<WallBSData> createWall2( const std::vector<Vector2f>& epts, float _height,
                                                   WallLastPointWrapT wlpw = WallLastPointWrap::Yes,
                                                   float _z = 0.0f,
                                                   uint32_t wf = WallFlags::WF_HasSkirting | WallFlags::WF_HasCoving,
                                                   int64_t _linkedHash = 0,
                                                   SequencePart sequencePart = 0 );

	// Query
	static bool contains( const WallBSData* w, const Vector2f& pos );
	static void getPlasterMiddlePoints( const WallBSData* w, std::vector<Vector3f>& mpoints );
	static void getSegmentUShapePoint( const WallBSData* w, const int index, Vector2f& us1, Vector2f& us2, Vector2f& usm, Vector2f& usn, const float off = 0.0001f );
	static bool checkIndexAreInMiddleAnyUSHape( const WallBSData* w, int i1, int i2 );
	static Vector3f middlePointAt( const WallBSData* w, size_t index );
	static bool findElementAt( const WallBSData* w, const Vector2f& pos, Vector2f& w1 );
	static float segmentLenght( const WallBSData* w, size_t index );
	static void getArchSegments( const WallBSData* w, const int32_t floorNumber, const int32_t wallNumber, std::vector<ArchSegment>& ws );
	static bool intersectLine2d( const WallBSData* w, Vector2f const& p0, Vector2f const& p1, Vector2f& i );
	static void intersectLine2dMin( WallBSData* w, Vector2f const& p0, Vector2f const& p1, Vector2f& i, float& minDist, ArchIntersection& ret, uint32_t filterFlags = 0 );
	static QuadVector3fList vertsForWallAt( const WallBSData* w, int t, const std::vector<std::vector<Vector3f>>& cd );
	static bool checkUShapeIndexStartIsDoorOrWindow( const WallBSData* w, size_t index );

	static void addToArchSegment( const WallBSData* w, int32_t floorNumber, int32_t wallNumber, std::vector<ArchSegment>& ws );
	static void addToArchSegmentInternal( const WallBSData* w, int32_t floorNumber, int32_t wallNumber, std::vector<ArchSegment>& ws );

	// update
    static void update( WallBSData* w );
    static void update( WallBSData* w, const std::vector<Vector2f>& epts );
	static void rescale( WallBSData* w, float _scale );
	static void calcBBox( WallBSData* w );
    static void updateUShapes( WallBSData* w );
    static void ushapesReconciliation(WallBSData* w );
    static void removeUnPairedUShapes( WallBSData *w );
	static void makeTriangles2d( WallBSData* w );

	// Calcs
    static void perimeterFromSegments( const std::vector<std::vector<ArchSegment>>& segments,
                                             std::vector<Vector2f>& perimeterSegments, float& perimeterLength );
};
