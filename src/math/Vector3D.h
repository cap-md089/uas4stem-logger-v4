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

		void rotate_yaw(double theta);
		void rotate_roll(double theta);
		void rotate_pitch(double theta);
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

class Matrix3D {
	public:
		Matrix3D();
		Matrix3D(double**);

		double get(int, int);

		void set(int, int, double);

		Matrix3D* multiply(Matrix3D*);
		Vector3D* apply(Vector3D*);
	private:
		double data[3][3];
};

void get_xy_view_of_uav(
	double* x, double* y,
	double pitch, double roll, double yaw,
	double altitude,
	double lat, double lng,
	double left, double forward
);

#endif
