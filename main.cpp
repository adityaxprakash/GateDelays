#include <bits/stdc++.h>
#include <sstream>
#define all(x) (x).begin(),(x).end()
using namespace std;

//AND->AND_1:delay, AND_2:delay,...
map<string,vector<pair<double,string>>> gates_delay;
map<string,double> gates_area;
ifstream inFile;
set<string> primary_inputs;
set<string> primary_outputs;
int total_signals=0;
double needed_delay=0;
vector<vector<int>> all_sequence;

class Gate_Node
{
    public:
    double val = 0;
    string type="INP";
    Gate_Node *prev1;
    Gate_Node *prev2;
    int active_impl=0;
    int index;
    vector<Gate_Node*> path;

    Gate_Node(double v, Gate_Node *prv1 = nullptr, Gate_Node *prv2 = nullptr)
    {
        val = v;
        prev1 = prv1;
        prev2 = prv2;
    }

    void set_active(int acti)
    {
        active_impl=acti;
    }
};

bool perform(vector<pair<int,Gate_Node*>> &freq_of_all);


map<string,Gate_Node*> signals_to_pointers;
map<int,Gate_Node*> index_to_signal;


vector<pair<int,Gate_Node*>> getFrequency(vector<Gate_Node*> inp)
{
    vector<pair<int,Gate_Node*>>toreturn;
    map<Gate_Node*,int> freq;
    for(auto i:signals_to_pointers)
    {
        freq[i.second]=0;
    }
    for(auto i:inp)
    {
        for(auto j:i->path)
        {
            freq[j]++;
        }
    }
    for(auto i:freq)
    {
        toreturn.push_back({i.second,i.first});
    }
    sort(toreturn.rbegin(),toreturn.rend());
    return toreturn;
}

double get_delay(Gate_Node* Signal){
    double valOfGate;

    if(Signal->type=="INP" || Signal->type=="DFF")
    {
        valOfGate=0;
    }
    else
    {
        valOfGate=gates_delay[Signal->type][Signal->active_impl].first;
    }   
    if(Signal->prev1 == nullptr && Signal->prev2 == nullptr )
    {        
    }
    else if(Signal->prev1 == nullptr)
    {
        Signal->val = valOfGate+ get_delay(Signal->prev2);
    }
    else if(Signal->prev2 == nullptr){
        Signal->val = valOfGate + get_delay(Signal->prev1);
    }
    else
    {
        Signal->val = valOfGate + max(get_delay(Signal->prev1),get_delay(Signal->prev2));                             
    }
    return Signal->val;
}

vector<pair<Gate_Node*,double>> getAllDelays()
{
    vector<pair<Gate_Node*,double>> toreturn;
    for(auto i:primary_outputs)
    {
        double d=get_delay(signals_to_pointers[i]);
        toreturn.push_back({signals_to_pointers[i],d});
    }
    return toreturn;
}

void setOutputNodePath(Gate_Node* n)
{
    if(n==nullptr)
        return;
    queue<Gate_Node*> q;
    q.push(n);
    while(!q.empty())
    {
        n->path.push_back(q.front());
        Gate_Node* k=q.front();
        q.pop();
        if(k->prev1!=nullptr)
        {
            q.push(k->prev1);
        }
        if(k->prev2!=nullptr)
        {
            q.push(k->prev2);
        }
    }
}

double calculateCurrArea()
{
    double area=0;
    for(auto i:signals_to_pointers)
    {
        double area_i;
        if(i.second->type=="INP" || i.second->type=="DFF")
            area_i=0;
        else
            area_i=gates_area[gates_delay[i.second->type][i.second->active_impl].second];
        area+=area_i;
    }
    return area;
}

double getdeltaT(Gate_Node* g)
{
    if(g->active_impl==0 || (g->type=="INP" || g->type=="DFF"))
        return 0; 
    double t1=gates_delay[g->type][g->active_impl].first;
    double t2=gates_delay[g->type][(g->active_impl)-1].first;
    return (t1-t2);
}

double getdeltaA(Gate_Node* g)
{
    if(g->active_impl==0 || (g->type=="INP" || g->type=="DFF"))
        return 0; 
    double t1=gates_area[gates_delay[g->type][g->active_impl].second];
    double t2=gates_area[gates_delay[g->type][(g->active_impl)-1].second];
    return (t2-t1);
}

bool MultiSame(vector<pair<int,Gate_Node*>> &freq_of_all)
{
    int max_freq=freq_of_all[0].first;
    vector<Gate_Node*> gates_with_max_effect;
    for(int i=0;i<freq_of_all.size();i++)
    {
        if(freq_of_all[i].first==max_freq)
        {
            gates_with_max_effect.push_back(freq_of_all[i].second);
        }
    }
    map<Gate_Node*, double> priority;
    double mx=DBL_MIN;
    for(auto i:gates_with_max_effect)
    {
        double a=getdeltaA(i);
        double t=getdeltaT(i);
        if(a==0)
        {
            priority[i]=DBL_MIN;
        }
        else
        {
            priority[i]=t;
        }
        mx=max(mx,priority[i]);
    }
    if(mx==DBL_MIN)
    {
        while(count(all(gates_with_max_effect), freq_of_all[0].second)==1)
        {
            freq_of_all.erase(freq_of_all.begin());
        }
        return perform(freq_of_all);
    }
    else
    {
        for(auto i:priority)
        {
            if(i.second==mx)
            {
                i.first->set_active((i.first->active_impl)-1);
                return true; 
            }
        }
        return false;
    }
}

bool singleSame(vector<pair<int,Gate_Node*>> &freq_of_all)
{
    Gate_Node* to_change=freq_of_all[0].second;
    if(to_change->active_impl==0)
    {
        freq_of_all.erase(freq_of_all.begin());
        return perform(freq_of_all);
    }
    else
    {
        to_change->set_active(to_change->active_impl-1);
        return true;
    }
}

bool perform(vector<pair<int,Gate_Node*>> &freq_of_all)
{
    if(freq_of_all.size()==0)
    {
        return false;  
    }
    if(freq_of_all.size()==1)
    {
        return singleSame(freq_of_all);
    }
    if(freq_of_all[0].first==freq_of_all[1].first)
    {
        return MultiSame(freq_of_all);
    }
    else
    {
        return singleSame(freq_of_all);
    }
}

double efficientArea()
{
    vector<Gate_Node*> overshooting_signals;
    for(auto i:primary_outputs)
    {
        double d=get_delay(signals_to_pointers[i]);
        if(d>needed_delay)
        {
            overshooting_signals.push_back(signals_to_pointers[i]);
        }
    }
    if(overshooting_signals.size()==0)
    {
        return calculateCurrArea();
    }
    vector<pair<int,Gate_Node*>> freq_of_all=getFrequency(overshooting_signals);
    if(perform(freq_of_all))
        return efficientArea();
    else
    {
        return -1;
    }
}

void generateSequences(int n, vector<int>& sequence) 
{
    if (n == 0) 
    {
        all_sequence.push_back(sequence);
        return;
    }
    for (int i = 0; i <= 2; i++) {
        sequence.push_back(i);
        generateSequences(n - 1, sequence);
        sequence.pop_back();
    }
}


double bruteArea()
{
    double area=DBL_MAX;
    vector<int> sequence;
    generateSequences(total_signals-primary_inputs.size(),sequence);

    for(auto i:all_sequence)
    {
        int k=0;
        for(int j=0;j<total_signals;j++)
        {
            if(index_to_signal[j]->type!="INP" && index_to_signal[j]->type!="DFF")
                index_to_signal[j]->set_active(i[k++]);
        }
        bool check=false;
        for(auto i:primary_outputs)
        {
            if(needed_delay<get_delay(signals_to_pointers[i]))
                check=true;
        }
        if(check)
            continue;
        area=min(area,calculateCurrArea());
    }
    return area;
}

double minArea()
{
    if(total_signals-primary_inputs.size()<=10)
    {
        return bruteArea();
    }
    else
    {
        return efficientArea();
    }
}

//Input of gate delays
void input_gatedelays(string file)
{ 
    inFile.open("gate_delays.txt");
    if (!inFile) {
        cout << "File error" << endl;
        return;
    }
    string gateImpl; double gateDelay; string gateType; double gateArea;
    string line;
    while(getline(inFile,line))
    {
        if(line.empty() || all_of(line.begin(), line.end(), [](char chr) {return isspace(chr);})) 
            continue;
        if (line.empty() || line.find("//") == 0 || line.find_first_not_of(" ") == string::npos) 
        {
            continue;
        }
        stringstream ss(line);
        ss>>gateImpl>>gateType>>gateDelay>>gateArea;
        gates_delay[gateType].push_back({gateDelay,gateImpl});
        gates_area[gateImpl]=gateArea;
        sort(gates_delay[gateType].begin(),gates_delay[gateType].end());
    }
    inFile.close(); 
}

//Circuit Input
void input_circuit(string file)
{  
    int gates_num=0;
    inFile.open("circuit.txt");
    if (!inFile) 
    {
        cout << "File error" << endl;
        return ;
    }

    string line;
    while(getline(inFile,line))
    {
        if(line.empty() || all_of(line.begin(), line.end(), [](char chr) {return isspace(chr);})) 
            continue;
        if (line.empty() || line.find("//") == 0 || line.find_first_not_of(" ") == string::npos) 
        {
            continue;
        }
        stringstream ss(line);
        string type;
        ss>>type;

        if(type=="PRIMARY_INPUTS")
        {
            string node;
            while(ss>>node)
            {
                primary_inputs.insert(node);
                Gate_Node *n=new Gate_Node(0);
                signals_to_pointers[node]=n;
                n->index=gates_num++;
                index_to_signal[gates_num-1]=n;
            }
        }
        
        else if(type=="PRIMARY_OUTPUTS")
        {
        
            string node;
            while(ss>>node)
            {
                primary_outputs.insert(node);
                Gate_Node *n=new Gate_Node(0);
                signals_to_pointers[node]=n;
                n->index=gates_num++;
                index_to_signal[gates_num-1]=n;
            }
        }

        else if(type=="INTERNAL_SIGNALS")
        {
            string node;
            while(ss>>node)
            {
                Gate_Node *n=new Gate_Node(0);
                signals_to_pointers[node]=n;
                n->index=gates_num++;
                index_to_signal[gates_num-1]=n;
            }
        }

        else if(type=="INV")
        {
            string in,out;
            ss>>in>>out;
            signals_to_pointers[out]->prev1=signals_to_pointers[in];
            signals_to_pointers[out]->val=gates_delay[type][0].first;
            signals_to_pointers[out]->type=type;
        }
        
        else if(type=="DFF")
        {
            string in,out;
            ss>>in>>out;
            primary_inputs.insert(out);
            primary_outputs.insert(in);
            signals_to_pointers[out]->type=type;
        }

        else if(type=="AND2"||type=="NAND2"||type=="NOR2"||type=="OR2")
        {
            string in1,in2,out;
            ss>>in1>>in2>>out;
            signals_to_pointers[out]->prev1=signals_to_pointers[in1];
            signals_to_pointers[out]->prev2=signals_to_pointers[in2];
            signals_to_pointers[out]->val=gates_delay[type][0].first;
            signals_to_pointers[out]->type=type;
        }
    }
    total_signals=signals_to_pointers.size();
    inFile.close();
}

//Input the required delay
double input_maximumdelay(string file)
{
    inFile.open("delay_constraint.txt");
    if (!inFile) {
        cout << "File error" << endl;
        return -1;
    }
    double max_delay;
    string line;
    while(getline(inFile,line))
    {
        if(line.empty() || all_of(line.begin(), line.end(), [](char chr) {return isspace(chr);})) 
            continue;
        if (line.empty() || line.find("//") == 0 || line.find_first_not_of(" ") == string::npos) {
           continue;
       }
       stringstream ss(line);
       ss>>max_delay;
    }
    return max_delay;
}

int main(int nargs, char *args[])
{
    ofstream outfile;

    string choice=args[1];
    if(choice=="A")
    {
        outfile.open(args[4]);
        streambuf *cout_buffer = cout.rdbuf();
        cout.rdbuf(outfile.rdbuf());
        input_gatedelays(args[3]);
        input_circuit(args[2]);
        double ans=-1;
        for(auto i:primary_outputs)
        {
            ans=max(ans,get_delay(signals_to_pointers[i]));
        }
        cout<<ans<<"\n";
        outfile.close();
        cout.rdbuf(cout_buffer);
    }
    else if(choice=="B")
    {
        outfile.open(args[5]);
        streambuf *cout_buffer = cout.rdbuf();
        cout.rdbuf(outfile.rdbuf());
        input_gatedelays(args[3]);
        input_circuit(args[2]);
        needed_delay=input_maximumdelay(args[4]);
        for(auto i:signals_to_pointers)
        {
            i.second->active_impl=2;
        }

        for(auto i:primary_outputs)
        {
            setOutputNodePath(signals_to_pointers[i]);
        }
        double outp=0;
        outp=minArea();
        if(outp==-1)
            cout<<"NOT possible\n";
        else
            cout<<outp<<endl;
        outfile.close();
        cout.rdbuf(cout_buffer);
    }
    else
        cout<<"Wrong choice";

    for(auto i:signals_to_pointers)
        delete i.second;
}