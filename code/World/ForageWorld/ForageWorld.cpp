#include "ForageWorld.h"
//#include "../../Utilities/Utilities.h"

shared_ptr<ParameterLink<int>> ForageWorld::evaluationsPerGenerationPL =
    Parameters::register_parameter("WORLD_Forage-evaluationsPerGeneration", 3,
    "how many times should each organism be tested in each generation?");

shared_ptr<ParameterLink<int>> ForageWorld::xDimPL =
    Parameters::register_parameter("WORLD_Forage-xDim", 15,
    "Size of the world in x direction");

shared_ptr<ParameterLink<int>> ForageWorld::yDimPL =
    Parameters::register_parameter("WORLD_Forage-yDim", 15,
    "Size of the world in y direction");

shared_ptr<ParameterLink<bool>> ForageWorld::verbosePL =
    Parameters::register_parameter("WORLD_Forage-verbose", false,
    "Should the world print extra data?");

shared_ptr<ParameterLink<int>> ForageWorld::numTimestepsPL =
    Parameters::register_parameter("WORLD_Forage-numTimesteps", 300,
    "Number of timesteps agent(s) have to collect food");

shared_ptr<ParameterLink<int>> ForageWorld::numAgentsPL =
    Parameters::register_parameter("WORLD_Forage-numAgents", 10,
    "Number of agents per swarm");

shared_ptr<ParameterLink<std::string>> ForageWorld::resourcePropsPL =
    Parameters::register_parameter("WORLD_Forage-resourceProp", (std::string) "0.4,0.4",
    "Proportion of world that is resource at index.");

shared_ptr<ParameterLink<double>> ForageWorld::compMutationRatePL =
    Parameters::register_parameter("WORLD_Forage-compMutationRate", 0.3,
    "Probability a swarm swaps one of its agents types");

shared_ptr<ParameterLink<int>> ForageWorld::initialAgent1PL =
    Parameters::register_parameter("WORLD_Forage-initialAgent1", -1,
    "Number of initial agents the swarm starts with. If -1, will default to numAgents/2");

shared_ptr<ParameterLink<int>> ForageWorld::taskOneIDPL =
    Parameters::register_parameter("WORLD_Forage-taskOneID", 1,
    "ID of task1, defaults to 1 (XOR)");

shared_ptr<ParameterLink<int>> ForageWorld::taskTwoIDPL =
    Parameters::register_parameter("WORLD_Forage-taskTwoID", 2,
    "ID of task2, defaults to 2 (Symbolic Regression)");

shared_ptr<ParameterLink<int>> ForageWorld::taskRewardPL =
    Parameters::register_parameter("WORLD_Forage-taskReward", 1,
    "Reward for solving a task and collecting a resource");

shared_ptr<ParameterLink<int>> ForageWorld::taskPenaltyPL =
    Parameters::register_parameter("WORLD_Forage-taskPenalty", -2,
    "Reward for attempting to solve a task, and failing");

shared_ptr<ParameterLink<double>> ForageWorld::r1replaceRatePL =
    Parameters::register_parameter("WORLD_Forage-r1replaceRate", .5,
    "Proportion of replaced rewards that will be resource 1");

ForageWorld::ForageWorld(shared_ptr<ParametersTable> PT) : AbstractWorld(PT) {
    
    //localize a parameter value for faster access
    evaluationsPerGeneration = evaluationsPerGenerationPL->get(PT);
    xDim = xDimPL->get(PT);
    yDim = yDimPL->get(PT);
    verbose = verbosePL->get(PT);
    timesteps = numTimestepsPL->get(PT);
    numAgents = numAgentsPL->get(PT);
    //initialize rProp from param table
    convertCSVListToVector(resourcePropsPL->get(PT), rProp);
    mRate = compMutationRatePL->get(PT);
    taskOneID = taskOneIDPL->get(PT);
    taskTwoID = taskTwoIDPL->get(PT);
    taskReward = taskRewardPL->get(PT);
    taskPenalty = taskPenaltyPL->get(PT);
    r1replaceRate = r1replaceRatePL->get(PT);
    //if not specified, set to half the swarm
    if (initialAgent1PL->get(PT) == -1){
        initialAgent1 = static_cast<int>(numAgents/2);
    }
    else{
        initialAgent1 = initialAgent1PL->get(PT);
    }
    
    
    // popFileColumns tell MABE what data should be saved to pop.csv files
    auto myPT = Parameters::root->getTable("brain::");
    auto myPT2 = Parameters::root->getTable("brain2::");
    brain1Name = myPT->lookupString("BRAIN-brainType") + "1";
    brain2Name = myPT2->lookupString("BRAIN-brainType") + "2";

	popFileColumns.clear();
    popFileColumns.push_back("score");
    popFileColumns.push_back("brain1Num");
    popFileColumns.push_back(brain1Name + "_Resource" + std::to_string(taskOneID) + "_Completed");
    popFileColumns.push_back(brain1Name + "_Resource" + std::to_string(taskOneID) + "_Attempted");

    popFileColumns.push_back(brain1Name + "_Resource" + std::to_string(taskTwoID) + "_Completed");
    popFileColumns.push_back(brain1Name + "_Resource" + std::to_string(taskTwoID) + "_Attempted");

    popFileColumns.push_back(brain2Name + "_Resource" + std::to_string(taskOneID) + "_Completed");
    popFileColumns.push_back(brain2Name + "_Resource" + std::to_string(taskOneID) + "_Attempted");
    
    popFileColumns.push_back(brain2Name + "_Resource" + std::to_string(taskTwoID) + "_Completed");
    popFileColumns.push_back(brain2Name + "_Resource" + std::to_string(taskTwoID) + "_Attempted");
}

//TODO add these to header file?
//Sensors can have these values for a square on the grid
const int wall_val = 11;
const int agent_val = 10;
const int ID_XOR = 1;
const int ID_linReg = 2;
const int ID_AND = 3;
const int ID_FREE = 4;
const int empty_val = 0;
//For now 4 orientations
//Do not change vals, math for roatating depends on them
const int up = 0;
const int left = 1;
const int down = 2;
const int right = 3;

//Lin reg range should be adjustable
const int SR_MAX = 5;

void printWorld(const std::vector<int> positions, const std::vector<ForageWorld::Resource> world, int xDim, int yDim, const std::vector<int> orientations) {
    for (int y = 0; y < yDim; ++y) {
        for (int x = 0; x < xDim; ++x) {
            int pos = y * xDim + x;
            auto it = std::find(positions.begin(), positions.end(), pos);
            //Print orientation if agent
            if (it != positions.end()) {
                auto index = std::distance(positions.begin(), it);
                switch(orientations[index]){
                    case up:
                        std::cout << "^ ";
                        break;
                    case left:
                        std::cout << "< ";
                        break;                   
                    case down:
                        std::cout << "~ ";
                        break;                    
                    case right:
                        std::cout << "> ";
                        break;    
                }
            } 
            //Print resource type otherwise
            else {
                std::cout << world[pos].kind << " ";
            }
        }
        // Start a new line for the next row
        std::cout << '\n';
    }
    std::cout << "\n\n";
}

ForageWorld::Resource createResource(int k){
    ForageWorld::Resource r;
    r.kind = k;
    switch(k){
        //XOR
        case ID_XOR:
            r.f1 = Random::getInt(0, 1);
            r.f2 = Random::getInt(0, 1);
            break;
        //LIN REG
        case ID_linReg:
            r.f1 = Random::getInt(-SR_MAX, SR_MAX);
            r.f2 = Random::getInt(-SR_MAX, SR_MAX);
            break;
        //AND 
        case ID_AND:
            r.f1 = Random::getInt(0, 1);
            r.f2 = Random::getInt(0, 1);
            break;
        case ID_FREE:
            r.f1 = 0;
            r.f2 = 0;
            break;
        // no resource
        case empty_val:
            r.f1 = 0;
            r.f2 = 0;
            break;
        default:
            throw std::invalid_argument("Unknown resource type on creation: " + std::to_string(k));
            break;
    }
    return r;
}

double ForageWorld::calcTask(int out1, ForageWorld::Resource r){
    switch(r.kind){
        case ID_XOR:{
            return (out1 == r.f1 ^ r.f2) ? taskReward : taskPenalty;
            }break;
        case ID_linReg:{
            double diff = abs(out1 - static_cast<double>(r.f1 * r.f2));
            double threshold = 100.0;
            if (diff < 0.01){
                return taskReward;
            }
            return (diff < threshold) ? 0 : taskPenalty;
            }break;
        case ID_AND:{
            return (out1 == r.f1 & r.f2) ? taskReward : taskPenalty;
            }break;
        case ID_FREE:{
            return taskReward;
            }break;
        case empty_val:{
            return 0;
            }break;

        default:
            throw std::invalid_argument("Unknown resource type on evaluation: " + std::to_string(r.kind));
            break;
    }
    //Should never be reached
    return 0;
}

//Binarize sensor inputs
std::vector<bool> ForageWorld::binarizeInputs(const std::vector<int>& agentPerception){
    std::vector<bool> binaryIns;
    //for all 4 outward sensors, front, front2, right, left
    for(int i = 0; i < 4; i++){
        //task1, task2
        binaryIns.push_back(agentPerception[i] == taskOneID);
        binaryIns.push_back(agentPerception[i] == taskTwoID);
    }
    //Does the agent see a wall in the front sensor?
    binaryIns.push_back(agentPerception[0] == wall_val);
    //Add currentPos sensor
    binaryIns.push_back(agentPerception[4] == taskOneID);
    binaryIns.push_back(agentPerception[4] == taskTwoID); 

    return binaryIns;
}

// the evaluate function gets called every generation. evaluate should set values on organisms datamaps
// that will be used by other parts of MABE for things like reproduction and archiving
auto ForageWorld::evaluate(map<string, shared_ptr<Group>>& groups, int analyze, int visualize, int debug) -> void {
    //std::vector<int> scores;
    int popSize = groups[groupName]->population.size(); 

    //a set of starting conditions for this generation's evaluations
    std::vector<std::vector<int>> initPositionsAll;
    std::vector<std::vector<int>> initOrientationsAll;
    std::vector<std::vector<ForageWorld::Resource>> initWorldAll;

    for(int z = 0; z < evaluationsPerGeneration; z++){
        //Every agent in a swarm has a position, orientation, and brain, with matching indicies
        std::vector<int> initPositions = genAgentPositions();
        std::vector<int> initOrientations = genAgentOrientations();
        std::vector<ForageWorld::Resource> initWorld = genTaskWorld(initPositions);
        initPositionsAll.push_back(initPositions);
        initOrientationsAll.push_back(initOrientations);
        initWorldAll.push_back(initWorld);
    }
    
    // For every swarm
    for (int i = 0; i < popSize; i++) {
        // create a shortcut to access the organism and organisms brain
        auto org = groups[groupName]->population[i];
        //if brain1Num already exists, possibly mutate it
        if(org->dataMap.fieldExists("brain1Num")){
            int numAgent1 = mutateComp(org->dataMap.getAverage("brain1Num"));
            org->dataMap.set("brain1Num", numAgent1);
        }
        //otherwise set to initial value
        else{
            org->dataMap.set("brain1Num", initialAgent1);
        }

        auto brain = org->brains["brain::"];
        auto brain2 = org->brains["brain2::"];

        int brain1Num = org->dataMap.getAverage("brain1Num");
        
        // For every evaluation
        for(int j = 0; j < evaluationsPerGeneration; j++){
            // clear the brain - resets brain state including memory
            brain->resetBrain();
            brain2->resetBrain();
            std::vector<std::tuple<std::shared_ptr<AbstractBrain>, std::string>> brainInfo;

            for(int q = 0; q < numAgents; q++){
                //Add brains proportional to number of agents that will have them 
                if(q < brain1Num){
                    brainInfo.push_back(std::make_tuple(brain->makeCopy(brain->PT), brain1Name));
                }
                else{
                    brainInfo.push_back(std::make_tuple(brain2->makeCopy(brain2->PT), brain2Name));
                }
            }
            //Get copys for this swarm
            std::vector<int> positions = initPositionsAll[j];
            std::vector<int> orientations = initOrientationsAll[j];
            std::vector<ForageWorld::Resource> world = initWorldAll[j];
            
            ForageWorld::Tracker tracker = forageTask(brainInfo, world, positions, orientations, false);
            
            for (const auto& item : tracker.data) {
                // Append key-value pairs to the new map
                org->dataMap.append(item.first, item.second);
                //std::cout << item.first << item.second << std::endl;
            }
        }
    }
    if(Global::update==Global::updatesPL->get()){
        double bestest = -1000000000;
        auto bestBrain = groups[groupName]->population[0]->brains["brain::"];
        auto bestBrain2 = groups[groupName]->population[0]->brains["brain2::"];
        std::string bestComp;
        for (int i = 0; i < popSize; i++) {
            auto org = groups[groupName]->population[i];
            double s = org->dataMap.getAverage("score");
            if(s > bestest){
                bestest = s;
                bestBrain = org->brains["brain::"];
                bestBrain2 = org->brains["brain2::"];
                bestComp = std::to_string(org->dataMap.getAverage("brain1Num"));
            }
        }
        if(verbose){
            showBestTaskBrain(bestBrain, bestBrain2, stoi(bestComp));
        }
        bestBrain->saveStructure("brain.txt");
        bestBrain2->saveStructure("brain2.txt");
        std::cout << "bestComp: " << bestComp << std::endl;
    }
}

//Generate positions (stored as integers) for each agent
std::vector<int> ForageWorld::genAgentPositions(){
    std::vector<int> possiblePositions(xDim*yDim, 0);
    std::iota(possiblePositions.begin(), possiblePositions.end(), 0);
    std::shuffle(possiblePositions.begin(), possiblePositions.end(), Random::getCommonGenerator());
    possiblePositions.resize(numAgents);
    return possiblePositions;
}

std::vector<int> ForageWorld::genAgentOrientations(){
    //For now 4 directions, 0=up, 1=left, 2=down, 3=right
    std::vector<int> orientations(numAgents);
    for(int &o : orientations){
        o = Random::getInt(0, 3);
    }
    return orientations;
}

std::vector<ForageWorld::Resource> ForageWorld::genTaskWorld(std::vector<int> positions){
        int size = xDim*yDim;
        std::vector<ForageWorld::Resource> world(size);

        int numResource1 = static_cast<int>(rProp[0] * size);
        int numResource2 = static_cast<int>(rProp[1] * size);

        for(int i = 0; i < numResource1; i++){
            world[i] = createResource(taskOneID);
        }
        for(int j = 0; j < numResource2; j++){
            world[numResource1 + j] = createResource(taskTwoID);
        }
        
        //Make the rest empty 
        std::fill(world.begin() + (numResource1 + numResource2), world.end(), createResource(empty_val));
        //Shuffle to make random order
        std::shuffle(world.begin(), world.end(), Random::getCommonGenerator());
        
        return world;
}

std::vector<int> ForageWorld::getPerception(const int pos, const std::vector<ForageWorld::Resource>& world, const int agentOrient, const std::vector<int> &positions){
    std::vector<int> perception;
    //(0, 0 is top left) 
    int x = pos % xDim;  // Get x-coordinate
    int y = pos / xDim;  // Get y-coordinate
    std::vector<int> pos_front;
    std::vector<int> pos_front2;
    std::vector<int> pos_right;
    std::vector<int> pos_left;

    // Function to check if a position is out of bounds
    auto isOutOfBounds = [](int newX, int newY, int xDim, int yDim) {
        return newX < 0 || newY < 0 || newX >= xDim || newY >= yDim;
    };

    switch(agentOrient){
        case up:
            pos_front.assign({x, y-1});
            pos_front2.assign({x, y-2});
            pos_right.assign({x+1, y});
            pos_left.assign({x-1, y});
            break;
        case left:
            pos_front.assign({x-1, y});
            pos_front2.assign({x-2, y});
            pos_right.assign({x, y-1});
            pos_left.assign({x, y+1});
            break;                    
        case down:
            pos_front.assign({x, y+1});
            pos_front2.assign({x, y+2});
            pos_right.assign({x-1, y});
            pos_left.assign({x+1, y});
            break;                    
        case right:
            pos_front.assign({x+1, y});
            pos_front2.assign({x+2, y});
            pos_right.assign({x, y+1});
            pos_left.assign({x, y-1});
            break;    
    }
    //agents inputs from front and sides
    //remove pos_front2 for now
    //for (const auto& v : {pos_front, pos_front2, pos_right, pos_left}) {
    for (const auto& v : {pos_front, pos_front2, pos_right, pos_left}) {
        if(!isOutOfBounds(v[0], v[1], xDim, yDim)){
            int p = v[1] * xDim + v[0];
            //do we see a resource, or another agent?
            if(std::find(positions.begin(), positions.end(), p) == positions.end()){
                perception.push_back(world[v[1] * xDim + v[0]].kind);
            }
            else{
                perception.push_back(agent_val);
            }
        }
        else{
            perception.push_back(wall_val);
        }
    }
    //agents current position type and inputs
    perception.push_back(world[pos].kind);

    perception.push_back(world[pos].f1);
    perception.push_back(world[pos].f2);
    //kind front, kind front2, kind right, kind left, kind curr, in1, in2
    return perception;
}

ForageWorld::Tracker ForageWorld::createTracker() {
    ForageWorld::Tracker t;

    t.data["score"] = 0.0;
    t.data[brain1Name + "_Resource" + std::to_string(taskOneID) + "_Completed"] = 0;
    t.data[brain1Name + "_Resource" + std::to_string(taskOneID) + "_Attempted"] = 0;

    t.data[brain1Name + "_Resource" + std::to_string(taskTwoID) + "_Completed"] = 0;
    t.data[brain1Name + "_Resource" + std::to_string(taskTwoID) + "_Attempted"] = 0;

    t.data[brain2Name + "_Resource" + std::to_string(taskOneID) + "_Completed"] = 0;
    t.data[brain2Name + "_Resource" + std::to_string(taskOneID) + "_Attempted"] = 0;

    t.data[brain2Name + "_Resource" + std::to_string(taskTwoID) + "_Completed"] = 0;
    t.data[brain2Name + "_Resource" + std::to_string(taskTwoID) + "_Attempted"] = 0;
    return t;
};

ForageWorld::Tracker ForageWorld::forageTask(const std::vector<std::tuple<std::shared_ptr<AbstractBrain>, std::string>> brainInfo, std::vector<ForageWorld::Resource> &world, std::vector<int> &positions, std::vector<int> &orientations, bool printing){
    //tracker will store some stats
    ForageWorld::Tracker tracker = createTracker();

    //Main foraging loop for a single swarm
    for (size_t j = 0; j < timesteps; j++){
        //Every timestep, make a shuffled list of agents to be evaluated
        std::vector<int> waitingAgents(numAgents);
        std::iota(waitingAgents.begin(), waitingAgents.end(), 0);           
        std::shuffle(waitingAgents.begin(), waitingAgents.end(), Random::getCommonGenerator());
        while (!waitingAgents.empty()){
            int curr_agent_ID = waitingAgents[0];
            waitingAgents.erase(waitingAgents.begin());
            int curr_agent_pos = positions[curr_agent_ID];
            int curr_agent_orient = orientations[curr_agent_ID];

            auto& info = brainInfo[curr_agent_ID]; 
            std::shared_ptr<AbstractBrain> brain = std::get<0>(info);
            std::string brainName = std::get<1>(info);
            Resource r = world[curr_agent_pos];

            //kind front, kind front2, kind right, kind left, kind curr, in1, in2
            std::vector<int> agentPerception = getPerception(curr_agent_pos, world, curr_agent_orient, positions);
            //4 sensors, 2 binary resource IDs each. Wall in front sensor
            std::vector<bool> sensorVals = binarizeInputs(agentPerception);

            //Sensor inputs 
            for(int i = 0; i < sensorVals.size(); i++){
                if(sensorVals[i]){
                    brain->setInput(i, 1);
                }
                else{
                    brain->setInput(i, 0);
                }
            }
            //Task inputs
            brain->setInput(sensorVals.size(), agentPerception[5]);
            brain->setInput(sensorVals.size() + 1, agentPerception[6]);

            // run a brain update (i.e. ask the brain to convert it's inputs into outputs)
            brain->update();

            int move1 = Bit(brain->readOutput(0));
            int move2 = Bit(brain->readOutput(1));
            auto taskr = (r.kind != ID_linReg) ? Bit(brain->readOutput(2)) : brain->readOutput(2);
            //Allow brains to evolve to NOT solve a task, to not incur penalty
            int rejectSolve = Bit(brain->readOutput(3));

            // update agent or world state here
            // move if going forward, try to solve task if staying still or rotating 
            if(move1 == 1 && move2 == 1){
                positions[curr_agent_ID] = calcAgentMove(curr_agent_pos, curr_agent_orient);
            }
            else{
                orientations[curr_agent_ID] = calcAgentRotate(curr_agent_orient, move1, move2);
                //Try to solve task
                if(!rejectSolve){
                    if(r.kind != empty_val){
                        tracker.data[brainName + "_Resource" + std::to_string(r.kind) + "_Attempted"] += 1;
                    }

                    double s = calcTask(taskr, world[curr_agent_pos]);
                    tracker.data["score"] += s;
                    //If successful (score > 0) 
                    if(s > 0){
                        //Set current position to empty
                        world[curr_agent_pos] = createResource(empty_val);
                        //Create new random resource according to r1replaceRate
                        spawnResource(world, positions);
                        tracker.data[brainName + "_Resource" + std::to_string(r.kind) + "_Completed"] += 1;
                    }
                }
            }
        } 
        if(printing){
            printWorld(positions, world, xDim, yDim, orientations);
        }
    }
    return tracker;
}

//Currently calcs agents desired move
int ForageWorld::calcAgentMove(const int pos, const int orient) {
    int x = pos % xDim;  // Get current x-coordinate
    int y = pos / xDim;  // Get current y-coordinate
    switch(orient){
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
    //If you're requesting a position out of bounds, no. 
    if(x >= xDim || y >= yDim || x < 0 || y < 0){
        return pos;
    }
    else{
        int new_pos = y * xDim + x;
        return new_pos; 
    }
}

//Currently calcs agents desired orientation change
int ForageWorld::calcAgentRotate(const int curr_orient, const int lMotor, const int rMotor) {
    int orient = curr_orient;
    if(lMotor > rMotor){
        orient -= 1;
        if(orient == -1){
            orient = 3;
        }
    }
    else if(lMotor < rMotor){
        orient += 1;
        if(orient == 4){
            orient = 0;
        }
    }
    //Just return original orientation if both motors are 0
    return orient;
}


int ForageWorld::mutateComp(int oldAgent1Num){
    //if mutate
    if(Random::getDouble(0.0, 1.0) < mRate){
        //choose to go up or down
        int newAgent1Num = (Random::getInt(0, 1) == 1) ? oldAgent1Num + 1 : oldAgent1Num - 1;
        //keep in bounds
        return std::min(std::max(newAgent1Num, 1), numAgents - 1);
    }
    else{
        return oldAgent1Num;
    }
}

void ForageWorld::spawnResource(std::vector<ForageWorld::Resource> &world, const std::vector<int> &positions){
    double sample = Random::getDouble(0.0, 1.0);
    int choice = (sample < r1replaceRate) ? taskOneID : taskTwoID; 
    ForageWorld::Resource r = createResource(choice);
    int pos = Random::getInt(0, xDim*yDim - 1);
    //make sure generated position is not where an agent is currently, and also the position is empty
    while(std::find(positions.begin(), positions.end(), pos) != positions.end() || world[pos].kind != empty_val){
        pos = Random::getInt(0, xDim*yDim - 1);
    }
    world[pos] = r;
}

void ForageWorld::showBestTaskBrain(std::shared_ptr<AbstractBrain> brain, std::shared_ptr<AbstractBrain> brain2, int bestNumAgent1){
    std::vector<int> positions = genAgentPositions();
    std::vector<int> orientations = genAgentOrientations();
    std::vector<ForageWorld::Resource> world = genTaskWorld(positions);
    brain->resetBrain();
    brain2->resetBrain();
    std::vector<std::tuple<std::shared_ptr<AbstractBrain>, std::string>> brainInfo;

    for(int q = 0; q < numAgents; q++){
        //Add brains proportional to number of agents that will have them 
        if(q < bestNumAgent1){
            brainInfo.push_back(std::make_tuple(brain->makeCopy(brain->PT), brain1Name));
        }
        else{
            brainInfo.push_back(std::make_tuple(brain2->makeCopy(brain2->PT), brain2Name));
        }
    }
    printWorld(positions, world, xDim, yDim, orientations);
    ForageWorld::Tracker tracker = forageTask(brainInfo, world, positions, orientations, true);
    std::cout << "Tracker Contents:" << std::endl;
    for (const auto& pair : tracker.data) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
}

// // the requiredGroups function lets MABE know how to set up populations of organisms that this world needs
auto ForageWorld::requiredGroups() -> unordered_map<string,unordered_set<string>> {
    //4 sensors, 2 binary resource IDs each. Wall in front sensor. Task ins. 
    // 4 x 2 + 1 + 2 
	return { { groupName, { "B:brain::,13,4", "B:brain2::,13,4" } } };
        
        // this tells MABE to make a group called "root::" with a brain called "root::" that takes 2 inputs and has 1 output
        // "root::" here also indicates the namespace for the parameters used to define these elements.
        // "root::" is the default namespace, so parameters defined without a namespace are "root::" parameters
}
