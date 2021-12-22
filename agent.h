/**
 * Framework for NoGo and similar games (C++ 11)
 * agent.h: Define the behavior of variants of the player
 *
 * Author: Theory of Computer Games (TCG 2021)
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"
#include <fstream>
#include <vector>
#include <set>
using namespace std;

struct node{
	float value = 0;
	int explore_num = 0;
	int place;
	node* parent = NULL;
	board board;
	vector<node*> child;
};

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

/**
 * base agent for agents with randomness
 */
class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * random player for both side
 * put a legal piece randomly
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=random role=unknown " + args),
		space(board::size_x * board::size_y), who(board::empty) {
		if (name().find_first_of("[]():; ") != std::string::npos)
			throw std::invalid_argument("invalid name: " + name());
		if (role() == "black") who = board::black;
		if (role() == "white") who = board::white;
		if (who == board::empty)
			throw std::invalid_argument("invalid role: " + role());
		for (size_t i = 0; i < space.size(); i++)
			space[i] = action::place(i, who);
	}

	virtual action take_action(const board& state) {
		node root;
		node* cur_node;
		root.board = state;
		root.place = -1;

		for(int i=1; i<=500; i++){
			cur_node = &root;
			bool find_explode_node = false;
			while(!find_explode_node){
				int legal_num = 0;
				for (const action::place& move : space) {
					board after = cur_node->board;
					if (move.apply(after) == board::legal)
						legal_num+=1;
				}
				if (cur_node->child.size() < legal_num)
					find_explode_node = true;
				else{
					float max_score = 0;
					for(int j=1; j<=cur_node->child.size(); j++){
						node* cur_child = (cur_node->child)[j-1];
						float score = cur_child->value + sqrt(2*log(i)/cur_child->explore_num);
						if (score > max_score){
							max_score = score;
							cur_node = cur_child;
						}
					}
				}
			}		
		}
		// std::shuffle(space.begin(), space.end(), engine);
		// for (const action::place& move : space) {
		// 	board after = state;
		// 	if (move.apply(after) == board::legal)
		// 		return move;
		// }
		return action();
	}

private:
	std::vector<action::place> space;
	board::piece_type who;
};
