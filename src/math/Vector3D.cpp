#include "Vector3D.h"
#include <stdlib.h>

Vector3D::Vector3D(): x(0), y(0), z(0) {}
Vector3D::Vector3D(Vector3D* other): x(other->getX()), y(other->getY()), z(other->getZ()) {}
Vector3D::Vector3D(double xc, double yc, double zc): x(xc), y(yc), z(zc) {}

double Vector3D::getX() {
	return x;
}

double Vector3D::getY() {
	return y;
}

double Vector3D::getZ() {
	return z;
}

void Vector3D::setX(double xc) {
	x = xc;
}

void Vector3D::setY(double yc) {
	y = yc;
}

void Vector3D::setZ(double zc) {
	z = zc;
}

VectorLine3D::VectorLine3D(Vector3D* vector, double height): vec(vector), offset(height) {}

Vector3D* VectorLine3D::apply(double t) {
	return new Vector3D(
		vec->getX() * t,
		vec->getY() * t,
		vec->getZ() * t
	);
}

Vector3D* VectorLine3D::intersectWithXY() {
	double t = offset / (vec->getZ());

	return apply(t);
}
