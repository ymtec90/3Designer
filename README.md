# 3Designer - Parametric Solid Modeling CAD

O **3Designer** é um modelador CAD paramétrico robusto projetado para construção e modificação de sólidos 3D. Ele integra um motor geométrico de alto desempenho desenvolvido em C++ (utilizando o kernel **OpenCASCADE**) com uma interface visual dinâmica e interativa em Python utilizando **PySide6** e **PyQtGraph**.

A comunicação direta entre a camada de negócio em C++ e a camada visual em Python é mediada de forma ultrarrápida por bindings gerados com **pybind11**.

---

## Funcionalidades

- **Modelagem Paramétrica Baseada em Esboços (Sketches)**: Criação de perfis 2D (círculos e polígonos fechados) em planos de trabalho globais ou locais extraídos diretamente de faces de sólidos existentes.
- **Operações de Extrusão e Revolução**: Geração de sólidos 3D por meio de extrusões e revoluções com suporte a operações booleanas (`Fuse/Union` e `Cut/Subtract`).
- **Modificações de Arestas**: Suporte a arredondamento de quinas (**Fillet**) e chanfros (**Chamfer**) com indexação baseada em arestas.
- **Validação Topológica e Transações Atômicas**: Análise topológica rígida via `BRepCheck_Analyzer` após qualquer operação. Falhas geométricas geram exceções e ativam rollbacks automáticos na árvore de histórico para manter o modelo consistente.
- **Exportação de Malha 3D**: Discretização e exportação de sólidos para o formato **STL** e **BREP**.
- **Interface Gráfica Completa**: Visualizador 3D interativo com sombreadores OpenGL, painel de histórico paramétrico, modificações de aresta em tempo real e desfazer/refazer integrado.
- **Interface de Linha de Comando (CLI)**: Console CAD interativo integrado ao executável C++.

---

## Pré-requisitos

### Sistema Operacional
- Linux (Debian, Ubuntu e derivados).

### Dependências de Sistema
Para compilar e executar o projeto, você precisará do compilador C++17, CMake, OpenCASCADE e das bibliotecas de desenvolvimento do Python e X11.

No Debian/Ubuntu, instale usando o comando:
```bash
sudo apt update
sudo apt install -y build-essential cmake libocct-ocaf-dev libocct-data-exchange-dev \
                    libtbb-dev python3-dev libxkbcommon0 libxkbcommon-x11-0 libxcb-cursor0 \
                    libxcb-icccm4 libxcb-keysyms1 libxcb-xinerama0
```

---

## Como Instalar/Compilar

### 1. Clonar e Compilar o Projeto (C++ & Python Bindings)
Clone o repositório e compile o projeto usando CMake. O submódulo `pybind11` é configurado automaticamente.

```bash
# Clone o repositório
git clone https://github.com/seu-usuario/3Designer.git
cd 3Designer

# Inicialize o pybind11 (caso não tenha clonado as dependências externas)
# Se a pasta external/pybind11 estiver vazia, execute:
# git clone --depth 1 https://github.com/pybind/pybind11.git external/pybind11

# Configure e compile o projeto
cmake -B build -S .
cmake --build build --parallel
```
Isso gerará:
- O executável da CLI: `build/3designer`
- O módulo Python compartilhado: `build/designer_engine.cpython-...so`

---

## Como Executar

### 1. Criando e Configurando o Ambiente Virtual Python
É altamente recomendado utilizar um ambiente virtual (`venv`) para executar a interface gráfica em Python:

```bash
# Crie o ambiente virtual local
python3 -m venv venv

# Ative o ambiente virtual
source venv/bin/activate

# Instale os pacotes necessários
pip install -r requirements.txt
```

### 2. Executar a Interface Gráfica (GUI)
Com o ambiente virtual ativo e o motor C++ compilado no diretório `build/`, execute o aplicativo gráfico:

```bash
python3 ui/app.py
```
Isso abrirá a janela 3D contendo o visualizador e a barra de ferramentas lateral.

### 3. Executar o Console de Linha de Comando (CLI)
Você também pode interagir com o motor diretamente pela linha de comando executando o executável C++:

```bash
./build/3designer
```

### 4. Executar Scripts de Teste Automatizados
O diretório `scripts/` contém exemplos de design paramétricos. Para rodar um teste da ponte:

```bash
python3 scripts/test_bridge.py
```

---

## Licença

Este projeto é distribuído sob a Licença MIT. Veja o arquivo `LICENSE` para mais detalhes.
