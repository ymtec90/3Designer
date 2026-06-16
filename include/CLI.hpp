#pragma once

#include "FeatureTree.hpp"
#include <memory>
#include <string>
#include <vector>
#include <map>

class CLI {
public:
    CLI();
    void Run();

private:
    FeatureTree m_featureTree;
    std::shared_ptr<SketchFeature> m_currentSketch;
    
    // Auto-increment counters for default feature naming
    int m_sketchCounter;
    int m_extrudeCounter;
    int m_revolveCounter;
    int m_filletCounter;
    int m_chamferCounter;

    // Helper functions for parsing
    void PrintHelp() const;
    void ParseCommand(const std::string& line);
    std::vector<std::string> Tokenize(const std::string& str) const;
    std::map<std::string, std::string> ParseFlags(const std::vector<std::string>& tokens) const;

    void HandleSketch(const std::map<std::string, std::string>& flags);
    void HandleExtrude(const std::map<std::string, std::string>& flags);
    void HandleRevolve(const std::map<std::string, std::string>& flags);
    void HandleFillet(const std::map<std::string, std::string>& flags);
    void HandleChamfer(const std::map<std::string, std::string>& flags);
    void HandleListFaces();
    void HandleExport(const std::map<std::string, std::string>& flags);
};
