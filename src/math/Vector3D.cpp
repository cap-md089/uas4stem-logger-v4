#include "Vector3D.h"
#include <stdlib.h>
#include <iostream>
#include <math.h>

#define FRONT_CAMERA_ANGLE 26.064664078303848196356278789571
#define SIDE_CAMERA_ANGLE 46.367543999867315345946421705557

#define EARTH_RADIUS 6378137

#ifndef PI
#define PI 3.14159265358979
#endif

#define DEG_TO_RAD(deg) ((deg) / 180.0 * PI)
#define RAD_TO_DEG(rad) ((rad) * 180.0 / PI)

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

// Z is rotated for yaw
void Vector3D::rotate_yaw(double theta) {
	double sinTheta = sin(theta);
	double cosTheta = cos(theta);

	x = (x * cosTheta) - (y * sinTheta);
	y = (y * cosTheta) + (x * sinTheta);
}

// X is rotated for pitch
void Vector3D::rotate_pitch(double theta) {
	double sinTheta = sin(theta);
	double cosTheta = cos(theta);

	x = (x * cosTheta) - (z * sinTheta);
	z = (z * cosTheta) + (x * sinTheta);
}

// Y is rotated for roll
void Vector3D::rotate_roll(double theta) {
	double sinTheta = sin(theta);
	double cosTheta = cos(theta);

	std::cout << "theta: " << theta << std::endl;
	std::cout << "theta (deg): " << RAD_TO_DEG(theta) << std::endl;
	std::cout << "sin: " << sinTheta << std::endl;
	std::cout << "cos: " << cosTheta << std::endl;
	std::cout << "cos2,sin2: " << (sinTheta * sinTheta) << "," << (cosTheta * cosTheta) << std::endl;
	std::cout << "cos2 + sin2: " << (sinTheta * sinTheta) + (cosTheta * cosTheta) << std::endl;

	std::cout << "y,z: " << y << "," << z << std::endl;
	std::cout << "y2,z2: " << (y * y) << "," << (z * z) << std::endl;
	std::cout << "y2 + z2: " << ((y * y) + (z * z)) << std::endl;

	y = (y * cosTheta) - (z * sinTheta);
	z = (z * cosTheta) + (y * sinTheta);

	std::cout << "y,z: " << y << "," << z << std::endl;
	std::cout << "y2,z2: " << (y * y) << "," << (z * z) << std::endl;
	std::cout << "y2 + z2: " << ((y * y) + (z * z)) << std::endl;
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
	double t = -offset / vec->getZ();

	std::cout << "t: " << t << std::endl;

	return apply(t);
}

Matrix3D::Matrix3D() {
	data[0][0] = 0;
	data[0][1] = 0;
	data[0][2] = 0;
	data[1][0] = 0;
	data[1][1] = 0;
	data[1][2] = 0;
	data[2][0] = 0;
	data[2][1] = 0;
	data[2][2] = 0;
}
Matrix3D::Matrix3D(double** newData) {
	data[0][0] = newData[0][0];
	data[0][1] = newData[0][1];
	data[0][2] = newData[0][2];
	data[1][0] = newData[1][0];
	data[1][1] = newData[1][1];
	data[1][2] = newData[1][2];
	data[2][0] = newData[2][0];
	data[2][1] = newData[2][1];
	data[2][2] = newData[2][2];
}

void Matrix3D::set(int ix, int iy, double val) {
	data[ix][iy] = val;
}

double Matrix3D::get(int ix, int iy) {
	return data[ix][iy];
}

void get_xy_view_of_uav(
	double* x, double* y,
	double pitch, double roll, double yaw,
	double altitude,
	double lat, double lng,
	double right, double forward
) {
	std::cout << std::endl;

	*x = lat;
	*y = lng;

	Vector3D uav = { 0, 0, -1 };
	Vector3D* uav_target;
	VectorLine3D line = { &uav, altitude };

	std::cout << "x,y,z: " << uav.getX() << "," << uav.getY() << "," << uav.getZ() << std::endl;
	std::cout << "pitch,roll,yaw: " << ((pitch + FRONT_CAMERA_ANGLE * forward)) << "," << ((roll - SIDE_CAMERA_ANGLE * right)) << "," << ((yaw)) << std::endl;
	
/*	double pitch_offset = DegToRad(FRONT_CAMERA_ANGLE * forward);
	double roll_offset = DegToRad(SIDE_CAMERA_ANGLE * right);*/

	uav.rotate_pitch(DEG_TO_RAD(pitch + FRONT_CAMERA_ANGLE * forward));
	uav.rotate_roll(DEG_TO_RAD(roll - SIDE_CAMERA_ANGLE * right));
	uav.rotate_yaw(DEG_TO_RAD(yaw));

	std::cout << "x,y,z: " << uav.getX() << "," << uav.getY() << "," << uav.getZ() << std::endl;

	uav_target = line.intersectWithXY();
	std::cout << "intersect x,y,z: " << uav_target->getX() << "," << uav_target->getY() << "," << uav_target->getZ() << std::endl;

	double dnorth = uav_target->getX() / EARTH_RADIUS;
	double deast = uav_target->getY() / (EARTH_RADIUS * cos(DEG_TO_RAD(lat)));

	std::cout << "dnorth,deast: " << dnorth << "," << deast << std::endl;

	*x = lat + RAD_TO_DEG(dnorth);
	*y = lng + RAD_TO_DEG(deast);

	delete uav_target;
}
