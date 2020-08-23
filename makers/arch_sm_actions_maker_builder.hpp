//
// Created by dado on 04/06/2020.
//

#pragma once

struct ClearEverthing {
    void operator()( RoomBuilder *rb, ArchOrchestrator& asg ) noexcept {
        rb->clear();
        HouseService::clearHouse(asg.H());
        asg.showIMHouse();
    }
};

static inline void
updateSourceImagesIntoScene( SceneGraph& sg, ArchOrchestrator& asg, const SourceImages& sourceImages ) {
    auto binPropertyId = asg.H()->propertyId + "_bin";
    auto sourceBim = sg.get<RawImage>(binPropertyId);
    if ( sourceBim ) {
        memcpy(sourceBim->data(), sourceImages.sourceFileImageBin.data, sourceBim->size());
        sg.updateRawImage(binPropertyId);
    } else {
        auto sourceBinParams = getImageParamsFromMat(sourceImages.sourceFileImageBin);
        auto sourceBinImage = RawImage{ sourceBinParams.width, sourceBinParams.height, sourceBinParams.channels,
                                        sourceImages.sourceFileImageBin.data };
        sg.addRawImageIM(binPropertyId, sourceBinImage);
    }
}

struct UpdateHMB {
    void operator()( SceneGraph& sg, ArchOrchestrator& asg ) {
        updateSourceImagesIntoScene(sg, asg,
                                    HouseMakerBitmap::prepareImages(asg.H(), *sg.get<RawImage>(asg.H()->propertyId)));
    }
};

struct ChangeElevation {
    void operator()( ArchOrchestrator& asg ) {
        asg.H()->reElevate(asg.H()->elevation);
        asg.showIMHouse();
        asg.pushHouseChange();
    }
};


static inline void
prepareProperty( const PropertyListing& property, ArchOrchestrator& asg, SceneGraph& sg,
                 const std::string& mediaFolder ) {

    auto floorplanImage = RawImage{ FM::readLocalFileC(mediaFolder + property.floorplanUrl) };
    sg.addRawImageIM(property._id, floorplanImage);
    asg.loadHouse(property._id, [&, property]() {
        HouseMakerBitmap::prepareImages(asg.H(), *sg.get<RawImage>(property._id));
        asg.centerCameraMiddleOfHouse();
        asg.onEvent(ArchIOEvents::AIOE_OnLoad);
    }, [&, property]() {
        asg.setHouse(HouseMakerBitmap::makeEmpty(property, *sg.get<RawImage>(property._id)));
        asg.centerCameraMiddleOfHouse();
        asg.onEvent(ArchIOEvents::AIOE_OnLoad);
    });

}

struct CreateHouseTextures {
    void operator()( SceneGraph& sg, ArchOrchestrator& asg ) {
        HouseMakerBitmap::prepareImages(asg.H(), *sg.get<RawImage>(asg.H()->propertyId));
        updateSourceImagesIntoScene(sg, asg, HouseMakerBitmap::getSourceImages());
        asg.showIMHouse();
        asg.onEvent(ArchIOEvents::AIOE_OnLoadComplete);
    }
};

JSONDATA(ExcaliburPostBody, url, upsert)
    std::string url;
    bool upsert = false;
    ExcaliburPostBody( const std::string& url, bool upsert ) : url(url), upsert(upsert) {}
};

struct ImportExcaliburLink {
    void operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc,
                     const CLIParamMap& cli,
                     OnImportExcaliburLinkEvent event ) {
        auto body = ExcaliburPostBody{ event.excaliburLink, false };
        Http::post(Url{ "/property/fetch/floorplan/excalibur" }, body.serialize(),
                   [&]( HttpResponeParams params ) {
                       PropertyListing property{ params.BufferString() };
                       prepareProperty(property, asg, sg, *cli.getParam("mediaFolder"));
//                    asg.saveHouse();
                   });
    }
};

struct CreateNewPropertyFromFloorplanImage {
    void operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc,
                     const CLIParamMap& cli,
                     OnCreateNewPropertyFromFloorplanImageEvent event ) {
        Http::post(Url{ "/property/newFromImage/" + url_encode(getFileName(event.floorplanFileName)) },
                   FM::readLocalFileC(event.floorplanFileName),
                   [&]( HttpResponeParams params ) {
                        auto bs = params.BufferString();
                       rsg.SG().addGenericCallback( [&, bs, cli]() {
                           PropertyListing property{ bs };
                           prepareProperty(property, asg, sg, *cli.getParam("mediaFolder"));
                           asg.saveHouse();
                       });
                   }
        );
    }
};

struct LoadFloorPlan {
    void operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc,
                     const CLIParamMap& cli,
                     OnLoadFloorPlanEvent event ) {
        prepareProperty(event.property, asg, sg, *cli.getParam("mediaFolder"));
    }
};

struct ElaborateHouseBitmap {
    void operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        auto newHouse = HouseMakerBitmap::make(asg.H(), *sg.get<RawImage>(asg.H()->propertyId), asg.FurnitureMap());
        asg.setHouse(newHouse);
        asg.showIMHouse();
        asg.pushHouseChange();
    }
};

struct GlobalRescale {
    void operator()( OnGlobalRescaleEvent event, ArchRenderController& arc, ArchOrchestrator& asg ) {
        float oldScaleFactor = event.oldScaleFactor;
        float currentScaleFactorMeters = event.currentScaleFactorMeters;
        if ( asg.H() ) {
            // We do 2 re-scale because we do not want to have accuracy problems on chaining floating point operations
            // and we also want an absolute number as a scale factor that we can easily serialize.
            // The reason why we need to do 2 rescale is that we do not have a "1.0" scale factor as that depends
            // on the result of the ocr scan of the floorplan, so first we need to invert the current scale
            // then apply the new scale. It's a bit awkward but works.
            asg.H()->reRoot(1.0f / oldScaleFactor, ArchRescaleSpace::FloorplanScaling);
            asg.H()->sourceData.rescaleFactor = metersToCentimeters(currentScaleFactorMeters);
            asg.H()->reRoot(asg.H()->sourceData.rescaleFactor, ArchRescaleSpace::FloorplanScaling);
            // We need a full rebuild of the fittings because scaling doesn't go well with furnitures, IE we cannot
            // scale a sofa, hence only scaling the position will move the sofa away from it's desired location
            // which for example would be "against a wall". So because we cannot apply "scale" to furnitures we need
            // to re-run the complete algorithm to refit everything with the new scale.
            HouseService::guessFittings(asg.H(), asg.FurnitureMap());
            asg.showIMHouse();
            asg.centerCameraMiddleOfHouse();
            asg.pushHouseChange();
        }
    }
};

struct SpecialSpaceToggleFeatureManipulation {
    bool operator()( ArchRenderController& arc, HouseMakerStateMachine& hm, ArchOrchestrator& asg ) noexcept {
        arc.toggleElementsOnSelectionList([&]( const ArchStructuralFeatureDescriptor& asf ) {
            HouseMakerBitmap::makeFromSwapDoorOrWindow(asg.H(), asf.elem->hash);
            asg.pushHouseChange();
        });
        return true;
    }
};


struct KeyToggleFeatureManipulation {
    bool operator()( ArchRenderController& arc, ArchOrchestrator& asg, OnKeyToggleEvent keyEvent ) noexcept {
        if ( keyEvent.keyCode == GMK_A ) {
            arc.splitFirstEdgeOnSelectionList([&]( const ArchStructuralFeatureDescriptor& asf, const V2f& offset ) {
                WallService::splitEdgeAndAddPointInTheMiddle(asf, offset);
                asg.H()->calcBBox();
            });
            asg.pushHouseChange();
            arc.resetSelection();
            return true;
        }
        if ( keyEvent.keyCode == GMK_D ) {
            auto fus = WallService::createTwoShapeAt(asg.H(), keyEvent.viewportPos);
            if ( FloorService::isFloorUShapeValid(fus) ) {
                HouseMakerBitmap::makeAddDoor(asg.H(), fus);
            }
            asg.pushHouseChange();
            return true;
        }
        return false;
    }
};

struct TouchMoveFeatureManipulation {
    bool operator()( const OnTouchMoveViewportSpaceEvent& mouseEvent, ArchOrchestrator& asg,
                     ArchRenderController& arc ) noexcept {
        auto is = mouseEvent.viewportPos;
        arc.moveSelectionList(is, [&]( const ArchStructuralFeatureDescriptor& asf, const V2f& offset ) {
            if ( asf.feature == ArchStructuralFeature::ASF_Poly ) {
                HouseService::moveArch(asg.H(), dynamic_cast<ArchStructural *>(asf.elem), offset);
                asg.pushHouseChange();
            } else {
                WallService::moveFeature(asf, offset, false);
                asg.H()->calcBBox();
                HouseMakerBitmap::makeFromWalls(asg.H());
                asg.pushHouseChange();
            }
        });
        asg.showIMHouse();
        return true;
    }
};
