#include "WorkPlane.hpp"
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <gp_Ax3.hxx>
#include <ElSLib.hxx>
#include <Precision.hxx>
#include <iostream>

WorkPlane::WorkPlane() : m_isValid(true) {
    // Default is the XY plane: origin (0,0,0), normal (0,0,1)
    m_plane = gp_Pln(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
}

WorkPlane::WorkPlane(const gp_Pnt& origin, const gp_Dir& normal, const gp_Dir& xDirection) : m_isValid(true) {
    // gp_Ax3 constructs a right-handed coordinate system using origin, normal vector, and X direction
    gp_Ax3 axisSystem(origin, normal, xDirection);
    m_plane = gp_Pln(axisSystem);
}

WorkPlane::WorkPlane(const TopoDS_Shape& shape, int faceIndex) : m_isValid(false) {
    if (faceIndex <= 0) {
        // Fallback to default XY plane if face index is 0 or negative
        m_plane = gp_Pln(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        m_isValid = true;
    } else {
        m_isValid = ExtractFromFace(shape, faceIndex);
    }
}

WorkPlane::WorkPlane(const TopoDS_Shape& shape, const gp_Dir& targetNormal, const gp_Pnt& targetOrigin, int fallbackIndex) : m_isValid(false) {
    if (ExtractByGeometry(shape, targetNormal, targetOrigin)) {
        m_isValid = true;
    } else if (fallbackIndex > 0) {
        std::cout << "Warning: Face not found geometrically. Falling back to face index: " << fallbackIndex << std::endl;
        m_isValid = ExtractFromFace(shape, fallbackIndex);
    } else {
        // Fallback to default XY plane
        m_plane = gp_Pln(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        m_isValid = true;
    }
}

gp_Pnt WorkPlane::To3D(double u, double v) const {
    // ElSLib maps 2D parameters (u, v) on the analytical plane definition to a 3D coordinate point
    return ElSLib::Value(u, v, m_plane);
}

bool WorkPlane::ExtractFromFace(const TopoDS_Shape& shape, int faceIndex) {
    if (shape.IsNull()) {
        std::cerr << "Error: Shape is null when extracting face plane." << std::endl;
        return false;
    }

    TopExp_Explorer explorer(shape, TopAbs_FACE);
    int current = 1;
    TopoDS_Face targetFace;
    bool found = false;

    while (explorer.More()) {
        if (current == faceIndex) {
            targetFace = TopoDS::Face(explorer.Current());
            found = true;
            break;
        }
        current++;
        explorer.Next();
    }

    if (!found || targetFace.IsNull()) {
        std::cerr << "Error: Face index " << faceIndex << " not found in shape." << std::endl;
        return false;
    }

    BRepAdaptor_Surface surface(targetFace);
    if (surface.GetType() != GeomAbs_Plane) {
        std::cerr << "Error: Face " << faceIndex << " is not planar. Only planar faces can be used as workplanes." << std::endl;
        return false;
    }

    m_plane = surface.Plane();

    // Adjust plane orientation based on topological face orientation
    if (targetFace.Orientation() == TopAbs_REVERSED) {
        gp_Ax3 pos = m_plane.Position();
        gp_Dir zDir = pos.Direction();
        gp_Dir xDir = pos.XDirection();
        
        // Reverse Z axis, maintain right-handed system by preserving X axis direction
        gp_Ax3 reversedPos(pos.Location(), -zDir, xDir);
        m_plane.SetPosition(reversedPos);
    }

    return true;
}

bool WorkPlane::ExtractByGeometry(const TopoDS_Shape& shape, const gp_Dir& targetNormal, const gp_Pnt& targetOrigin) {
    if (shape.IsNull()) {
        return false;
    }

    TopExp_Explorer explorer(shape, TopAbs_FACE);
    while (explorer.More()) {
        TopoDS_Face face = TopoDS::Face(explorer.Current());
        BRepAdaptor_Surface surface(face);
        
        if (surface.GetType() == GeomAbs_Plane) {
            gp_Pln plane = surface.Plane();
            gp_Dir faceNormal = plane.Position().Direction();

            // Adjust normal direction based on topological face orientation
            if (face.Orientation() == TopAbs_REVERSED) {
                faceNormal = -faceNormal;
            }

            // Verify if normals align within angular tolerance
            if (faceNormal.IsEqual(targetNormal, Precision::Angular())) {
                // Verify if the target origin lies on the plane within Precision::Confusion()
                double distance = plane.Distance(targetOrigin);
                if (distance < Precision::Confusion()) {
                    m_plane = plane;

                    // Apply the topological normal adjustment if reversed
                    if (face.Orientation() == TopAbs_REVERSED) {
                        gp_Ax3 pos = m_plane.Position();
                        gp_Dir zDir = pos.Direction();
                        gp_Dir xDir = pos.XDirection();
                        gp_Ax3 reversedPos(pos.Location(), -zDir, xDir);
                        m_plane.SetPosition(reversedPos);
                    }
                    return true;
                }
            }
        }
        explorer.Next();
    }

    return false;
}

