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

class maxWorld : public AbstractWorld {

public:
    // parameters for group and brain namespaces
    static shared_ptr<ParameterLink<int>> evaluationsPerGenerationPL;
    static shared_ptr<ParameterLink<int>> xDimPL;
    static shared_ptr<ParameterLink<int>> yDimPL;
    static shared_ptr<ParameterLink<bool>> verbosePL;
    static shared_ptr<ParameterLink<int>> numTimestepsPL;
    static shared_ptr<ParameterLink<int>> numAgentsPL;
    static shared_ptr<ParameterLink<std::string>> resourcePropsPL;
    static shared_ptr<ParameterLink<double>> compMutationRatePL;
    static shared_ptr<ParameterLink<int>> taskOneIDPL;
    static shared_ptr<ParameterLink<int>> taskTwoIDPL;
    static shared_ptr<ParameterLink<int>> initialAgent1PL;

    // a local variable used for faster access to the ParameterLink value
    int evaluationsPerGeneration;
    int xDim;
    int yDim;
    bool verbose;
    int timesteps;
    int numAgents;
    int taskOneID;
    int taskTwoID;
    int initialAgent1;
    
    std::vector<double> rProp;
    double mRate;

    std::string groupName = "root::";
    std::string brainName = "root::";
    
    maxWorld(shared_ptr<ParametersTable> PT);
    virtual ~maxWorld() = default;

    std::string brain1Name;
    std::string brain2Name;

    class Resource{
    public:
        int kind;
        int f1;
        int f2;
    };

    class Tracker{
    public:
        std::map<std::string, int> data;

    };

    Tracker createTracker();

    virtual auto evaluate(map<string, shared_ptr<Group>>& /*groups*/, int /*analyze*/, int /*visualize*/, int /*debug*/) -> void override;
    std::vector<int> genAgentPositions();
    std::vector<int> genAgentOrientations();
    std::vector<Resource> genTaskWorld(std::vector<int> positions);
    std::vector<int> getPerception(const int pos, const std::vector<Resource>& world, const int agentOrient, const std::vector<int> &positions);
    Tracker forageTask(std::vector<std::tuple<std::shared_ptr<AbstractBrain>, std::string>> brainInfo, std::vector<Resource> &world, std::vector<int> &positions, std::vector<int> &orientations, bool printing);
    int calcAgentMove(const int pos, const int orient);
    int calcAgentRotate(const int orient, const int lMotor, const int rMotor);
    int mutateComp(int oldAgent1Num);
    void showBestTaskBrain(std::shared_ptr<AbstractBrain> brain, std::shared_ptr<AbstractBrain> brain2, int bestNumAgent1);
    std::vector<bool> binarizeInputs(const std::vector<int>& agentPerception);

    virtual auto requiredGroups() -> unordered_map<string,unordered_set<string>> override;
};

