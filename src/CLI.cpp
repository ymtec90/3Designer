#include "CLI.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

#include <BRepTools.hxx>
#include <StlAPI_Writer.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <GeomAbs_SurfaceType.hxx>

CLI::CLI() 
    : m_sketchCounter(1), m_extrudeCounter(1), m_revolveCounter(1), m_filletCounter(1), m_chamferCounter(1) {
    m_currentSketch = nullptr;
}

void CLI::Run() {
    std::cout << "========================================================" << std::endl;
    std::cout << "       3Designer - Parametric Solid Modeling CLI       " << std::endl;
    std::cout << "             Geometric Kernel: OpenCASCADE             " << std::endl;
    std::cout << "========================================================" << std::endl;
    std::cout << "Type 'help' for a list of commands." << std::endl;

    std::string line;
    while (true) {
        std::cout << "\n3d_designer> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        // Trim leading/trailing spaces
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty()) {
            continue;
        }

        if (line == "exit" || line == "quit") {
            std::cout << "Exiting 3Designer. Goodbye!" << std::endl;
            break;
        }

        ParseCommand(line);
    }
}

void CLI::PrintHelp() const {
    std::cout << "\nAvailable Commands:" << std::endl;
    std::cout << "  sketch [options]            Create a 2D sketch profile on a workplane." << std::endl;
    std::cout << "    --on-face <index>         Select parent face index (1-based). If 0 or omitted, sketches on XY plane." << std::endl;
    std::cout << "    --shape <circle|polygon>  Define profile shape." << std::endl;
    std::cout << "    --radius <val>            Set circle radius (required for circle)." << std::endl;
    std::cout << "    --center <U,V>            Set circle center coordinates (default: 0,0)." << std::endl;
    std::cout << "    --points \"U1,V1 U2,V2 ...\" Set polygon vertices in local 2D coordinates." << std::endl;
    std::cout << std::endl;
    std::cout << "  extrude [options]           Extrude active sketch into a 3D solid." << std::endl;
    std::cout << "    --depth <val>             Extrusion depth (positive or negative)." << std::endl;
    std::cout << "    --add                     Fuse result with parent solid (default if parent exists)." << std::endl;
    std::cout << "    --cut                     Subtract result from parent solid." << std::endl;
    std::cout << "    --new                     Create a separate new solid body." << std::endl;
    std::cout << std::endl;
    std::cout << "  revolve [options]           Revolve active sketch around a local axis." << std::endl;
    std::cout << "    --angle <degrees>         Rotation angle in degrees." << std::endl;
    std::cout << "    --axis <x|y|z>            Local workplane axis of revolution (default: y)." << std::endl;
    std::cout << "    --add | --cut | --new     Boolean operation type." << std::endl;
    std::cout << std::endl;
    std::cout << "  fillet [options]            Round edges of the active solid." << std::endl;
    std::cout << "    --edge <index>            Edge index (1-based)." << std::endl;
    std::cout << "    --radius <val>            Fillet radius." << std::endl;
    std::cout << std::endl;
    std::cout << "  chamfer [options]           Bevel edges of the active solid." << std::endl;
    std::cout << "    --edge <index>            Edge index (1-based)." << std::endl;
    std::cout << "    --dist <val>              Chamfer distance." << std::endl;
    std::cout << std::endl;
    std::cout << "  faces                       List faces of the current active solid." << std::endl;
    std::cout << "  tree                        Show parametric history feature tree." << std::endl;
    std::cout << "  clear                       Clear the history tree." << std::endl;
    std::cout << "  export [options]            Export the active solid shape." << std::endl;
    std::cout << "    --file <path>             Export file path." << std::endl;
    std::cout << "    --format <stl|brep>       Export format (default: auto-detected from file name)." << std::endl;
    std::cout << std::endl;
    std::cout << "  help                        Show this command reference list." << std::endl;
    std::cout << "  exit | quit                 Close the application." << std::endl;
}

std::vector<std::string> CLI::Tokenize(const std::string& str) const {
    std::vector<std::string> tokens;
    std::string token;
    bool inQuotes = false;
    for (char c : str) {
        if (c == '"' || c == '\'') {
            inQuotes = !inQuotes;
        } else if (c == ' ' && !inQuotes) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token.push_back(c);
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

std::map<std::string, std::string> CLI::ParseFlags(const std::vector<std::string>& tokens) const {
    std::map<std::string, std::string> flags;
    for (size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i].rfind("--", 0) == 0) {
            std::string key = tokens[i];
            std::string val = "true"; // Default for standalone flags like --cut, --add, etc.
            if (i + 1 < tokens.size() && tokens[i+1].rfind("--", 0) != 0) {
                val = tokens[i+1];
                i++;
            }
            flags[key] = val;
        }
    }
    return flags;
}

void CLI::ParseCommand(const std::string& line) {
    std::vector<std::string> tokens = Tokenize(line);
    if (tokens.empty()) return;

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd == "help") {
        PrintHelp();
        return;
    }
    if (cmd == "tree" || cmd == "history") {
        m_featureTree.PrintTree();
        return;
    }
    if (cmd == "clear") {
        m_featureTree.Clear();
        m_currentSketch = nullptr;
        std::cout << "Parametric tree cleared." << std::endl;
        return;
    }
    if (cmd == "faces") {
        HandleListFaces();
        return;
    }

    std::map<std::string, std::string> flags = ParseFlags(tokens);

    if (cmd == "sketch") {
        HandleSketch(flags);
    } else if (cmd == "extrude") {
        HandleExtrude(flags);
    } else if (cmd == "revolve") {
        HandleRevolve(flags);
    } else if (cmd == "fillet") {
        HandleFillet(flags);
    } else if (cmd == "chamfer") {
        HandleChamfer(flags);
    } else if (cmd == "export") {
        HandleExport(flags);
    } else {
        std::cout << "Unknown command '" << tokens[0] << "'. Type 'help' for command usage." << std::endl;
    }
}

void CLI::HandleSketch(const std::map<std::string, std::string>& flags) {
    if (flags.find("--shape") == flags.end()) {
        std::cerr << "Error: Flag --shape <circle|polygon> is required." << std::endl;
        return;
    }

    std::string shape = flags.at("--shape");
    std::transform(shape.begin(), shape.end(), shape.begin(), ::tolower);

    int faceIndex = 0;
    if (flags.find("--on-face") != flags.end()) {
        try {
            faceIndex = std::stoi(flags.at("--on-face"));
        } catch (...) {
            std::cerr << "Error: Invalid face index '" << flags.at("--on-face") << "'." << std::endl;
            return;
        }
    }

    // Verify faceIndex exists if user is drawing on active shape
    if (faceIndex > 0) {
        TopoDS_Shape activeShape = m_featureTree.GetActiveShape();
        if (activeShape.IsNull()) {
            std::cerr << "Error: Cannot sketch on Face #" << faceIndex << " because no active solid exists." << std::endl;
            return;
        }
        
        // Quick boundary check using Explorer
        TopExp_Explorer explorer(activeShape, TopAbs_FACE);
        int faceCount = 0;
        while (explorer.More()) {
            faceCount++;
            explorer.Next();
        }
        if (faceIndex > faceCount) {
            std::cerr << "Error: Face index " << faceIndex << " is out of bounds. Active solid has " << faceCount << " faces." << std::endl;
            return;
        }
    }

    SketchProfile profile;
    profile.shapeType = shape;

    if (shape == "circle") {
        if (flags.find("--radius") == flags.end()) {
            std::cerr << "Error: Circle requires --radius <val>." << std::endl;
            return;
        }
        try {
            profile.radius = std::stod(flags.at("--radius"));
        } catch (...) {
            std::cerr << "Error: Invalid radius '" << flags.at("--radius") << "'." << std::endl;
            return;
        }

        // Parse center U,V (default: 0,0)
        if (flags.find("--center") != flags.end()) {
            std::string centerStr = flags.at("--center");
            double u = 0, v = 0;
            char comma;
            std::stringstream ss(centerStr);
            if (ss >> u >> comma >> v && comma == ',') {
                profile.center = gp_Pnt2d(u, v);
            } else {
                std::cerr << "Error: Invalid center '" << centerStr << "'. Format must be U,V." << std::endl;
                return;
            }
        }
    } 
    else if (shape == "polygon") {
        if (flags.find("--points") == flags.end()) {
            std::cerr << "Error: Polygon requires --points \"U1,V1 U2,V2 ...\"" << std::endl;
            return;
        }

        std::string pointsStr = flags.at("--points");
        std::stringstream ss(pointsStr);
        std::string pointToken;
        while (ss >> pointToken) {
            double u = 0, v = 0;
            char comma;
            std::stringstream pts(pointToken);
            if (pts >> u >> comma >> v && comma == ',') {
                profile.points.push_back(gp_Pnt2d(u, v));
            } else {
                std::cerr << "Error: Invalid point coordinate '" << pointToken << "'. Format must be U,V." << std::endl;
                return;
            }
        }

        if (profile.points.size() < 3) {
            std::cerr << "Error: Polygon requires at least 3 points." << std::endl;
            return;
        }
    } 
    else {
        std::cerr << "Error: Shape type must be either 'circle' or 'polygon'." << std::endl;
        return;
    }

    std::string name = "Sketch" + std::to_string(m_sketchCounter++);
    auto sketchFeat = std::make_shared<SketchFeature>(name, faceIndex, profile);
    
    // Wire up parent solid (genealogy)
    auto parent = m_featureTree.GetLastSolidFeature();
    sketchFeat->SetParent(parent);

    // Dry run evaluation to validate geometries
    try {
        if (sketchFeat->Evaluate()) {
            m_featureTree.AddFeature(sketchFeat);
            m_currentSketch = sketchFeat;
            std::cout << "Sketch '" << name << "' successfully created on " 
                      << (faceIndex > 0 ? "Face #" + std::to_string(faceIndex) : "global XY plane") << "." << std::endl;
        } else {
            std::cerr << "Error: Failed to construct valid sketch geometry." << std::endl;
            m_sketchCounter--; // Revert naming counter on failure
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception validating sketch geometry: " << e.what() << std::endl;
        m_sketchCounter--;
    }
}

void CLI::HandleExtrude(const std::map<std::string, std::string>& flags) {
    if (!m_currentSketch) {
        std::cerr << "Error: No active sketch. Create a sketch command first." << std::endl;
        return;
    }

    if (flags.find("--depth") == flags.end()) {
        std::cerr << "Error: Flag --depth <val> is required." << std::endl;
        return;
    }

    double depth = 0.0;
    try {
        depth = std::stod(flags.at("--depth"));
    } catch (...) {
        std::cerr << "Error: Invalid depth '" << flags.at("--depth") << "'." << std::endl;
        return;
    }

    // Determine Boolean Op
    BooleanOpType opType = BooleanOpType::None;
    if (m_featureTree.GetLastSolidFeature() != nullptr) {
        opType = BooleanOpType::Add; // Default: Union if active shape exists
    }

    if (flags.find("--op") != flags.end()) {
        std::string opVal = flags.at("--op");
        std::transform(opVal.begin(), opVal.end(), opVal.begin(), ::tolower);
        if (opVal == "add" || opVal == "union" || opVal == "fuse") {
            opType = BooleanOpType::Add;
        } else if (opVal == "subtract" || opVal == "cut" || opVal == "sub") {
            opType = BooleanOpType::Subtract;
        } else if (opVal == "none" || opVal == "new") {
            opType = BooleanOpType::None;
        }
    } else {
        // Fallback to standalone flags
        if (flags.find("--cut") != flags.end() || flags.find("--subtract") != flags.end()) {
            opType = BooleanOpType::Subtract;
        } else if (flags.find("--add") != flags.end() || flags.find("--union") != flags.end()) {
            opType = BooleanOpType::Add;
        } else if (flags.find("--new") != flags.end()) {
            opType = BooleanOpType::None;
        }
    }

    std::string name = "Extrude" + std::to_string(m_extrudeCounter++);
    auto extrudeFeat = std::make_shared<ExtrudeFeature>(name, m_currentSketch, depth, opType);
    
    // Setup solid genealogy parent
    auto parent = m_featureTree.GetLastSolidFeature();
    extrudeFeat->SetParent(parent);

    // Re-evaluate entire tree to build the new shape
    m_featureTree.AddFeature(extrudeFeat);
    if (m_featureTree.Rebuild()) {
        m_currentSketch = nullptr; // Reset current sketch (consumed)
        std::cout << "Solid '" << name << "' generated successfully. Active shape updated." << std::endl;
    } else {
        std::cerr << "Error: Extrusion failed. Rolling back feature tree state." << std::endl;
        m_featureTree.RemoveLastFeature();
        m_featureTree.Rebuild(); // Re-evaluate back to the last valid state
        m_extrudeCounter--;
    }
}

void CLI::HandleRevolve(const std::map<std::string, std::string>& flags) {
    if (!m_currentSketch) {
        std::cerr << "Error: No active sketch. Create a sketch command first." << std::endl;
        return;
    }

    if (flags.find("--angle") == flags.end()) {
        std::cerr << "Error: Flag --angle <degrees> is required." << std::endl;
        return;
    }

    double degrees = 0.0;
    try {
        degrees = std::stod(flags.at("--angle"));
    } catch (...) {
        std::cerr << "Error: Invalid angle value '" << flags.at("--angle") << "'." << std::endl;
        return;
    }

    double angleRad = degrees * M_PI / 180.0;

    std::string axis = "Y";
    if (flags.find("--axis") != flags.end()) {
        axis = flags.at("--axis");
        std::transform(axis.begin(), axis.end(), axis.begin(), ::toupper);
    }

    if (axis != "X" && axis != "Y" && axis != "Z") {
        std::cerr << "Error: Axis must be X, Y, or Z." << std::endl;
        return;
    }

    // Determine Boolean Op
    BooleanOpType opType = BooleanOpType::None;
    if (m_featureTree.GetLastSolidFeature() != nullptr) {
        opType = BooleanOpType::Add; // Default: Union if active shape exists
    }

    if (flags.find("--op") != flags.end()) {
        std::string opVal = flags.at("--op");
        std::transform(opVal.begin(), opVal.end(), opVal.begin(), ::tolower);
        if (opVal == "add" || opVal == "union" || opVal == "fuse") {
            opType = BooleanOpType::Add;
        } else if (opVal == "subtract" || opVal == "cut" || opVal == "sub") {
            opType = BooleanOpType::Subtract;
        } else if (opVal == "none" || opVal == "new") {
            opType = BooleanOpType::None;
        }
    } else {
        // Fallback to standalone flags
        if (flags.find("--cut") != flags.end() || flags.find("--subtract") != flags.end()) {
            opType = BooleanOpType::Subtract;
        } else if (flags.find("--add") != flags.end() || flags.find("--union") != flags.end()) {
            opType = BooleanOpType::Add;
        } else if (flags.find("--new") != flags.end()) {
            opType = BooleanOpType::None;
        }
    }

    std::string name = "Revolve" + std::to_string(m_revolveCounter++);
    auto revolveFeat = std::make_shared<RevolveFeature>(name, m_currentSketch, angleRad, axis, opType);
    
    // Setup solid genealogy parent
    auto parent = m_featureTree.GetLastSolidFeature();
    revolveFeat->SetParent(parent);

    m_featureTree.AddFeature(revolveFeat);
    if (m_featureTree.Rebuild()) {
        m_currentSketch = nullptr; // Reset current sketch (consumed)
        std::cout << "Solid '" << name << "' generated successfully. Active shape updated." << std::endl;
    } else {
        std::cerr << "Error: Revolution failed. Rolling back feature tree state." << std::endl;
        m_featureTree.RemoveLastFeature();
        m_featureTree.Rebuild(); // Re-evaluate back to the last valid state
        m_revolveCounter--;
    }
}

void CLI::HandleListFaces() {
    TopoDS_Shape shape = m_featureTree.GetActiveShape();
    if (shape.IsNull()) {
        std::cout << "No active solid shape." << std::endl;
        return;
    }

    std::cout << "\n--- Active Solid Faces ---" << std::endl;
    TopExp_Explorer explorer(shape, TopAbs_FACE);
    int faceIndex = 1;
    while (explorer.More()) {
        TopoDS_Face face = TopoDS::Face(explorer.Current());
        BRepAdaptor_Surface surface(face);
        std::string type = "Generic";

        switch (surface.GetType()) {
            case GeomAbs_Plane:       type = "Plane"; break;
            case GeomAbs_Cylinder:    type = "Cylinder"; break;
            case GeomAbs_Cone:        type = "Cone"; break;
            case GeomAbs_Sphere:      type = "Sphere"; break;
            case GeomAbs_Torus:       type = "Torus"; break;
            default:                  type = "Complex"; break;
        }

        std::cout << "  Face #" << faceIndex << " | Surface: " << type;
        if (surface.GetType() == GeomAbs_Plane) {
            gp_Pln plane = surface.Plane();
            gp_Pnt origin = plane.Location();
            gp_Dir normal = plane.Position().Direction();

            // Adjust normal display direction according to orientation
            if (face.Orientation() == TopAbs_REVERSED) {
                normal = -normal;
            }

            std::cout << " | Origin: (" << origin.X() << ", " << origin.Y() << ", " << origin.Z() << ")"
                      << " | Normal: (" << normal.X() << ", " << normal.Y() << ", " << normal.Z() << ")";
        }
        std::cout << std::endl;

        faceIndex++;
        explorer.Next();
    }
    std::cout << "--------------------------\n" << std::endl;
}

void CLI::HandleExport(const std::map<std::string, std::string>& flags) {
    TopoDS_Shape activeShape = m_featureTree.GetActiveShape();
    if (activeShape.IsNull()) {
        std::cerr << "Error: No active shape to export." << std::endl;
        return;
    }

    if (flags.find("--file") == flags.end()) {
        std::cerr << "Error: Flag --file <path> is required." << std::endl;
        return;
    }

    std::string path = flags.at("--file");
    
    std::string format = "brep";
    if (flags.find("--format") != flags.end()) {
        format = flags.at("--format");
        std::transform(format.begin(), format.end(), format.begin(), ::tolower);
    } else {
        // Auto-detect from file extension
        size_t dotIndex = path.find_last_of(".");
        if (dotIndex != std::string::npos) {
            std::string ext = path.substr(dotIndex + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == "stl" || ext == "brep") {
                format = ext;
            }
        }
    }

    if (format == "brep") {
        std::cout << "Exporting to BREP format: " << path << " ..." << std::endl;
        if (BRepTools::Write(activeShape, path.c_str())) {
            std::cout << "BREP export successful." << std::endl;
        } else {
            std::cerr << "Error: Failed to write BREP file." << std::endl;
        }
    } 
    else if (format == "stl") {
        std::cout << "Triangulating shape and exporting to STL: " << path << " ..." << std::endl;
        if (m_featureTree.ExportToSTL(path, 0.1)) {
            std::cout << "STL export successful." << std::endl;
        } else {
            std::cerr << "Error: STL export failed." << std::endl;
        }
    } 
    else {
        std::cerr << "Error: Unsupported export format '" << format << "'. Use 'stl' or 'brep'." << std::endl;
    }
}

void CLI::HandleFillet(const std::map<std::string, std::string>& flags) {
    TopoDS_Shape activeShape = m_featureTree.GetActiveShape();
    if (activeShape.IsNull()) {
        std::cerr << "Error: No active solid shape to apply fillet." << std::endl;
        return;
    }

    if (flags.find("--edge") == flags.end()) {
        std::cerr << "Error: Flag --edge <index> is required." << std::endl;
        return;
    }

    if (flags.find("--radius") == flags.end()) {
        std::cerr << "Error: Flag --radius <val> is required." << std::endl;
        return;
    }

    int edgeIndex = 0;
    try {
        edgeIndex = std::stoi(flags.at("--edge"));
    } catch (...) {
        std::cerr << "Error: Invalid edge index '" << flags.at("--edge") << "'." << std::endl;
        return;
    }

    double radius = 0.0;
    try {
        radius = std::stod(flags.at("--radius"));
    } catch (...) {
        std::cerr << "Error: Invalid radius '" << flags.at("--radius") << "'." << std::endl;
        return;
    }

    // Boundary check using Explorer
    TopExp_Explorer explorer(activeShape, TopAbs_EDGE);
    int edgeCount = 0;
    while (explorer.More()) {
        edgeCount++;
        explorer.Next();
    }
    if (edgeIndex <= 0 || edgeIndex > edgeCount) {
        std::cerr << "Error: Edge index " << edgeIndex << " is out of bounds. Active solid has " << edgeCount << " edges." << std::endl;
        return;
    }

    std::string name = "Fillet" + std::to_string(m_filletCounter++);
    auto filletFeat = std::make_shared<FilletFeature>(name, edgeIndex, radius);

    // Setup parent
    auto parent = m_featureTree.GetLastSolidFeature();
    filletFeat->SetParent(parent);

    // Add and rebuild
    m_featureTree.AddFeature(filletFeat);
    if (m_featureTree.Rebuild()) {
        std::cout << "Solid '" << name << "' generated successfully (Fillet on Edge #" << edgeIndex << " with radius " << radius << "). Active shape updated." << std::endl;
    } else {
        std::cerr << "Error: Fillet operation failed. Rolling back feature tree state." << std::endl;
        m_featureTree.RemoveLastFeature();
        m_featureTree.Rebuild(); // Revert
        m_filletCounter--;
    }
}

void CLI::HandleChamfer(const std::map<std::string, std::string>& flags) {
    TopoDS_Shape activeShape = m_featureTree.GetActiveShape();
    if (activeShape.IsNull()) {
        std::cerr << "Error: No active solid shape to apply chamfer." << std::endl;
        return;
    }

    if (flags.find("--edge") == flags.end()) {
        std::cerr << "Error: Flag --edge <index> is required." << std::endl;
        return;
    }

    if (flags.find("--dist") == flags.end()) {
        std::cerr << "Error: Flag --dist <val> is required." << std::endl;
        return;
    }

    int edgeIndex = 0;
    try {
        edgeIndex = std::stoi(flags.at("--edge"));
    } catch (...) {
        std::cerr << "Error: Invalid edge index '" << flags.at("--edge") << "'." << std::endl;
        return;
    }

    double dist = 0.0;
    try {
        dist = std::stod(flags.at("--dist"));
    } catch (...) {
        std::cerr << "Error: Invalid distance '" << flags.at("--dist") << "'." << std::endl;
        return;
    }

    // Boundary check using Explorer
    TopExp_Explorer explorer(activeShape, TopAbs_EDGE);
    int edgeCount = 0;
    while (explorer.More()) {
        edgeCount++;
        explorer.Next();
    }
    if (edgeIndex <= 0 || edgeIndex > edgeCount) {
        std::cerr << "Error: Edge index " << edgeIndex << " is out of bounds. Active solid has " << edgeCount << " edges." << std::endl;
        return;
    }

    std::string name = "Chamfer" + std::to_string(m_chamferCounter++);
    auto chamferFeat = std::make_shared<ChamferFeature>(name, edgeIndex, dist);

    // Setup parent
    auto parent = m_featureTree.GetLastSolidFeature();
    chamferFeat->SetParent(parent);

    // Add and rebuild
    m_featureTree.AddFeature(chamferFeat);
    if (m_featureTree.Rebuild()) {
        std::cout << "Solid '" << name << "' generated successfully (Chamfer on Edge #" << edgeIndex << " with distance " << dist << "). Active shape updated." << std::endl;
    } else {
        std::cerr << "Error: Chamfer operation failed. Rolling back feature tree state." << std::endl;
        m_featureTree.RemoveLastFeature();
        m_featureTree.Rebuild(); // Revert
        m_chamferCounter--;
    }
}
