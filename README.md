# 3Designer: Parametric Solid Modeling CLI

A lightweight, feature-based parametric solid modeling command-line interface (CLI) built on top of the **OpenCASCADE Technology (OCCT)** geometric kernel in C++.

## Core Features
*   **Work Plane Mapping**: Local 2D sketching plane $(U, V)$ coordinates dynamically mapped to 3D space $(X, Y, Z)$ using `gp_Ax3` and `gp_Pln`.
*   **Planar Face Extraction**: Ability to select any planar face on a 3D solid, retrieve its normal and origin, and construct a new `WorkPlane` aligned to that face (accounting for topological face orientations).
*   **Feature-Based History Tree**: Rebuilds the solid representation from the root when parameters change, replicating the regeneration loop used in professional CAD systems (e.g. FreeCAD, SolidWorks).
*   **Boolean Operations**: Features can be integrated using Union (`BRepAlgoAPI_Fuse`) or Cut (`BRepAlgoAPI_Cut`).
*   **Mesh & Export**: Dynamic triangulation of solids using `BRepMesh_IncrementalMesh` and exporting to **BREP** or **STL** (via `StlAPI_Writer`).

---

## Architecture Design

The project uses a structured, decoupled layout:

```
3Designer/
├── CMakeLists.txt         # CMake build configuration
├── README.md              # Documentation
├── include/
│   ├── CLI.hpp            # CLI engine & command flags parser
│   ├── Feature.hpp        # Geometry features (Sketch, Extrude, Revolve)
│   ├── FeatureTree.hpp    # State history tree & rebuild loop
│   └── WorkPlane.hpp      # Local-to-global 2D/3D plane transformation
└── src/
    ├── CLI.cpp
    ├── Feature.cpp
    ├── FeatureTree.cpp
    ├── main.cpp           # Application entrypoint
    └── WorkPlane.cpp
```

---

## Getting Started

### 1. Install Dependencies
On Debian, Ubuntu, or derivative distributions, install the OpenCASCADE development libraries and CMake build chain:

```bash
sudo apt update
sudo apt install -y \
    cmake \
    build-essential \
    libocct-modeling-algorithms-dev \
    libocct-modeling-data-dev \
    libocct-foundation-dev \
    libocct-data-exchange-dev \
    libocct-draw-dev \
    occt-misc
```

### 2. Compile the Project
From the workspace root, run:

```bash
# Create and enter the build directory
mkdir build && cd build

# Configure the build system
cmake ..

# Compile the executable
make
```

### 3. Run the CLI
Execute the generated binary:

```bash
./3designer
```

---

## Command Reference Tutorial

Run the executable and type these commands in sequence to model a parametric plate with a hollow cylinder cut out of its center.

### Step 1: Draw a 2D Rectangle Profile on the Default XY Plane
Create a 2D polygon profile (20x20 unit square) centered on $(0,0)$ in local plane coordinates:
```text
sketch --shape polygon --points "-10,-10 10,-10 10,10 -10,10"
```

### Step 2: Extrude the Sketch into a 3D Base Plate
Extrude the rectangular profile by a depth of 5 units to form a solid plate:
```text
extrude --depth 5 --new
```

### Step 3: Find the Face Index of the Top Surface
List the faces of the newly created plate to find the index of the face pointing upwards along the Z-axis (normal: `0,0,1`):
```text
faces
```
*Note: This command will display each face index, surface type, origin, and normal vector.*

### Step 4: Sketch a Circle on the Top Planar Face
Assuming **Face #6** is the top face (Normal: `0,0,1`), sketch a circle with radius 4 centered on $(0,0)$ directly on that face:
```text
sketch --on-face 6 --shape circle --radius 4 --center "0,0"
```

### Step 5: Subtract the Cylinder from the Base Plate
Cut a circular hole through the plate by extruding the circle profile:
```text
extrude --depth 5 --cut
```

### Step 6: Verify the Feature History Tree
Display the genealogy of the active shape to review your operations:
```text
tree
```

### Step 7: Export the Final Geometry
Export the resulting solid body to STL (which runs mesh triangulation) and BREP:
```text
export --file model.stl
export --file model.brep
```
To quit the CLI, type:
```text
exit
```
