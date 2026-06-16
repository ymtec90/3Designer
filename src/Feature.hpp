#pragma once

#include "WorkPlane.hpp"
#include <TopoDS_Shape.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Ax1.hxx>
#include <string>
#include <vector>
#include <memory>

enum class FeatureType {
    Sketch,
    Extrude,
    Revolve,
    Fillet,
    Chamfer
};

enum class BooleanOpType {
    None,       // Creates a new independent solid
    Add,        // Fuse (union) with parent solid
    Subtract    // Cut (difference) from parent solid
};

// Represents a geometric feature node in the parametric tree
class Feature {
public:
    Feature(const std::string& name, FeatureType type)
        : m_name(name), m_type(type), m_booleanOp(BooleanOpType::None) {}

    virtual ~Feature() = default;

    std::string GetName() const { return m_name; }
    FeatureType GetType() const { return m_type; }

    void SetParent(std::shared_ptr<Feature> parent) { m_parent = parent; }
    std::shared_ptr<Feature> GetParent() const { return m_parent; }

    void SetBooleanOp(BooleanOpType op) { m_booleanOp = op; }
    BooleanOpType GetBooleanOp() const { return m_booleanOp; }

    TopoDS_Shape GetResultShape() const { return m_resultShape; }
    
    // Virtual method that subclasses implement to build geometry
    virtual bool Evaluate() = 0;

protected:
    std::string m_name;
    FeatureType m_type;
    BooleanOpType m_booleanOp;
    std::shared_ptr<Feature> m_parent;
    TopoDS_Shape m_resultShape;
};

// Represents a 2D sketch profile parameters
struct SketchProfile {
    std::string shapeType;           // "circle" or "polygon"
    double radius = 0.0;             // For circles
    gp_Pnt2d center = gp_Pnt2d(0,0); // For circles
    std::vector<gp_Pnt2d> points;    // For polygons (closed sequence)
};

// A SketchFeature creates a planar 2D profile mapped onto a 3D WorkPlane
class SketchFeature : public Feature {
public:
    SketchFeature(const std::string& name, int faceIndex, const SketchProfile& profile)
        : Feature(name, FeatureType::Sketch), m_faceIndex(faceIndex), m_profile(profile), m_hasGeometry(false) {}

    bool Evaluate() override;

    WorkPlane GetWorkPlane() const { return m_workPlane; }
    int GetFaceIndex() const { return m_faceIndex; }
    SketchProfile GetProfile() const { return m_profile; }

private:
    int m_faceIndex; // 0 for global XY plane, > 0 for a face index on the parent shape
    SketchProfile m_profile;
    WorkPlane m_workPlane;

    // Geometric tracking parameters to mitigate index changes during rebuilds
    bool m_hasGeometry;
    gp_Dir m_targetNormal;
    gp_Pnt m_targetOrigin;
};

// An ExtrudeFeature sweeps a SketchFeature along its plane normal
class ExtrudeFeature : public Feature {
public:
    ExtrudeFeature(const std::string& name, std::shared_ptr<SketchFeature> sketch, double depth, BooleanOpType op)
        : Feature(name, FeatureType::Extrude), m_sketch(sketch), m_depth(depth) {
        m_booleanOp = op;
    }

    bool Evaluate() override;
    
    std::shared_ptr<SketchFeature> GetSketch() const { return m_sketch; }
    double GetDepth() const { return m_depth; }

private:
    std::shared_ptr<SketchFeature> m_sketch;
    double m_depth;

    bool ApplyBoolean(const TopoDS_Shape& solid);
};

// A RevolveFeature rotates a SketchFeature around a specified local axis (e.g. Y or X)
class RevolveFeature : public Feature {
public:
    RevolveFeature(const std::string& name, std::shared_ptr<SketchFeature> sketch, double angleRad, const std::string& localAxisName, BooleanOpType op)
        : Feature(name, FeatureType::Revolve), m_sketch(sketch), m_angleRad(angleRad), m_localAxisName(localAxisName) {
        m_booleanOp = op;
    }

    bool Evaluate() override;

    std::shared_ptr<SketchFeature> GetSketch() const { return m_sketch; }
    double GetAngle() const { return m_angleRad; }
    std::string GetLocalAxisName() const { return m_localAxisName; }

private:
    std::shared_ptr<SketchFeature> m_sketch;
    double m_angleRad;
    std::string m_localAxisName; // "X" or "Y" local axes

    bool ApplyBoolean(const TopoDS_Shape& solid);
};

// A FilletFeature rounds specified edges of its parent solid shape
class FilletFeature : public Feature {
public:
    FilletFeature(const std::string& name, int edgeIndex, double radius)
        : Feature(name, FeatureType::Fillet), m_edgeIndex(edgeIndex), m_radius(radius) {}

    bool Evaluate() override;
    
    int GetEdgeIndex() const { return m_edgeIndex; }
    double GetRadius() const { return m_radius; }
    void SetRadius(double new_radius);

private:
    int m_edgeIndex;
    double m_radius;
};

// A ChamferFeature cuts a flat bevel on specified edges of its parent solid shape
class ChamferFeature : public Feature {
public:
    ChamferFeature(const std::string& name, int edgeIndex, double distance)
        : Feature(name, FeatureType::Chamfer), m_edgeIndex(edgeIndex), m_distance(distance) {}

    bool Evaluate() override;

    int GetEdgeIndex() const { return m_edgeIndex; }
    double GetDistance() const { return m_distance; }

private:
    int m_edgeIndex;
    double m_distance;
};

