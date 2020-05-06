//
//  person.hpp
//  sixthview
//
//  Created by Dado on 05/10/2017.
//
//

#pragma once

#include "core/math/vector3f.h"

class Person  {
public:
	Vector3f Position() const { return mPosition; }
	void Position( const Vector3f& val ) { mPosition = val; }
private:

private:
	Vector3f mPosition;
};
