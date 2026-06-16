import sys
import os

# Resolve o caminho do diretório build para importar designer_engine
current_dir = os.path.dirname(os.path.abspath(__file__))
build_path = os.path.join(os.path.dirname(current_dir), 'build')
if build_path not in sys.path:
    sys.path.append(build_path)

try:
    import designer_engine
except ImportError as e:
    # Fallback caso já esteja instalado no path global do Python
    try:
        import designer_engine
    except ImportError:
        raise ImportError(
            f"Não foi possível importar 'designer_engine'. Certifique-se de compilar o projeto. Detalhes: {e}"
        )

class EngineProxy:
    """
    Classe Proxy que encapsula as chamadas C++ do designer_engine,
    oferecendo métodos de alto nível para a futura interface gráfica.
    """
    def __init__(self):
        self.tree = designer_engine.FeatureTree()

    def get_features(self):
        """Retorna a lista de features cadastradas no motor."""
        return self.tree.GetFeatures()

    def rebuild(self) -> bool:
        """Recontrói toda a geometria paramétrica a partir do histórico."""
        return self.tree.Rebuild()

    def clear(self):
        """Limpa todo o histórico da árvore paramétrica."""
        self.tree.Clear()

    def remove_last_feature(self):
        """Remove a última feature adicionada e reconstrói o motor."""
        self.tree.RemoveLastFeature()
        return self.rebuild()

    def print_tree(self):
        """Imprime a árvore de features no console (útil para debug)."""
        self.tree.PrintTree()

    def get_active_shape(self):
        """Retorna a representação do TopoDS_Shape ativo (acumulado)."""
        return self.tree.GetActiveShape()

    def export_to_stl(self, filename: str, deflection: float = 0.1) -> bool:
        """Exporta o sólido ativo atual para um arquivo de malha STL."""
        return self.tree.ExportToSTL(filename, deflection)

    def add_sketch(self, name: str, face_index: int, shape_type: str, radius: float = 0.0, center_uv=(0.0, 0.0), points_uv=None) -> bool:
        """
        Adiciona uma SketchFeature ao motor paramétrico.
        Retorna True em caso de sucesso.
        """
        profile = designer_engine.SketchProfile()
        profile.shapeType = shape_type.lower()
        
        if profile.shapeType == "circle":
            profile.radius = radius
            profile.center = designer_engine.gp_Pnt2d(center_uv[0], center_uv[1])
        elif profile.shapeType == "polygon":
            if not points_uv:
                raise ValueError("Polígono requer uma lista de coordenadas U,V (points_uv).")
            profile.points = [designer_engine.gp_Pnt2d(pt[0], pt[1]) for pt in points_uv]
        else:
            raise ValueError(f"Formato de esboço '{shape_type}' não suportado.")

        sketch = designer_engine.SketchFeature(name, face_index, profile)
        
        # Vincula o pai (se houver um sólido anterior)
        parent = self.tree.GetLastSolidFeature()
        sketch.SetParent(parent)

        # Validação seca (dry run) antes de inserir definitivamente na árvore
        if sketch.Evaluate():
            self.tree.AddFeature(sketch)
            return True
        return False

    def add_extrude(self, name: str, sketch_name: str, depth: float, op_type: str = "none") -> bool:
        """
        Adiciona uma ExtrudeFeature (Extrusão) associada ao sketch especificado.
        """
        sketch = self.tree.FindFeature(sketch_name)
        if not sketch:
            raise ValueError(f"Sketch '{sketch_name}' não encontrado.")

        if not isinstance(sketch, designer_engine.SketchFeature):
            raise ValueError(f"A feature '{sketch_name}' não é uma SketchFeature válida.")

        op_map = {
            "none": designer_engine.BooleanOpType.None_,
            "add": designer_engine.BooleanOpType.Add,
            "subtract": designer_engine.BooleanOpType.Subtract
        }
        op = op_map.get(op_type.lower(), designer_engine.BooleanOpType.None_)

        extrude = designer_engine.ExtrudeFeature(name, sketch, depth, op)
        
        parent = self.tree.GetLastSolidFeature()
        extrude.SetParent(parent)

        self.tree.AddFeature(extrude)
        return self.rebuild()

    def add_revolve(self, name: str, sketch_name: str, angle_degrees: float, axis: str = "Y", op_type: str = "none") -> bool:
        """
        Adiciona uma RevolveFeature (Revolução) associada ao sketch especificado.
        """
        sketch = self.tree.FindFeature(sketch_name)
        if not sketch:
            raise ValueError(f"Sketch '{sketch_name}' não encontrado.")

        if not isinstance(sketch, designer_engine.SketchFeature):
            raise ValueError(f"A feature '{sketch_name}' não é uma SketchFeature válida.")

        angle_rad = angle_degrees * 3.141592653589793 / 180.0
        op_map = {
            "none": designer_engine.BooleanOpType.None_,
            "add": designer_engine.BooleanOpType.Add,
            "subtract": designer_engine.BooleanOpType.Subtract
        }
        op = op_map.get(op_type.lower(), designer_engine.BooleanOpType.None_)

        revolve = designer_engine.RevolveFeature(name, sketch, angle_rad, axis, op)
        
        parent = self.tree.GetLastSolidFeature()
        revolve.SetParent(parent)

        self.tree.AddFeature(revolve)
        return self.rebuild()

    def add_fillet(self, name: str, edge_index: int, radius: float) -> bool:
        """
        Adiciona uma FilletFeature (Arredondamento de Aresta) ao sólido ativo.
        """
        edge_count = self.tree.GetActiveShapeEdgeCount()
        if edge_count == 0:
            raise ValueError("Não há sólido ativo disponível para aplicar o arredondamento (Fillet).")
        if edge_index > edge_count or edge_index <= 0:
            raise ValueError(f"ID da aresta inválido: {edge_index}. O sólido atual possui apenas {edge_count} arestas.")

        fillet = designer_engine.FilletFeature(name, edge_index, radius)
        
        parent = self.tree.GetLastSolidFeature()
        fillet.SetParent(parent)

        self.tree.AddFeature(fillet)
        return self.rebuild()

    def add_chamfer(self, name: str, edge_index: int, distance: float) -> bool:
        """
        Adiciona uma ChamferFeature (Chanfro de Aresta) ao sólido ativo.
        """
        edge_count = self.tree.GetActiveShapeEdgeCount()
        if edge_count == 0:
            raise ValueError("Não há sólido ativo disponível para aplicar o chanfro (Chamfer).")
        if edge_index > edge_count or edge_index <= 0:
            raise ValueError(f"ID da aresta inválido: {edge_index}. O sólido atual possui apenas {edge_count} arestas.")

        chamfer = designer_engine.ChamferFeature(name, edge_index, distance)
        
        parent = self.tree.GetLastSolidFeature()
        chamfer.SetParent(parent)

        self.tree.AddFeature(chamfer)
        return self.rebuild()

    def update_fillet_radius(self, name: str, new_radius: float) -> bool:
        """Busca uma FilletFeature pelo nome, altera seu raio e reconstrói."""
        feat = self.tree.FindFeature(name)
        if not feat:
            raise ValueError(f"Feature '{name}' não encontrada.")
        
        if not hasattr(feat, "SetRadius"):
            raise ValueError(f"A feature '{name}' não é uma FilletFeature editável.")
            
        feat.SetRadius(new_radius)
        return self.rebuild()
