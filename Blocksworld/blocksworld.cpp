#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <map>

using namespace std;

class Node;
class MyQueue;
class State;

int max_frontier_size = 0;
int goal_tested = 0;
int max_depth = 0;
int total_iteration = 0;
int num_blocks;
int num_stacks;
map<int, int> database;

class State
{
public:
	vector< vector<int> > state;
	
	State(vector< vector<int> > state1)
	{
		state = state1;
	}
	
	vector< vector<int> > create_new_state(int from, int to)
	{
		vector< vector<int> > new_state = state;
		new_state[to].push_back(new_state[from].back());
		new_state[from].pop_back();
		return new_state;
	}
	
	int num_block_in_stack(int stack_num)
	{
		return state[stack_num].size();
	}
};

void display(State* state_to_display)
{
	cout << endl;
	for(unsigned int i = 0; i < state_to_display->state.size(); i++)
	{
		cout << i << ":  ||";
		for(unsigned int j = 0; j < state_to_display->state[i].size(); j++)
		{
			cout << state_to_display->state[i][j] << " ";
		}
			cout << endl;
	}
	cout << endl;
}

int calculate_heuristic_value(State* state_to_calcuate)
{
	int num_blocks_finished = 0;
	bool check_finished_complete = false;
	int ret = 0;
	for(unsigned int i = 0; i < state_to_calcuate->state.size(); i++)
	{
		int num_block_in_this_stack = state_to_calcuate->state[i].size();
		for(int j = 0; j < num_block_in_this_stack; j++)
		{
			if(i == 0)
			{
				if(num_block_in_this_stack != 0)
				{	
					if((state_to_calcuate->state[i][j] == (signed) j+1) && !check_finished_complete)	num_blocks_finished ++;
					else
					{
						check_finished_complete = false;
						ret += (num_blocks + 1) * 2;
					}
				}
			}
			else
			{
				ret += (num_blocks + 1 - state_to_calcuate->state[i][j]) * (num_block_in_this_stack - j - 1);
			}
		}
	}
	
	return (ret + num_blocks - num_blocks_finished);
}

class Node
{
public:
	State* current_state;
	Node* parent;
	int depth;
	
	Node(State* state1)
	{
		current_state = state1;
		depth = 0;
		parent = NULL;
	}
	
	Node(State* state1, Node* p)
	{
		current_state = state1;
		parent = p;
		depth = parent->depth + 1;
	}
	
	vector<Node*> succesors()
	{
		vector<Node*> ret;

		for(unsigned char i = 0; i < current_state->state.size(); i++)
		{
			if(i == 0 && (current_state->state[i].size() != 0))
				if(current_state->state[i].back() == ((signed) current_state->state[i].size()))	continue;
			if(current_state->num_block_in_stack(i) != 0)
			{
				for(unsigned char j = 0; j < current_state->state.size(); j++)
				{
					if(j == i)
						continue;
					else{
						ret.push_back(new Node(new State(current_state->create_new_state(i, j)), this));
					}
				}
			}
			else continue;
		}
		return ret;
	}
	
	vector<Node*> traceback()
	{
		Node* check_node = this;
		vector<Node*> ret;
		ret.push_back(check_node);
		while(check_node->parent != NULL)
		{
			check_node = check_node->parent;
			ret.push_back(check_node);
		}
		return ret;
	}
};

bool check_complete(Node * node_to_check)
{
	if(node_to_check->current_state->state[0].size() != (unsigned) num_blocks)
		return false;
	else
	{
		for(int i = 1; i < num_blocks+1; i++)
		{
			if(node_to_check->current_state->state[0][i-1] != i)
			{
				return false;
			}
		}
		return true;
	}
}


class MyQueue
{
public:
	vector<Node*> queue;
	
	void push(Node* newNode)
	{
		if(queue.size() == 0) queue.push_back(newNode);
		else{
			int heuristic_value_2Add = calculate_heuristic_value(newNode->current_state) + newNode->parent->depth;
			for(unsigned int i = 0; i < queue.size(); i++)
			{
				if(heuristic_value_2Add <= calculate_heuristic_value(queue[i]->current_state) + queue[i]->parent->depth)
				{
					queue.insert(queue.begin()+i, newNode);
					break;
				}
			}
		}
	}
	
	Node* pop()
	{
		Node* ret = queue.front();
		queue.erase(queue.begin());
		return ret;
	}
	
	int get_size()
	{
		return queue.size();
	}
};

int generate_key(Node* state_to_process)
{
	int ret = 0;
	int constant = 1;
	for(unsigned int i = 0; i < state_to_process->current_state->state.size(); i++)
	{
		for(unsigned int j = 0; j < state_to_process->current_state->state[i].size(); j++)
		{
			ret += state_to_process->current_state->state[i][j] * constant;
		}
		constant = constant * 20;
	}
	int function_result = calculate_heuristic_value(state_to_process->current_state);
	return (ret - function_result);
}

Node* Search(Node* initial_state)
{
	MyQueue frontier;
	frontier.push(initial_state);
	while(frontier.get_size() != 0)
	{
		Node* being_exe = frontier.pop();
		
		int huristic_value = calculate_heuristic_value(being_exe->current_state) + being_exe->depth;
		if(frontier.get_size()> max_frontier_size)	max_frontier_size = frontier.get_size();
		if(being_exe->depth > max_depth)	max_depth = being_exe->depth;
		cout << "iter= " << total_iteration << ", f = g+h = " << huristic_value;
		cout<<  ", queue= " << frontier.get_size() << ", depth = " << being_exe->depth << endl;
		if(check_complete(being_exe))
		{

			cout << endl <<  "Success!!" << "depth =" << max_depth;
			cout << ", total_goal_tests=" << goal_tested << ", max_queue_size=" << max_frontier_size << endl;
			return being_exe;
		}
		else
		{
			vector<Node*> successor = being_exe->succesors();
			for(unsigned int i = 0; i < successor.size(); i++)
			{
				int key = generate_key(successor[i]);
				if(!database.count(key))
				{
					database[key] = successor[i]->depth;
					frontier.push(successor[i]);
					goal_tested++;
				}
				else{
					if(database[key] >=successor[i]->depth)
					{
						database[key] = successor[i]->depth;
						frontier.push(successor[i]);
					}
				}
			}
		}
		total_iteration++;
	}
	
	return NULL;
}

State* generate_problem(int stack1, int block1)
{
	vector< vector<int> > ret;
	ret.resize(stack1);
	for(int i = 0; i < block1;i ++)
	{
		ret[rand() % stack1].push_back(i+1);
	}	
	return new State(ret);
}

void display_result(Node* result)
{
	vector<Node*> path = result->traceback();
	cout << "Solution path:" << endl;
	for(int i = path.size() -1; i > -1; i--)
	{
		display(path[i]->current_state);
	}
}

int main(int argc, char *argv[])
{
	vector< vector<int> > test;
	num_blocks = atoi(argv[1]);
	num_stacks = atoi(argv[2]);
	
	State test1 = *generate_problem(num_stacks,num_blocks);
	cout << "initial state:" << endl;
	display(&test1);
	Node test2(&test1);
	test2.depth = 0;
	display_result(Search(&test2));
    return 0;
}
