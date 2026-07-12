#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <exception>
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
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

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

	class Window {
		public:	
			Window() = default;

			~Window() {
				close();
			}

			bool initWin(int w, int h){
				width = w;
				height = h;
				calcArea();

				// SDL_Init(SDL_INIT_EVERYTHING);
				if (SDL_Init(SDL_INIT_VIDEO) != 0){
					std::cerr << "An error occured when opening window: " << SDL_GetError() << "\n";
					return false;
				}

				
				win = SDL_CreateWindow("Emergence Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);

				if (win == nullptr) {
					std::cerr << "Window creation failed: " << SDL_GetError() << "\n";
					SDL_Quit();
					return false;
				}

				renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

				if (renderer == nullptr) {
					std::cerr << "Renderer creation failed: " << SDL_GetError() << "\n";

					SDL_DestroyWindow(win);
					win = nullptr;
					SDL_Quit();
					return false;
				}

				state = true;
				std::cout << "[*] Emergence Engine Initialized !" << std::endl; 
				return true;
			}

			bool isRunning() const {
				return state;
			}

			void processEvents() {
				while (SDL_PollEvent(&event)) {
					switch (event.type) {
						case SDL_QUIT:
							state = false;
							break;
						case SDL_KEYDOWN:
							if (event.key.keysym.sym == SDLK_ESCAPE) {
								state = false;
							}
							break;
					}
				}
			}

			void beginFrame() {
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				SDL_RenderClear(renderer);
			}

			void endFrame() {
				SDL_RenderPresent(renderer);
			}

			SDL_Renderer* getRenderer() const {
				return renderer;
			}

			int getWidth() const {
				return width;
			}

			int getHeight() const {
				return height;
			}

			void close() {
				state = false;

				if (renderer != nullptr) {
					SDL_DestroyRenderer(renderer);
					renderer = nullptr;
				}

				if (win != nullptr) {
					SDL_DestroyWindow(win);
					win = nullptr;
				}

				SDL_Quit();
			}

		private:
			bool state = false;
			int width = 600;
			int height = 600;
			int area = 0;
			
			SDL_Event event {};
			SDL_Renderer* renderer = nullptr;
			SDL_Window* win = nullptr;	
			
			void calcArea() {
				this->area = this->width * this->height;
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
			

			bool connected(const Automaton& a, const Automaton& b){
				return root(a.getID()) == root(b.getID());
			}

			bool chargeCheck(const Automaton& a, const Automaton& b) const {
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
	
	// text based render
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

	// SDL based render
	void renderSDLFrame(SDL_Renderer* renderer, int windowWidth, int windowHeight, const std::vector<std::vector<bool>>& map, const std::vector<Automaton>& agents) {
		if (map.empty() || map[0].empty()) {
			return;
		}

		const int rows = static_cast<int>(map.size());
		const int cols = static_cast<int>(map[0].size());

		const float cellWidth = static_cast<float>(windowWidth) / cols;
		const float cellHeight = static_cast<float> (windowHeight) / rows;
		
		// Draw blocked map cells
		SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);

		for (int y = 0; y < rows; y++) {
			for (int x = 0; x < static_cast<int>(map[y].size()); x++) {
				if (!map[y][x]) {
					SDL_Rect wall {
						static_cast<int>(x * cellWidth),
						static_cast<int>(y * cellHeight),
						static_cast<int>(cellWidth + 1.0f),
						static_cast<int>(cellHeight + 1.0f)
					};

					SDL_RenderFillRect(renderer, &wall);
				}
			}
		}

		// Draw Agents
		for (const Automaton& agent : agents) {
			const int centerX = static_cast<int>((agent.getX() + 0.5f) * cellWidth);
			const int centerY = static_cast<int>((agent.getY() + 0.5f) * cellHeight);

			const int radius = std::max(2, static_cast<int>(std::min(cellWidth, cellHeight) * 0.35f));

			if (agent.getCharge()) {
				SDL_SetRenderDrawColor(renderer, 230, 90, 90, 255);
			} else {
				SDL_SetRenderDrawColor(renderer, 90, 140, 240, 255);
			}

			SDL_Rect particle {
				centerX - radius, centerY - radius, radius * 2, radius * 2
			};
			
			SDL_RenderFillRect(renderer, &particle);
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
		int dx = std::abs(a.getX() - b.getX());
		int dy = std::abs(a.getY() - b.getY());

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
		try {
			const std::string mapPath = "maps/map_3.txt"; 
			auto map = emergence::loadMap(mapPath);

			if (map.empty() || map[0].empty()) {
				throw std::runtime_error("Loaded map is empty.");
			}
			
			const int rows = static_cast<int>(map.size());
			const int cols = static_cast<int>(map[0].size());
			const int area = rows * cols;
			
			emergence::World worldUF(area, map);

			int agentCount = 100;
			auto agents = emergence::spawnAgents(map, agentCount);

			std::cout << "[+] Loaded map: "
				<< rows << " x " << cols
				<< " = " << area << " cells" << std::endl;
			
			emergence::Window window;

			if (!window.initWin(800, 800)) {
				return 1;
			}

			const Uint64 updateInterval = 150;
			Uint64 previousUpdate = SDL_GetTicks64();
			
			while (window.isRunning()) {
				window.processEvents();
				
				const Uint64 currentTime = SDL_GetTicks64();

				if (currentTime - previousUpdate >= updateInterval) {
					emergence::updateAgents(agents, map);
					previousUpdate = currentTime;
				}

				window.beginFrame();

				emergence::renderSDLFrame(window.getRenderer(), window.getWidth(), window.getHeight(), map, agents);

				// emergence::updateAgents(agents, map);

				window.endFrame();
			}
		} 
		catch (const std::exception& error) {
			std::cerr << "[!] Emergence Engine error: " << error.what() << "\n";
			return 1;
		}
		return 0;
	}
			
			
	



