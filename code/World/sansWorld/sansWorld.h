#pragma once

#include <World/AbstractWorld.h>
#include <string>
#include <memory> // shared_ptr
#include <map>

#include <cstdlib>
#include <thread>
#include <vector>

using std::shared_ptr;
using std::string;
using std::map;
using std::unordered_map;
using std::unordered_set;
using std::to_string;

class sansWorld : public AbstractWorld {

public:
  static std::shared_ptr<ParameterLink<int>> evaluationsPerGenerationPL;

  int evaluationsPerGeneration;
  std::string groupName = "root::";
  std::string brainName = "root::";

  sansWorld(std::shared_ptr<ParametersTable> PT);
  virtual ~sansWorld() = default;

  virtual auto evaluate(map<string, shared_ptr<Group>>& /*groups*/, int /*analyze*/, int /*visualize*/, int /*debug*/) -> void override;

  double doTask(std::shared_ptr<AbstractBrain> brain);
  double doTask2(std::shared_ptr<AbstractBrain> brain);

  virtual auto requiredGroups() -> unordered_map<string,unordered_set<string>> override;

};
