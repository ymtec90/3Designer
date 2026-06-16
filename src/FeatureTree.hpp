#pragma once

#include "Feature.hpp"
#include <TopoDS_Shape.hxx>
#include <vector>
#include <memory>
#include <string>

class FeatureTree {
public:
    FeatureTree() = default;

    // Adds a feature to the end of the history tree
    void AddFeature(std::shared_ptr<Feature> feature);

    // Re-evaluates all features sequentially from the root (parametric rebuild)
    bool Rebuild();

    // Returns the current active solid shape (result of the latest solid modifier)
    TopoDS_Shape GetActiveShape() const { return m_activeShape; }

    // Returns the number of edges in the active solid shape
    int GetActiveShapeEdgeCount() const;

    // Returns the list of all features in the history tree
    const std::vector<std::shared_ptr<Feature>>& GetFeatures() const { return m_features; }

    // Finds a feature in the tree by its name
    std::shared_ptr<Feature> FindFeature(const std::string& name) const;

    // Returns the last solid feature in the history (used as default parent for new features)
    std::shared_ptr<Feature> GetLastSolidFeature() const;

    // Prints the feature history tree showing genealogy (parent and boolean relationships)
    void PrintTree() const;

    // Resets the feature history tree
    void Clear();

    // Removes the last added feature from the history tree (rollback)
    void RemoveLastFeature();

    // Triangulates the active solid shape using BRepMesh_IncrementalMesh and exports to STL
    bool ExportToSTL(const std::string& filename, double deflection = 0.1) const;

private:
    std::vector<std::shared_ptr<Feature>> m_features;
    TopoDS_Shape m_activeShape;
};
