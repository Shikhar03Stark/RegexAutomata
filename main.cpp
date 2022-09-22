#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <queue>
#include <algorithm>

/**
 * @brief The workflow of Regex to DFS is as follows:
 * 1) FACompiler is initialized with alphabet and null character
 * 2) FACompiler accepts a regex in compile() method which is checked for sytactical mistakes and then converts it into NFA and DFA and returns a FA object
 * 3) FA holds NFA and DFA machines and can be used to check acceptance of a string
 * 
 * All the classes and struct starting 'D' are used for Deterministic nature, and 'ND' for Non-deterministic nature
 * 
 */

struct DNode{
    int id;
    std::map<char, DNode*> next;
    DNode(int id){
        this->id = id;
    }
    DNode(int id, std::map<char, DNode*> next){
        this->id;
        this->next = next;
    }
};

struct NDNode{
    int id;
    std::map<char, std::vector<NDNode*>> next;

    std::set<NDNode*> null_transition(char nullchar, char input){
        std::set<NDNode*> result;
        std::stack<std::pair<NDNode*, bool>> S;
        // result.insert(this);
        S.push({this, false});
        while(!S.empty()){
            std::pair<NDNode*, bool> state = S.top(); S.pop();
            NDNode *node = state.first;
            bool charUsed = state.second;
            if(result.find(node) == result.end()){
                result.insert(node);
                if(node->next.count(nullchar) > 0){
                    for(auto &null_next: node->next[nullchar]){
                        if(result.find(null_next) == result.end()){
                            S.push({null_next, false});
                        }
                    }
                }

                if(!charUsed){
                    for(auto& next: node->next[input]){
                        if(result.find(next) == result.end()){
                            S.push({next, true});
                        }
                    }
                }
            }
        }

        return result;
    }

    // return the null-closure of current node
    std::vector<NDNode*> epsilon_closure(char nullchar){
        std::set<NDNode*> visited;
        std::stack<NDNode*> closure;
        closure.push(this);
        std::vector<NDNode*> result;
        while(!closure.empty()){
            NDNode* node = closure.top();
            closure.pop();
            if(visited.find(node) == visited.end()){
                visited.insert(node);
                for(auto& next_node : node->next[nullchar]){
                    closure.push(next_node);
                }
            }
        }

        return std::vector<NDNode*>(visited.begin(), visited.end());

    }

    NDNode(int id){
        this->id = id;
    }
    NDNode(int id, std::map<char, std::vector<NDNode*>> next){
        this->id = id;
        this->next = next;
    }
};

class Machine{
    public:
    virtual void print_machine_table() = 0;
    virtual bool accepted(const std::string &s) = 0;
};

class DMachine: public Machine{
    public:
        std::string alphabet;
        std::set<DNode*> final_states;
        DNode* start;
        DNode* end;

        /// @brief First state is the starting state, states begining with * are final states.
        void print_machine_table(){

            std::cout << "States starting with * are final states" << std::endl;
            std::cout << "Starting state is " << start->id << std::endl;
            std::cout << std::endl;
            int sz = alphabet.size();
            std::set<DNode*> visited;
            std::stack<DNode*> S;
            S.push(start);
            while(!S.empty()){
                DNode* node = S.top(); S.pop();
                if(visited.find(node) == visited.end()){
                    //Final states start with *
                    if(final_states.find(node) != final_states.end()){
                        std::cout << "*";
                    }
                    visited.insert(node);
                    std::cout << node->id << "\t=>\t";
                    for(int i = 0; i<sz; i++){
                        DNode* next = node->next[alphabet[i]];
                        std::cout << alphabet[i] << ":" << next->id << "\t";
                        if(visited.find(next) == visited.end()){
                            S.push(next);
                        }
                    }
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
        }

        bool accepted(const std::string &s) {

            for(auto& c: s){
                bool check = std::binary_search(alphabet.begin(), alphabet.end(), c);
                if(!check){
                    std::cerr << "Input string has non-alphabet: " << s << std::endl;
                    return false;
                }
            }

            DNode* t = start;
            int sz = s.size();
            for(int i = 0; i<sz; i++){
                t = t->next[s[i]];
            }

            bool isFinal = (final_states.find(t) != final_states.end());
            if(isFinal){
                return true;
            }
            
            return false;
        }

        std::vector<int> trace_states(const std::string &s){
            for(auto& c: s){
                bool check = std::binary_search(alphabet.begin(), alphabet.end(), c);
                if(!check){
                    std::cerr << "Input string has non-alphabet: " << s << std::endl;
                    return {};
                }
            }
            std::vector<int> result;
            DNode* t = start;

            result.push_back(t->id);
            int sz = s.size();
            for(int i = 0; i<sz; i++){
                t = t->next[s[i]];
                result.push_back(t->id);
            }

            return result;
        }
};

class NDMachine: public Machine{

    public:
        std::string alphabet;
        char nullchar;
        std::set<NDNode*> final_states;
        NDNode* start;
        NDNode* end;


        void print_machine_table(){

        }

        bool accepted(const std::string &s){
            return false;
        }
};

void DBG_print(const std::set<NDNode*> &state){
    std::cerr << "(";
    for(auto& e: state){
        std::cerr << e->id << ",";
    }
    std::cerr << ") ";
}

void DBG_table(const std::map<std::set<NDNode*>, std::vector<std::set<NDNode*>>> &table){
    for(auto& row: table){
        DBG_print(row.first);
        std::cerr << " :: ";
        for(auto& s: row.second){
            DBG_print(s);
            std::cerr << ", ";
        }
        std::cerr << std::endl;
    }
}

class FA{

    private:
        NDMachine *ndm;
        DMachine *dm;
        std::string alphabet;
        std::string regex;
        char nullchar;
        int nd_state_id;
        int d_state_id;

        /// @brief Performs Union operation according to thompson's rule
        /// @param a NDMachine A
        /// @param b NDMachine B
        /// @return returns new NDMachine with +2 states and +4 null transistions => (A+B)
        NDMachine* thompson_union(NDMachine *a, NDMachine *b){
            NDMachine *machine = new NDMachine();

            NDNode *q0, *q1;
            q0 = new NDNode(nd_state_id++);
            q1 = new NDNode(nd_state_id++);

            q0->next[nullchar].push_back(a->start);
            q0->next[nullchar].push_back(b->start);
            a->end->next[nullchar].push_back(q1);
            b->end->next[nullchar].push_back(q1);

            machine->start = q0;
            machine->end = q1;
            machine->final_states.insert(q1);
            machine->alphabet = this->alphabet;
            machine->nullchar = nullchar;
            return machine;
        }

        /// @brief Performs Kleene-closure according to thompson's rule
        /// @param a Kleene-closure of a
        /// @return returns new NDMachine with +2 states and +4 null transitions => (a*)
        NDMachine* thompson_kleene_closure(NDMachine *a){
            NDMachine *machine = new NDMachine();

            NDNode *q0, *q1;
            q0 = new NDNode(nd_state_id++);
            q1 = new NDNode(nd_state_id++);

            q0->next[nullchar].push_back(a->start);
            a->end->next[nullchar].push_back(q1);
            a->end->next[nullchar].push_back(a->start);
            q0->next[nullchar].push_back(q1);

            machine->start = q0;
            machine->end = q1;
            machine->final_states.insert(q1);
            machine->alphabet = this->alphabet;
            machine->nullchar = nullchar;
            return machine;
        }

        /// @brief Performs concatenation operation according to thompson's rule
        /// @param a NDMachine A
        /// @param b NDMacine B
        /// @return returns new NDMachine with +1 null transition => (AB)
        NDMachine* thompson_concatenate(NDMachine *a, NDMachine *b){
            NDMachine *machine = new NDMachine();
            
            NDNode* a_end = a->end, *b_start = b->start;
            a_end->next[nullchar].push_back(b_start);

            machine->start = a->start;
            machine->end = b->end;
            machine->final_states.insert(b->end);
            machine->alphabet = this->alphabet;
            machine->nullchar = this->nullchar;

            return machine;

        }

        /// @brief Converts char data-type to NDMachine
        /// @param c Transition character
        /// @return returns NDMachine with 2 states and 1 transition => (q0 -c-> q1)
        NDMachine* token_to_machine(char c){
            NDMachine *machine = new NDMachine();
            NDNode *q0, *q1;
            q0 = new NDNode(nd_state_id++);
            q1 = new NDNode(nd_state_id++);

            q0->next[c].push_back(q1);

            machine->start = q0;
            machine->end = q1;
            machine->final_states.insert(q1);
            machine->alphabet = this->alphabet;
            machine->nullchar = this->nullchar;
            return machine;
        }

        /// @brief Apply thompson's rule step-wise based on post-fix notation of Regex
        void construct_NFA(){
            int sz = regex.size();
            std::stack<NDMachine*> M;
            std::string ops = "+*.";
            for(int i = 0; i<sz; i++){
                int isOps = -1;
                for(int j = 0; j<3; j++){
                    if(regex[i] == ops[j]){
                        isOps = j;
                        break;
                    }
                }

                if(isOps == 0){
                    NDMachine *rm = M.top(); M.pop();
                    NDMachine *lm = M.top(); M.pop();
                    NDMachine *machine = thompson_union(lm, rm);
                    M.push(machine);
                }
                else if(isOps == 1){
                    NDMachine *m = M.top(); M.pop();
                    NDMachine *machine = thompson_kleene_closure(m);
                    M.push(machine);
                }
                else if(isOps == 2){
                    NDMachine *rm = M.top(); M.pop();
                    NDMachine *lm = M.top(); M.pop();
                    NDMachine *machine = thompson_concatenate(lm, rm);
                    M.push(machine);
                }
                else{
                    NDMachine *machine = token_to_machine(regex[i]);
                    M.push(machine);
                }
            }

            NDMachine *final_NFA = M.top(); M.pop();
            std::cout << "Number of states in NFA " << nd_state_id << std::endl;
            this->ndm = final_NFA;

        }

        /// @brief Construct DFA using NFA using subset construction method
        void construct_DFA(){
            //  state_subset |  a0 | a1 | a2 ..
            // {q0} -> {q1, q2} | {q1, q3} ...
            int sz = alphabet.size();
            std::map<std::set<NDNode*>, std::vector<std::set<NDNode*>>> dfa_table;
            std::queue<std::set<NDNode*>> Q;
            std::vector<NDNode*> start_closure = this->ndm->start->epsilon_closure(nullchar);
            std::set<NDNode*> start_state(start_closure.begin(), start_closure.end());
            std::set<std::set<NDNode*>> final_states;

            {
                bool isFinal = false;
                for(auto& state: start_state){
                    if(this->ndm->final_states.find(state) != this->ndm->final_states.end()){
                        isFinal = true;
                    }
                }

                if(isFinal){
                    final_states.insert(start_state);
                }
            }

            dfa_table[start_state] = std::vector<std::set<NDNode*>>(sz);
            Q.push(start_state);
            while(!Q.empty()){
                std::set<NDNode*> nd_state = Q.front(); Q.pop();
                // Find null-closure of current subset
                // Build subset for alphabet[i] transition from nd_state
                // set value of nd_state --alphabet[i]--> target
                // Check if target is a final state
                // Add newly discovered subset in dfa_table

                for(int i = 0; i<sz; i++){
                    std::set<NDNode*> total_next;
                    for(auto& state: nd_state){
                        if(state->next.count(alphabet[i]) > 0){
                            std::vector<NDNode*> trans = state->next[alphabet[i]];
                            for(auto& s: trans){
                                total_next.insert(s);
                            }
                        }
                    }

                    std::set<NDNode*> total_closure;
                    for(auto& state: total_next){
                        std::vector<NDNode*> closure = state->epsilon_closure(nullchar);
                        for(auto& e: closure){
                            total_closure.insert(e);
                        }
                    }

                    dfa_table[nd_state][i] = total_closure;

                    bool isFinal = false;
                    for(auto& state: total_closure){
                        if(this->ndm->final_states.find(state) != this->ndm->final_states.end()){
                            isFinal = true;
                        }
                    }

                    if(isFinal){
                        final_states.insert(total_closure);
                    }

                    if(dfa_table.count(total_closure) == 0){
                        dfa_table[total_closure] = std::vector<std::set<NDNode*>>(sz);
                        Q.push(total_closure);
                    }


                }

            }

            std::cout << "Number of states in DFA " << dfa_table.size() << std::endl;

            // Map (subset of NDNodes) to DNode
            std::map<std::set<NDNode*>, DNode*> dfa_states;
            for(auto& row: dfa_table){
                dfa_states[row.first] = new DNode(d_state_id++);
            }

            // set the transition for each alphabet
            for(auto& row: dfa_table){
                DNode* state = dfa_states[row.first];
                for(int i = 0; i<sz; i++){
                    state->next[alphabet[i]] = dfa_states[row.second[i]];
                }
            }

            // Construct DFA Machine 
            DMachine *machine = new DMachine();
            machine->alphabet = alphabet;
            machine->start = dfa_states[start_state];
            machine->end = nullptr;
            //fill final states
            for(auto& state: final_states){
                machine->final_states.insert(dfa_states[state]);
            }

            this->dm = machine;

        }

    public:
        FA(){

        }

        FA(const std::string &s, const std::string &alphabet, char nullchar){
            this->regex = s;
            this->alphabet = alphabet;
            this->nullchar = nullchar;
            this->nd_state_id = 0;
            this->d_state_id = 0;

            construct_NFA();
            construct_DFA();

        }

        void print_transition_table(){
            std::cout << "Transition table of DFA" << std::endl;
            this->dm->print_machine_table();
        }

        bool check(const std::string &s){
            return this->dm->accepted(s);
        }

        std::vector<int> trace_states(const std::string &s){
            return this->dm->trace_states(s);
        }

};

class FACompiler{
    private:
        std::string alphabet;
        char nullchar;

        bool check_bracket_balance(const std::string &s){
            int sz = s.size();
            std::string brackets = "()[]{}";
            std::stack<char> open;
            for(int i = 0; i<sz; i++){
                int isBracket = -1;
                for(int j = 0; j<6; j++){
                    if(s[i] == brackets[j]){
                        isBracket = j;
                        break;
                    }
                }

                if(isBracket>=0){
                    if(isBracket%2 == 0){
                        open.push(isBracket);
                    }
                    else{
                        if(!open.empty() && open.top()+1 == isBracket){
                            open.pop();
                        }
                        else{
                            return false;
                        }
                    }
                }
            }

            if(open.size() > 0){
                return true;
            }

            return true;

        }

        bool contains_alphabet(const std::string &s){
            std::string specials = "(){}[].+*";
            int sz = s.size();
            for(int i = 0; i<sz; i++){
                bool isSpecial = false;
                for(int j = 0; j<9; j++){
                    if(s[i] == specials[j]){
                        isSpecial = true;
                        break;
                    }
                }

                if(!isSpecial){
                    bool found = std::binary_search(this->alphabet.begin(), this->alphabet.end(), s[i]);
                    if(!found) return false;
                }
            }

            return true;
        }

        std::string infix_postfix(const std::string &s){
            int sz = s.size();
            std::string brackets = "()[]{}";
            std::string biops = "+.";
            std::string unops = "*";

            std::string ans;

            std::stack<char> operators;
            for(int i = 0; i<sz; i++){
                bool isBracket = false, isBiops = false, isUnops = false;
                int bidx = -1;
                for(int j = 0; j<6; j++){
                    if(s[i] == brackets[j]){
                        isBracket = true;
                        bidx = j;
                        break;
                    }
                }
                for(int j = 0; j<2; j++){
                    if(s[i] == biops[j]){
                        isBiops = true;
                        break;
                    }
                }
                for(int j = 0; j<1; j++){
                    if(s[i] == unops[j]){
                        isUnops = true;
                        break;
                    }
                }

                if(isBracket){
                    if(bidx%2 == 0){
                        operators.push(s[i]);
                    }
                    else{
                        while(!operators.empty()){
                            char t = operators.top(); operators.pop();
                            if(t == brackets[bidx-1]) break;
                            ans.push_back(t);
                        }
                    }
                }

                else if(isBiops){
                    operators.push(s[i]);
                }

                else if(isUnops){
                    ans.push_back(s[i]);
                }
                else{
                    ans.push_back(s[i]);
                }
            }

            while(!operators.empty()){
                char t = operators.top(); operators.pop();
                ans.push_back(t);
            }

            return ans;
        }
    
    public:
        FACompiler(const std::string &s){
            if(s.size() > 1){
                this->nullchar = s[0];
                this->alphabet = s.substr(1);
                std::sort(this->alphabet.begin(), this->alphabet.end());
                std::cout << "Null character is: " << this->nullchar << std::endl;
                std::cout << "Alphabet allowed: " << this->alphabet << std::endl;
            }
            else{
                std::string err = "FACompiler ctor accepts string of atleast 2, first character is null character\n";
                throw err;
            }
        }

        FA compile(const std::string &s){

            bool check_balance = check_bracket_balance(s);
            if(!check_balance){
                std::string err = "Bracket mis-match in regular expression provided";
                throw err;
            }

            bool check_alphabet = contains_alphabet(s);
            if(!check_alphabet){
                std::string err = "FACompiler does not have the alphabet provided in regular expression";
                throw err;
            }

            std::string postfix = infix_postfix(s);

            std::cout << "For input " << s << " postfix notation is: " << postfix << std::endl;

            return FA(postfix, this->alphabet, nullchar);
        }
};

int main(){
    std::cout << "First character in string of FACompiler constructor is nullcharacter" << std::endl;
    std::cout << "+ symbol denotes OR. a+b means either a or b." << std::endl;
    std::cout << "* symbol denotes Kleene-Closure. a* means 0 or more instances of a." << std::endl;
    std::cout << ". symbol denotes concatenation. a.b means b comes after a." << std::endl;
    std::cout << std::endl;

    try
    {    
        std::cout << "\nEnter FACompiler string (first charcter denotes null character, following substring is accepted alphabet)\nExample \"0ab\" denotes 0 as null, 'a' and 'b' as alphabets" << std::endl;
        std::string args;
        std::cin >> args;
        FACompiler fac(args);

        std::cout << "\nEnter regex (+ denotes union, * denotes Kleene-closure, . denotes concatenation)" << std::endl;
        std::string regex;
        std::cin >> regex;
        FA fa = fac.compile(regex);

        std::cout << "\nDFA Transition table for " << regex << std::endl;
        fa.print_transition_table();

        std::cout << "\nEnter number of test cases" << std::endl;
        int tc = 0;
        std::cin >> tc;
        while(tc--){
            std::cout << "\nEnter string to check against regex" << std::endl;
            std::string test;
            std::cin >> test;

            bool accepted = fa.check(test);
            if(accepted){
                std::cout << test << " accepted!" << std::endl;
                std::cout << "Trace of states" << std::endl;
                std::vector<int> trace = fa.trace_states(test);
                int sz = trace.size();
                for(int i = 0; i<sz; i++){
                    if(i >= test.size()){
                        std::cout << ":" << trace[i] << " ";
                    }
                    else{
                        std::cout << test[i] << ":" << trace[i] << " ";
                    }
                }
                std::cout << std::endl;
            }
            else{
                std::cout << test << " failed!" << std::endl;
            }
        }


    }
    catch(const std::string& e)
    {
        std::cerr << e << '\n';
    }
    return 0;
}
