#pragma once

#include <array>
#include <cmath>
#include <stdexcept>
#include <functional>
#include <iostream>

struct FVector4
{
public:
    FVector4(float X, float Y, float Z, float W) : X(X), Y(Y), Z(Z), W(W) {}
    FVector4() = default;
    float X;
    float Y;
    float Z;
    float W;

    bool operator==(const FVector4& Other) const
    {
        return (X == Other.X && Y == Other.Y && Z == Other.Z && W == Other.W);
    }
};

struct FVector3
{
public:
    FVector3(float X, float Y, float Z): X(X), Y(Y), Z(Z) {}
    FVector3() = default;
public:
    float X;
    float Y;
    float Z;

    FVector3 GetNormalized() const
    {
        float L = std::sqrt(X * X + Y * Y + Z * Z);

        if (L <= 0.000001f)
        {
            throw std::runtime_error("Failed to normalize FVector3");
        }

        return FVector3(X / L, Y / L, Z / L);
    }

    bool operator==(const FVector3& Other) const
    {
        return (X == Other.X && Y == Other.Y && Z == Other.Z);
    }
};

template<> struct std::hash<FVector3> {
    size_t operator()(FVector3 const &Vector) const {
        size_t hash = 0;
        hash = std::hash<float>{}(Vector.X) ^
                ((std::hash<float>{}(Vector.Y) << 1) >> 1) ^
                (std::hash<float>{}(Vector.Z) << 1);

        return hash;
    }
};

FVector3 operator*(const FVector3& L, const FVector3& R)
{
    return FVector3(L.Y * R.Z - R.Y * L.Z, L.Z * R.X - R.Z * L.X, L.X * R.Y - R.X * L.Y);
}

FVector3 operator-(const FVector3& A, const FVector3& B)
{
    return FVector3(A.X - B.X, A.Y - B.Y, A.Z - B.Z);
}

float Dot(const FVector3& L, const FVector3& R)
{
    return L.X * R.X + L.Y * R.Y + L.Z * R.Z;
}

struct FVector2
{
public:
    FVector2(float X, float Y) : X(X), Y(Y) {};
    FVector2() = default;
public:
    float X;
    float Y;

    bool operator==(const FVector2& Other) const
    {
        return (X == Other.X && Y == Other.Y);
    }
};

template<> struct std::hash<FVector2> {
    size_t operator()(FVector2 const &Vector) const {
        size_t hash = 0;
        hash = std::hash<float>{}(Vector.X) ^
               ((std::hash<float>{}(Vector.Y) << 1) >> 1);

        return hash;
    }
};

struct FMatrix3
{
public:
    FMatrix3() : Data({FVector3{1.f, 0.f, 0.f}, FVector3{0.f, 1.f, 0.f}, FVector3{0.f, 0.f, 1.f}}){}
    std::array<FVector3, 3> Data;
};

struct FMatrix4 {
public:
    FMatrix4() : Data({FVector4{1.f, 0.f, 0.f, 0.f}, FVector4{0.f, 1.f, 0.f, 0.f}, FVector4{0.f, 0.f, 1.f, 0.f}, FVector4{0.f, 0.f, 0.f, 1.f}}){}
    std::array<FVector4, 4> Data;
};

static FMatrix4 Rotate(float Angle, const FVector3& Axis)
{
    auto C = std::cos(Angle);
    auto S = std::sin(Angle);

    FMatrix4 RotationMatrix;

    RotationMatrix.Data[0].X = C + (1.f - C) * Axis.X * Axis.X;
    RotationMatrix.Data[0].Y = (1.f - C) * Axis.X * Axis.Y - S * Axis.Z;
    RotationMatrix.Data[0].Z = (1.f - C) * Axis.X * Axis.Z + S * Axis.Y;
    RotationMatrix.Data[0].W = 0.f;

    RotationMatrix.Data[1].X = (1.f - C) * Axis.X * Axis.Y + S * Axis.Z;
    RotationMatrix.Data[1].Y = C + (1.f - C) * Axis.Y * Axis.Y;
    RotationMatrix.Data[1].Z = (1.f - C) * Axis.Y * Axis.Z - S * Axis.X;
    RotationMatrix.Data[1].W = 0.f;

    RotationMatrix.Data[2].X = (1.f - C) * Axis.X * Axis.Z - S * Axis.Y;
    RotationMatrix.Data[2].Y = (1.f - C) * Axis.Y * Axis.Z + S * Axis.X;
    RotationMatrix.Data[2].Z = C + (1.f - C) * Axis.Z * Axis.Z;
    RotationMatrix.Data[2].W = 0.f;

    RotationMatrix.Data[3].X = 0.f;
    RotationMatrix.Data[3].Y = 0.f;
    RotationMatrix.Data[3].Z = 0.f;
    RotationMatrix.Data[3].W = 1.f;

    return RotationMatrix;
}

static FMatrix4 LookAt(const FVector3& Eye, const FVector3& Point, const FVector3& Up)
{
    FMatrix4 RotationMatrix;

    FVector3 F = (Point - Eye).GetNormalized();
    FVector3 R = (F * Up).GetNormalized();
    FVector3 U = (R * F).GetNormalized();

    RotationMatrix.Data[0].X = R.X;
    RotationMatrix.Data[0].Y = U.X;
    RotationMatrix.Data[0].Z = -F.X;
    RotationMatrix.Data[0].W = 0.f;

    RotationMatrix.Data[1].X = R.Y;
    RotationMatrix.Data[1].Y = U.Y;
    RotationMatrix.Data[1].Z = -F.Y;
    RotationMatrix.Data[1].W = 0.f;

    RotationMatrix.Data[2].X = R.Z;
    RotationMatrix.Data[2].Y = U.Z;
    RotationMatrix.Data[2].Z = -F.Z;
    RotationMatrix.Data[2].W = 0.f;

    RotationMatrix.Data[3].X = -Dot(R, Eye);
    RotationMatrix.Data[3].Y = -Dot(U, Eye);
    RotationMatrix.Data[3].Z = Dot(F, Eye);
    RotationMatrix.Data[3].W = 1.f;

    return RotationMatrix;
}

static FMatrix4 GetPerspective(float FOV, float AspectRatio, float NearDistance, float FarDistance)
{
    FMatrix4 PerspectiveMatrix;

    auto TanHalfFOV = std::tan(FOV / 2.f);

    PerspectiveMatrix.Data[0].X = 1.f / (AspectRatio * TanHalfFOV);
    PerspectiveMatrix.Data[1].Y = -1.f / TanHalfFOV;
    PerspectiveMatrix.Data[2].Z = -(FarDistance + NearDistance) / (FarDistance - NearDistance);
    PerspectiveMatrix.Data[2].W = -1.f;
    PerspectiveMatrix.Data[3].Z = -(FarDistance * NearDistance) / (FarDistance - NearDistance);
    PerspectiveMatrix.Data[3].W = 0.f;

    return PerspectiveMatrix;
}
