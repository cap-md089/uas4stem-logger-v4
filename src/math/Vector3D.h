#ifndef _VECTOR3D_H_
#define _VECTOR3D_H_

class Vector3D {
	public:
		Vector3D();
		Vector3D(Vector3D* other);
		Vector3D(double xc, double yc, double zc);
		double getX();
		double getY();
		double getZ();

		void setX(double xc);
		void setY(double yc);
		void setZ(double zc);
	private:
		double x;
		double y;
		double z;
};

class VectorLine3D {
	public:
		VectorLine3D(Vector3D* vector, double height);
		Vector3D* apply(double t);
		Vector3D* intersectWithXY();
	private:
		Vector3D* vec;
		double offset;
};

void get_xy_view_of_uav(
	double* x, double* y,
	double pitch, double roll, double yaw,
	double lat, double lng
);

#endif
