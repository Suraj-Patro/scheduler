#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <limits>
#include <algorithm>

using namespace std;

const int CREATED = 0;
const int TRANS_TO_READY=1;
const int TRANS_TO_RUN= 2;
const int TRANS_TO_BLOCK = 3;
const int TRANS_TO_PREEMPT = 4;
const int DONE = 5;

static int stat_id =0;

// Classes

class Process {
	std::vector<int> at_tc_cb_io;
	//int state; // 0-created, 1-ready, 2-running, 3-blocked
public:
	const static int AT = 0;
	const static int TC = 1;
	const static int CB = 2;
	const static int IO = 3;
	int PRIO;
	int dynamic_prio;
	int start_time;
	int FT;
	int TT;
	int IT;
	int CW;
	int cpu_time;
	int ready_start;
	int pid;
	int cpu_burst_rem;
	
	void init(int id) {
		for (int i =0;i<4;i++) {
			at_tc_cb_io.push_back(-1);
		}
		cpu_time =0;
		start_time = -1;
		pid = id;
		IT = 0;
		CW = 0;
		cpu_burst_rem = -1;
		dynamic_prio = PRIO;

	}
	bool de_prio() {
		if (dynamic_prio == 0) {
			dynamic_prio = PRIO;
			return false;
		}
		else {
			dynamic_prio--;
			return true;
		}
	}
	std::vector<int> getALL() {
		return at_tc_cb_io;
	}
	int getTT() {
		return FT - getAT();
	}
	int getAT() {
		return at_tc_cb_io[AT];
	}
	void setAT(int at) {
		at_tc_cb_io[AT] = at;
	}
	int getTC() {
		return at_tc_cb_io[TC];
	}
	void setTC(int tc) {
		at_tc_cb_io[TC] = tc;
	}
	int getCB() {
		return at_tc_cb_io[CB];
	}
	void setCB(int cb) {
		at_tc_cb_io[CB] = cb;
	}
	int getIO() {
		return at_tc_cb_io[IO];
	}
	void setIO(int io) {
		at_tc_cb_io[IO] = io;
	}
};


class Event {
public:
	int timestamp;
	int pid;
	int transition;
	int duration;
	int created_time;
	string last_state;

	void init(int t, int p, int tr) {
		timestamp = t;
		pid = p;
		transition = tr;
	}
};



class DES_Layer {
	std::vector<Event> eventQ;
public:
	Event get_event(){
		if (eventQ.empty()) {
			Event e;
			e.init(-1,-1,-1);
			return e;
		}
		Event e = eventQ[0];
		eventQ.erase(eventQ.begin());
		return e;
	}
	void put_event(Event e) {
		int i =0;
		while (i < eventQ.size() && e.timestamp >= eventQ[i].timestamp) {
			i++;
		}
		eventQ.insert(eventQ.begin()+i,e);
	}
	int get_next_time() {
		if (eventQ.empty()) return -1;
		return eventQ.front().timestamp;
	}
	void print_list() {
		for (int i=0; i< eventQ.size();i++) {
			cout << "timestamp: "<<eventQ[i].timestamp << " pid: "<<eventQ[i].pid << endl;
		}
	}
};


class Scheduler {
protected:
	std::vector<Process *> runqueue;
public:
	virtual void add_process(Process * p, bool quant) = 0;
	virtual Process * get_next_process() = 0;
	virtual void print_runq() = 0;
};

class FCFSScheduler : public Scheduler {
public:
	Process * get_next_process() {
		Process *p;
		if(!runqueue.empty()) {
			p = runqueue.front();
			runqueue.erase(runqueue.begin());
		} else return nullptr;
		return p;
	}
	void add_process(Process *p,bool quant) {
		runqueue.push_back(p);
	}

	void print_runq() {
		cout << "runqueue: ";
		for (int i=0; i<runqueue.size();i++) {
			cout << "pid: " <<runqueue[i]->pid <<endl;
		}
		if (runqueue.empty()) cout <<"EMPTY" <<endl;
	}
};

class LCFSScheduler : public Scheduler {
public:
	Process * get_next_process() {
		Process *p;
		if(!runqueue.empty()) {
			p = runqueue.back();
			runqueue.pop_back();
		} else return nullptr;
		return p;
	}
	void add_process(Process *p,bool quant) {
		runqueue.push_back(p);
	}
	void print_runq() {
		cout << "runqueue: ";
		for (int i=0; i<runqueue.size();i++) {
			cout << "pid: " <<runqueue[i]->pid <<endl;
		}
		if (runqueue.empty()) cout <<"EMPTY" <<endl;
	}
};

class SJFScheduler : public Scheduler {
public:
	Process * get_next_process() {
		Process *p;
		if(runqueue.empty()) {
			return nullptr;
		}
		int min = runqueue[0]->getTC() - runqueue[0]->cpu_time;
		int shortest = 0;
		for (int i =1; i<runqueue.size();i++) {
			if (min> runqueue[i]->getTC() - runqueue[i]->cpu_time) {
				min = runqueue[i]->getTC()-runqueue[i]->cpu_time;
				shortest = i;
			}
		}
		p = runqueue[shortest];
		runqueue.erase(runqueue.begin()+shortest);
		return p;
	}
	void add_process(Process *p,bool quant) {
		runqueue.push_back(p);
	}
	void print_runq() {
		cout << "runqueue: ";
		for (int i=0; i<runqueue.size();i++) {
			cout << "pid: " <<runqueue[i]->pid <<endl;
		}
		if (runqueue.empty()) cout <<"EMPTY" <<endl;
	}
};

class RoundRobin : public Scheduler {
public:
	Process * get_next_process() {
		Process *p;
		if(!runqueue.empty()) {
			p = runqueue.front();
			runqueue.erase(runqueue.begin());
		} else return nullptr;
		return p;
	}
	void add_process(Process *p, bool quant) {
		runqueue.push_back(p);
	}
	void print_runq() {
		cout << "runqueue: ";
		for (int i=0; i<runqueue.size();i++) {
			cout << "pid: " <<runqueue[i]->pid <<endl;
		}
		if (runqueue.empty()) cout <<"EMPTY" <<endl;
	}
};

class PriorityScheduler : public Scheduler {
	std::vector<Process *> activeQ_0;
	std::vector<Process *> activeQ_1;
	std::vector<Process *> activeQ_2;
	std::vector<Process *> activeQ_3;

	std::vector<Process *> expiredQ_0;
	std::vector<Process *> expiredQ_1;
	std::vector<Process *> expiredQ_2;
	std::vector<Process *> expiredQ_3;
public:
	bool all_empty() {
		return (activeQ_0.empty() && activeQ_1.empty() && activeQ_2.empty() && activeQ_3.empty() &&
			expiredQ_3.empty() && expiredQ_2.empty() && expiredQ_1.empty() && expiredQ_0.empty());
	}
	Process * get_next_process() {
		if (all_empty()) return nullptr;
		if (activeQ_0.empty() && activeQ_1.empty() && activeQ_2.empty() && activeQ_3.empty()) {
			activeQ_0.swap(expiredQ_0);
			activeQ_1.swap(expiredQ_1);
			activeQ_2.swap(expiredQ_2);
			activeQ_3.swap(expiredQ_3);
		}
		Process *p;
		if (!activeQ_3.empty()) {
			p = activeQ_3.front();
			activeQ_3.erase(activeQ_3.begin());
		}
		else if(!activeQ_2.empty()) {
			p = activeQ_2.front();
			activeQ_2.erase(activeQ_2.begin());
		}
		else if(!activeQ_1.empty()) {
			p = activeQ_1.front();
			activeQ_1.erase(activeQ_1.begin());
		}
		else if (!activeQ_0.empty()) {
			p = activeQ_0.front();
			activeQ_0.erase(activeQ_0.begin());
		}
		else cout << "problem !! -------"<<endl;
		return p;
	}
	void add_process(Process *p, bool quant) {
		int pri = p->dynamic_prio;
		if (quant) {
			if (pri == 0) { //expired queue
				pri = p->PRIO;
				p->dynamic_prio = pri;
				if (pri == 3) {
					expiredQ_3.push_back(p);
				}
				else if (pri==2) {
					expiredQ_2.push_back(p);
				}
				else if (pri==1) {
					expiredQ_1.push_back(p);
				}
				else if (pri==0) {
					expiredQ_0.push_back(p);
				}	
				return;
			}
			else {	//active queue lower prio
				pri--;
				p->dynamic_prio = pri;
			}
		}
		if (pri == 3) {
			activeQ_3.push_back(p);
		}
		else if (pri==2) {
			activeQ_2.push_back(p);
		}
		else if (pri==1) {
			activeQ_1.push_back(p);
		}
		else if (pri==0) {
			activeQ_0.push_back(p);
		}
	}
	void print_runq() {
		cout << "active ~~~~~~~" << endl;
		cout << "PRIO 3 -" << endl;
		for (int i=0;i<activeQ_3.size();i++) {
			cout << "<pid " <<activeQ_3[i]->pid << ">"<< endl;
		}
		cout << "PRIO 2 -" <<endl;
		for (int i=0;i<activeQ_2.size();i++) {
			cout << "<pid " <<activeQ_2[i]->pid << ">" << endl;
		}
		cout << "Prio 1 -" <<endl;
		for (int i=0;i<activeQ_1.size();i++) {
			cout << "<pid " <<activeQ_1[i]->pid << ">" << endl;
		}
		cout << "Prio 0 -" <<endl;
		for (int i=0;i<activeQ_0.size();i++) {
			cout << "<pid " <<activeQ_0[i]->pid << ">" << endl;
		}
		cout << "expired ~~~~~~~~" << endl;
		cout << "PRIO 3 -" << endl;
		for (int i=0;i<expiredQ_3.size();i++) {
			cout << "<pid " <<expiredQ_3[i]->pid << ">" << endl;
		}
		cout << "PRIO 2 -" <<endl;
		for (int i=0;i<expiredQ_2.size();i++) {
			cout << "<pid " <<expiredQ_2[i]->pid << ">" << endl;
		}
		cout << "Prio 1 -" <<endl;
		for (int i=0;i<expiredQ_1.size();i++) {
			cout << "<pid " <<expiredQ_1[i]->pid << ">" << endl;
		}
		cout << "Prio 0 -" <<endl;
		for (int i=0;i<expiredQ_0.size();i++) {
			cout << "<pid " <<expiredQ_0[i]->pid << ">" << endl;
		}
	}
};


// Constants & Globals ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



static ifstream file;
static string randoms_file;
static int randoms_counter;
static std::vector<Process> processes; 
std::vector<int> randoms;




// Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void read_randoms(string rand_file) {
	file.open(rand_file);
	string w;
	randoms_counter = 0;
	while(file>>w) {
		randoms.push_back(std::stoi(w));
	}
	file.close();
}

int myrandom(int burst) {
	if (randoms_counter == randoms.size()) randoms_counter = 0;
	return 1 + (randoms[++randoms_counter] % burst);
}


vector<Process> read_file(string filename) {

	file.open(filename);
	string word;
	int i =0;

	while(file>>word) {
		//Init process
		Process p;

		p.PRIO = (myrandom(4));
		p.PRIO--;

		p.init(i++);

		//Arrival Time
		p.setAT(std::stoi(word));

		//Total CPU Time
		if (!(file>>word)) {
			cout << "problem 1" <<endl;;
			exit(1);
		}
		p.setTC(std::stoi(word));

		//CPU Burst
		if (!(file>>word)) {
			cout << "problem 2" <<endl;;
			exit(1);
		}
		p.setCB(std::stoi(word));

		//IO Burst
		if (!(file>>word)) {
			cout << "problem 3" <<endl;;
			exit(1);
		}
		p.setIO(std::stoi(word));
		
		//Add process to processes vector
		processes.push_back(p);
	}

	file.close();

	return processes;
}



int main(int argc, char* argv[]) {
	//Get Commandline arguments
	Scheduler *scheduler;
	int quantum;
	string type_sched;

	bool verbose = false;
	int i = 1;
	if ('v' == argv[i][1]) {
		verbose = true;
		i++;
	}

	if (argv[i][2]=='F') {
		scheduler = new FCFSScheduler();
		quantum = 100000;
		type_sched = "FCFS";
	}
	else if (argv[i][2] == 'L') {
		scheduler = new LCFSScheduler();
		quantum = 100000;
		type_sched = "LCFS";
	}
	else if (argv[i][2] == 'S') {
		scheduler = new SJFScheduler();
		quantum = 100000;
		type_sched = "SJF";
	}
	else if (argv[i][2]=='R') {
		// init RR scheduler
		scheduler = new RoundRobin();
		int l = strlen(argv[i]);
		string q;
		for (int j=3;j<l; j++) {
			q += argv[i][j];
		}
		quantum = std::stoi(q);
		type_sched = "RR "+q;
	}
	else if (argv[i][2]=='P') {
		// init PRIO scheduler
		scheduler = new PriorityScheduler();
		int l = strlen(argv[i]);
		string q;
		for (int j=3;j<l; j++) {
			q += argv[i][j];
		}
		quantum = std::stoi(q);
		type_sched = "PRIO "+q;
	}

	i++;
	string filename;
	for (int j=0; j<strlen(argv[i]); j++) {
		filename += argv[i][j];
	}
	i++;
	string rand_file;
	for (int j=0; j<strlen(argv[i]); j++) {
		rand_file += argv[i][j];
	}


	/*
	cout << "verbose " << verbose <<endl;
	cout << "input " << filename<<endl;
	cout << "file " <<rand_file<<endl; 
	cout << "quantum " << quantum<<endl;
	*/


	// Init randoms
	randoms_counter = 0;
	read_randoms(rand_file);

	// Parse input file
	read_file(filename);


	// Initialzie DES Layer
	DES_Layer des;
	for (int i=0; i<processes.size(); i++){
		Event e;
		e.init(processes[i].getAT(),i,TRANS_TO_READY);
		e.last_state = "CREATED";
		e.created_time = processes[i].getAT();
		e.duration = 0;
		des.put_event(e);
	}


	
	//des.print_list();

	//Initialize data for simulation
	Process *process_running = nullptr;
	bool call_sched;
	call_sched = false;
	int last_event_finish;
	int cur_time;
	int burst;
	Event evt;

	int total_io =0;
	int io_end;
	Process *process_blocked = nullptr;


	//Begin simulation
	for (evt = des.get_event();evt.timestamp != -1; evt = des.get_event()) {
		cur_time = evt.timestamp;
		int i = evt.pid;

/*		
		if (cur_time > 45 && cur_time < 55) {
			scheduler->print_runq();
			cout << "static:" <<processes[i].PRIO << " dynamic:" << processes[i].dynamic_prio<<endl;
		}
		*/
		if (verbose)
			cout << cur_time << " " << i ;

		// -> READY ----------------------------------------------------
		if (evt.transition == TRANS_TO_READY) {
			if (verbose) cout << " "<<(cur_time-evt.created_time) << ": " <<evt.last_state<<" -> "<<"READY"<<endl;

			if (process_blocked == &processes[i]) {
				process_blocked = nullptr;
			}

			processes[i].ready_start = cur_time;
			scheduler->add_process(&processes[i],false);
			call_sched = true;

		}
		
		// READY -> RUN ------------------------------------------------
		else if(evt.transition == TRANS_TO_RUN) {
			if (verbose) cout<<" "<<(cur_time-processes[i].ready_start)<<": "<< evt.last_state <<" -> "<<"RUNNG";
			process_running = &processes[i];
			// init burst
			if (processes[i].cpu_burst_rem > 0) {
				burst = processes[i].cpu_burst_rem;
			}
			else {
				burst= myrandom(processes[i].getCB());
			}

			if(processes[i].start_time == -1) {
				processes[i].start_time = cur_time;
			}

			if (burst >= processes[i].getTC() - processes[i].cpu_time) {
				burst = processes[i].getTC() - processes[i].cpu_time;
			}

			if (verbose) cout <<" cb="<<burst<<" rem="<<(processes[i].getTC()-processes[i].cpu_time)
					<< " prio="<< processes[i].dynamic_prio << endl;

			processes[i].CW += cur_time - processes[i].ready_start;

			//check for preempt
			if (burst > quantum) {
				processes[i].cpu_time +=quantum;
				processes[i].cpu_burst_rem = burst - quantum;
				Event e;
				e.duration = quantum;
				e.created_time = cur_time;
				e.init(cur_time+quantum,i,TRANS_TO_PREEMPT);
				e.last_state = "RUNNG";
				des.put_event(e);
			}

			else if (burst+processes[i].cpu_time >= processes[i].getTC()) {
				processes[i].cpu_time += burst;
				//DONE event
				Event e;
				e.init(cur_time+burst,i,DONE);
				e.duration = burst;
				e.created_time = cur_time;
				e.last_state = "RUNNG";
				des.put_event(e);
			}
			else if (burst+processes[i].cpu_time < processes[i].getTC()) {
				// Make Block event
				processes[i].cpu_burst_rem = -1;
				processes[i].cpu_time += burst;
				Event e;
				e.init(cur_time+burst,i,TRANS_TO_BLOCK);
				e.created_time = cur_time;
				e.duration = burst;
				e.last_state = "RUNNG";
				des.put_event(e);
			}
		}

		//RUN -> Block ------------------------------------------------------
		else if(evt.transition == TRANS_TO_BLOCK) {
			if (verbose) cout << " "<<(cur_time-evt.created_time) << ": " << evt.last_state <<" -> "<<"BLOCK";

			if (process_running != &processes[i]) {cout<<"problem"<< endl;} 
			process_running = nullptr;
			int ib;
			ib = myrandom(processes[i].getIO());
			processes[i].IT += ib;

			if (verbose) cout << " ib="<<ib<<" rem="<< (processes[i].getTC()-processes[i].cpu_time) << endl;

			//checks?
			Event e;
			e.init(cur_time+ib,i,TRANS_TO_READY);
			e.created_time = cur_time;
			e.duration = ib;
			e.last_state = "BLOCK";
			des.put_event(e);

			//not sure about this, lets see
			processes[i].dynamic_prio = processes[i].PRIO;

			if (process_blocked == nullptr) {
				total_io += ib;
				io_end = ib + cur_time;
				process_blocked = &processes[i];
			}
			else {
				if (cur_time + ib > io_end) {
					total_io += ib +cur_time - io_end;
					io_end = ib + cur_time;
					process_blocked = &processes[i];
				}
			}

			call_sched = true;

		}

		// RUN -> PREEMPT ----------------------------------------------------
		else if(evt.transition == TRANS_TO_PREEMPT) {
			//figure out
			if (verbose) cout << " "<<(cur_time-evt.created_time) << ": " << evt.last_state <<" -> "<<"READY";
			if (verbose) cout << " cb="<<processes[i].cpu_burst_rem<<" rem="
				<<processes[i].getTC()-processes[i].cpu_time<< " prio="<<processes[i].dynamic_prio<<endl;
			process_running = nullptr;
			scheduler->add_process(&processes[i],true);
			processes[i].ready_start = cur_time;
			call_sched = true;
		}

		// DONE!!! ------------------------------------------------------------
		else if(evt.transition == DONE) {
			if (verbose) cout << " "<<(cur_time-evt.created_time) << ": " << evt.last_state <<" -> "<<"DONE" <<endl;
			processes[i].FT = cur_time;
			process_running = nullptr;
			last_event_finish =cur_time;
			call_sched = true;
			//continue; //Not sure about this???
		}

		if (call_sched) {
			if (des.get_next_time() == cur_time) continue;
			call_sched = false;
			if (process_running == nullptr) {
				process_running = scheduler->get_next_process();
				if (process_running == nullptr) {
					continue;
				}
				Event e;
				e.init(cur_time,process_running->pid,TRANS_TO_RUN);
				e.created_time = cur_time;
				e.last_state = "READY";
				e.duration = 0;
				des.put_event(e);
				//scheduler->print_runq();
				process_running = nullptr;
			}
		}
	}


	// Done with simulation printing time
	//cout <<endl;
	int total_cpu =0;
	//int total_io = 0;
	int total_turnaround=0;
	int total_cpu_waiting_time =0;

	cout << type_sched << endl;
	for (int i =0; i < processes.size(); i++) {
		total_cpu+= processes[i].getTC();
		//total_io+= processes[i].IT;
		total_turnaround +=processes[i].getTT();
		total_cpu_waiting_time += processes[i].CW;

		printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
		    i,
		    processes[i].getAT(), //
		    processes[i].getTC(),
			processes[i].getCB(),
			processes[i].getIO(),
			processes[i].PRIO+1,processes[i].FT,
			processes[i].getTT(),
			processes[i].IT,
			processes[i].CW);
	}
	double cpu_util = (total_cpu/(double)last_event_finish);
	double io_util = total_io/((double)last_event_finish);

	printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",
		last_event_finish, // last event finish
		(100.0 *cpu_util), // percent cpu utilization
		(100.0*io_util), // percent io wait
		(total_turnaround/(double)processes.size()), // average turnaround
		(total_cpu_waiting_time/(double)processes.size()), //average waiting time
		(processes.size()*100/(double)last_event_finish)); // average something

	return 0;

}



