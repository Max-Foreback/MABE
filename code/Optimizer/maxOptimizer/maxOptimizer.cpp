//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include "maxOptimizer.h"

std::shared_ptr<ParameterLink<int>> maxOptimizer::tournamentSizePL =
	Parameters::register_parameter("OPTIMIZER_MAX-tournamentSize", 5, "number of organisims compaired in each tournament");

std::shared_ptr<ParameterLink<int>> maxOptimizer::numberParentsPL =
	Parameters::register_parameter("OPTIMIZER_MAX-numberParents", 1, "number of parents used to produce offspring (each parent will be selected by a unique tournament)");

std::shared_ptr<ParameterLink<bool>> maxOptimizer::minimizeErrorPL =
Parameters::register_parameter("OPTIMIZER_MAX-minimizeError", false, "if true, Tournament Optimizer will select lower optimizeValues");

std::shared_ptr<ParameterLink<std::string>> maxOptimizer::optimizeValuePL =
	Parameters::register_parameter("OPTIMIZER_MAX-optimizeValue", (std::string) "DM_AVE[score]", "value to optimize (MTree)");


int maxOptimizer::selectParent(int tournamentSize, bool minimizeError, std::vector<double> scores, int popSize){
	int winner, challanger;
	winner = Random::getIndex(popSize);
	for (int i = 0; i < tournamentSize - 1; i++) {
		challanger = Random::getIndex(popSize);
		if (minimizeError?scores[challanger] < scores[winner]:scores[challanger] > scores[winner]) {
			winner = challanger;
		}
	}
	return winner;
}


maxOptimizer::maxOptimizer(std::shared_ptr<ParametersTable> PT_)
	: AbstractOptimizer(PT_) {

	tournamentSize = tournamentSizePL->get(PT);
	numberParents = numberParentsPL->get(PT);
	minimizeError = minimizeErrorPL->get(PT);

	optimizeValueMT = stringToMTree(optimizeValuePL->get(PT));

	if (!minimizeError) {
		optimizeFormula = optimizeValueMT; // set this so Archivist knows which org is max
	}
	else {
		optimizeFormula = stringToMTree("0-("+optimizeValuePL->get(PT)+")");; // set this so Archivist knows which org is max
	}

	popFileColumns.clear();
	popFileColumns.push_back("optimizeValue");
}

void maxOptimizer::optimize(std::vector<std::shared_ptr<Organism>> &population) {
	auto popSize = population.size();

	std::vector<double> scores(popSize, 0);
	double aveScore = 0;
	double aveComp = 0;
	double maxScore = optimizeValueMT->eval(population[0]->dataMap, PT)[0];
	double minScore = maxScore;

	killList.clear();

	for (size_t i = 0; i < popSize; i++) {
		killList.insert(population[i]);
		double opVal = optimizeValueMT->eval(population[i]->dataMap, PT)[0];
		scores[i] = opVal;
		aveScore += opVal;
		aveComp += population[i]->dataMap.getAverage("brain1Num");
		population[i]->dataMap.set("optimizeValue", opVal);
		maxScore = std::max(maxScore, opVal);
		minScore = std::min(minScore, opVal);
	}
	
	aveScore /= popSize;
	aveComp /= popSize;

	std::vector<std::shared_ptr<Organism>> parents;

	for (int i = 0; i < popSize; i++) {
		if (numberParents == 1) {
			auto parent = population[selectParent(tournamentSize, minimizeError, scores, popSize)];
			auto child = parent->makeMutatedOffspringFrom(parent);
			child->dataMap.set("brain1Num", static_cast<int>(parent->dataMap.getAverage("brain1Num")));
			population.push_back(child); // add to population
		}
		//DIFF COMPS ONLY SUPPORTED FOR ASEXUAL REPRO RN
		else {
			std::cout << "STOP DOING MUTLI PARENT" << std::endl;
			parents.clear();
			parents.push_back(population[selectParent(tournamentSize, minimizeError, scores, popSize)]);
			while (static_cast<int>(parents.size()) < numberParents) {
				parents.push_back(population[selectParent(tournamentSize, minimizeError, scores, popSize)]); // select from culled
			}
			population.push_back(parents[0]->makeMutatedOffspringFromMany(parents)); // push to population
		}
	}

	for (int i = 0; i < popSize; i++) {
		population[i]->dataMap.set("tournament_numOffspring", population[i]->offspringCount);
	}

	if (!minimizeError) {
		std::cout << "max = " << std::to_string(maxScore) << "   ave = " << std::to_string(aveScore) << "   comp = " << std::to_string(aveComp);
	}
	else {
		std::cout << "min = " << std::to_string(minScore) << "   ave = " << std::to_string(aveScore) << "   comp = " << std::to_string(aveComp);
	}
}

