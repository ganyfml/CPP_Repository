#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <cmath>

using namespace std;

class Node;
class Vertex;
class MyQueue_priority;

vector< vector<int> > edges;
vector< Vertex* > vertexs;
int max_frontier_size = 0;
int vertex_visited = 0;
int total_iteration = 0;

class Node
{
public:
	int v;
	Node* parent;
	int depth;
	
	Node(int i)
	{
		v = i;
		depth = 0;
		parent = NULL;
	}
	
	Node(int i, Node* p)
	{
		v = i;
		parent = p;
		depth = parent->depth + 1;
	}
	
	vector<Node*> succesors()
	{
		vector<Node*> ret;
		
		for(unsigned char i = 0; i < edges[v].size(); i++)
		{
			ret.push_back(new Node(edges[v][i], this));
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

class Vertex
{
public:
	int x,y,v;
	int min_depth;
	
	Vertex(int v1)
	{
		v = v1;
		x = -1;
		y = -1;
		min_depth = -1;
	}
	
	Vertex(int v1, int x1, int y1)
	{
		x = x1;
		y = y1;
		v = v1;
		min_depth = -1;
	}
	
	void change_min_depth(int depth1)
	{
		min_depth = depth1;
	} 
};

double calcuate_distance(Node* from, Vertex* to)
{
	double ret = pow((pow((abs(vertexs[from->v]->x)- abs(vertexs[to->v]->x)),2) + pow((abs(vertexs[from->v]->y)- abs(vertexs[to->v]->y)),2)),0.5);
	return ret;
}

class MyQueue_priority
{
public:
	vector<Node*> queue;
	
	void push(Node* newNode, Vertex* target)
	{
		if(queue.size() == 0) queue.push_back(newNode);
		else{
			double distance_2Add = calcuate_distance(newNode, target);
			for(unsigned int i = 0; i < queue.size(); i++)
			{
				if(distance_2Add <= calcuate_distance(queue[i],target))
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



Node* Search(Node* initial_state, Vertex* goal)
{
	MyQueue_priority frontier;
	vertexs[initial_state->v]->change_min_depth(0);
	frontier.push(initial_state, goal);
	vertex_visited++;
	while(frontier.get_size() != 0)
	{
		total_iteration++;
		if(frontier.get_size()> max_frontier_size)	max_frontier_size = frontier.get_size();
		Node* being_exe = frontier.pop();
		cout << "iter=" << total_iteration << ", frontier =" << frontier.get_size() << ", popped=" << being_exe->v << " (" << vertexs[being_exe->v]->x << ","
			 << vertexs[being_exe->v]->y << "), depth=" << vertexs[being_exe->v]->min_depth << endl;
		if(being_exe->v == goal->v)
		{
			return being_exe;
		}
		else
		{
			vector<Node*> successor = being_exe->succesors();
			for(unsigned int i = 0; i < successor.size(); i++)
			{
				if(vertexs[successor[i]->v]->min_depth == -1 || vertexs[successor[i]->v]->min_depth > (being_exe->depth+1))
				{
					if(vertexs[successor[i]->v]->min_depth == -1)	vertex_visited++;
					cout << "pushed " << successor[i]->v << " (" << vertexs[successor[i]->v]->x << "," << vertexs[successor[i]->v]->y << ") " << endl;
					frontier.push(new Node(successor[i]->v, being_exe), goal);
					vertexs[successor[i]->v]->change_min_depth(being_exe->depth+1);
				}
				
			}
		}
	}
	
	return NULL;
}

void dataBase_create(string* file_name)
{
	ifstream infile(file_name->c_str());
	string mark;
	int num;
	
	infile >> mark >> num;
	cout << "vertices=" << num << ", ";
	for(int i = 0; i < num; i++)
	{
		int index, x, y;
		infile >> index >> x >> y;	
		vertexs.push_back(new Vertex(i, x, y));
	}
	
	infile >> mark >> num;
	cout << "edges=" << num << ", " << endl;
	edges.resize(num);
	for(int i = 0; i < num; i++)
	{
		int index, x, y;
		infile >> index >> x >> y;
		edges[x].push_back(y);
		edges[y].push_back(x);
	}
}

int find_node(int x1, int y1)
{
	for(unsigned int i = 0; i < vertexs.size(); i++)
	{
		if(vertexs[i]->x == x1 && vertexs[i]->y == y1)
			return i;
	}
	
	return -1;
}

void display(int from_x, int from_y, int to_x, int to_y)
{
	int node_from = find_node(from_x, from_y);
	int node_to = find_node(to_x, to_y);
	cout << "start=(" << from_x << "," << from_y << "), goal=(" << to_x << "," << to_y << "), " << "vertices: " << node_from << " and " << node_to << endl;
	Node root(node_from);
	vector<Node*> result = Search(&root, new Vertex(find_node(to_x, to_y)))->traceback();
	cout << "=========================================================" << endl;
	cout << "solution path:" << endl;
	for(int i = result.size()-1; i >=0; i--)
	{
		cout << " vertex " << result[i]->v << " (" << vertexs[result[i]->v]->x << "," << vertexs[result[i]->v]->y << ")" <<endl;
	}
	cout << "search algorithm  = BFS" << endl;
	cout << "total iteration   = " << total_iteration << endl;
	cout << "max frontier size = " << max_frontier_size << endl;
	cout << "vertices visted   = " << vertex_visited << "/" << vertexs.size() << endl;
	cout << "path length       = " << result.size()-1 << endl;
}

int main(int argc, char *argv[])
{
	string fileName = argv[1];
	dataBase_create(&fileName);
	display(atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),atoi(argv[5]));
    return 0;
}
