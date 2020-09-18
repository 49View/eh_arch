//
// Created by dado on 23/07/2020.
//

#pragma once

#include <string>
#include <core/math/vector3f.h>
#include <core/math/vector4f.h>
#include <core/math/anim.h>
#include <core/math/plane3f.h>
#include <core/math/aabb.h>
#include <core/resources/resource_metadata.hpp>
#include <eh_arch/models/htypes.hpp>
#include <eh_arch/models/arch_structural_service.hpp>
#include <eh_arch/controller/gui/remote_entity_selector.hpp>

class RenderOrchestrator;
class ArchOrchestrator;

struct HouseBSData;
struct AggregatedInputData;
struct InputMods;
struct FittedFurniture;

static constexpr float explorerFullDotOpacityValue = 0.75f;
static constexpr float explorerDotFadeTime = 0.15f;

class FurnitureExplorerReplacer {
public:
    void addMetadataListFromTag( const std::string& _keyTag, CRefResourceMetadataList el,
                                 const std::string& _initialIndexCheck );
    std::optional<EntityMetaData> findCandidate( const std::string& _keyTag );
private:
    std::unordered_map<std::string, ResourceMetadataList> replaceFurniture;
    std::unordered_map<std::string, uint64_t> replacingIndex;
};


class ArchExplorer {
public:
    ArchExplorer( RemoteEntitySelector& res ) : res(res) {}
    void tickControlKey( ArchOrchestrator& asg, RenderOrchestrator& rsg, const AggregatedInputData& aid, const std::string& mediaFolder );

    // Events
    void touchMoveWithModKeyCtrl( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg );
    void firstTimeTouchDownCtrlKey( const V3f& _dir, RenderOrchestrator& rsg );
    bool touchUpWithModKeyCtrl( RenderOrchestrator& rsg );
    void singleClickSelection( RenderOrchestrator& rsg );
    void spaceToggle( RenderOrchestrator& rsg );
    void deleteSelected( RenderOrchestrator& rsg );
    void cloneSelected( HouseBSData *_house, RenderOrchestrator& rsg );
    void replaceFurnitureWithOneOfItsKind( ArchOrchestrator& asg, RenderOrchestrator& rsg );
    void addNewFurniture( ArchOrchestrator& asg, RenderOrchestrator& rsg );

private:
    void updateFurnitureSelection( RenderOrchestrator& rsg, const AggregatedInputData& aid, const V3f& centerBottomPos,
                                   const C4f& _dotColor );
    [[nodiscard]] bool isMouseOverFurnitureInnerSelector( const V3f& _origin, const V3f& _dir ) const;
    [[nodiscard]] bool canBeManipulated() const;
    [[nodiscard]] bool isActivelySelecting( GHTypeT _ghTypeCheck ) const;
    void
    replaceFurnitureFinal( const EntityMetaData& _furnitureCandidate, ArchOrchestrator& asg, RenderOrchestrator& rsg );
    void cloneInternal( HouseBSData *_house, FittedFurniture *sourceFurniture,
                        const std::shared_ptr<FittedFurniture>& clonedFurniture );
private:
    RemoteEntitySelector& res;
    FeatureIntersection fd;

    FadeInOutSwitch furnitureSelectionAlphaAnim{ explorerFullDotOpacityValue, explorerDotFadeTime };
    bool bRoomBboxCheck = false;
    bool bFurnitureTargetLocked = false;
    bool bFillFullFurnitureOutline = false;
    bool bFurnitureDirty = false;
    bool bColorMaterialWidgetActive = false;
    Plane3f furniturePlane;
    FittedFurniture *furnitureSelected = nullptr;
    V3f centerBottomFurnitureSelected{ V3fc::ZERO };
    std::vector<V3f> furnitureSelectionOutline;
    JMATH::AABB centerBottomBBox;
    V3f prevFurnitureMovePosition{};
    FeatureIntersection fdFurniture;
    FurnitureExplorerReplacer furnitureReplacer;
};
