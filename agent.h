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

class node : board {
	public:
		node(const board& state, node* parent = nullptr) : board(state),
			win(0), visit(0), child(), parent(parent) {}

		/**
		 * run MCTS for N cycles and retrieve the best action
		 */
		action run_mcts(size_t N, std::default_random_engine& engine) {
			for(size_t i = 0; i < N; i++){
				std::vector<node*> path = select();
				node* leaf = path.back()->expand(engine);
				if (leaf != path.back())
					path.push_back(leaf);
				update(path, leaf->simulate(engine));
			}
			return take_action();
		}

	protected:

		/**
		 * select from the current node to a leaf node by UCB and return all of them
		 * a leaf node can be either a node that is not fully expanded or a terminal node
		 */
		std::vector<node*> select() {
			int count=0;
			std::vector<node*> path = { this };
			node* cur_node = this;
			node* max_node = nullptr;
			float max_score = 0;
			while(cur_node->is_selectable()){
				max_score = -1;
				for(size_t i=0; i<cur_node->child.size();i++){
					if(cur_node->child[i].ucb_score() > max_score){
						if (cur_node->child[i].ucb_score()<0) count++;
						max_score = cur_node->child[i].ucb_score();
						max_node = &cur_node->child[i];
					}
				}
				cur_node = max_node;
				path.push_back(cur_node);
			}
			return path;
		}
		/**
		 * expand the current node and return the newly expanded child node
		 * if the current node has no unexpanded move, it returns itself
		 */

		node* expand(std::default_random_engine& engine) {
			board cur_board = *this;
			std::vector<int> moves = all_moves(engine);
			for (int move : moves) {
				size_t i = 0;
				for(i=0; i<child.size();i++){
					if (child[i].info().last_move.i == move){
						break;
					}
				}
				if (i == child.size() && cur_board.place(move) == board::legal){
					// this->child.push_back(cur_board);
					this->child.push_back(node(cur_board, this));
					return &child.back();
				}		
			}
			return this;
		}

		/**
		 * simulate the current node and return the winner
		 */
		unsigned simulate(std::default_random_engine& engine) {
			board cur_board = *this;
			bool is_game_end = false;
			std::vector<int> moves = all_moves(engine);

			while(!is_game_end){
				bool is_find_legal = false;
				for (int move : moves)
					if (cur_board.place(move) == board::legal){
						is_find_legal = true;
						break;
					}
				if (!is_find_legal)
					is_game_end = true;
			}

			return  (cur_board.info().who_take_turns == board::white)? board::black:board::white;
		}

		/**
		 * update statistics for all nodes saved in the path
		 */
		void update(std::vector<node*>& path, unsigned winner) {
			for (node* path_node : path) {
				path_node->visit++;
				if (winner == info().who_take_turns)
					path_node->win++;
			}
		}

		/**
		 * pick the best action by visit counts
		 */
		action take_action() const {
			int max_visit = -1;
			const node* best_node = NULL;
			for(size_t i=0; i<child.size();i++){
				if(int(child[i].visit) > max_visit){
					max_visit = int(child[i].visit);
					best_node = &child[i];
				}
			}
			if (best_node != NULL)
				return action::place(best_node->info().last_move, info().who_take_turns);
			else
				return action();
		}

	private:

		/**
		 * check whether this node is a fully-expanded non-terminal node
		 */
		bool is_selectable() const {
			size_t legal_moves = 0;
			for (int move=0; move<81; move++){
				board cur_board = board(*this);
				if (cur_board.place(move) == board::legal)
					legal_moves++;
			}			
			if (legal_moves == 0) // leaf_node
				return false;
			else if (legal_moves == child.size()) // fully-expanded
				return true;
			else // non-fully-expanded
				return false; 

		}

		/**
		 * get the ucb score of this node
		 */
		float ucb_score(float c = std::sqrt(2)) const {
			float exploit = float(win)/visit;
			float explore = sqrt(log(parent->visit)/visit);
			return exploit + c*explore;
		}

		/**
		 * get all moves in shuffled order
		 */
		std::vector<int> all_moves(std::default_random_engine& engine) const {
			std::vector<int> moves;
			for(int i=0; i<81;i++) moves.push_back(i);
			std::shuffle(moves.begin(), moves.end(), engine);
			return moves;
		}

	private:
		size_t win, visit;
		std::vector<node> child;
		node* parent;
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
		size_t N = 200;
		N = meta["N"];
		return node(state).run_mcts(N, engine);
	}

private:
	std::vector<action::place> space;
	board::piece_type who;
};


class noob_player : public random_agent {
public:
	noob_player(const std::string& args = "") : random_agent("name=random role=unknown " + args),
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
		std::shuffle(space.begin(), space.end(), engine);
		for (const action::place& move : space) {
			board after = state;
			if (move.apply(after) == board::legal)
				return move;
		}
		return action();
	}

private:
	std::vector<action::place> space;
	board::piece_type who;
};
