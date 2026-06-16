#pragma once

#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>

class WorkPlane {
public:
    // Default constructor: creates a plane aligned with the global XY plane
    WorkPlane();

    // Constructor with custom origin, normal, and xDirection
    WorkPlane(const gp_Pnt& origin, const gp_Dir& normal, const gp_Dir& xDirection);

    // Constructor that extracts the plane from a face of a shape by index (1-based)
    WorkPlane(const TopoDS_Shape& shape, int faceIndex);

    // Constructor that extracts the plane geometrically (normal and origin) or falls back to index
    WorkPlane(const TopoDS_Shape& shape, const gp_Dir& targetNormal, const gp_Pnt& targetOrigin, int fallbackIndex = 0);

    // Maps local 2D (u, v) coordinates to a 3D point in space
    gp_Pnt To3D(double u, double v) const;

    // Returns the underlying OpenCASCADE plane representation
    gp_Pln GetPlane() const { return m_plane; }

    // Returns whether this plane is valid
    bool IsValid() const { return m_isValid; }

    // Coordinate system queries
    gp_Pnt GetOrigin() const { return m_plane.Location(); }
    gp_Dir GetNormal() const { return m_plane.Position().Direction(); }
    gp_Dir GetXAxis() const { return m_plane.Position().XDirection(); }
    gp_Dir GetYAxis() const { return m_plane.Position().YDirection(); }

private:
    gp_Pln m_plane;
    bool m_isValid;

    // Internal helper to find and extract a planar face by index
    bool ExtractFromFace(const TopoDS_Shape& shape, int faceIndex);

    // Internal helper to find and extract a planar face geometrically
    bool ExtractByGeometry(const TopoDS_Shape& shape, const gp_Dir& targetNormal, const gp_Pnt& targetOrigin);
};

