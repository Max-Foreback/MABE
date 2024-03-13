//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License


#include "sansWorld.h"

shared_ptr<ParameterLink<int>> sansWorld::evaluationsPerGenerationPL =
    Parameters::register_parameter("WORLD_sans-evaluationsPerGeneration", 5,
    "how many times should each organism be tested in each generation?");

sansWorld::sansWorld(std::shared_ptr<ParametersTable> PT): AbstractWorld(PT) {
  evaluationsPerGeneration = evaluationsPerGenerationPL->get(PT);
  // columns to be added to ave file
  popFileColumns.clear();
  popFileColumns.push_back("score");
}

auto sansWorld::evaluate(map<string, shared_ptr<Group>>& groups, int analyze, int visualize, int debug) -> void {
    int popSize = groups[groupName]->population.size(); 
    
    // For every agent
    for (int i = 0; i < popSize; i++) {
        // create a shortcut to access the organism and organisms brain
        auto org = groups[groupName]->population[i];
        auto brain = org->brains["root::"];
        //auto brain2 = org->brains["bob::"];

        // For every evaluation
        for(int j = 0; j < evaluationsPerGeneration; j++){
            // clear the brain - resets brain state including memory
            brain->resetBrain();
            //brain2->resetBrain();
            
            int score = doTask2(brain);
            
            org->dataMap.append("score", score);
        }
    } // end of population loop
}


double sansWorld::doTask2(std::shared_ptr<AbstractBrain> brain){
    double score = 0;
    for(int i = 0; i < 100; i++){
        int in1 = Random::getInt(-2, 2);
        int in2 = Random::getInt(-2, 2);
        // set input based on agent and world state (can be int or double)
        brain->setInput(0, in1);
        brain->setInput(1, in2); 

        // run a brain update (i.e. ask the brain to convert it's inputs into outputs)
        for(int j = 0; j < 10; j++){
            brain->update();
        }
        // read brain output 0
        double out = brain->readOutput(0);
        double diff = out - static_cast<double>((in1 * in2) + (in1 - in2));
        score += -1*pow(diff, 2);
    }
    return score;
}

double sansWorld::doTask(std::shared_ptr<AbstractBrain> brain){
    double score = 0;
    for(int i = 0; i < 100; i++){
        int in1 = Random::getInt(0, 1);
        int in2 = Random::getInt(0, 1);
        // set input based on agent and world state (can be int or double)
        brain->setInput(0, in1);
        brain->setInput(1, in2); 

        // run a brain update (i.e. ask the brain to convert it's inputs into outputs)
        for(int j = 0; j < 10; j++){
            brain->update();
        }
        // read brain output 0
        double out = brain->readOutput(0);
        if(out == in1 ^ in2){
            score++;
        }
    }
    return score;
}

// the requiredGroups function lets MABE know how to set up populations of organisms that this world needs
auto sansWorld::requiredGroups() -> unordered_map<string,unordered_set<string>> {
	return { { groupName, { "B:bob::,2,1", "B:root::,2,1"} } };
        
        // this tells MABE to make a group called "root::" with a brain called "root::" that takes 2 inputs and has 1 output
        // "root::" here also indicates the namespace for the parameters used to define these elements.
        // "root::" is the default namespace, so parameters defined without a namespace are "root::" parameters
}
