//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#pragma once    // directive to insure that this .h file is only included one time

#include <World/AbstractWorld.h> // AbstractWorld defines all the basic function templates for worlds
#include <string>
#include <memory> // shared_ptr
#include <map>

using std::shared_ptr;
using std::string;
using std::map;
using std::unordered_map;
using std::unordered_set;
using std::to_string;

class maxNavWorld : public AbstractWorld {

public:
    // parameters for group and brain namespaces
    static shared_ptr<ParameterLink<int>> evaluationsPerGenerationPL;
    static shared_ptr<ParameterLink<int>> xDimPL;
    static shared_ptr<ParameterLink<int>> yDimPL;
    static shared_ptr<ParameterLink<int>> xDimStartPL;
    static shared_ptr<ParameterLink<int>> yDimStartPL;
    static shared_ptr<ParameterLink<int>> numTimestepsPL;
    static shared_ptr<ParameterLink<int>> numAgentsPL;
    static shared_ptr<ParameterLink<int>> numObsPL;
    static shared_ptr<ParameterLink<double>> maxPenPL;
    static shared_ptr<ParameterLink<int>> selectionPL;
    
    // a local variable used for faster access to the ParameterLink value
    int evaluationsPerGeneration;
    int xDim;
    int yDim;
    int xDimStart;
    int yDimStart;
    int numTimesteps;
    int numAgents;
    int numObs;
    double maxPen;
    int selection;

    class Agent{
        public:
            int id;
            double health;
            int x;
            int y;
            int orient;
            std::shared_ptr<AbstractBrain> brain;
    };
    
    std::string groupName = "root::";
    std::string brainName = "root::";
    
    maxNavWorld(shared_ptr<ParametersTable> PT);
    virtual ~maxNavWorld() = default;

    bool isWithinBounds(const int x, const int y, const std::vector<std::pair<int, int>> obs); 
    double navigate(std::vector<Agent> &agents, int numTimesteps, bool printing, int rep, const std::vector<std::pair<int, int>> obs);
    void moveAgent(Agent &agentRef, const std::vector<Agent>& agents);
    void calcMoveAgentNoR(const int lmotor, const int rmotor, Agent &agentRef, const std::vector<Agent>& agents, const std::vector<std::pair<int, int>> obs);
    //void moveAgentNoR(std::unordered_map<int, std::pair<int,int>> desired_moves, std::vector<Agent>& agents);

    virtual auto evaluate(map<string, shared_ptr<Group>>& /*groups*/, int /*analyze*/, int /*visualize*/, int /*debug*/) -> void override;

    virtual auto requiredGroups() -> unordered_map<string,unordered_set<string>> override;
};

