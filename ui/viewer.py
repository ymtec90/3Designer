import os
import sys
import numpy as np
import pyqtgraph.opengl as gl

def read_stl(file_path):
    """
    Lê um arquivo STL (suporta tanto o formato Binário padrão quanto o ASCII)
    e retorna os vértices e as faces formatados para o PyQtGraph.
    """
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"Arquivo não encontrado: {file_path}")

    with open(file_path, 'rb') as f:
        header = f.read(80)
        num_triangles_data = f.read(4)
        if len(num_triangles_data) < 4:
            raise ValueError("Arquivo STL corrompido ou incompleto.")
        
        num_triangles = int(np.frombuffer(num_triangles_data, dtype=np.uint32)[0])
        
        # Verifica se o tamanho do arquivo condiz com o formato binário padrão
        f.seek(0, os.SEEK_END)
        file_size = f.tell()
        expected_size = 80 + 4 + num_triangles * 50
        
        if file_size == expected_size:
            # Formato Binário
            f.seek(84)
            dtype = np.dtype([
                ('normal', np.float32, (3,)),
                ('vertices', np.float32, (3, 3)),
                ('attr', np.uint16)
            ])
            data = np.fromfile(f, dtype=dtype, count=num_triangles)
            vertices = data['vertices'].reshape(-1, 3)
            faces = np.arange(num_triangles * 3).reshape(-1, 3)
            return vertices, faces
        else:
            # Formato ASCII (Fallback)
            f.seek(0)
            content = f.read().decode('utf-8', errors='ignore')
            vertices = []
            for line in content.split('\n'):
                line = line.strip()
                if line.startswith('vertex'):
                    parts = line.split()
                    if len(parts) >= 4:
                        vertices.append([float(parts[1]), float(parts[2]), float(parts[3])])
            vertices = np.array(vertices, dtype=np.float32)
            if len(vertices) % 3 != 0:
                # Truncamento de segurança se faltarem vértices
                vertices = vertices[:(len(vertices) // 3) * 3]
            faces = np.arange(len(vertices)).reshape(-1, 3)
            return vertices, faces

class ModelViewer(gl.GLViewWidget):
    """
    Widget de visualização 3D herdando do GLViewWidget do PyQtGraph.
    Renderiza malhas de triângulos provenientes de arquivos STL.
    """
    def __init__(self, parent=None):
        super().__init__(parent)
        self.mesh_item = None
        self.opts['distance'] = 25.0  # Distância de visualização padrão
        
        # Adiciona grade de referência no plano XY
        grid = gl.GLGridItem()
        grid.setSize(40, 40)
        grid.setSpacing(2, 2)
        grid.translate(0, 0, 0)
        self.addItem(grid)
        
        # Adiciona eixos de coordenadas (Vermelho = X, Verde = Y, Azul = Z)
        axis = gl.GLAxisItem()
        axis.setSize(10, 10, 10)
        self.addItem(axis)

    def load_stl(self, file_path):
        """
        Lê e renderiza a malha 3D a partir do arquivo STL fornecido.
        """
        if not os.path.exists(file_path):
            print(f"Erro: Arquivo STL '{file_path}' não foi gerado.")
            return

        try:
            vertices, faces = read_stl(file_path)
            
            # Remove a malha anterior se já houver uma renderizada
            if self.mesh_item is not None:
                self.removeItem(self.mesh_item)
                
            if len(vertices) == 0:
                print("Aviso: STL carregado não possui geometria de triângulos.")
                return

            # Cria o objeto de malha e o adiciona ao widget
            mesh_data = gl.MeshData(vertexes=vertices, faces=faces)
            
            # Shader 'viewNormalColor' pinta as faces com base em suas normais em relação à câmera
            # (dá um aspecto de iluminação 3D excelente sem requerer luzes manuais)
            self.mesh_item = gl.GLMeshItem(
                meshdata=mesh_data,
                smooth=True,
                color=(0.3, 0.65, 0.9, 1.0),       # Azul suave
                edgeColor=(0.15, 0.15, 0.15, 0.2), # Bordas escuras suaves
                drawEdges=True,
                shader='viewNormalColor'
            )
            
            self.addItem(self.mesh_item)
            
            # Centraliza a câmera no centróide aproximado do modelo carregado
            center = vertices.mean(axis=0)
            self.setCameraPosition(pos=center)
            
        except Exception as e:
            print(f"Erro inesperado ao carregar/visualizar STL: {e}")
