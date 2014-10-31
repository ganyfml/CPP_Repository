#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

using namespace std;

class Constrain;
class Domain;

vector<string> vars;
vector<Constrain*> constrains;
vector<Domain*> domains;

volatile int iteration = 1;

template<typename Type>
int find_in_vector(vector<Type> source, Type element)
{
	if (source.size() == 0)	return 0;
	for (unsigned int i = 0; i < source.size(); i++)
	{
		if (source[i] == element)
			return i;
	}
	return source.size();
}

class Constrain
{
public:
	vector<int> target;
	int self_index;

	Constrain(int self_index1)
	{
		self_index = self_index1;
		target = vector<int>();
	}

	void add(int target1)
	{
		target.push_back(target1);
	}

	bool satisfied(map<int, string>* data, string color)
	{
		if (data->size() == 0)	return true;
		for (unsigned int i = 0; i <target.size(); i++)
		{
			if (data->count(target[i]) > 0)
				if (data->at(target[i]).compare(color) == 0)	return false;
		}
		return true;
	}

	int size()
	{
		return target.size();
	}
};

class Domain
{
public:
	vector<string> value;
	vector<string> satisfied_value;

	Domain(vector<string> value1)
	{
		value = value1;
		satisfied_value = value1;
	}

	string get_value(int value_index)
	{
		return value[value_index];
	}

	string get_satisfied_value(int value_index)
	{
		return satisfied_value[value_index];
	}

	int size()
	{
		return value.size();
	}

	int satisfied_size()
	{
		return satisfied_value.size();
	}

	void set_satisfied_value(vector<string>* satisfied_value1)
	{
		satisfied_value = *satisfied_value1;
	}
};

void read_data(char* datafile_name)
{
	ifstream datafile(datafile_name);
	string line;
	string temp_string;
	getline(datafile, line);
	getline(datafile, line);
	stringstream ss;
	ss << line;
	ss >> temp_string;
	while ((vars.size() == 0) || (temp_string.compare(vars.back()) != 0))
	{
		vars.push_back(temp_string);
		ss >> temp_string;
	}
	constrains.resize(vars.size());
	getline(datafile, line);
	getline(datafile, line);
	while (line[0] != '#')
	{
		stringstream ss;
		string var_temp;
		string temp_domines_single;
		vector<string> temp_domines;
		ss << line;
		ss >> var_temp;
		ss >> temp_domines_single;
		temp_domines.push_back(temp_domines_single);
		ss >> temp_domines_single;
		temp_domines.push_back(temp_domines_single);
		ss >> temp_domines_single;
		temp_domines.push_back(temp_domines_single);
		domains.push_back(new Domain(temp_domines));
		getline(datafile, line);
	}
	while (getline(datafile, line))
	{
		stringstream ss;
		ss << line;
		ss >> temp_string;
		ss >> temp_string;
		int constrains_1 = find_in_vector(vars, temp_string);
		ss >> temp_string;
		int constrains_2 = find_in_vector(vars, temp_string);
		if (constrains[constrains_1] == NULL)
			constrains[constrains_1] = new Constrain(constrains_1);
		constrains[constrains_1]->add(constrains_2);
		if (constrains[constrains_2] == NULL)
			constrains[constrains_2] = new Constrain(constrains_2);
		constrains[constrains_2]->add(constrains_1);
	}
	for (unsigned int i = 0; i < constrains.size(); i++)
	{
		if (constrains[i] == NULL)	constrains[i] = new Constrain(i);
	}
}

int select_unassignment_var_default(map<int, string>* assignment)
{
	iteration++;
	for (unsigned int i = 0; i < vars.size(); i++)
	{
		if (assignment->count(i) == 0)
		{
			cout << "Adding unassignment var: " << vars[i] << endl;
			return i;
		}
	}
	return -1;
}

void display_assignment(map<int, string>* assignment)
{
	for (unsigned int i = 0; i < vars.size(); i++)
	{
		if (assignment->count(i) > 0)	cout << vars[i] << " -- " << assignment->at(i) << " | ";
	}
	cout << endl;
}

int calcuate_unassignment_satisfied_var_and_return_min_remaining(map<int, string>* assignment)
{
	iteration++;
	int min_index = 0;
	int min_value = INT_MAX;
	for (unsigned int i = 0; i < vars.size(); i++)
	{
		if (assignment->count(i) == 0)
		{
			vector<string> satisfied_temp;
			for (unsigned int j = 0; j < domains[i]->size(); j++)
			{
				if (constrains[i]->satisfied(assignment, domains[i]->get_value(j)))
					satisfied_temp.push_back(domains[i]->get_value(j));
			}
			domains[i]->set_satisfied_value(new vector<string> (satisfied_temp));
			if (min_value > satisfied_temp.size())
			{
				min_value = satisfied_temp.size();
				min_index = i;
			}
		}
	}
	return min_index;
}

map<int, string>* backtrack(map<int, string>* assignment, int mode)
{
	if (assignment->size() == vars.size())
		return assignment;
	int var_select = (mode == 1) ? calcuate_unassignment_satisfied_var_and_return_min_remaining(assignment) : select_unassignment_var_default(assignment);
	for (int i = 0; i < ((mode == 1) ? domains[var_select]->satisfied_size() : domains[var_select]->size()); i++)
	{
		string color_select = (mode == 1) ? domains[var_select]->get_satisfied_value(i) : domains[var_select]->get_value(i);
		cout << "Try " << vars[var_select] << " with color: " << color_select << endl;
		if (constrains[var_select]->satisfied(assignment, color_select))
		{
			cout << "OK for now, move on" << endl;
			assignment->insert(pair<int, string>(var_select, color_select));
		}
		else
		{
			cout << "Not OK, try another color" << endl;
			continue;
		}
		cout << endl << "Current Assignment: ";
		display_assignment(assignment);
		map<int, string> result = *backtrack(assignment, mode);
		if (result.size() != 0)	return new map<int, string>(result);
		cout << vars[var_select] << " color: " << color_select << " not satisfied, try another one" << endl;
		assignment->erase(var_select);
	}
	return new map<int, string>();
}

int main(int argc, char *argv[])
{
	char * filename = "C:\\data";
	read_data(filename);
	map<int, string> result;
	result = *backtrack(&result, 1);
	cout << endl << endl << "Finished!! Total iterations used: " << iteration << " , Result:" << endl << endl;
	display_assignment(&result);
	return 1;
}
