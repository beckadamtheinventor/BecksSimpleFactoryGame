#ifndef __VEC3_H__
#define __VEC3_H__

#include "../../../lib/raylib/raylib.h"

namespace Components {
    template<class T, class S=double>
    class Vec3 {
        public:
        T x, y, z;
        Vec3() : Vec3(0, 0, 0) {}
        Vec3(T x, T y, T z) {
            this->x = x;
            this->y = y;
            this->z = z;
        }
        void operator+=(Vec3 p) {
            this->x += p.x;
            this->y += p.y;
            this->z += p.z;
        }
        void operator-=(Vec3 p) {
            this->x -= p.x;
            this->y -= p.y;
            this->z -= p.z;
        }
        void operator*=(Vec3 p) {
            this->x *= p.x;
            this->y *= p.y;
            this->z *= p.z;
        }
        void operator/=(Vec3 p) {
            this->x /= p.x;
            this->y /= p.y;
            this->z /= p.z;
        }
        void operator*=(S s) {
            this->x *= s;
            this->y *= s;
            this->z *= s;
        }
        void operator/=(S s) {
            this->x /= s;
            this->y /= s;
            this->z /= s;
        }
        Vec3 operator-() {
            return Vec3(-this->x, -this->y, -this->z);
        }
        Vec3 operator+(Vec3 p) {
            return Vec3(this->x + p.x, this->y + p.y, this->z + p.z);
        }
        Vec3 operator-(Vec3 p) {
            return Vec3(this->x - p.x, this->y - p.y, this->z - p.z);
        }
        Vec3 operator*(Vec3 p) {
            return Vec3(this->x * p.x, this->y * p.y, this->z * p.z);
        }
        Vec3 operator/(Vec3 p) {
            return Vec3(this->x / p.x, this->y / p.y, this->z / p.z);
        }
        Vec3 operator*(S s) {
            return Vec3(this->x * s, this->y * s, this->z * s);
        }
        Vec3 operator/(S s) {
            return Vec3(this->x / s, this->y / s, this->z / s);
        }
        bool operator<(Vec3 other) {
            return (this->x < other.x && this->y < other.y && this->z < other.z);
        }
        bool operator>(Vec3 other) {
            return (this->x > other.x && this->y > other.y && this->z > other.z);
        }
        bool operator<=(Vec3 other) {
            return (this->x <= other.x && this->y <= other.y && this->z <= other.z);
        }
        bool operator>=(Vec3 other) {
            return (this->x >= other.x && this->y >= other.y && this->z >= other.z);
        }
        bool operator<(S other) {
            return (this->x < other && this->y < other && this->z < other);
        }
        bool operator>(S other) {
            return (this->x > other && this->y > other && this->z > other);
        }
        bool operator<=(S other) {
            return (this->x <= other && this->y <= other && this->z <= other);
        }
        bool operator>=(S other) {
            return (this->x >= other && this->y >= other && this->z >= other);
        }
    };

    typedef Vec3<long long> Vec3i;
    class Vec3d : public Vec3<double> {
        public:
        Vec3d(double x, double y, double z) {
            this->x = x;
            this->y = y;
            this->z = z;
        }
        Vec3d(Vector3 other) {
            this->x = other.x;
            this->y = other.y;
            this->z = other.z;
        }
    };
}

#endif