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

    // Helper functions for parsing
    void PrintHelp() const;
    void ParseCommand(const std::string& line);
    std::vector<std::string> Tokenize(const std::string& str) const;
    std::map<std::string, std::string> ParseFlags(const std::vector<std::string>& tokens) const;

    // Command Handlers
    void HandleSketch(const std::map<std::string, std::string>& flags);
    void HandleExtrude(const std::map<std::string, std::string>& flags);
    void HandleRevolve(const std::map<std::string, std::string>& flags);
    void HandleListFaces();
    void HandleExport(const std::map<std::string, std::string>& flags);
};
