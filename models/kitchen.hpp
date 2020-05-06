//
//  kitchen.hpp
//
//  Created by Dado on 07/03/2017.
//
//

#ifndef kitchen_hpp
#define kitchen_hpp

#include "house_bsdata.hpp"
#include <core/resources/profile.hpp>

struct KitchenCabinetFiller {
	Vector3f p1;
	Vector3f p2;
	Vector3f normal;
	Vector3f inwardN;
};

class KitchenData {
public:
	const std::vector<std::vector<Vector3f>>& WorktopSegments() const {
		return mWorktopSegments;
	}

	const Profile& KitchenSkirtingProfile() const { return mKitchenSkirtingProfile; }
	const std::vector<Vector3f>& KitchenSkirting() const { return kitchenSkirting; }
private:
	void initCtor();
	void createSkirting();

private:
	std::vector<std::vector<Vector3f>> mWorktopSegments;
	std::vector<Vector3f> kitchenWorktopPath;
	std::vector<Vector3f> kitchenSkirting;

	Profile mKitchenSkirtingProfile;

	// kitchen lengths, dimensions, etc...
	float mDefaultApplianceOverlapOnWotkop;
	float kitchenWorktopDepth;
	float kitchenWorktopHeight;
	float drawerDepth;
	float drawerRecessFromWorktop;
	float worktopHeight;
	float kitchenSkirtingRecess;
	Vector2f drawersPadding;
	Vector2f longDrawersSize;
	Vector2f kitchenSkirtingSize;
	Vector2f kitchenFillerSize;

	friend class KitchenRender;
};

#endif /* kitchen_hpp */
