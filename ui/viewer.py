import os
import sys
import numpy as np
import pyqtgraph.opengl as gl
from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QVector3D

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
    ruler_changed = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.mesh_item = None
        self.opts['distance'] = 25.0  # Distância de visualização padrão
        
        # Medições (Régua)
        self.ruler_pt1 = None
        self.ruler_pt2 = None
        self.ruler_line = None
        self.ruler_dots = None

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

    def clear_ruler(self):
        """Limpa as marcações e referências da régua de medição."""
        self.ruler_pt1 = None
        self.ruler_pt2 = None
        if self.ruler_line is not None:
            self.removeItem(self.ruler_line)
            self.ruler_line = None
        if self.ruler_dots is not None:
            self.removeItem(self.ruler_dots)
            self.ruler_dots = None

    def update_ruler_graphics(self):
        """Desenha ou remove os pontos e a linha de medição conforme o estado da régua."""
        if self.ruler_line is not None:
            self.removeItem(self.ruler_line)
            self.ruler_line = None
        if self.ruler_dots is not None:
            self.removeItem(self.ruler_dots)
            self.ruler_dots = None
            
        pts = []
        if self.ruler_pt1 is not None:
            pts.append(self.ruler_pt1)
        if self.ruler_pt2 is not None:
            pts.append(self.ruler_pt2)
            
        if not pts:
            return
            
        # Adiciona marcadores de ponto (vermelhos)
        self.ruler_dots = gl.GLScatterPlotItem(
            pos=np.array(pts),
            color=(0.9, 0.1, 0.1, 1.0),
            size=10,
            pxMode=True
        )
        self.addItem(self.ruler_dots)
        
        # Se temos dois pontos, desenha a linha de conexão (amarela)
        if len(pts) == 2:
            self.ruler_line = gl.GLLinePlotItem(
                pos=np.array(pts),
                color=(0.9, 0.9, 0.1, 1.0),
                width=3,
                antialias=True
            )
            self.addItem(self.ruler_line)

    def handle_ruler_selection(self, vertex):
        """Gerencia a seleção sequencial dos dois pontos e cálculo da distância."""
        if self.ruler_pt1 is None or (self.ruler_pt1 is not None and self.ruler_pt2 is not None):
            self.ruler_pt1 = vertex
            self.ruler_pt2 = None
            self.update_ruler_graphics()
            self.ruler_changed.emit(f"Ponto 1: ({vertex[0]:.2f}, {vertex[1]:.2f}, {vertex[2]:.2f}). Selecione o segundo ponto.")
        else:
            self.ruler_pt2 = vertex
            self.update_ruler_graphics()
            dist = np.linalg.norm(self.ruler_pt1 - self.ruler_pt2)
            self.ruler_changed.emit(f"Ponto 1: ({self.ruler_pt1[0]:.2f}, {self.ruler_pt1[1]:.2f}, {self.ruler_pt1[2]:.2f}) | Ponto 2: ({vertex[0]:.2f}, {vertex[1]:.2f}, {vertex[2]:.2f}) | Distância: {dist:.3f} mm")

    def mousePressEvent(self, event):
        """Intercepta o clique do mouse para fazer picking de vértices da malha."""
        if event.button() == Qt.LeftButton and self.mesh_item is not None:
            pos = event.pos()
            mesh_data = self.mesh_item.opts.get('meshdata')
            if mesh_data is not None:
                vertices = mesh_data.vertexes()
                if vertices is not None and len(vertices) > 0:
                    # Matriz de transformação combinada View * Projection
                    m = self.projectionMatrix() * self.viewMatrix()
                    
                    best_vertex = None
                    min_dist = float('inf')
                    
                    width = self.width()
                    height = self.height()
                    
                    # Projeta os vértices para achar o mais próximo do clique 2D
                    for v in vertices:
                        pt = m.map(QVector3D(v[0], v[1], v[2]))
                        # Converte de NDC [-1, 1] para coordenadas de janela [0, W], [0, H]
                        sx = (pt.x() + 1) / 2.0 * width
                        sy = (1 - pt.y()) / 2.0 * height
                        
                        dist = np.hypot(sx - pos.x(), sy - pos.y())
                        if dist < min_dist:
                            min_dist = dist
                            best_vertex = v
                            
                    # Se clicou a menos de 25 pixels de um vértice real, captura o ponto
                    if min_dist < 25.0 and best_vertex is not None:
                        self.handle_ruler_selection(best_vertex)
                        
        super().mousePressEvent(event)

    def load_stl(self, file_path):
        """
        Lê e renderiza a malha 3D a partir do arquivo STL fornecido.
        """
        if not os.path.exists(file_path):
            print(f"Erro: Arquivo STL '{file_path}' não foi gerado.")
            return

        try:
            self.clear_ruler()
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
