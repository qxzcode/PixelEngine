//
//  World.cpp
//  PixelWorld
//
//  Created by Quinn on 9/11/14.
//
//

#include "World.h"

#include "game_shaders.h"

World::World():noise(time(NULL)),chunkLoader(this) {
	
}

World::~World() {
	
}


void World::init() {
	chunkLoader.startWorker();
	spawnEntity(player = new Player(this, 0, 0));
	gameShader = gl::GlslProg(game_vert_src, game_frag_src);
	gameShader.bind();
}

void World::update(float dt) {
	// update particles
//	for (int i = 0; i < particles.size(); i++) {
//		particle& p = particles[i];
//		
//		float f = 1 + (0.3*dt);
//		p.sx /= f;
//		p.sy /= f;
//		p.sy += 7.0*dt;
//		p.x += p.sx;
//		p.y += p.sy;
//		
//		int x = int(p.x), y = int(p.y);
//		bool dead = false;
//		if (!chunkLoaded(getChunkCoord(x), getChunkCoord(y))) {
//			dead = true;
//		} else if (getTile(x, y) != 0) {
//			setTile(x, y, 0);
//			dead = true;
//		}
//		if (dead)
//			particles.erase(particles.begin()+(i--));
//	}
	
	// update entities
	std::forward_list<EntityRef>::iterator it = entities.begin(), lastIt;
	while (it != entities.end()) {
		if ((*it)->update(dt)) {
			// update returned true; remove entity
			bool first = it==entities.begin();
			it++;
			if (first)
				entities.pop_front();
			else
				entities.erase_after(lastIt);
		} else lastIt = it++;
	}
}

void World::draw(int winWidth, int winHeight) {
	// camera translation
	glPushMatrix();
	winWidth /= 2; winHeight /= 2;
	int camX = this->camX(), camY = this->camY();
	gl::translate(winWidth-camX*PIXEL_SIZE, winHeight-camY*PIXEL_SIZE);
	
	// calculate the visible range of chunks
	int w = winWidth /CHUNK_PIXEL_SIZE + 1;
	int h = winHeight/CHUNK_PIXEL_SIZE + 1;
	int camCX = getChunkCoord(camX), camCY = getChunkCoord(camY);
	int cxMin =	camCX - w;
	int cxMax = camCX + w;
	int cyMin =	camCY - h;
	int cyMax = camCY + h;
	
	// draw the chunks
	glEnable(GL_TEXTURE_2D);
	glColor3f(1, 1, 1);
	if (cxMin!=this->cxMin||cyMin!=this->cyMin||cxMax!=this->cxMax||cyMax!=this->cyMax) {
		// if visible chunk range has changed, update the list
		visibleChunks.clear();
		for (int cx = cxMin-1; cx <= cxMax+1; cx++) {
			bool xEdge = cx==cxMin-1 || cx==cxMax+1;
			for (int cy = cyMin-1; cy <= cyMax+1; cy++) {
				bool yEdge = cy==cyMin-1 || cy==cyMax+1;
				bool edge = xEdge || yEdge;
				
				if (edge) {
					chunkLoader.requestLoadChunk({cx, cy});
				} else {
					Chunk* chunk = &loadChunk(cx, cy);
					visibleChunks.push_front(chunk);
					chunk->draw(this);
				}
			}
		}
		this->cxMin=cxMin;this->cyMin=cyMin;this->cxMax=cxMax;this->cyMax=cyMax;
	} else {
		for (auto it = visibleChunks.begin(); it != visibleChunks.end(); it++) {
			(*it)->draw(this);
		}
	}
	
	// draw entities
	for (auto it = entities.begin(); it != entities.end(); it++) {
		(*it)->draw();
	}
	
	// draw particles
//	w = winWidth /PIXEL_SIZE + 1;
//	h = winHeight/PIXEL_SIZE + 1;
//	glDisable(GL_TEXTURE_2D);
//	for (particle& p : particles) {
//		if (p.x<camX-w||p.x>camX+w||p.y<camY-h||p.y>camY+h) continue;
//		glColor3ub(p.r, p.g, p.b);
//		float x = util::floor(p.x)*PIXEL_SIZE, y = util::floor(p.y)*PIXEL_SIZE;
//		gl::drawSolidRect(Rectf(x, y, x+PIXEL_SIZE, y+PIXEL_SIZE));
//	}
	
	glPopMatrix(); // camera translation
}


tileID World::getTile(int x, int y) {
	int cx = getChunkCoord(x), cy = getChunkCoord(y);
	if (!chunkLoaded(cx, cy)) return 0;
	else return loadChunk(cx, cy).getTile(getLocalCoord(x), getLocalCoord(y));
}

void World::setTile(int x, int y, tileID tile) {
	int cx = getChunkCoord(x), cy = getChunkCoord(y);
	if (chunkLoaded(cx, cy)) {
		loadChunk(cx, cy).setTile(getLocalCoord(x), getLocalCoord(y), tile);
	}
}

#include "Acid.h"

void World::spawnParticle(int x, int y) {
#define rand ((random()/2147483647.0*2.0)-1.0)
	spawnEntity(new Acid(this, double(x), double(y), rand*1000, rand*1000));
#undef rand
}

void World::spawnEntity(Entity *e) {
	entities.emplace_front(e);
}

Chunk& World::loadChunk(int cx, int cy) {
	chunkCoords cc = {cx, cy};
	try {
		std::lock_guard<std::mutex> lock(chunkLoader.chunksMutex);
		return chunks.at(cc);
	} catch (std::out_of_range ex) {
		chunkLoader.loadChunk(cc);
		std::lock_guard<std::mutex> lock(chunkLoader.chunksMutex);
		return chunks.at(cc);
	}
}
