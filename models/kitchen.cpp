//
//  kitchen.cpp
//
//  Created by Dado on 07/03/2017.
//
//

#include "kitchen.hpp"
#include "arch_creator.hpp"

void KitchenData::initCtor() {
	mDefaultApplianceOverlapOnWotkop = 0.02f;
	kitchenWorktopDepth = .7f;
	kitchenWorktopHeight = .04f;
	drawerDepth = 0.015f;
	drawerRecessFromWorktop = 0.02f;
	kitchenSkirtingRecess = 0.04f;
	worktopHeight = 0.90f;// mHouse->FP()->worktopHeight;
	drawersPadding = Vector2f{ 0.0025f };
	longDrawersSize = Vector2f{ 0.90f, 0.40f };
	kitchenFillerSize = Vector2f{ 0.015f, 0.80f };
	kitchenSkirtingSize = Vector2f{ 0.015f, 0.1f };
//	FloorType( FloorMatType::Tiles );
}

void KitchenData::createSkirting() {
	mKitchenSkirtingProfile = loadKitchenWorktopProfile( kitchenSkirtingSize );
	mKitchenSkirtingProfile.move( Vector2f::Y_AXIS*mKitchenSkirtingProfile.height()*0.5f );
}
