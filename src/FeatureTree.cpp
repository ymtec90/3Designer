#include "FeatureTree.hpp"
#include <iostream>
#include <stdexcept>
#include <BRepMesh_IncrementalMesh.hxx>
#include <StlAPI_Writer.hxx>

void FeatureTree::AddFeature(std::shared_ptr<Feature> feature) {
    m_features.push_back(feature);
}

bool FeatureTree::Rebuild() {
    // Guarda o estado anterior para garantir rollback atômico em caso de falha geométrica ou exceção
    TopoDS_Shape originalActiveShape = m_activeShape;
    try {
        m_activeShape.Nullify();
        for (auto& feature : m_features) {
            // Garante que cada feature do histórico tenha seu pai atualizado no fluxo de modelagem paramétrica
            // O pai de uma feature de sólido é sempre o sólido resultante anterior
            if (feature->GetType() == FeatureType::Extrude || feature->GetType() == FeatureType::Revolve) {
                auto parent = GetLastSolidFeature();
                // O parent de um nó não pode ser ele mesmo nas iterações
                if (parent && parent != feature) {
                    feature->SetParent(parent);
                }
            }

            // Executa a avaliação paramétrica
            if (!feature->Evaluate()) {
                std::cerr << "Error: Rebuild falhou na Feature '" << feature->GetName() << "'." << std::endl;
                m_activeShape = originalActiveShape;
                return false;
            }
            
            // O sólido ativo representa o acumulado geométrico da última operação de sólido
            if (feature->GetType() == FeatureType::Extrude || feature->GetType() == FeatureType::Revolve) {
                m_activeShape = feature->GetResultShape();
            }
        }
        return true;
    } catch (const std::exception& e) {
        // Captura exceções críticas (como BRepCheck_Analyzer disparando falhas topológicas)
        std::cerr << "Exception capturada no Rebuild: " << e.what() << std::endl;
        m_activeShape = originalActiveShape;
        return false;
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
    // Varre no sentido reverso para obter o sólido ativo anterior da pilha histórica
    for (auto it = m_features.rbegin(); it != m_features.rend(); ++it) {
        if ((*it)->GetType() == FeatureType::Extrude || (*it)->GetType() == FeatureType::Revolve) {
            // Retorna se o recurso já possuir geometria validada
            if (!(*it)->GetResultShape().IsNull()) {
                return *it;
            }
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

void FeatureTree::RemoveLastFeature() {
    if (!m_features.empty()) {
        m_features.pop_back();
    }
}

bool FeatureTree::ExportToSTL(const std::string& filename, double deflection) const {
    if (m_activeShape.IsNull()) {
        std::cerr << "Error: Nao ha solido ativo para exportar." << std::endl;
        return false;
    }

    try {
        // Triangulação incremental da malha (de deflection=0.1mm padrão para discretização)
        BRepMesh_IncrementalMesh mesher(m_activeShape, deflection);
        if (!mesher.IsDone()) {
            std::cerr << "Error: Triangulacao (BRepMesh) falhou." << std::endl;
            return false;
        }

        // Grava a malha triangulada em arquivo STL
        StlAPI_Writer writer;
        writer.Write(m_activeShape, filename.c_str());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception durante exportacao STL: " << e.what() << std::endl;
        return false;
    }
}

