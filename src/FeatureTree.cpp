#include "FeatureTree.hpp"
#include <iostream>

void FeatureTree::AddFeature(std::shared_ptr<Feature> feature) {
    m_features.push_back(feature);
}

bool FeatureTree::Rebuild() {
    TopoDS_Shape originalActiveShape = m_activeShape;
    try {
        m_activeShape.Nullify();
        for (auto& feature : m_features) {
            if (!feature->Evaluate()) {
                std::cerr << "Rebuild failed at feature: " << feature->GetName() << std::endl;
                m_activeShape = originalActiveShape;
                return false;
            }
            
            // Active shape is the result of the last 3D solid feature
            if (feature->GetType() == FeatureType::Extrude || feature->GetType() == FeatureType::Revolve) {
                m_activeShape = feature->GetResultShape();
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Validation or Geometry Exception during Rebuild: " << e.what() << std::endl;
        m_activeShape = originalActiveShape;
        return false;
    }
}

void FeatureTree::RemoveLastFeature() {
    if (!m_features.empty()) {
        m_features.pop_back();
    }
}

std::shared_ptr<Feature> FeatureTree::FindFeature(const std::string& name) const {
    for (const auto& feature : m_features) {
        if (feature->GetName() == name) {
            return feature;
        }
    }
    return nullptr;
}

std::shared_ptr<Feature> FeatureTree::GetLastSolidFeature() const {
    // Traverse backwards to find the latest Extrude or Revolve feature
    for (auto it = m_features.rbegin(); it != m_features.rend(); ++it) {
        if ((*it)->GetType() == FeatureType::Extrude || (*it)->GetType() == FeatureType::Revolve) {
            return *it;
        }
    }
    return nullptr;
}

void FeatureTree::PrintTree() const {
    std::cout << "\n--- Parametric Feature History Tree ---" << std::endl;
    if (m_features.empty()) {
        std::cout << "  (Empty Tree)" << std::endl;
        return;
    }
    
    for (size_t i = 0; i < m_features.size(); ++i) {
        const auto& feat = m_features[i];
        std::cout << "  [" << i + 1 << "] Name: " << feat->GetName() << " | Type: ";
        
        switch (feat->GetType()) {
            case FeatureType::Sketch: {
                std::cout << "Sketch";
                auto sketch = std::dynamic_pointer_cast<SketchFeature>(feat);
                if (sketch) {
                    if (sketch->GetFaceIndex() > 0) {
                        std::cout << " (on Face #" << sketch->GetFaceIndex() << " of " 
                                  << (sketch->GetParent() ? sketch->GetParent()->GetName() : "None") << ")";
                    } else {
                        std::cout << " (on global XY plane)";
                    }
                }
                break;
            }
            case FeatureType::Extrude: {
                std::cout << "Extrude";
                auto extrude = std::dynamic_pointer_cast<ExtrudeFeature>(feat);
                if (extrude) {
                    std::cout << " (Depth: " << extrude->GetDepth();
                    if (extrude->GetSketch()) {
                        std::cout << ", Sketch: " << extrude->GetSketch()->GetName();
                    }
                    std::cout << ")";
                }
                break;
            }
            case FeatureType::Revolve: {
                std::cout << "Revolve";
                auto revolve = std::dynamic_pointer_cast<RevolveFeature>(feat);
                if (revolve) {
                    std::cout << " (Angle: " << revolve->GetAngle() 
                              << " rad, Axis: local " << revolve->GetLocalAxisName();
                    if (revolve->GetSketch()) {
                        std::cout << ", Sketch: " << revolve->GetSketch()->GetName();
                    }
                    std::cout << ")";
                }
                break;
            }
        }
        
        // Output Boolean Relationship
        switch (feat->GetBooleanOp()) {
            case BooleanOpType::None:
                std::cout << " | Op: New Solid";
                break;
            case BooleanOpType::Add:
                std::cout << " | Op: Fuse (Add)";
                break;
            case BooleanOpType::Subtract:
                std::cout << " | Op: Cut (Subtract)";
                break;
        }
        
        if (feat->GetResultShape().IsNull()) {
            std::cout << " [Not Evaluated]";
        } else {
            std::cout << " [OK]";
        }
        std::cout << std::endl;
    }
    std::cout << "---------------------------------------\n" << std::endl;
}

void FeatureTree::Clear() {
    m_features.clear();
    m_activeShape.Nullify();
}
