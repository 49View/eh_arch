//
// Created by dado on 14/05/2020.
//

#include "kitchen_render.hpp"
#include <core/math/path_util.h>
#include <core/resources/profile.hpp>
#include <core/v_data.hpp>
#include <core/resources/resource_builder.hpp>
#include <poly/scene_graph.h>
#include <poly/vdata_assembler.h>
#include <graphics/renderer.h>
#include <poly/follower.hpp>
#include "../models/house_bsdata.hpp"
#include "../models/room_service.hpp"
#include "house_render.hpp"

namespace KitchenRender {

    struct KitchenCabinetFiller {
        Vector3f p1;
        Vector3f p2;
        Vector3f normal;
        Vector3f inwardN;
    };

    struct KitchenData {
        std::vector<Vector3f> kitchenWorktopPath;
        std::vector<Vector3f> kitchenSkirtingPath;
        std::vector<Vector3f> kitchenUnitsPath;

        // kitchen lengths, dimensions, etc...
        float kitchenWorktopDepth = 0.59f;
        float kitchenWorktopHeight = 0.05f;
        float worktopHeight = 0.88f;
        float kitchenSkirtingRecess = 0.06f;
        float kitchenUnitsRecess = 0.02f;
        Vector2f drawersPadding = V2f::ONE*.01f;
        float drawersThickness = 0.02f;
        Vector2f longDrawersSize = V2f{ 0.4f, 0.7f };
        Vector2f kitchenSkirtingSize;

        std::string worktopMaterial = "granite";
    };

    void calcLShapeOffset( const V3f& p1, const V3f& p2, const V3f& p3, const V2f& n1, const V2f& n2, float depth, float offset, std::vector<Vector3f>& path ) {
        Vector2f p1s = ( p1 + ( Vector3f(n1, 0.0f) * ( depth - offset ))).xy();
        Vector2f p3s = ( p3 + ( Vector3f(n2, 0.0f) * ( depth - offset ))).xy();
        Vector2f p2s = Vector2f::ZERO;
        if ( !intersection(p1s, p1s + normalize(p2.xy() - p1.xy()) * 10000.0f, p3s,
                           p3s + normalize(p2.xy() - p3.xy()) * 10000.0f, p2s)) {
            ASSERT(0);
        }
        path.emplace_back(p1s);
        path.emplace_back(p2s);
        path.emplace_back(p3s);
    }

    void createMasterPath( SceneGraph& sg, RoomBSData *w, KitchenData& _kitchenData ) {
        const ArchSegment *ls = RoomService::longestSegment(w);
        std::pair<size_t, size_t> oppositeWallForLShape;
        Vector2f iPointlShape = Vector2f::ZERO;

        bool foundOpposite = RoomService::findOppositeWallFromPoint(w, ls->p2, ls->normal, oppositeWallForLShape,
                                                                    iPointlShape);
        if ( !foundOpposite ) {
            _kitchenData.kitchenWorktopPath.emplace_back(ls->p1);
            _kitchenData.kitchenWorktopPath.emplace_back(ls->p2);
            _kitchenData.kitchenSkirtingPath.emplace_back(ls->p1);
            _kitchenData.kitchenSkirtingPath.emplace_back(ls->p2);
        } else {
            ArchSegment *lso = &w->mWallSegments[oppositeWallForLShape.first][oppositeWallForLShape.second];
            Vector2f iPoint = Vector2f::ZERO;
            if ( distance(lso->p1, iPointlShape) < distance(lso->p2, iPointlShape)) {
                iPoint = lso->p1 + ( lso->crossNormal * _kitchenData.kitchenWorktopDepth * 0.5f );
            } else {
                iPoint = lso->p2 + ( -lso->crossNormal * _kitchenData.kitchenWorktopDepth * 0.5f );
            }
            if ( RoomService::findOppositeWallFromPoint(w, iPoint, lso->normal, oppositeWallForLShape,
                                                        iPointlShape)) {
                Vector3f p1 = { ( ls->p1 + ls->normal * _kitchenData.kitchenWorktopDepth * 0.5f ),
                                _kitchenData.worktopHeight };
                Vector3f p2 = { ( iPointlShape +
                                  w->mWallSegments[oppositeWallForLShape.first][oppositeWallForLShape.second].normal *
                                  _kitchenData.kitchenWorktopDepth * 0.5f ), _kitchenData.worktopHeight };
                Vector3f p3 = { iPoint, _kitchenData.worktopHeight };
                _kitchenData.kitchenWorktopPath.push_back(p1);
                _kitchenData.kitchenWorktopPath.push_back(p2);
                _kitchenData.kitchenWorktopPath.push_back(p3);

                // Skirting
                calcLShapeOffset(p1, p2, p3, ls->normal, lso->crossNormal, _kitchenData.kitchenWorktopDepth * 0.5f, _kitchenData.kitchenSkirtingRecess, _kitchenData.kitchenSkirtingPath );
                calcLShapeOffset(p1, p2, p3, ls->normal, lso->crossNormal, _kitchenData.kitchenWorktopDepth * 0.5f, _kitchenData.kitchenUnitsRecess, _kitchenData.kitchenUnitsPath );
            }
        }

//        return std::make_shared<Follower>( mPrefs.mAccuracy, FollowerFlags::Defaults, "KitchenWorktop",
//                                           MappingDirection::Y_POS, true );
        ASSERT(_kitchenData.kitchenWorktopPath.size() > 0);
    }

    void addKitchenDrawer( SceneGraph& sg, const Vector3f& p1, const Vector3f& p2, const Vector2f& drawersSize,
                           const Vector2f& drawersPadding, float drawerDepth, int verticalIndex ) {
        std::vector<Vector3f> vcabinet;
        Vector3f pDir = normalize(p2 - p1);

        Vector3f a = p1 + ( pDir * drawersPadding.x());
        Vector3f b = p2 - ( pDir * drawersPadding.x());
        Vector3f hOffset1 =
                ( Vector3f::Z_AXIS * ( drawersSize.y() * verticalIndex )) + ( Vector3f::Z_AXIS * drawersPadding.y());
        Vector3f hOffset2 = ( Vector3f::Z_AXIS * ( drawersSize.y() * ( verticalIndex + 1 ))) -
                            ( Vector3f::Z_AXIS * drawersPadding.y());
        vcabinet.push_back(a - hOffset1);
        vcabinet.push_back(b - hOffset1);
        vcabinet.push_back(b - hOffset2);
        vcabinet.push_back(a - hOffset2);

        sg.GB<GT::Extrude>(V2nff{ drawersSize, V3f::UP_AXIS, drawersSize.y() }, XZY::C(p1));
    }

    void createUnits( SceneGraph& sg, RoomBSData *w, KitchenData& _kitchenData ) {

//        Follower fl_kitchenWorktop(accuracyNone, FollowerFlags::Defaults, "KitchenWorktop", MappingDirection::Y_POS, true);
//        auto wtop = Profile{};
//        wtop.createRect(V2f{_kitchenData.kitchenWorktopDepth, _kitchenData.kitchenWorktopHeight });
//        FollowerService::extrude( fl_kitchenWorktop, XZY::C(_kitchenData.kitchenWorktopPath), wtop, MappingDirection::Y_POS );

//        fl_kitchenWorktop.extrude(_kitchenData.kitchenWorktopPath, wtop,
//                                                  FollowerGap(_kitchenData.kitchenWorktopPath.size()));
        std::vector<Vector3f> wcoords = _kitchenData.kitchenUnitsPath;
        std::vector<KitchenCabinetFiller> fillerPs;

//        for ( size_t t = wcoords.size() - _kitchenData.kitchenWorktopPath.size(); t < wcoords.size() - 1; t++ ) {
        for ( size_t t = 0; t < wcoords.size()-1; t++ ) {
            Vector3f p1 = { wcoords[t], _kitchenData.worktopHeight - _kitchenData.kitchenWorktopHeight * 0.5f };
            Vector3f p2 = { wcoords[cai(t + 1, wcoords.size())],
                            _kitchenData.worktopHeight - _kitchenData.kitchenWorktopHeight * 0.5f };
            Vector3f pDir = normalize(p2 - p1);
            Vector3f inwardN = -normalize(cross(pDir, Vector3f::Z_AXIS));

//            p1 += ( inwardN * ( _kitchenData.drawerDepth + _kitchenData.drawerRecessFromWorktop ));
//            p2 += ( inwardN * ( _kitchenData.drawerDepth + _kitchenData.drawerRecessFromWorktop ));
            float widthOfSegment = distance(p1, p2);
            int numDrawers = (int) ( widthOfSegment /
                                     ( _kitchenData.longDrawersSize.x() + _kitchenData.drawersPadding.x()));
            float finalPadding = fmodf(widthOfSegment, _kitchenData.longDrawersSize.x());
            int extraPaddingI = finalPadding > 0.0f ? 1 : 0;
            for ( int q = 0; q < numDrawers + extraPaddingI; q++ ) {
                float drawW = _kitchenData.longDrawersSize.x();
                if (( q == ( numDrawers + extraPaddingI - 1 )) && ( extraPaddingI > 0 )) {
                    drawW = finalPadding;
                }
                Vector3f a = p1 + ( pDir * _kitchenData.longDrawersSize.x()) * (float) q;
                Vector3f b = a + ( pDir * drawW );
                int numDrawersInVertical = static_cast<int>(
                        ( _kitchenData.worktopHeight - _kitchenData.kitchenWorktopHeight * 0.5f ) /
                        _kitchenData.longDrawersSize.y());
                _kitchenData.kitchenSkirtingSize.setY(max(_kitchenData.kitchenSkirtingSize.y(),
                                                          fmod(( _kitchenData.worktopHeight -
                                                                 _kitchenData.kitchenWorktopHeight * 0.5f ),
                                                               _kitchenData.longDrawersSize.y())));
                for ( auto m = 0; m < numDrawersInVertical; m++ ) {
                    auto linex = FollowerService::createLinePath(a, b, _kitchenData.drawersThickness, 0.2f);
                    sg.GB<GT::Extrude>(PolyOutLine{linex, V3f::UP_AXIS, 0.5f });
                }
            }
            fillerPs.push_back({ p1, p2, pDir, inwardN });
        }

        // Filller(s)
//        Profile kitchenFillerProfile = loadKitchenWorktopProfile(_kitchenData.kitchenFillerSize);
//        kitchenFillerProfile.move(Vector2f::Y_AXIS * kitchenFillerProfile.height() * 0.5f + Vector2f::Y_AXIS *
//                                                                                            ( _kitchenData.kitchenSkirtingSize.y() -
//                                                                                              _kitchenData.kitchenWorktopHeight *
//                                                                                              0.5f -
//                                                                                              _kitchenData.drawersPadding.y()));
//        for ( size_t t = 0; t < fillerPs.size() - 1; t++ ) {
//            std::vector<Vector3f> fillerPath;
//            Vector2f p1 = fillerPs[t].p2.xy() - fillerPs[t].inwardN.xy() * _kitchenData.kitchenFillerSize.x() * 0.5f +
//                          fillerPs[t].normal.xy() * _kitchenData.drawersPadding.x();
//            Vector2f p3 =
//                    fillerPs[t + 1].p1.xy() - fillerPs[t + 1].inwardN.xy() * _kitchenData.kitchenFillerSize.x() * 0.5f -
//                    fillerPs[t + 1].normal.xy() * _kitchenData.drawersPadding.x();
//            Vector2f p2 = Vector2f::ZERO;
//            if ( !intersection(p1, p1 + fillerPs[t].normal.xy() * 10000.0f,
//                               p3, p3 - fillerPs[t + 1].normal.xy() * 10000.0f, p2)) {
//                ASSERT(0);
//            }
//            fillerPath.push_back(p1);
//            fillerPath.push_back(p2);
//            fillerPath.push_back(p3);
//            auto ch = rootH->addChildren(
//                    Follower(mAccuracy, FollowerFlags::Defaults, "KitchenFiller", MappingDirection::X_POS,
//                             true).extrude(fillerPath, kitchenFillerProfile, FollowerGap(fillerPath.size())));
//        }
    }

    void render( SceneGraph& sg, RoomBSData *w, HouseRenderContainer& ret ) {
        KitchenData kitchenData;
        createMasterPath(sg, w, kitchenData);

        auto wtop = std::make_shared<Profile>();
        wtop->createRect(V2f{ kitchenData.kitchenWorktopDepth, kitchenData.kitchenWorktopHeight });
        sg.GB<GT::Follower>(XZY::C(kitchenData.kitchenWorktopPath), wtop, GT::M(kitchenData.worktopMaterial));

        auto wtop2 = std::make_shared<Profile>();
        wtop2->createRect(V2f{ 0.02f, 0.1f });
        sg.GB<GT::Follower>(XZY::C(kitchenData.kitchenSkirtingPath), wtop2);

        createUnits(sg, w, kitchenData);
    }

}

