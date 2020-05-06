//
//  room_render.hpp
//  sixthmaker
//
//  Created by Dado on 19/03/2017.
//
//

#pragma once

#include <core/resources/resource_types.hpp>

class SceneGraph;
struct RoomBSData;
class Renderer;
struct RoomBSData;
struct RDSPreMult;
enum class Use2dDebugRendering;

namespace RoomRender {
    void make2dGeometry( Renderer& rr, SceneGraph& sg, const RoomBSData *data, Use2dDebugRendering bDrawDebug,
                         const RDSPreMult &_pm );
    void make3dGeometry( SceneGraph& sg, RoomBSData* mData );
    GeomSPContainer createCovingSegments( SceneGraph& sg, RoomBSData* mData );
    GeomSPContainer createSkirtingSegments( SceneGraph& sg, RoomBSData* mData );
}

//class RoomController;
//class FloorRender;
//
//class RoomRender : public HouseBaseRender {
//public:
//	RoomRender( ArchMVPData<RoomBSData, RoomRender> _cw );
//	virtual void makeLights();
//
//	virtual void make2dGeometry();
//	virtual void make3dGeometry();
//	virtual void makeGeometryImpl() {};
//	virtual std::shared_ptr<ArchStructural> Data() { return std::dynamic_pointer_cast<ArchStructural>( mData ); }
//
//	void mapWalls( WallMatType wmt );
//
//	FloorRender* Dad() const;
//
//	void createFloor();
//
//	GeomSP RootH() { return mRootH; };
//
//	std::shared_ptr<RoomController>& Controller() { return mController; }
//	void Controller( std::shared_ptr<RoomController>& val ) { mController = val; }
//
//protected:
//	void startMake3d( const std::string& _roomName );
//	void createCovingSegments();
//	void createSkirtingSegments();
//	void createCeiling( const CeilingType _ct, const CeilingMatType _cmt );
//	void createSockets();
////	void createFloorMat( const std::vector<Vector2f>& fpoints, const FloorMatTypeT _fmt );
//	void createCeilingMesh( const std::vector<Vector2f>& fpoints, const CeilingMatType _cmt );
//	void createFittedFurnitures();
//	std::string floorMatName() const;
//protected:
//	GeomSP mFloorH;
//	GeomSP mCeilingH;
//	GeomSP mSkirtingH;
//	GeomSP mCovingH;
//	GeomSP mSocketsH;
//
//	std::shared_ptr<RoomBSData> mData;
//	std::shared_ptr<RoomController> mController;
//
//	int64_t mFloorHashOffset = 1000000;
//	int64_t mCeilingHashOffset = 2000000;
//	int64_t mSkirtingHashOffset = 3000000;
//	int64_t mCovingHashOffset = 4000000;
//	int64_t mSocketsHashOffset = 5000000;
//};
//
//class GenericRoomRender : public RoomRender {
//public:
//	GenericRoomRender( ArchMVPData<RoomBSData, GenericRoomRender> _cw ) :
//					   RoomRender( _cw.template ddHier<RoomBSData, RoomRender>(_cw.mData)) {
//		makeGeometry();
//	}
//
//	virtual ~GenericRoomRender() {}
//	virtual void make2dGeometry();
//	virtual void make3dGeometry();
//	virtual void makeGeometryImpl();
//};
