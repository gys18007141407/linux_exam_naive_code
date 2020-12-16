#include <iostream>
#include <vector>

extern "C" void concate(std::vector<std::string>& parameters){

    // parameters[0] is the function's name
    // others are parameters

    std::string res = "";

    for(auto it = parameters.begin()+1; it != parameters.end(); ++it) res += *it;

    parameters.clear();

    parameters.push_back(res);


}