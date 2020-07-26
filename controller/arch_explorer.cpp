//
// Created by dado on 23/07/2020.
//

#include "arch_explorer.hpp"
#include "arch_orchestrator.hpp"
#include <core/camera.h>
#include <core/util.h>
#include <core/file_manager.h>
#include <core/resources/resource_metadata.hpp>
#include <graphics/ghtypes.hpp>
#include <graphics/renderer.h>
#include <graphics/mouse_input.hpp>
#include <render_scene_graph/render_orchestrator.h>

#include <eh_arch/models/house_service.hpp>
#include <eh_arch/models/room_service.hpp>
#include <eh_arch/models/room_service_furniture.hpp>
#include <eh_arch/render/ui/house_ui_material_properties.hpp>

inline V4f furnitureMoveDotColor() {
    return C4f::PASTEL_GREEN;
}

bool ArchExplorer::canBeManipulated() const {
    return ( furnitureSelected && ( bFurnitureTargetLocked || bFillFullFurnitureOutline ) && fd.room );
}

bool ArchExplorer::isActivelySelectingWall() const {
    return !bFurnitureTargetLocked && !bFillFullFurnitureOutline && fd.hasHit() && fd.intersectedType == GHType::Wall;
}

bool ArchExplorer::isActivelySelectingFloor() const {
    return !bFurnitureTargetLocked && !bFillFullFurnitureOutline && fd.hasHit() && fd.intersectedType == GHType::Floor;
}

void ArchExplorer::updateFurnitureSelection( RenderOrchestrator& rsg, const AggregatedInputData& aid,
                                             const V3f& centerBottomPos, const C4f& _dotColor ) {
    auto sm3 = DShaderMatrix{ DShaderMatrixValue3dColor };

    auto mouseScreenPos = aid.mousePosYInv(TOUCH_ZERO);
    mouseScreenPos.setX( mouseScreenPos.x() + getScreenSizef.x() * 0.02f );
    if ( isActivelySelectingWall() && !bColorMaterialWidgetActive ) {
        slimMaterialAndColorPropertyMemo( mouseScreenPos, fd.archSegment->wallMaterial.materialHash.empty() ? fd.room->wallsMaterial : fd.archSegment->wallMaterial, rsg.TH(S::WHITE) );
    } else if ( isActivelySelectingFloor() && !bColorMaterialWidgetActive ) {
        slimMaterialAndColorPropertyMemo(mouseScreenPos, fd.room->floorMaterial, rsg.TH(S::WHITE) );
    }
    if ( furnitureSelected ) {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) << furnitureSelectionAlphaAnim.value();
        std::string nameTag = _dotColor.toString() + "furnitureBBox" + stream.str();

        rsg.RR().drawTriangleQuad(CommandBufferLimits::CameraMousePointers, furnitureSelectionOutline,
                                  _dotColor.A(furnitureSelectionAlphaAnim.value() * 0.5f), nameTag + "a");

        rsg.RR().draw<DLine>(CommandBufferLimits::CameraMousePointers, furnitureSelectionOutline,
                             _dotColor.A(furnitureSelectionAlphaAnim.value()), sm3, nameTag, true, 0.015f);
    }

}

bool ArchExplorer::isMouseOverFurnitureInnerSelector( const V3f& _origin, const V3f& _dir ) const {
    float farV = std::numeric_limits<float>::max();
    float vNear = 0.0f;
    return centerBottomBBox.intersectLine(_origin, _dir, vNear, farV);
}

void ArchExplorer::touchMoveWithModKeyCtrl( [[maybe_unused]] const HouseBSData *_house, const V3f& _dir,
                                            RenderOrchestrator& rsg ) {
    if ( bFurnitureTargetLocked ) {
        bool inters = false;
        auto planeHit = furniturePlane.intersectRay(rsg.DC()->getPosition(), _dir, inters);
        V3f off = planeHit - prevFurnitureMovePosition;
        auto potentialBBox = furnitureSelected->bbox;
        potentialBBox.translate(XZY::C2(off));
        if ( !bRoomBboxCheck || RS::checkBBoxInsideRoom(fd.room, potentialBBox) ) {
            centerBottomFurnitureSelected += off;
            for ( auto& v : furnitureSelectionOutline ) v += off;
            RoomServiceFurniture::moveFurniture(fd.room, furnitureSelected, off, rsg.SG());
            bFurnitureDirty = true;
        }
        prevFurnitureMovePosition = planeHit;
    }
    rsg.DC()->enableInputs(!bFurnitureTargetLocked);
}

void ArchExplorer::firstTimeTouchDownCtrlKey( const V3f& _dir, RenderOrchestrator& rsg ) {
    if ( furnitureSelected ) {
        bFurnitureTargetLocked = isMouseOverFurnitureInnerSelector(rsg.DC()->getPosition(), _dir);
        if ( bFurnitureTargetLocked ) {
            if ( furnitureSelected->checkIf(FittedFurnitureFlags::FF_CanBeHanged) ) {
                furniturePlane = Plane3f{ fd.normal, centerBottomFurnitureSelected };
            } else {
                furniturePlane = Plane3f{ centerBottomFurnitureSelected, centerBottomFurnitureSelected + V3f::X_AXIS,
                                          centerBottomFurnitureSelected + V3f::Z_AXIS };
            }
            bool inters = false;
            prevFurnitureMovePosition = furniturePlane.intersectRay(rsg.DC()->getPosition(), _dir, inters);
        }
    }
    rsg.setMICursorCapture(!bFurnitureTargetLocked,
                           bFurnitureTargetLocked || bFillFullFurnitureOutline ? MouseCursorType::HAND
                                                                               : MouseCursorType::ARROW);
}

void ArchExplorer::singleClickSelection( RenderOrchestrator& rsg ) {
    // If we are on a manipulable object do nothing
    if ( !bColorMaterialWidgetActive ) {
        bColorMaterialWidgetActive = true;
        if ( isActivelySelectingWall() ) {
            res.prepare(fd.intersectedType, "", ResourceGroup::Color);
        } else if ( isActivelySelectingFloor() ) {
            res.prepare(fd.intersectedType, "", ResourceGroup::Material);
        }
    } else {
        bColorMaterialWidgetActive = false;
    }

}

void ArchExplorer::spaceToggle( RenderOrchestrator& rsg ) {
    if ( canBeManipulated() ) {
        Quaternion quat = furnitureSelected->checkIf(FittedFurnitureFlags::FF_CanBeHanged)
                          ? QuaternionC::QuarterRollRotation : QuaternionC::QuarterYawRotation;
        RoomServiceFurniture::rotateFurniture(fd.room, furnitureSelected, quat, rsg.SG());
    }
}

void ArchExplorer::replaceFurnitureFinal( const EntityMetaData& _furnitureCandidate, ArchOrchestrator& asg,
                                          RenderOrchestrator& rsg ) {
    auto ff = FittedFurniture{
            { _furnitureCandidate.hash, _furnitureCandidate.bboxSize },
            furnitureSelected->keyTag, furnitureSelected->symbolRef };
    ff.flags = furnitureSelected->flags;
    auto clonedFurniture = EntityFactory::cloneHashed(ff);
    cloneInternal(asg.H(), furnitureSelected, clonedFurniture);
    RoomServiceFurniture::removeFurniture(fd.room, furnitureSelected, rsg.SG());
    asg.make3dHouse([&]() {
        LOGRS("Respawn an house after changing furniture")
    });
    asg.pushHouseChange();
}

void ArchExplorer::replaceFurnitureWithOneOfItsKind( ArchOrchestrator& asg, RenderOrchestrator& rsg ) {
    if ( canBeManipulated() ) {
        if ( auto furnitureCandidate = furnitureReplacer.findCandidate(
                    furnitureSelected->keyTag); furnitureCandidate ) {
            replaceFurnitureFinal(*furnitureCandidate, asg, rsg);
        } else {
            ResourceMetaData::getListOf(ResourceGroup::Geom, furnitureSelected->keyTag,
                                        [&]( CRefResourceMetadataList el ) {
                                            if ( !el.empty() ) {
                                                furnitureReplacer.addMetadataListFromTag(furnitureSelected->keyTag, el,
                                                                                         furnitureSelected->name);
                                                replaceFurnitureFinal(
                                                        *furnitureReplacer.findCandidate(furnitureSelected->keyTag),
                                                        asg, rsg);
                                            }
                                        });
        }
    }
}

void ArchExplorer::deleteSelected( RenderOrchestrator& rsg ) {
    if ( canBeManipulated() ) {
        RoomServiceFurniture::removeFurniture(fd.room, furnitureSelected, rsg.SG());
    }
}

void ArchExplorer::cloneInternal( HouseBSData *_house, FittedFurniture *sourceFurniture,
                                  const std::shared_ptr<FittedFurniture>& clonedFurniture ) {
    auto depthOffset = sourceFurniture->checkIf(FittedFurnitureFlags::FF_CanBeHanged) ? sourceFurniture->depthNormal *
                                                                                        sourceFurniture->width
                                                                                      : V2fc::ZERO;
    V2f pos = XZY::C2(hitPosition) + depthOffset;
    auto f = HouseService::findFloorOf(_house, fd.room->hash);
    RS::placeManually(
            FurnitureRuleParams{ f, fd.room, clonedFurniture, pos, sourceFurniture->heightOffset,
                                 sourceFurniture->rotation,
                                 FRPFurnitureRuleFlags{ forceManualFurnitureFlags } });
}

void ArchExplorer::cloneSelected( HouseBSData *_house, RenderOrchestrator& rsg ) {
    auto clonedFurniture = EntityFactory::cloneHashed(*furnitureSelected);
    cloneInternal(_house, furnitureSelected, clonedFurniture);
}

bool ArchExplorer::touchUpWithModKeyCtrl( RenderOrchestrator& rsg ) {
    rsg.DC()->enableInputs(true);
    rsg.DC()->LockScrollWheelMovements(false);
    rsg.setMICursorCapture(true, bFillFullFurnitureOutline ? MouseCursorType::HAND : MouseCursorType::ARROW);
    furnitureSelected = nullptr;
    bFurnitureTargetLocked = false;
    bool ret = bFurnitureDirty;
    bFurnitureDirty = false;
    return ret;
}

std::vector<V3f> createBBoxOutline( const V3f& input, const V3f& axis1, const V3f& axis2 ) {
    std::vector<V3f> ret{};
    ret.push_back(input + axis1 * 0.5f - axis2 * 0.5f);
    ret.push_back(input + axis1 * 0.5f + axis2 * 0.5f);
    ret.push_back(input - axis1 * 0.5f + axis2 * 0.5f);
    ret.push_back(input - axis1 * 0.5f - axis2 * 0.5f);
    return ret;
}

void ArchExplorer::tickControlKey( ArchOrchestrator& asg, RenderOrchestrator& rsg, const AggregatedInputData& aid ) {

    auto _dir = aid.mouseViewportDir(TouchIndex::TOUCH_ZERO, rsg.DC().get());

    if ( !bFurnitureTargetLocked ) {
        fdFurniture = HouseService::rayFeatureIntersect(asg.H(), RayPair3{ rsg.DC()->getPosition(), _dir },
                                                        FeatureIntersectionFlags::FIF_Furnitures);
        fd = HouseService::rayFeatureIntersect(asg.H(), RayPair3{ rsg.DC()->getPosition(), _dir },
                                               FeatureIntersectionFlags::FIF_Walls |
                                               FeatureIntersectionFlags::FIF_Floors);

        res.update(asg, &bColorMaterialWidgetActive, "/home/dado/media/media/", rsg, fd.room);

        if ( fdFurniture.hasHit() && fdFurniture.nearV < fd.nearV ) {
            hitPosition = rsg.DC()->getPosition() + ( _dir * fd.nearV );
            V3f refNormal = V3f::UP_AXIS;
            furnitureSelectionAlphaAnim.fadeIn();
            furnitureSelected = dynamic_cast<FittedFurniture *>(fdFurniture.arch);

            if ( furnitureSelected->checkIf(FittedFurnitureFlags::FF_CanBeHanged) ) {
                float closestDist = dot(fd.normal, V3f::X_AXIS);
                centerBottomFurnitureSelected = furnitureSelected->bbox3d.centreFront();
                if ( auto d = dot(fd.normal, V3f::X_AXIS_NEG); d > closestDist ) {
                    centerBottomFurnitureSelected = furnitureSelected->bbox3d.centreBack();
                    closestDist = d;
                }
                if ( auto d = dot(fd.normal, V3f::Z_AXIS); d > closestDist ) {
                    centerBottomFurnitureSelected = furnitureSelected->bbox3d.centreLeft();
                    closestDist = d;
                }
                if ( auto d = dot(fd.normal, V3f::Z_AXIS_NEG); d > closestDist ) {
                    centerBottomFurnitureSelected = furnitureSelected->bbox3d.centreRight();
                }
                furnitureSelectionOutline = createBBoxOutline(centerBottomFurnitureSelected,
                                                              V3f::UP_AXIS * furnitureSelected->bbox3d.calcHeight(),
                                                              V3f::Z_AXIS * furnitureSelected->bbox3d.calcDepth());
                refNormal = fd.normal;
            } else {
                centerBottomFurnitureSelected = furnitureSelected->bbox3d.centreBottom();
                furnitureSelectionOutline = furnitureSelected->bbox3d.bottomFace();
            }

            centerBottomBBox = AABB{ furnitureSelectionOutline };
            bFillFullFurnitureOutline = isMouseOverFurnitureInnerSelector(rsg.DC()->getPosition(), _dir);
        } else {
            furnitureSelectionAlphaAnim.fadeOut();
            bFillFullFurnitureOutline = false;
        }
        bool isMouseOverSelection = isMouseOverFurnitureInnerSelector(rsg.DC()->getPosition(), _dir);
        rsg.DC()->LockScrollWheelMovements(isMouseOverSelection);
        rsg.setMICursorCapture(true, isMouseOverSelection ? MouseCursorType::HAND : MouseCursorType::ARROW);
    }
//    if ( furnitureSelected ) {
    updateFurnitureSelection(rsg, aid, centerBottomFurnitureSelected, furnitureMoveDotColor());
//    }
}

void
FurnitureExplorerReplacer::addMetadataListFromTag( const std::string& _keyTag, CRefResourceMetadataList _metadataList,
                                                   const std::string& _initialIndexCheck ) {
    if ( !_metadataList.empty() ) {
        // This inserts the main key tag (_keyTag) maps to an array of all possible candidates of EntityMetaData (_metadataList)
        replaceFurniture.emplace(_keyTag, _metadataList);
        // For start assumes that the current index on the metadata array is the first one (0)
        replacingIndex.emplace(_keyTag, 0);

        // Next we split the keyTag and furniture names into string tokens, intersect them and point to the next element
        // We do this in order to not the start the next cycle from the same element that is currently selected;
        //
        // IE:
        // metadataList = ["blue,sofa,modern", "yellow,sofa,old', "green,sofa,tall,luxury] ...
        // current selected furniture = "sofa,green"
        // Each string in metadataList is converted into an array IE "blue,sofa,modern" = ["blue","sofa","modern"]
        // current selected furniture is converted to an array IE "sofa,green" = ["sofa","green"]
        // If we find an intersection with any array with the current selected furniture array then we set the index
        // to the element next to it, in this case the iterator will circular scroll through the entire list leaving
        // the current selected element as last one to occur.
        if ( const auto& listIter = replaceFurniture.find(_keyTag); listIter != replaceFurniture.end() ) {
            auto list = listIter->second;
            auto tagsArray = split_tags(_initialIndexCheck);
            std::sort(tagsArray.begin(), tagsArray.end());
            for ( auto lIndex = 0u; lIndex < list.size(); lIndex++ ) {
                auto nameArray = split_tags(getFileNameOnly(list[lIndex].name));
                auto v3 = vectorsIntersection(tagsArray, nameArray);
                if ( v3 == tagsArray ) {
                    replacingIndex[_keyTag] = cai(lIndex + 1, list.size());
                    break;
                }
            }
        }
    }
}

std::optional<EntityMetaData>
FurnitureExplorerReplacer::findCandidate( const std::string& _keyTag ) {
    if ( auto listIter = replaceFurniture.find(_keyTag); listIter != replaceFurniture.end() ) {
        auto list = listIter->second;
        auto fIndex = replacingIndex[_keyTag];
        // Circularly scrolling through all possible candidates for keyTag
        replacingIndex[_keyTag] = cai(replacingIndex[_keyTag] + 1, list.size());
        return list[fIndex];
    }
    return std::nullopt;
}
