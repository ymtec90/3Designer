#include "Feature.hpp"
#include <Standard_Failure.hxx>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Precision.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>

#include <gp_Circ.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax2.hxx>
#include <iostream>
#include <stdexcept>

// ==========================================
// SketchFeature Implementation
// ==========================================

bool SketchFeature::Evaluate() {
    // Determine the WorkPlane context
    if (m_faceIndex > 0) {
        if (!m_parent) {
            std::cerr << "Error in Sketch '" << m_name << "': Face index was specified, but parent shape is null." << std::endl;
            return false;
        }
        
        TopoDS_Shape parentShape = m_parent->GetResultShape();
        if (parentShape.IsNull()) {
            std::cerr << "Error in Sketch '" << m_name << "': Parent shape is empty/null." << std::endl;
            return false;
        }

        if (m_hasGeometry) {
            // Rebuild context: find face geometrically to mitigate volatility of face indices
            m_workPlane = WorkPlane(parentShape, m_targetNormal, m_targetOrigin, m_faceIndex);
        } else {
            // First time evaluation: find face by index and record its geometry for future rebuilds
            m_workPlane = WorkPlane(parentShape, m_faceIndex);
            if (m_workPlane.IsValid()) {
                m_targetNormal = m_workPlane.GetNormal();
                m_targetOrigin = m_workPlane.GetOrigin();
                m_hasGeometry = true;
            }
        }
    } else {
        // Face index <= 0 indicates sketching on default global XY plane
        m_workPlane = WorkPlane();
    }

    if (!m_workPlane.IsValid()) {
        std::cerr << "Error in Sketch '" << m_name << "': Failed to create a valid WorkPlane." << std::endl;
        return false;
    }

    // Generate profile geometry on WorkPlane
    if (m_profile.shapeType == "circle") {
        if (m_profile.radius <= 0.0) {
            std::cerr << "Error in Sketch '" << m_name << "': Circle radius must be positive." << std::endl;
            return false;
        }

        // Map local 2D center to 3D space
        gp_Pnt center3D = m_workPlane.To3D(m_profile.center.X(), m_profile.center.Y());
        
        // Define positioning: normal to workplane, with x-axis aligned to local x-axis
        gp_Ax2 axes(center3D, m_workPlane.GetNormal(), m_workPlane.GetXAxis());
        gp_Circ circle(axes, m_profile.radius);

        BRepBuilderAPI_MakeEdge edgeMaker(circle);
        if (!edgeMaker.IsDone()) {
            std::cerr << "Error in Sketch '" << m_name << "': Failed to create circle edge." << std::endl;
            return false;
        }

        BRepBuilderAPI_MakeWire wireMaker(edgeMaker.Edge());
        if (!wireMaker.IsDone()) {
            std::cerr << "Error in Sketch '" << m_name << "': Failed to create circle wire." << std::endl;
            return false;
        }

        BRepBuilderAPI_MakeFace faceMaker(wireMaker.Wire());
        if (!faceMaker.IsDone()) {
            std::cerr << "Error in Sketch '" << m_name << "': Failed to create circle face." << std::endl;
            return false;
        }

        m_resultShape = faceMaker.Shape();
        return true;
    } 
    else if (m_profile.shapeType == "polygon") {
        if (m_profile.points.size() < 3) {
            std::cerr << "Error in Sketch '" << m_name << "': Polygon must have at least 3 points." << std::endl;
            return false;
        }

        BRepBuilderAPI_MakePolygon polyMaker;
        for (const auto& pt2d : m_profile.points) {
            gp_Pnt pt3d = m_workPlane.To3D(pt2d.X(), pt2d.Y());
            polyMaker.Add(pt3d);
        }
        polyMaker.Close();

        if (!polyMaker.IsDone()) {
            std::cerr << "Error in Sketch '" << m_name << "': Failed to construct polygon wire." << std::endl;
            return false;
        }

        BRepBuilderAPI_MakeFace faceMaker(polyMaker.Wire());
        if (!faceMaker.IsDone()) {
            std::cerr << "Error in Sketch '" << m_name << "': Failed to create face from polygon." << std::endl;
            return false;
        }

        m_resultShape = faceMaker.Shape();
        return true;
    }

    std::cerr << "Error in Sketch '" << m_name << "': Unknown profile shape type '" << m_profile.shapeType << "'." << std::endl;
    return false;
}

// ==========================================
// ExtrudeFeature Implementation
// ==========================================

bool ExtrudeFeature::Evaluate() {
    if (!m_sketch) {
        std::cerr << "Error in Extrude '" << m_name << "': Input sketch is null." << std::endl;
        return false;
    }

    // Make sure the sketch evaluates first
    if (!m_sketch->Evaluate()) {
        std::cerr << "Error in Extrude '" << m_name << "': Failed to evaluate sketch '" << m_sketch->GetName() << "'." << std::endl;
        return false;
    }

    TopoDS_Shape sketchShape = m_sketch->GetResultShape();
    if (sketchShape.IsNull()) {
        std::cerr << "Error in Extrude '" << m_name << "': Sketch shape is null." << std::endl;
        return false;
    }

    // Extrusion vector is along the normal of the workplane multiplied by depth
    gp_Dir planeNormal = m_sketch->GetWorkPlane().GetNormal();
    gp_Vec extrudeVec(planeNormal);
    extrudeVec.Multiply(m_depth);

    BRepPrimAPI_MakePrism prismMaker(sketchShape, extrudeVec);
    if (!prismMaker.IsDone()) {
        std::cerr << "Error in Extrude '" << m_name << "': BRepPrimAPI_MakePrism failed." << std::endl;
        return false;
    }

    TopoDS_Shape solid = prismMaker.Shape();
    return ApplyBoolean(solid);
}

bool ExtrudeFeature::ApplyBoolean(const TopoDS_Shape& solid) {
    if (m_booleanOp == BooleanOpType::None || !m_parent) {
        // Run validation analyzer on the new solid itself
        BRepCheck_Analyzer analyzer(solid);
        if (!analyzer.IsValid()) {
            throw std::runtime_error("Validation Error: Extruded solid shape is invalid.");
        }
        m_resultShape = solid;
        return true;
    }

    TopoDS_Shape parentShape = m_parent->GetResultShape();
    if (parentShape.IsNull()) {
        std::cerr << "Error in Extrude '" << m_name << "': Parent shape is null for boolean operation." << std::endl;
        return false;
    }

    TopoDS_Shape tempShape;
    if (m_booleanOp == BooleanOpType::Add) {
        BRepAlgoAPI_Fuse fuser(parentShape, solid);
        if (!fuser.IsDone()) {
            std::cerr << "Error in Extrude '" << m_name << "': Boolean FUSE operation failed." << std::endl;
            return false;
        }
        tempShape = fuser.Shape();
    } 
    else if (m_booleanOp == BooleanOpType::Subtract) {
        BRepAlgoAPI_Cut cutter(parentShape, solid);
        if (!cutter.IsDone()) {
            std::cerr << "Error in Extrude '" << m_name << "': Boolean CUT operation failed." << std::endl;
            return false;
        }
        tempShape = cutter.Shape();
    } else {
        tempShape = solid;
    }

    // Mandatory validity check on resulting shape
    BRepCheck_Analyzer analyzer(tempShape);
    if (!analyzer.IsValid()) {
        throw std::runtime_error("Validation Error: The shape resulting from Extrude boolean operation is topologically invalid.");
    }

    m_resultShape = tempShape;
    return true;
}

// ==========================================
// RevolveFeature Implementation
// ==========================================

bool RevolveFeature::Evaluate() {
    if (!m_sketch) {
        std::cerr << "Error in Revolve '" << m_name << "': Input sketch is null." << std::endl;
        return false;
    }

    // Make sure the sketch evaluates first
    if (!m_sketch->Evaluate()) {
        std::cerr << "Error in Revolve '" << m_name << "': Failed to evaluate sketch '" << m_sketch->GetName() << "'." << std::endl;
        return false;
    }

    TopoDS_Shape sketchShape = m_sketch->GetResultShape();
    if (sketchShape.IsNull()) {
        std::cerr << "Error in Revolve '" << m_name << "': Sketch shape is null." << std::endl;
        return false;
    }

    // Determine the axis of revolution in 3D using local workplane axes
    WorkPlane wp = m_sketch->GetWorkPlane();
    gp_Pnt origin = wp.GetOrigin();
    gp_Dir direction = wp.GetYAxis(); // Default is Y axis

    if (m_localAxisName == "X" || m_localAxisName == "x") {
        direction = wp.GetXAxis();
    } else if (m_localAxisName == "Z" || m_localAxisName == "z") {
        direction = wp.GetNormal();
    }

    gp_Ax1 revolveAxis(origin, direction);

    BRepPrimAPI_MakeRevol revolMaker(sketchShape, revolveAxis, m_angleRad);
    if (!revolMaker.IsDone()) {
        std::cerr << "Error in Revolve '" << m_name << "': BRepPrimAPI_MakeRevol failed." << std::endl;
        return false;
    }

    TopoDS_Shape solid = revolMaker.Shape();
    return ApplyBoolean(solid);
}

bool RevolveFeature::ApplyBoolean(const TopoDS_Shape& solid) {
    if (m_booleanOp == BooleanOpType::None || !m_parent) {
        // Run validation analyzer on the new solid itself
        BRepCheck_Analyzer analyzer(solid);
        if (!analyzer.IsValid()) {
            throw std::runtime_error("Validation Error: Revolved solid shape is invalid.");
        }
        m_resultShape = solid;
        return true;
    }

    TopoDS_Shape parentShape = m_parent->GetResultShape();
    if (parentShape.IsNull()) {
        std::cerr << "Error in Revolve '" << m_name << "': Parent shape is null for boolean operation." << std::endl;
        return false;
    }

    TopoDS_Shape tempShape;
    if (m_booleanOp == BooleanOpType::Add) {
        BRepAlgoAPI_Fuse fuser(parentShape, solid);
        if (!fuser.IsDone()) {
            std::cerr << "Error in Revolve '" << m_name << "': Boolean FUSE operation failed." << std::endl;
            return false;
        }
        tempShape = fuser.Shape();
    } 
    else if (m_booleanOp == BooleanOpType::Subtract) {
        BRepAlgoAPI_Cut cutter(parentShape, solid);
        if (!cutter.IsDone()) {
            std::cerr << "Error in Revolve '" << m_name << "': Boolean CUT operation failed." << std::endl;
            return false;
        }
        tempShape = cutter.Shape();
    } else {
        tempShape = solid;
    }

    // Mandatory validity check on resulting shape
    BRepCheck_Analyzer analyzer(tempShape);
    if (!analyzer.IsValid()) {
        throw std::runtime_error("Validation Error: The shape resulting from Revolve boolean operation is topologically invalid.");
    }

    m_resultShape = tempShape;
    return true;
}

// ==========================================
// FilletFeature Implementation
// ==========================================

bool FilletFeature::Evaluate() {
    if (!m_parent) {
        std::cerr << "Error in Fillet '" << m_name << "': Parent solid shape is null." << std::endl;
        return false;
    }

    TopoDS_Shape parentShape = m_parent->GetResultShape();
    if (parentShape.IsNull()) {
        std::cerr << "Error in Fillet '" << m_name << "': Parent geometry is empty." << std::endl;
        return false;
    }

    // Explores edges to find the one matching m_edgeIndex
    TopExp_Explorer explorer(parentShape, TopAbs_EDGE);
    int current = 1;
    TopoDS_Edge targetEdge;
    bool found = false;

    while (explorer.More()) {
        if (current == m_edgeIndex) {
            targetEdge = TopoDS::Edge(explorer.Current());
            found = true;
            break;
        }
        current++;
        explorer.Next();
    }

    if (!found || targetEdge.IsNull()) {
        std::cerr << "Error in Fillet '" << m_name << "': Edge index " << m_edgeIndex << " not found in shape." << std::endl;
        return false;
    }

    if (m_radius <= Precision::Confusion()) {
        std::cerr << "Error in Fillet '" << m_name << "': Fillet radius must be greater than Confusion tolerance." << std::endl;
        return false;
    }

    try {
        BRepFilletAPI_MakeFillet filletMaker(parentShape);
        filletMaker.Add(m_radius, targetEdge);
        filletMaker.Build();

        if (!filletMaker.IsDone()) {
            std::cerr << "Error in Fillet '" << m_name << "': BRepFilletAPI_MakeFillet construction failed." << std::endl;
            return false;
        }

        TopoDS_Shape result = filletMaker.Shape();

        // Validamos a topologia da peça arredondada resultante para evitar quebras geométricas
        BRepCheck_Analyzer analyzer(result);
        if (!analyzer.IsValid()) {
            throw std::runtime_error("Validation Error: O arredondamento (Fillet) resultou em uma topologia invalida (raio muito grande?).");
        }

        m_resultShape = result;
        return true;
    } catch (const Standard_Failure& sf) {
        std::cerr << "OpenCASCADE Exception em Fillet '" << m_name << "': " << sf.GetMessageString() << std::endl;
        throw std::runtime_error(std::string("OpenCASCADE Error: ") + sf.GetMessageString());
    } catch (const std::exception& e) {
        std::cerr << "Exception em Fillet '" << m_name << "': " << e.what() << std::endl;
        throw; // Repassa a exceção para que a FeatureTree possa capturar e efetuar rollback
    }
}

// ==========================================
// ChamferFeature Implementation
// ==========================================

bool ChamferFeature::Evaluate() {
    if (!m_parent) {
        std::cerr << "Error in Chamfer '" << m_name << "': Parent solid shape is null." << std::endl;
        return false;
    }

    TopoDS_Shape parentShape = m_parent->GetResultShape();
    if (parentShape.IsNull()) {
        std::cerr << "Error in Chamfer '" << m_name << "': Parent geometry is empty." << std::endl;
        return false;
    }

    // Explores edges to find the one matching m_edgeIndex
    TopExp_Explorer explorer(parentShape, TopAbs_EDGE);
    int current = 1;
    TopoDS_Edge targetEdge;
    bool found = false;

    while (explorer.More()) {
        if (current == m_edgeIndex) {
            targetEdge = TopoDS::Edge(explorer.Current());
            found = true;
            break;
        }
        current++;
        explorer.Next();
    }

    if (!found || targetEdge.IsNull()) {
        std::cerr << "Error in Chamfer '" << m_name << "': Edge index " << m_edgeIndex << " not found in shape." << std::endl;
        return false;
    }

    if (m_distance <= Precision::Confusion()) {
        std::cerr << "Error in Chamfer '" << m_name << "': Chamfer distance must be greater than Confusion tolerance." << std::endl;
        return false;
    }

    try {
        BRepFilletAPI_MakeChamfer chamferMaker(parentShape);
        chamferMaker.Add(m_distance, targetEdge);
        chamferMaker.Build();

        if (!chamferMaker.IsDone()) {
            std::cerr << "Error in Chamfer '" << m_name << "': BRepFilletAPI_MakeChamfer construction failed." << std::endl;
            return false;
        }

        TopoDS_Shape result = chamferMaker.Shape();

        // Validação topológica
        BRepCheck_Analyzer analyzer(result);
        if (!analyzer.IsValid()) {
            throw std::runtime_error("Validation Error: O chanfro (Chamfer) resultou em uma topologia invalida (distancia muito grande?).");
        }

        m_resultShape = result;
        return true;
    } catch (const Standard_Failure& sf) {
        std::cerr << "OpenCASCADE Exception em Chamfer '" << m_name << "': " << sf.GetMessageString() << std::endl;
        throw std::runtime_error(std::string("OpenCASCADE Error: ") + sf.GetMessageString());
    } catch (const std::exception& e) {
        std::cerr << "Exception em Chamfer '" << m_name << "': " << e.what() << std::endl;
        throw;
    }
}

