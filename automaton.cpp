#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <random>
#include <chrono>
#include <thread>
#include <sstream>
#include <algorithm>


typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

namespace emergence {
	// Automaton serves as a node for a UF style data structure to hold and organize the particles 
	class Automaton {
		public:	
			Automaton(i16 idx, const std::string& name)
				: nomen(name), 
				id(idx),	
				charge(false),
				position{0.0,0.0},
				velocity(0.0),
				linkNum(0)
			{
				memory.resize(100, " ");
				memory[0] = "Agent - " + name + " - was brought into Emergence Engine";
			}	
			

			i16 getID() const { return id; }
			void setID(i16 newRoot) { this->id = newRoot; }
			bool getCharge() const { return charge; }	
			float getSpeed() const { return velocity; }

			const std::vector<f64>& getPos() const { return position; }
			const std::vector<std::string>& getLinks() const { return links; }
			
			void setVelocity(f64 v) { velocity = v; }
			void flipCharge() { charge = !charge; }
			

			void moveX(f64 rate){ increasePos(rate, 0); }
			void moveY(f64 rate){ increasePos(rate, 1); }
			
			bool share(Automaton& receiver, const std::string& msg){
				// send out information -> maybe charge?
				std::cout << "Sharing with - " << receiver.id << " -\n"; 
				receiver.respond(id, nomen, charge, msg);
				return true;
			}

			void respond(int senderID, const std::string& senderName, bool senderCharge, const std::string& msg){
				// respond to incoming message -> take in variables from alt perceived constructor 
				if (senderCharge == charge) {
					std::cout << "Message ignored: like charges.\n";
					return;
				}

				std::cout << "Intaking message from " << senderName << " [" << senderID << "]\n";
				remember(msg);	
				
			}

			void setPos(f64 x, f64 y) {
				position[0] = x;
				position[1] = y;
			}

			int getX() const { return static_cast<int>(position[0]); }
			int getY() const { return static_cast<int>(position[1]); }

			void moveRandom(const std::vector<std::vector<bool>>& map) {
				static std::random_device rd;
				static std::mt19937 rng(rd());

				std::vector<std::pair<int, int>> dirs = {
					{0, -1}, // up
					{0, 1}, // down
					{-1, 0}, // left
					{1, 0} // right
				};

				std::shuffle(dirs.begin(), dirs.end(), rng);

				int x = getX();
				int y = getY();

				for (auto [dx, dy] : dirs) {
					int nx = x + dx;
					int ny = y + dy;

					if (ny >= 0 && ny < map.size() && nx >= 0 && nx < map[ny].size() && map[ny][nx]) {
						setPos(nx, ny);
						return;
					}
				}
			}
		
		private:
			std::string nomen;
			i16 id;
			bool charge;
			std::vector<f64> position;
			f64 velocity;
			std::vector<std::string> memory;
			std::vector<std::string> links;
			i32 linkNum;

			void remember(const std::string& msg) {
				for (std::string& slot : memory) {
					if (slot == " ") {
						slot = msg;
						return;
					}
				}

				memory.push_back(msg);
			}	

			void increasePos(f64 rate, int axis){
				if (axis >= 0 && axis < position.size()){
					// increase x value
					position[axis] += rate * velocity;
				} 
			}
	};

	class World {
		public:
			World(int N, std::vector<std::vector<bool>> map) :	
				chargeMap(map),
				components(N),
				N(N)
			{
				id.resize(N);
				sz.resize(N, 1);

				for (int i = 0; i < N; i++) {
					id[i] = i;
				}
			}

			int root(int i){
				while (i != id[i]) {
					id[i] = id[id[i]];
					i = id[i];
				}
				return i;
			}

			void QuickUnion(int p, int q){
				std::cout << "Starting weighted quick union.\n";
				
				int i = root(p);
				int j = root(q);

				if (this->id[p] == this->id[q]){
					return;
				} else if (this->sz[i] < this->sz[j]){
					this->id[i] = j;
					this->components--;
					this->sz[j] += this->sz[i];
				} else {
					this->id[j] = i;
					this->components--;
				}
			}
			

			bool connected(Automaton a, Automaton b){
				return a.getID() == b.getID();
			}

			bool chargeCheck(Automaton a, Automaton b){
				return a.getCharge() != b.getCharge();
			}

		private:
			std::vector<int> id;
			std::vector<int> sz;
			std::vector<std::vector<bool>> chargeMap;
			std::vector<std::vector<std::string>> rawCC;	
			i16 components;
			i16 N;

			
	};

	std::vector<std::vector<bool>> loadMap(const std::string& filePath){
		std::ifstream file(filePath);
				
		if (!file.is_open()) {
			throw std::runtime_error("Could not open map file: " + filePath);
		}

		std::vector<std::vector<bool>> map;
		std::string line;

		while (std::getline(file, line)) {
			std::vector<bool> row;

			for (char ch : line) {
				if (ch == ' ') {
					row.push_back(true); // ground
				} else if (ch == '*') {
					row.push_back(false); // blocked
				}
			}

			if (!row.empty()) {
				map.push_back(row);
			}
		}

		return map;
	}
	
	void renderFrame(const std::vector<std::vector<bool>>& map, const std::vector<Automaton>& agents) {
		std::vector<std::string> frame;

		for (const auto& row : map) {
			std::string line;

			for (bool ground : row) {
				line += ground ? ' ' : '*';
			}

			frame.push_back(line);
		}

		for (const Automaton& a : agents) {
			int x = a.getX();
			int y = a.getY();

			if (y >= 0 && y < frame.size() && x >= 0 && x < frame[y].size()) {
				frame[y][x] = a.getCharge() ? '+' : '-';
			}
		}

		for (const std::string& line : frame) {
			std::cout << line << '\n';
		}
	}

	std::vector<Automaton> spawnAgents(const std::vector<std::vector<bool>>& map, int count) {
		std::vector<std::pair<int, int>> walkable;

		for (int y = 0; y < map.size(); y++){
			for (int x = 0; x < map[y].size(); x++){
				if (map[y][x]) {
					walkable.push_back({x,y});
				}
			}
		}

		if (count > walkable.size()) {
			count = walkable.size();
		}

		std::random_device rd;
		std::mt19937 rng(rd());

		std::shuffle(walkable.begin(), walkable.end(), rng);

		std::vector<Automaton> agents;

		for (int i = 0; i < count; i++) {
			std::stringstream name;
			name << "Automaton-" << i;
			agents.emplace_back(i, name.str());
			agents.back().setPos(walkable[i].first, walkable[i].second);

			if (i % 2 == 0) {
				agents.back().flipCharge();
			}
		}

		return agents;
	}

	bool areAdjacent(const Automaton& a, const Automaton& b) {
		int dx = std::abs(a.getX() - a.getX());
		int dy = std::abs(a.getY() - a.getY());

		return (dx + dy) == 1;
	}

	void processInteractions(std::vector<Automaton>& agents) {
		for (int i =0; i < agents.size(); i++) {
			for (int j = i + 1; j < agents.size(); j++) {
				if (areAdjacent(agents[i], agents[j]) && agents[i].getCharge() != agents[j].getCharge()) {
					std::string msgA = "Interacted with Agent " + std::to_string(agents[j].getID());
					std::string msgB = "Interacted with Agent " + std::to_string(agents[i].getID());

					agents[i].share(agents[j], msgB);
					agents[j].share(agents[i], msgA);
				}
			}
		}
	}

	void updateAgents(std::vector<Automaton>& agents, const std::vector<std::vector<bool>>& map) {
		processInteractions(agents);

		for (Automaton& a : agents) {
			a.moveRandom(map);
		}
	}
}	
	

int main() {
		std::cout << "[!] Starting Emergence Engine. \n";
		
		std::string mapPath = "maps/map_3.txt"; 
		auto map = emergence::loadMap(mapPath);

		int rows = map.size();
		int cols = map[0].size();
		int area = rows * cols;
		
		emergence::World worldUF(area, map);

		int agentCount = 100;
		auto agents = emergence::spawnAgents(map, agentCount);

		std::cout << "[+] Loaded map: "
			<< rows << " x " << cols
			<< " = " << area << " cells\n";

		while (true) {
			std::system("clear"); // clear terminal
			
			emergence::renderFrame(map, agents);
			emergence::updateAgents(agents, map);

			std::this_thread::sleep_for(
					std::chrono::milliseconds(250)
				);
		}

		return 0;
	}
			
			
	



