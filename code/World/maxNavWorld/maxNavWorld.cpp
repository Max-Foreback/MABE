#include "maxNavWorld.h"
#include <algorithm>

shared_ptr<ParameterLink<int>> maxNavWorld::evaluationsPerGenerationPL =
    Parameters::register_parameter("WORLD_maxNav-evaluationsPerGeneration", 6,
    "how many times should each organism be tested in each generation?");

shared_ptr<ParameterLink<int>> maxNavWorld::xDimPL =
    Parameters::register_parameter("WORLD_maxNav-xDim", 100,
    "x dim of hallway");

shared_ptr<ParameterLink<int>> maxNavWorld::yDimPL =
    Parameters::register_parameter("WORLD_maxNav-yDim", 7,
    "y dim of hallway");

shared_ptr<ParameterLink<int>> maxNavWorld::xDimStartPL =
    Parameters::register_parameter("WORLD_maxNav-xDimStart", 7,
    "starting box xdims");

shared_ptr<ParameterLink<int>> maxNavWorld::yDimStartPL =
    Parameters::register_parameter("WORLD_maxNav-yDimStart", 7,
    "starting box y dims");

shared_ptr<ParameterLink<int>> maxNavWorld::numTimestepsPL =
    Parameters::register_parameter("WORLD_maxNav-numTimesteps", 150,
    "how many timesteps?");

shared_ptr<ParameterLink<int>> maxNavWorld::numAgentsPL =
    Parameters::register_parameter("WORLD_maxNav-numAgents", 3,
    "how many agents?");

shared_ptr<ParameterLink<int>> maxNavWorld::numObsPL =
    Parameters::register_parameter("WORLD_maxNav-numObs", 3,
    "how many obstacles?");

shared_ptr<ParameterLink<double>> maxNavWorld::maxPenPL =
    Parameters::register_parameter("WORLD_maxNav-penalty", 1.0,
    "Max penalty for being in the danger zone");

shared_ptr<ParameterLink<int>> maxNavWorld::selectionPL =
    Parameters::register_parameter("WORLD_maxNav-selection", 0,
    "what typa seelction (0 for max, 1 for min, 2 for avg)");

// the constructor gets called once when MABE starts up. use this to set things up
maxNavWorld::maxNavWorld(shared_ptr<ParametersTable> PT) : AbstractWorld(PT) {
    
    //localize a parameter value for faster access
    evaluationsPerGeneration = evaluationsPerGenerationPL->get(PT);
    xDim = xDimPL->get(PT);
    yDim = yDimPL->get(PT);
    xDimStart = xDimStartPL->get(PT);
    yDimStart = yDimStartPL->get(PT);
    numTimesteps = numTimestepsPL->get(PT);
    numAgents = numAgentsPL->get(PT);
    numObs = numObsPL->get(PT);
    maxPen = maxPenPL->get(PT);
    selection = selectionPL->get(PT);
    
    // popFileColumns tell MABE what data should be saved to pop.csv files
	popFileColumns.clear();
    popFileColumns.push_back("score");
}

//Do not change vals
const int up = 0;
const int left = 1;
const int down = 2;
const int right = 3;

void writeWorld(const std::vector<maxNavWorld::Agent> &agents, int t, int x){
    std::string filename = "harvestin" + std::to_string(x);
    std::string wStr = "U," + std::to_string(t);
    FileManager::openAndWriteToFile(filename, wStr);
    for (auto& agent : agents) {
        std::string agentStr = "M,";
        agentStr += std::to_string(agent.id) + ",";
        agentStr += std::to_string(agent.x + 0.5)+ ",";
        agentStr += std::to_string(agent.y + 0.5);
        FileManager::openAndWriteToFile(filename, agentStr);
    }
    FileManager::openAndWriteToFile(filename, "-");
}

void initWriteWorld(const std::vector<maxNavWorld::Agent> &agents, int xDim, int xDimStart, int yDim, int yDimStart, int x, const std::vector<std::pair<int, int>> obs){
    std::string filename = "harvestin" + std::to_string(x);
    std::string visualizeData = "**InitializeWorld**\n" + std::to_string(4) + "," + std::to_string(xDim + xDimStart) + "," + std::to_string(yDim) + "," + std::to_string(agents.size());
    FileManager::openAndWriteToFile(filename, visualizeData);
    std::string w = "";
    for (int y = 0; y < yDim; y++) {
        for (int x = 0; x < xDim + xDimStart; x++) {
            if (x < xDimStart){
                w += "0,";
            }   
            else{
                bool found = false;
                for(int i = 0; i < obs.size(); i++){
                    if(x == obs[i].first && y == obs[i].second){
                        found = true;
                    }
                }
                if(found){
                    w += "0,";
                }
                else{
                    w += "1,";
                }
            }
        }
        //for goal
        w += "2,";
        w += "\n";
    }
    FileManager::openAndWriteToFile(filename, w);
    FileManager::openAndWriteToFile(filename, "-");
    FileManager::openAndWriteToFile(filename, "**InitializeHarvesters**");
    for (auto& agent : agents) {
        std::string agentStr = "";
        agentStr += std::to_string(agent.id) + ",";
        agentStr += std::to_string(agent.x + 0.5) + ",";
        agentStr += std::to_string(agent.y + 0.5) + ",";
        agentStr += std::to_string(1) + ",";
        FileManager::openAndWriteToFile(filename, agentStr);
    }
    FileManager::openAndWriteToFile(filename, "\n-");
}

std::vector<std::vector<int>> init_positions(int evals, int numAgents, int xDimStart, int yDimStart){
    std::vector<std::vector<int>> positions;
    //Make a list of non overlapping starting locations to use
    for(int i = 0; i < evals; i++){
        std::vector<int> possiblePositions(xDimStart*yDimStart, 0);
        std::iota(possiblePositions.begin(), possiblePositions.end(), 0);
        std::shuffle(possiblePositions.begin(), possiblePositions.end(), Random::getCommonGenerator());
        possiblePositions.resize(numAgents);
        positions.push_back(possiblePositions);
    }
    return positions;
}

std::vector<std::vector<std::pair<int, int>>> init_obstacles(int evals, int xDimStart, int xDim, int yDimStart, int yDim, int numObs){
    std::vector<std::vector<std::pair<int, int>>> all_obs;

    int xSpacing = xDim / numObs;  
    for (int i = 0; i < evals; i++) {
        std::vector<std::pair<int, int>> obs;
        for (int i = 0; i < numObs; i++) {
            int xMin = xDimStart + i * xSpacing;
            int xMax = xDimStart + (i + 1) * xSpacing - 2;
            int x = Random::getInt(xMin, xMax);
            int y = Random::getInt(0, yDim - 2);
            
            obs.push_back(std::make_pair(x, y));
            obs.push_back(std::make_pair(x + 1, y));
            obs.push_back(std::make_pair(x, y + 1));
            obs.push_back(std::make_pair(x + 1, y + 1));
        }
        all_obs.push_back(obs);
    }

    return all_obs;
}


std::vector<maxNavWorld::Agent> genAgents(const std::shared_ptr<AbstractBrain> brain, int numAgents, std::vector<int> positions, int xDimStart, int yDimStart){
    std::vector<maxNavWorld::Agent> agents; 

    for (int i = 0; i < numAgents; i++){
        maxNavWorld::Agent a;
        //agent ID should be the index
        a.id = i;
        a.brain = brain->makeCopy(brain->PT);
        a.orient = Random::getInt(3);
        int pos = positions[i];
        a.x = pos % xDimStart; 
        a.y = pos / xDimStart;  
        if(i == 0){
            a.health = 50;
        }
        else{
            a.health = 15;
        }
        agents.push_back(a);
    }
    return agents;
}

std::vector<int> getPerception(const int agentID, const std::vector<maxNavWorld::Agent>& agents, int danger_start, const std::vector<std::pair<int, int>> obs){
    std::vector<int> pos_front;
    std::vector<int> pos_front2;
    std::vector<int> pos_right;
    std::vector<int> pos_left;
    std::vector<int> pos_frontR;
    std::vector<int> pos_frontL;
    std::vector<int> pos_farR;
    std::vector<int> pos_farL;
    int x = agents[agentID].x;
    int y = agents[agentID].y;

    pos_front.assign({x+1, y});
    pos_front2.assign({x+2, y});
    pos_right.assign({x, y+1});
    pos_left.assign({x, y-1});
    pos_frontR.assign({x+1, y+1});
    pos_frontL.assign({x+1, y-1});
    //pos_farR.assign({x, y+2});
    //pos_farL.assign({x, y-2});

    //for now 10 inputs. First 2 will be is there an agent better than me in first sensor, and 
    //is there an agent there but not better than me. Repeat for all 5 sensors. 
    std::vector<int> perception;
    //for (const auto& v : {pos_front, pos_right, pos_left}) {
    for (const auto& v : {pos_front, pos_right, pos_left, pos_frontR, pos_frontL}) {
    //for (const auto& v : {pos_front, pos_right, pos_left, pos_farR, pos_farL}) {
        const maxNavWorld::Agent* sensedAgent = nullptr; 
        for (const auto& a : agents){
            if (a.x == v[0] && a.y == v[1]){
                sensedAgent = &a;
            }
        }
        if(!sensedAgent){
            perception.push_back(0);
            perception.push_back(0);
        }
        else{
            if (sensedAgent->health > agents[agentID].health){
                perception.push_back(1);
                perception.push_back(1);
            }
            else{
                perception.push_back(1);
                perception.push_back(0);
            }
        }
    }
    if (agents[agentID].x >= danger_start){
        perception.push_back(1);
    }
    else{
        perception.push_back(0);
    }
    for (const auto& v : {pos_front, pos_right, pos_left, pos_frontR, pos_frontL}) {
        bool collide = false;
        for (const auto o : obs){
            if(o.first == v[0] && o.second == v[1]){
                collide = true;
            }
        }
        int sensed = (collide) ? 1 : 0;
        perception.push_back(sensed);
    }
    return perception;
}

bool maxNavWorld::isWithinBounds(const int x, const int y, const std::vector<std::pair<int, int>> obs) {
    // First rectangle defined by (0,0) to (xDimStart-1, yDimStart-1)
    int x1_min = 0;
    int x1_max = xDimStart - 1;
    int y1_min = 0;
    int y1_max = yDimStart - 1;
    
    // Second rectangle defined by (xDimStart, yDimStart) to (xDimStart + xDim - 1, yDimStart + yDim - 1)
    int x2_min = xDimStart;
    int x2_max = xDimStart + xDim - 1;
    int y2_min = yDimStart;
    int y2_max = yDimStart + yDim - 1;

    bool validy = (y >= 0 && y < yDim);
    //+1 for the goal area
    bool validx = (x >= 0 && x < xDimStart + xDim + 1);
    bool validObs = true;
    for(int i = 0; i < obs.size(); i++){
        if(x == obs[i].first && y == obs[i].second){
            validObs = false;
        }
    }
    return validx && validy && validObs;
}

bool hasAbove(const std::vector<maxNavWorld::Agent>& agents, int x, int y) {
    for (const auto& agent : agents) {
        if (agent.x == x && agent.y < y) {
            return true; 
        }
    }
    return false;
}

bool hasBelow(const std::vector<maxNavWorld::Agent>& agents, int x, int y) {
    for (const auto& agent : agents) {
        if (agent.x == x && agent.y > y) {
            return true; 
        }
    }
    return false;
}

void maxNavWorld::calcMoveAgentNoR(const int lmotor, const int rmotor, maxNavWorld::Agent &agentRef, const std::vector<maxNavWorld::Agent>& agents, const std::vector<std::pair<int, int>> obs){
    int x = agentRef.x;
    int y = agentRef.y;
    int move;
    if(lmotor && rmotor){
        move = right;
    }
    else if(lmotor && !rmotor){
        move = up;
    }
    else if(!lmotor && rmotor){
        move = down;
    }
    else{
        move = left;
    }
    switch(move){
        case up:
            y -= 1;
            break;
        case left:
            x -= 1;
            break;
        case down:
            y += 1;
            break;
        case right:
            x += 1;
            break;
    }
    bool taken = false;
    for (auto& agent : agents) {
        if(agent.id != agentRef.id && agent.x == x && agent.y == y){
            taken = true;
        }
    }
    if(isWithinBounds(x, y, obs) && !taken){
        agentRef.x = x;
        agentRef.y = y;
    }
}

double maxNavWorld::navigate(std::vector<maxNavWorld::Agent> &agents, int numTimesteps, bool printing, int rep, const std::vector<std::pair<int, int>> obs){
    for(int timestep = 0; timestep < numTimesteps; timestep++){
        std::unordered_map<int, std::pair<int,int>> desired_moves;
        if(printing){
            writeWorld(agents, timestep, rep);
        }
        //make sure agents are eval in random order
        std::shuffle(agents.begin(), agents.end(), Random::getCommonGenerator());
        for(int currAgent = 0; currAgent < agents.size(); currAgent++){
            //get agent 
            maxNavWorld::Agent& agentRef = agents[currAgent];
            //get agent percept
            std::vector<int> perception = getPerception(currAgent, agents, xDimStart, obs);
            //update sensor
            for (int i = 0; i < perception.size(); i++){
                agentRef.brain->setInput(i, perception[i]);
            }
            //update and read
            agentRef.brain->update();
            int lmotor = Bit(agentRef.brain->readOutput(0));
            int rmotor = Bit(agentRef.brain->readOutput(1));

            //move the agent if they ask for a valid move
            calcMoveAgentNoR(lmotor, rmotor, agentRef, agents, obs);
        }
        //take damage if not protected
        for (auto& agent : agents) {
            //make sure out of start and not at end 
            if(agent.x >= xDimStart && agent.x < xDim + xDimStart + 1){
                bool above = hasAbove(agents, agent.x, agent.y);
                bool below = hasBelow(agents, agent.x, agent.y);
                double pen = maxPen;
                double maxX = xDim + xDimStart - 1;
                double scaling = ((maxX)-(maxX - agent.x)) / maxX;
                double penalty = scaling * pen;
                //TEMPORARY TEST
                //penalty = 0.0;
                if (!above && !below) {
                    agent.health -= penalty;
                } else if (above && !below) {
                    agent.health -= penalty/2;
                } else if (!above && below) {
                    agent.health -= penalty/2;
                }
            }
        }
        if(printing && timestep == numTimesteps - 1){
            writeWorld(agents, timestep, rep);
        }
    }

    double max_score = 0.0;
    double min_score = 9999999999999;
    double total_score = 0.0;
    bool valid = false;
    for (const auto& a : agents){
        if(a.x > xDimStart - 1 && a.health > 0){
            valid = true;
            double score = a.x * a.health;
            total_score += score;
            if (score > max_score){
                max_score = score;
            }
            if (score < min_score){
                min_score = score;
            }
        }
        else{
            min_score = 0.0;
        }
    }
    if(!valid){
        min_score = 0.0;
    }
    if(selection == 1){
        return min_score; 
    }
    else if(selection == 2){
        return total_score / agents.size(); 
    }
    else{
        return max_score;
    }
}


auto maxNavWorld::evaluate(map<string, shared_ptr<Group>>& groups, int analyze, int visualize, int debug) -> void {
    int popSize = groups[groupName]->population.size(); 

    std::vector<std::vector<int>> start_positions = init_positions(evaluationsPerGeneration, numAgents, xDimStart, yDimStart);
    std::vector<std::vector<std::pair<int, int>>> all_obs = init_obstacles(evaluationsPerGeneration, xDimStart, xDim, yDimStart, yDim, numObs);
    // in this world, organisms do not interact, so we can just iterate over the population
    for (int i = 0; i < popSize; i++) {
        // create a shortcut to access the organism and organisms brain
        auto org = groups[groupName]->population[i];
        auto brain = org->brains[brainName]; 

        double score = 0;
        
        // evaluate this organism some number of times based on evaluationsPerGeneration
        for (int t = 0; t < evaluationsPerGeneration; t++) {
            // clear the brain - TODO FOR EVERY AGENT?
            brain->resetBrain();
    
            std::vector<maxNavWorld::Agent> agents = genAgents(brain, numAgents, start_positions[t], xDimStart, yDimStart);
            std::vector<std::pair<int, int>> obs = all_obs[t];

            double score = navigate(agents, numTimesteps, false, -1, obs);
    
            org->dataMap.append("score", score);
        }

    } // end of population loop
    if(Global::update==Global::updatesPL->get()){
        double bestest = -1000000000;
        auto bestBrain = groups[groupName]->population[0]->brains[brainName]; 
        for (int i = 0; i < popSize; i++) {
            auto org = groups[groupName]->population[i];
            double s = org->dataMap.getAverage("score");
            if(s > bestest){
                bestest = s;
                bestBrain = org->brains[brainName];
            }
        }
        bestBrain->saveStructure("brain.txt");
        int reps = 5;
        std::vector<std::vector<int>> start_positions = init_positions(reps, numAgents, xDimStart, yDimStart);
        std::vector<std::vector<std::pair<int, int>>> all_obs = init_obstacles(reps, xDimStart, xDim, yDimStart, yDim, numObs);
        for(int x = 0; x < reps; x++){
            bestBrain->resetBrain();
            std::vector<maxNavWorld::Agent> agents = genAgents(bestBrain, numAgents, start_positions[x], xDimStart, yDimStart);
            std::vector<std::pair<int, int>> obs = all_obs[x];
            initWriteWorld(agents, xDim, xDimStart, yDim, yDimStart, x, obs);
            double score = navigate(agents, numTimesteps, true, x, obs);
            for (const auto& a : agents){
                std::string filename = "harvestin" + std::to_string(x);
                std::string o = "----------------------------------------\n" + std::to_string(a.x) + " " + std::to_string(a.y) + " " + std::to_string(a.health);
                FileManager::openAndWriteToFile(filename, o);
            }
            FileManager::openAndWriteToFile("score", std::to_string(score));
        }
    }
}


// the requiredGroups function lets MABE know how to set up populations of organisms that this world needs
auto maxNavWorld::requiredGroups() -> unordered_map<string,unordered_set<string>> {
	return { { groupName, { "B:"+brainName+",16,2" } } };
    
}
