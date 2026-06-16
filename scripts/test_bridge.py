import sys
import os

# Adiciona o diretório build ao path para que o Python localize a biblioteca designer_engine.so
build_path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'build')
sys.path.append(build_path)

try:
    import designer_engine
    print("Módulo 'designer_engine' importado com sucesso!")
except ImportError as e:
    print(f"Erro ao importar 'designer_engine': {e}")
    sys.exit(1)

def main():
    # 1. Instanciar a árvore de features
    tree = designer_engine.FeatureTree()

    # 2. Criar o perfil do esboço (SketchProfile)
    profile = designer_engine.SketchProfile()
    profile.shapeType = "polygon"
    
    # Definir os pontos de um quadrado 10x10 no plano local
    p1 = designer_engine.gp_Pnt2d(0.0, 0.0)
    p2 = designer_engine.gp_Pnt2d(10.0, 0.0)
    p3 = designer_engine.gp_Pnt2d(10.0, 10.0)
    p4 = designer_engine.gp_Pnt2d(0.0, 10.0)
    profile.points = [p1, p2, p3, p4]

    # 3. Criar a SketchFeature (sobre o plano XY global: faceIndex = 0)
    sketch = designer_engine.SketchFeature("Sketch1", 0, profile)
    tree.AddFeature(sketch)

    # 4. Criar a ExtrudeFeature (Extrusão de 10 unidades, sem operação booleana)
    extrude = designer_engine.ExtrudeFeature(
        "Extrude1", 
        sketch, 
        10.0, 
        designer_engine.BooleanOpType.None_
    )
    tree.AddFeature(extrude)

    # 5. Adicionar modificação de aresta (Arredondamento / Fillet de raio 1.5 na Aresta 1)
    fillet = designer_engine.FilletFeature("Fillet1", 1, 1.5)
    tree.AddFeature(fillet)

    # 6. Reconstruir a árvore paramétrica
    print("\nExecutando o Rebuild do motor paramétrico...")
    if tree.Rebuild():
        print("Rebuild realizado com sucesso!")
        
        # Exibe a árvore formatada
        tree.PrintTree()
        
        # 7. Exportar para arquivo STL
        stl_name = "test_bridge.stl"
        print(f"Exportando sólido ativo para '{stl_name}'...")
        if tree.ExportToSTL(stl_name, 0.1):
            print("Exportação concluída com sucesso!")
        else:
            print("Falha ao exportar o arquivo STL.")
    else:
        print("Falha na reconstrução (Rebuild) da árvore de features.")

if __name__ == "__main__":
    main()
