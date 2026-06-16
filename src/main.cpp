#include "FeatureTree.hpp"
#include "Feature.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <gp_Pnt2d.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

int main() {
    std::cout << "========================================================" << std::endl;
    std::cout << "       3Designer - Sanity Integration Smoke Test        " << std::endl;
    std::cout << "========================================================" << std::endl;
    
    try {
        // 1. Instanciar o FeatureTree
        FeatureTree tree;
        std::cout << "[Step 1] FeatureTree initialized successfully." << std::endl;

        // 2. Criar uma SketchFeature (círculo de raio 10 no plano XY)
        SketchProfile baseSketchProfile;
        baseSketchProfile.shapeType = "circle";
        baseSketchProfile.radius = 10.0;
        baseSketchProfile.center = gp_Pnt2d(0, 0);

        // faceIndex = 0 significa plano XY global
        auto baseSketch = std::make_shared<SketchFeature>("BaseSketch", 0, baseSketchProfile);
        std::cout << "[Step 2] BaseSketch created." << std::endl;

        // 3. Criar uma ExtrudeFeature (unindo o esboço para criar o cilindro base)
        // Como é o primeiro sólido, usamos BooleanOpType::None (New Solid)
        auto baseCylinder = std::make_shared<ExtrudeFeature>("BaseCylinder", baseSketch, 30.0, BooleanOpType::None);
        std::cout << "[Step 3] BaseCylinder created." << std::endl;

        // Adicionar as features à FeatureTree e executar o Rebuild() inicial
        tree.AddFeature(baseSketch);
        tree.AddFeature(baseCylinder);

        std::cout << "[Step 4] Running initial Rebuild..." << std::endl;
        if (!tree.Rebuild()) {
            std::cerr << "[FAIL] Initial rebuild failed!" << std::endl;
            return 1;
        }
        std::cout << "[PASS] Initial rebuild succeeded. Base cylinder created." << std::endl;

        // 4. Adicionar a segunda SketchFeature (círculo menor de raio 5)
        // Usamos faceIndex = 2 (face circular superior) do cilindro base.
        // O SketchFeature irá extrair a face, obter a normal (+Z) e a origem (Z = 30) e salvá-los.
        SketchProfile holeSketchProfile;
        holeSketchProfile.shapeType = "circle";
        holeSketchProfile.radius = 5.0;
        holeSketchProfile.center = gp_Pnt2d(0, 0);

        auto holeSketch = std::make_shared<SketchFeature>("HoleSketch", 2, holeSketchProfile);
        holeSketch->SetParent(baseCylinder); // Vincula genealogia (pai do esboço é o cilindro)
        std::cout << "[Step 5] HoleSketch created on top surface (Face 2)." << std::endl;

        // 5. Criar uma ExtrudeFeature (operação de Subtract) para fazer um furo passante
        // Extrudamos para baixo (-40 unidades) para cortar o cilindro de altura 30.
        auto holeCut = std::make_shared<ExtrudeFeature>("HoleCut", holeSketch, -40.0, BooleanOpType::Subtract);
        holeCut->SetParent(baseCylinder); // O pai do corte sólido é o cilindro sólido
        std::cout << "[Step 6] HoleCut (Subtract operation) created." << std::endl;

        tree.AddFeature(holeSketch);
        tree.AddFeature(holeCut);

        // 6. Executar o Rebuild() final para validar a integridade topológica
        std::cout << "[Step 7] Running final Rebuild (generating through hole)..." << std::endl;
        if (!tree.Rebuild()) {
            std::cerr << "[FAIL] Final rebuild failed!" << std::endl;
            return 2;
        }
        std::cout << "[PASS] Final rebuild succeeded. Topological Cut complete." << std::endl;

        // 7. Chamar ExportToSTL("teste_integracao.stl")
        std::cout << "[Step 8] Exporting active solid to STL..." << std::endl;
        if (!tree.ExportToSTL("teste_integracao.stl", 0.1)) {
            std::cerr << "[FAIL] STL export failed!" << std::endl;
            return 3;
        }
        std::cout << "[PASS] STL export succeeded! File 'teste_integracao.stl' created." << std::endl;
        
        std::cout << "\n=== SMOKE TEST PASSED SUCCESSFULLY ===" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n[CRITICAL ERROR] Exception caught in main execution: " << e.what() << std::endl;
        return 99;
    }
}
