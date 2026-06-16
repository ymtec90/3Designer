import sys
import os

# Adiciona o diretório build ao path para que o Python localize a biblioteca designer_engine
current_dir = os.path.dirname(os.path.abspath(__file__))
build_path = os.path.join(os.path.dirname(current_dir), 'build')
sys.path.append(build_path)

try:
    import designer_engine
    print("[QA] Módulo 'designer_engine' importado com sucesso!")
except ImportError as e:
    print(f"[QA] Erro ao importar 'designer_engine': {e}")
    sys.exit(1)

def main():
    print("[QA] Inicializando o teste autônomo de rebuild paramétrico...")
    
    # 1. Instanciar a árvore
    tree = designer_engine.FeatureTree()

    # 2. Criar esboço quadrado 10x10 no plano XY
    profile = designer_engine.SketchProfile()
    profile.shapeType = "polygon"
    p1 = designer_engine.gp_Pnt2d(0.0, 0.0)
    p2 = designer_engine.gp_Pnt2d(10.0, 0.0)
    p3 = designer_engine.gp_Pnt2d(10.0, 10.0)
    p4 = designer_engine.gp_Pnt2d(0.0, 10.0)
    profile.points = [p1, p2, p3, p4]

    sketch = designer_engine.SketchFeature("Sketch1", 0, profile)
    tree.AddFeature(sketch)

    # 3. Criar a extrusão de 10.0 (Cubo)
    extrude = designer_engine.ExtrudeFeature("Extrude1", sketch, 10.0, designer_engine.BooleanOpType.None_)
    tree.AddFeature(extrude)

    # 4. Criar o Fillet na Aresta 1 com raio 1.0
    fillet = designer_engine.FilletFeature("Fillet1", 1, 1.0)
    tree.AddFeature(fillet)

    # 5. Executar o Rebuild Inicial
    print("[QA] Executando Rebuild Inicial...")
    if not tree.Rebuild():
        print("[QA] Erro: Rebuild Inicial falhou!")
        sys.exit(1)
        
    print("[QA] Rebuild Inicial com sucesso!")
    tree.PrintTree()
    
    # 6. Alterar o raio usando o novo método SetRadius
    print("\n[QA] Modificando o raio do Fillet de 1.0 para 2.5...")
    fillet.SetRadius(2.5)
    
    # 7. Executar o segundo Rebuild (re-avaliação do novo raio)
    print("[QA] Executando Rebuild após modificação do raio...")
    if not tree.Rebuild():
        print("[QA] Erro: Rebuild pós-modificação falhou!")
        sys.exit(1)
        
    print("[QA] Rebuild pós-modificação com sucesso!")
    tree.PrintTree()
    
    # 8. Validar se a alteração foi persistida no objeto
    print(f"[QA] Validando o raio persistido na feature: {fillet.GetRadius()}")
    if abs(fillet.GetRadius() - 2.5) > 1e-6:
        print("[QA] Erro: O raio retornado não corresponde a 2.5!")
        sys.exit(1)

    # 9. Exportar para STL
    stl_filename = os.path.join(current_dir, "test_rebuild_and_measure.stl")
    print(f"[QA] Exportando sólido final para {stl_filename}...")
    if tree.ExportToSTL(stl_filename, 0.1):
        print(f"[QA] Sucesso: STL exportado com sucesso para {stl_filename}!")
    else:
        print("[QA] Erro ao exportar para STL.")
        sys.exit(1)
        
    print("\n[QA] Teste de Rebuild e Medição automatizado concluído com 100% de SUCESSO!")

if __name__ == "__main__":
    main()
