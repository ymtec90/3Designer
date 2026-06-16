#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <gp_Pnt2d.hxx>
#include "Feature.hpp"
#include "FeatureTree.hpp"

namespace py = pybind11;

PYBIND11_MODULE(designer_engine, m) {
    m.doc() = "3Designer Geometric Modeler Engine Python Bindings";

    // gp_Pnt2d
    py::class_<gp_Pnt2d>(m, "gp_Pnt2d")
        .def(py::init<Standard_Real, Standard_Real>())
        .def("X", &gp_Pnt2d::X)
        .def("Y", &gp_Pnt2d::Y)
        .def("__repr__", [](const gp_Pnt2d& p) {
            return "(" + std::to_string(p.X()) + ", " + std::to_string(p.Y()) + ")";
        });

    // Enums
    py::enum_<FeatureType>(m, "FeatureType")
        .value("Sketch", FeatureType::Sketch)
        .value("Extrude", FeatureType::Extrude)
        .value("Revolve", FeatureType::Revolve)
        .value("Fillet", FeatureType::Fillet)
        .value("Chamfer", FeatureType::Chamfer);

    py::enum_<BooleanOpType>(m, "BooleanOpType")
        .value("None_", BooleanOpType::None)
        .value("None", BooleanOpType::None)
        .value("Add", BooleanOpType::Add)
        .value("Subtract", BooleanOpType::Subtract);

    // Structures
    py::class_<SketchProfile>(m, "SketchProfile")
        .def(py::init<>())
        .def_readwrite("shapeType", &SketchProfile::shapeType)
        .def_readwrite("radius", &SketchProfile::radius)
        .def_readwrite("center", &SketchProfile::center)
        .def_readwrite("points", &SketchProfile::points);

    // Feature Base Class
    py::class_<Feature, std::shared_ptr<Feature>>(m, "Feature")
        .def("GetName", &Feature::GetName)
        .def("GetType", &Feature::GetType)
        .def("GetBooleanOp", &Feature::GetBooleanOp)
        .def("SetBooleanOp", &Feature::SetBooleanOp)
        .def("SetParent", &Feature::SetParent)
        .def("GetParent", &Feature::GetParent)
        .def("Evaluate", &Feature::Evaluate);

    // Derived Features
    py::class_<SketchFeature, Feature, std::shared_ptr<SketchFeature>>(m, "SketchFeature")
        .def(py::init<const std::string&, int, const SketchProfile&>())
        .def("GetFaceIndex", &SketchFeature::GetFaceIndex)
        .def("GetProfile", &SketchFeature::GetProfile);

    py::class_<ExtrudeFeature, Feature, std::shared_ptr<ExtrudeFeature>>(m, "ExtrudeFeature")
        .def(py::init<const std::string&, std::shared_ptr<SketchFeature>, double, BooleanOpType>())
        .def("GetSketch", &ExtrudeFeature::GetSketch)
        .def("GetDepth", &ExtrudeFeature::GetDepth);

    py::class_<RevolveFeature, Feature, std::shared_ptr<RevolveFeature>>(m, "RevolveFeature")
        .def(py::init<const std::string&, std::shared_ptr<SketchFeature>, double, const std::string&, BooleanOpType>())
        .def("GetSketch", &RevolveFeature::GetSketch)
        .def("GetAngle", &RevolveFeature::GetAngle)
        .def("GetLocalAxisName", &RevolveFeature::GetLocalAxisName);

    py::class_<FilletFeature, Feature, std::shared_ptr<FilletFeature>>(m, "FilletFeature")
        .def(py::init<const std::string&, int, double>())
        .def("GetEdgeIndex", &FilletFeature::GetEdgeIndex)
        .def("GetRadius", &FilletFeature::GetRadius);

    py::class_<ChamferFeature, Feature, std::shared_ptr<ChamferFeature>>(m, "ChamferFeature")
        .def(py::init<const std::string&, int, double>())
        .def("GetEdgeIndex", &ChamferFeature::GetEdgeIndex)
        .def("GetDistance", &ChamferFeature::GetDistance);

    // Feature Tree
    py::class_<FeatureTree, std::shared_ptr<FeatureTree>>(m, "FeatureTree")
        .def(py::init<>())
        .def("AddFeature", &FeatureTree::AddFeature)
        .def("Rebuild", &FeatureTree::Rebuild)
        .def("GetFeatures", &FeatureTree::GetFeatures)
        .def("FindFeature", &FeatureTree::FindFeature)
        .def("GetLastSolidFeature", &FeatureTree::GetLastSolidFeature)
        .def("PrintTree", &FeatureTree::PrintTree)
        .def("Clear", &FeatureTree::Clear)
        .def("RemoveLastFeature", &FeatureTree::RemoveLastFeature)
        .def("ExportToSTL", &FeatureTree::ExportToSTL, py::arg("filename"), py::arg("deflection") = 0.1);
}
