import os
import sys
from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QGroupBox, QFormLayout, QSpinBox, QDoubleSpinBox,
    QListWidget, QMessageBox, QFileDialog, QLineEdit
)
from engine_proxy import EngineProxy
from viewer import ModelViewer

class CADApp(QMainWindow):
    """
    Janela principal da interface gráfica (GUI) do 3Designer.
    Integra o motor geométrico via EngineProxy e renderiza o 3D via ModelViewer.
    """
    def __init__(self):
        super().__init__()
        self.setWindowTitle("3Designer - Parametric Solid Modeling CAD")
        self.resize(1100, 750)
        
        # Inicialização do Proxy do Motor C++
        self.proxy = EngineProxy()
        
        # Caminho temporário para renderização de visualização
        self.temp_stl = os.path.join(os.path.dirname(os.path.abspath(__file__)), "temp_view.stl")
        
        self.init_ui()
        self.apply_dark_theme()
        
        # Carrega um modelo padrão ao abrir para não iniciar vazio
        self.create_demo_model()

    def init_ui(self):
        # Widget principal e layout horizontal dividido
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        main_layout = QHBoxLayout(main_widget)
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.setSpacing(10)
        
        # ==========================================
        # Painel Lateral de Controle (Esquerda)
        # ==========================================
        sidebar = QWidget()
        sidebar.setFixedWidth(320)
        sidebar_layout = QVBoxLayout(sidebar)
        sidebar_layout.setContentsMargins(0, 0, 0, 0)
        
        # Título
        title_label = QLabel("3Designer CAD")
        title_label.setStyleSheet("font-size: 20px; font-weight: bold; color: #3b82f6; margin-bottom: 5px;")
        sidebar_layout.addWidget(title_label)
        
        # Grupo: Ações Globais
        global_group = QGroupBox("Ações")
        global_layout = QVBoxLayout(global_group)
        
        self.btn_demo = QPushButton("Criar Modelo Demo")
        self.btn_demo.clicked.connect(self.create_demo_model)
        global_layout.addWidget(self.btn_demo)
        
        self.btn_rebuild = QPushButton("Rebuild Parametric Tree")
        self.btn_rebuild.setObjectName("btn_rebuild")
        self.btn_rebuild.clicked.connect(self.trigger_rebuild)
        global_layout.addWidget(self.btn_rebuild)
        
        self.btn_remove_last = QPushButton("Desfazer Última Feature")
        self.btn_remove_last.clicked.connect(self.remove_last_feature)
        global_layout.addWidget(self.btn_remove_last)

        self.btn_clear = QPushButton("Limpar Árvore")
        self.btn_clear.setObjectName("btn_clear")
        self.btn_clear.clicked.connect(self.clear_tree)
        global_layout.addWidget(self.btn_clear)
        
        sidebar_layout.addWidget(global_group)
        
        # Grupo: Adicionar Modificações de Aresta
        edge_group = QGroupBox("Modificar Arestas (Fillet / Chamfer)")
        edge_layout = QFormLayout(edge_group)
        
        self.spin_edge_id = QSpinBox()
        self.spin_edge_id.setMinimum(1)
        self.spin_edge_id.setMaximum(100)
        self.spin_edge_id.setValue(1)
        edge_layout.addRow("ID da Aresta:", self.spin_edge_id)
        
        self.spin_value = QDoubleSpinBox()
        self.spin_value.setMinimum(0.05)
        self.spin_value.setMaximum(50.0)
        self.spin_value.setSingleStep(0.5)
        self.spin_value.setValue(1.5)
        edge_layout.addRow("Raio / Distância:", self.spin_value)
        
        btn_fillet = QPushButton("Aplicar Fillet")
        btn_fillet.clicked.connect(self.apply_fillet)
        edge_layout.addRow(btn_fillet)
        
        btn_chamfer = QPushButton("Aplicar Chamfer")
        btn_chamfer.clicked.connect(self.apply_chamfer)
        edge_layout.addRow(btn_chamfer)
        
        sidebar_layout.addWidget(edge_group)
        
        # Grupo: Histórico de Features (Árvore Paramétrica)
        tree_group = QGroupBox("Histórico de Features")
        tree_layout = QVBoxLayout(tree_group)
        
        self.list_features = QListWidget()
        tree_layout.addWidget(self.list_features)
        
        sidebar_layout.addWidget(tree_group)
        
        # Exportação
        self.btn_export = QPushButton("Exportar como STL...")
        self.btn_export.clicked.connect(self.export_stl)
        sidebar_layout.addWidget(self.btn_export)
        
        main_layout.addWidget(sidebar)
        
        # ==========================================
        # Visualizador 3D OpenGL (Direita)
        # ==========================================
        self.viewer = ModelViewer()
        main_layout.addWidget(self.viewer, stretch=1)

    def apply_dark_theme(self):
        self.setStyleSheet("""
            QMainWindow {
                background-color: #121214;
            }
            QWidget {
                color: #e1e1e6;
                font-family: 'Segoe UI', sans-serif;
                font-size: 13px;
            }
            QGroupBox {
                background-color: #202024;
                border: 1px solid #323238;
                border-radius: 6px;
                margin-top: 15px;
                padding-top: 15px;
                font-weight: bold;
                color: #3b82f6;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                padding: 0 5px;
                left: 10px;
            }
            QLineEdit, QComboBox, QDoubleSpinBox, QSpinBox {
                background-color: #121214;
                border: 1px solid #323238;
                border-radius: 4px;
                padding: 4px;
                color: #f3f4f6;
            }
            QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus, QSpinBox:focus {
                border: 1px solid #3b82f6;
            }
            QPushButton {
                background-color: #29292e;
                border: 1px solid #323238;
                color: #e1e1e6;
                border-radius: 4px;
                padding: 6px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #323238;
                border-color: #4b5563;
            }
            QPushButton:pressed {
                background-color: #121214;
            }
            QPushButton#btn_rebuild {
                background-color: #10b981;
                border: none;
                color: #ffffff;
            }
            QPushButton#btn_rebuild:hover {
                background-color: #059669;
            }
            QPushButton#btn_rebuild:pressed {
                background-color: #047857;
            }
            QPushButton#btn_clear {
                background-color: #ef4444;
                border: none;
                color: #ffffff;
            }
            QPushButton#btn_clear:hover {
                background-color: #dc2626;
            }
            QPushButton#btn_clear:pressed {
                background-color: #b91c1c;
            }
            QListWidget {
                background-color: #121214;
                border: 1px solid #323238;
                border-radius: 4px;
                color: #c4c4cc;
            }
        """)

    def create_demo_model(self):
        """
        Cria um modelo paramétrico de demonstração:
        Uma placa com um furo e quinas arredondadas.
        """
        self.proxy.clear()
        
        # 1. Base Quadrada 15x15
        self.proxy.add_sketch("BaseSketch", 0, "polygon", points_uv=[(-7.5, -7.5), (7.5, -7.5), (7.5, 7.5), (-7.5, 7.5)])
        self.proxy.add_extrude("BaseSolid", "BaseSketch", 3.0, "none")
        
        # 2. Esboço de Furo Circular
        self.proxy.add_sketch("HoleSketch", 1, "circle", radius=2.5, center_uv=(0.0, 0.0))
        self.proxy.add_extrude("HoleCut", "HoleSketch", -3.0, "subtract")
        
        # 3. Arredondamento da aresta interna do furo
        self.proxy.add_fillet("FilletBase", 1, 0.5)
        
        self.trigger_rebuild()

    def update_feature_list(self):
        """Atualiza a lista visual das features cadastradas no motor."""
        self.list_features.clear()
        for idx, feat in enumerate(self.proxy.get_features()):
            type_str = str(feat.GetType()).split('.')[-1]
            status = "OK" if not feat.GetResultShape().IsNull() else "PENDENTE"
            self.list_features.addItem(f"[{idx+1}] {feat.GetName()} ({type_str}) - {status}")

    def trigger_rebuild(self):
        """Dispara a reconstrução (rebuild) no motor e atualiza a malha 3D."""
        try:
            if self.proxy.rebuild():
                # Exporta para arquivo STL de visualização temporária
                if self.proxy.export_to_stl(self.temp_stl, 0.1):
                    self.viewer.load_stl(self.temp_stl)
                self.update_feature_list()
            else:
                QMessageBox.warning(self, "Falha de Rebuild", "A reconstrução paramétrica falhou devido a erros geométricos.")
        except Exception as e:
            QMessageBox.critical(self, "Erro no Kernel", f"Exceção disparada pelo Kernel OpenCASCADE:\n\n{e}")
            self.update_feature_list()

    def remove_last_feature(self):
        """Remove a última feature paramétrica da árvore e reconstrói."""
        self.proxy.remove_last_feature()
        self.trigger_rebuild()

    def clear_tree(self):
        """Limpa toda a árvore de histórico."""
        self.proxy.clear()
        if os.path.exists(self.temp_stl):
            try:
                os.remove(self.temp_stl)
            except:
                pass
        self.viewer.removeItem(self.viewer.mesh_item)
        self.viewer.mesh_item = None
        self.update_feature_list()

    def apply_fillet(self):
        edge_id = self.spin_edge_id.value()
        radius = self.spin_value.value()
        
        name = f"Fillet_E{edge_id}"
        if self.proxy.add_fillet(name, edge_id, radius):
            self.trigger_rebuild()
        else:
            QMessageBox.warning(self, "Falha no Fillet", "Não foi possível aplicar o Fillet. Verifique se o ID da aresta está correto ou se o raio não é muito grande.")
            self.proxy.remove_last_feature()
            self.trigger_rebuild()

    def apply_chamfer(self):
        edge_id = self.spin_edge_id.value()
        distance = self.spin_value.value()
        
        name = f"Chamfer_E{edge_id}"
        if self.proxy.add_chamfer(name, edge_id, distance):
            self.trigger_rebuild()
        else:
            QMessageBox.warning(self, "Falha no Chamfer", "Não foi possível aplicar o Chamfer. Verifique se o ID da aresta está correto ou se a distância não é muito grande.")
            self.proxy.remove_last_feature()
            self.trigger_rebuild()

    def export_stl(self):
        """Exporta o sólido ativo do motor para um arquivo STL à escolha do usuário."""
        file_path, _ = QFileDialog.getSaveFileName(self, "Exportar STL", "", "Arquivos STL (*.stl)")
        if file_path:
            if self.proxy.export_to_stl(file_path, 0.05):
                QMessageBox.information(self, "Exportado", f"Arquivo exportado com sucesso para:\n{file_path}")
            else:
                QMessageBox.warning(self, "Falha", "Não foi possível exportar a malha. O sólido ativo está vazio ou é inválido.")

    def closeEvent(self, event):
        # Remove arquivo STL temporário ao fechar a aplicação
        if os.path.exists(self.temp_stl):
            try:
                os.remove(self.temp_stl)
            except:
                pass
        event.accept()

def main():
    # Inicialização da aplicação Qt
    app = QApplication(sys.argv)
    window = CADApp()
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
