#include "main.h"
#include "Configuration.h"
#include <iostream>
#include <fstream>
#include <string>
#include "ASPHelper.h"
#include "Network.h"

Network * network = new Network();

int main(int argc, char ** argv) {

    Configuration::parseConfig();

    std::string input_file_network, output_file;

    if(argc < 2)
    {
        std::cout << "Invalid number of arguments: " << argc << std::endl;
        return -1;
    }

    process_arguments(argc, argv, input_file_network, output_file);

    ASPHelper::parseNetwork(input_file_network, network);

    //main function that revises the model
    modelRevision(input_file_network);
    
};


// Function that initializes the program.
// Can process optional arguments or configurations
void process_arguments(const int argc, char const * const * argv, std::string & input_file_network, std::string & output_file) {

    input_file_network = argv[1];

    if(argc > 2)
        output_file = argv[2];

    return;
};



//Model revision procedure
// 1) tries to repair functions
// 2) tries to flip the sign of the edges
// 3) tries to add or remove edges
void modelRevision(std::string input_file_network) {

    bool is_consistent = false;
    int optimization = -2;

    std::vector<FunctionInconsistencies*> fInconsistencies = checkConsistencyFunc(input_file_network, optimization);

    //At this point we have an inconsistent network with functions candidates to be repaired

    //for each possible inconsistency solution, try to make the model consistent
    std::vector<Function*> newFunctions; 
    for(auto it = fInconsistencies.begin(), end = fInconsistencies.end(); it != end; it++)
    {
        if(optimization != 0)
        {
            newFunctions = repairFuncConsistency(input_file_network, (*it), optimization);
        }
        if(optimization == 0)
        {
            break;
        }
    }

    //TODO check quantity of the newFunction in order to see if every function was repaired
    for(auto it = newFunctions.begin(), end = newFunctions.end(); it!=end; it++)
    {
        std::cout << "REPAIR - Change function " << (*it)->node_ << " to " << (*it)->printFunction() << std::endl;
    }

};


//function reponsible to check the consistency of a model and return a set of possible function inconsistencies
std::vector<FunctionInconsistencies*> checkConsistencyFunc(std::string input_file_network, int & optimization) {

    std::vector<std::vector<std::string>> result_raw;
    std::vector<FunctionInconsistencies*> result;
    
    //consistency check
    if(Configuration::check_ASP)
    {
        // invoke the consistency check program in ASP
        optimization = ASPHelper::checkConsistency(input_file_network, result_raw);
    }
    else
    {
        //TODO Add other implementations
        //convert ASP to sat or other representation
        //test consistency
    }

    //TODO Consider repair edges
    if(optimization < 0)
    {
        std::cout << "It is not possible to repair this network for now." << std::endl;
    }

    if(optimization == 0)
    {
        std::cout << "This network is consistent!" << std::endl;
    }

    if(optimization > 0 && Configuration::check_ASP)
    {
        //parse the raw results to an internal representation.
        //this should be done at ASP level in the check consistency function
        //TODO
        result = ASPHelper::parseFunctionRepairResults(result_raw);
    }

    return result;
};



//This function receives an inconsistent model and a set of functions to be repaired and try to repair the target functions making the model consistent
//returns the set of new functions to be replaced
std::vector<Function*> repairFuncConsistency(std::string input_file_network, FunctionInconsistencies* repairSet, int & optimization){

    std::vector<Function*> result;
    std::vector<std::vector<Function*>> candidates;
    optimization = 0;

    if(Configuration::function_ASP)
    {
        //for each function to be repaired
        for(auto it = repairSet->generalization_.begin(), end = repairSet->generalization_.end(); it != end; it++)
        {
            //each function must have a list of replacement candidates and each msut be tested until it works
            Function* originalF = network->getNode((*it))->getFunction();
            if(originalF == nullptr)
            {
                continue;
            }

            //if the function only has 1 regulator then it is not possible to change the function
            //better try to flip the sign of the edge
            if(originalF->getNumberOfRegulators() < 2)
            {
                //std::cout << "WARN! Not possible to repair function of " << (*it) << std::endl;
                optimization++;
                std::cout << "REPAIR: Try to change the sign of the edge from " << originalF->getRegulatorsMap().begin()->first << " to " << (*it) << std::endl;
                continue;
            }

            //check top function for the necessity of flipping an edge
            if(!checkPointFunction(repairSet, originalF, true))
            {
                //TODO change 1 edge;
                Function* flipEdgeFunc = repairFuncConsistencyFlippingEdge(repairSet, originalF, true);
                if(flipEdgeFunc != nullptr)
                {
                    optimization++;
                    result.push_back(flipEdgeFunc);
                }
                else
                {
                    std::cout << "WARN: Not possible to flip an edge to repair functio " << originalF->node_ << std::endl;
                }
                continue;
            }

            // get the possible candidates to replace the inconsistent function
            bool functionRepaired = false;
            std::vector<Function*> tCandidates = ASPHelper::getFunctionReplace(originalF,true);
            while(!tCandidates.empty())
            {
                Function* candidate = tCandidates.front();
                tCandidates.erase (tCandidates.begin());

                if(isFuncConsistentWithLabel(repairSet, candidate))
                {
                    result.push_back(candidate);
                    optimization++;
                    functionRepaired = true;
                    break;
                }
                else{
                    std::vector<Function*> tauxCandidates = ASPHelper::getFunctionReplace(candidate,true);
                    if(!tauxCandidates.empty())
                        tCandidates.insert(tCandidates.end(),tauxCandidates.begin(),tauxCandidates.end());
                }
            }
            if(!functionRepaired)
            {
                //have to flip 1 edge
                //For 1 profile is not suposed to reach this point
                std::cout << "WARN: It was not possible to repair function " << (*it) << std::endl;
            }

            
        }


        //TODO change object in order to avoid repeating code
        for(auto it = repairSet->particularization_.begin(), end = repairSet->particularization_.end(); it != end; it++)
        {
            //each function must have a list of replacement candidates and each msut be tested until it works
            Function* originalF = network->getNode((*it))->getFunction();
            if(originalF == nullptr)
            {
                continue;
            }

            //if the function only has 1 regulator then it is not possible to change the function
            //better try to flip the sign of the edge
            if(originalF->getNumberOfRegulators() < 2)
            {
                //std::cout << "WARN! Not possible to repair function of " << (*it) << std::endl;
                optimization++;
                std::cout << "REPAIR: Try to change the sign of the edge from " << originalF->getRegulatorsMap().begin()->first << " to " << (*it) << std::endl;
                continue;
            }

            //check top function for the necessity of flipping an edge
            if(!checkPointFunction(repairSet, originalF, false))
            {
                //TODO change 1 edge;
                Function* flipEdgeFunc = repairFuncConsistencyFlippingEdge(repairSet, originalF, false);
                if(flipEdgeFunc != nullptr)
                {
                    optimization++;
                    result.push_back(flipEdgeFunc);
                }
                else
                {
                    std::cout << "WARN: Not possible to flip an edge to repair functio " << originalF->node_ << std::endl;
                }
                continue;
            }

            // get the possible candidates to replace the inconsistent function
            bool functionRepaired = false;
            std::vector<Function*> tCandidates = ASPHelper::getFunctionReplace(originalF,false);
            while(!tCandidates.empty())
            {
                Function* candidate = tCandidates.front();
                tCandidates.erase (tCandidates.begin());

                if(isFuncConsistentWithLabel(repairSet, candidate))
                {
                    result.push_back(candidate);
                    optimization++;
                    functionRepaired = true;
                    break;
                }
                else{
                    std::vector<Function*> tauxCandidates = ASPHelper::getFunctionReplace(candidate,false);
                    if(!tauxCandidates.empty())
                        tCandidates.insert(tCandidates.end(),tauxCandidates.begin(),tauxCandidates.end());
                }
            }
            if(!functionRepaired)
            {
                //have to flip 1 edge
                //For 1 profile is not suposed to reach this point
                std::cout << "WARN: It was not possible to repair function " << (*it) << std::endl;
            }

            
        }
        
    }
    else
    {
    //TODO support other solvers
    } 

    return result;
};


bool isFuncConsistentWithLabel(FunctionInconsistencies* labeling, Function* f)
{
    
    for(int i = 1; i <= f->nClauses_; i++)
    {
        bool isClauseSatisfiable = true;
        std::vector<std::string> clause = f->clauses_[i];
        for(auto it = clause.begin(), end = clause.end(); it!=end; it++)
        {
            Edge* e = network->getEdge((*it), f->node_);
            if(e != nullptr)
            {
                //positive interaction
                if(e->getSign() > 0)
                {
                    if(labeling->vlabel_[(*it)] == 0)
                    {
                        isClauseSatisfiable = false;
                        break;
                    }
                }
                //negative interaction
                else
                {
                    if(labeling->vlabel_[(*it)] > 0)
                    {
                        isClauseSatisfiable = false;
                        break;
                    }
                }
            }
            else{
                std::cout << "WARN: Missing edge from " << (*it) << " to " << f->node_ << std::endl;
                return false;
            }
        }
        if(isClauseSatisfiable)
            return true;
    }
    return false;
};

//checks thhe top or bottom function for consistency.
// Allows to check if it is possible to repair a function without changing the topology
bool checkPointFunction(FunctionInconsistencies* labeling, Function* f, bool generalize){
    std::map<std::string,int> map = f->getRegulatorsMap();
    if(generalize)
    {
        // disjunction of all regulators
        for(auto it = map.begin(), end = map.end(); it!= end; it++)
        {
            Edge* e = network->getEdge(it->first, f->node_);
            if(e != nullptr)
            {
                //positive interaction
                if(e->getSign() > 0 && labeling->vlabel_[it->first] > 0)
                {
                    return true;
                }
                //negative interaction
                if(e->getSign() == 0 && labeling->vlabel_[it->first] == 0)
                {
                    return true;
                }
            }
            else{
                std::cout << "WARN: Missing edge from " << it->first << " to " << f->node_ << std::endl;
                return false;
            }
        }
        return false;
    }
    else{
        // conjunction of all regulators
        for(auto it = map.begin(), end = map.end(); it!= end; it++)
        {
            Edge* e = network->getEdge(it->first, f->node_);
            if(e != nullptr)
            {
                //positive interaction
                if(e->getSign() > 0 && labeling->vlabel_[it->first] == 0)
                {
                    return false;
                }
                //negative interaction
                if(e->getSign() == 0 && labeling->vlabel_[it->first] > 0)
                {
                    return false;
                }
            }
            else{
                std::cout << "WARN: Missing edge from " << it->first << " to " << f->node_ << std::endl;
                return false;
            }
        }
        return true;
    }
    return true;

};


Function* repairFuncConsistencyFlippingEdge(FunctionInconsistencies* labeling, Function* f, bool generalize){
    std::map<std::string,int> map = f->getRegulatorsMap();
    std::vector<Function*> tCandidates;
    tCandidates.push_back(f);
    while(!tCandidates.empty())
    {
        Function* candidate = tCandidates.front();
        tCandidates.erase (tCandidates.begin());

        //try to flip each edge
        for(auto it = map.begin(), end = map.end(); it!= end; it++)
        {
             Edge* e = network->getEdge(it->first, f->node_);
             if(e!=nullptr)
             {
                 e->flipSign();
                 if(isFuncConsistentWithLabel(labeling, candidate))
                 {
                     e->flipSign();
                     std::cout << "REPAIR: Try to change the sign of the edge from " << it->first << " to " << f->node_ << std::endl;
                     return candidate;
                 }
             }
             else{
                std::cout << "WARN: Missing edge from " << it->first << " to " << f->node_ << std::endl;
                continue;
            }
        }

        //renew candidates
        std::vector<Function*> tauxCandidates = ASPHelper::getFunctionReplace(candidate,generalize);
        if(!tauxCandidates.empty())
            tCandidates.insert(tCandidates.end(),tauxCandidates.begin(),tauxCandidates.end());
    }

    return nullptr;
};
