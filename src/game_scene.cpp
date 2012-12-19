#include "game_scene.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <iostream>
#include <vector>
#include <list>
#include <sstream>

#include "defines.h"
#include "utils.h"
#include "input_engine.h"
#include "graphics_engine.h"

#include "game_reg.h"
#include "generator.h"

#include "player.h"
#include "person.h"
#include "police.h"

#include "hud.h"
#include "item.h"
#include "item_factory.h"
#include "menu_scene.h"

GameScene::GameScene(){

}

GameScene::~GameScene() {
	bg_music.stop();
}

void GameScene::initThread() {
	gameReg = GameReg::getInstance();
	gameReg->city = &city;
	gameReg->personList = &personList;
	gameReg->policeList = &policeList;
	gameReg->itemList = &itemList;
	gameReg->player = &player;
	gameReg->scene = this;

	GraphEng* graphics = GraphEng::getInstance();

	//Init map
	city.init(99,99, 64,64);
	//Init player
	player.Init();
	player.setPosition(city.getRandomStreet());

	//Init camera
	camera.reset(sf::FloatRect(0, 0,
				   (float) graphics->getCurrentVideoMode().width,
				   (float) graphics->getCurrentVideoMode().height));


        m_camViewportOrg.x =  (float) graphics->getCurrentVideoMode().width;
        m_camViewportOrg.y = (float) graphics->getCurrentVideoMode().height;

	//Init NPCS
	for (int i = 0; i < 600; ++i) spawnNewPerson();
	for (int i = 0; i < 30; ++i) spawnNewPolice();

	//Init Camera
	camera.setCenter(sf::Vector2f(0, 0));
        m_camZoom = 0.5f;

	//Init background music
	bg_music.openFromFile("audio/surrounding.ogg");
	bg_music.setLoop(true);
	bg_music.play();

	//Init hud
	hud.Init();

	//Le oc
	initThreadDone = true;
}

bool GameScene::Init() {

	initThreadDone = false;
	GraphEng* graphics = graphics->getInstance();

	/*
	sf::Sprite loadScene;
	loadScene.setTexture(*graphics->getTexture("img/loading.png"));
	loadScene.setPosition(App->getView().getSize().x*0.5f - loadScene.getTexture()->getSize().x*0.5f,
			      App->getView().getSize().y*0.5f - loadScene.getTexture()->getSize().y*0.5f);
	loadingText = sf::Text("Loading...");*/

	sf::Text titleText;
	titleText.setColor(sf::Color::Yellow);
	titleText.setStyle(sf::Text::Bold);
        titleText.setString("GiTA");
	titleText.setScale(3, 3);
	titleText.setPosition(
				(float) App->getSize().x*0.25f,
				(float) App->getSize().y*0.1f);

	sf::Clock timer;
	timer.restart();

	/*
	sf::Thread thr_init(&GameScene::initThread, this);
	thr_init.launch();
	while (!initThreadDone) {
		loadingText.setPosition(App->getSize().x*0.35f, App->getSize().y*0.75f);
		loadingText.setColor(sf::Color::Red);
		if (timer.getElapsedTime().asSeconds() > 2) {
			loadingText.setString(loadingText.getString()+".");
			timer.restart();
		}

		App->clear(sf::Color(255,255,255));
		graphics->Draw(&loadScene, GraphEng::GRAPH_LAYER_HUD);
		graphics->Draw(titleText);
		graphics->Draw(loadingText);
		graphics->DrawAll();
		App->display();
	}*/

	initThread();
	return true;
}

void GameScene::spawnNewMoney(sf::Vector2f pos) {
	Item* item = ItemFactory::MakeNewItem(ItemFactory::ITEM_MONEY);
	item->setPosition(pos);
	item->setTransPos(pos, pos + sf::Vector2f(
				  (float) Utils::randomInt(-16, 16),
				  (float) Utils::randomInt(-16, 16)));
	itemList.push_back(*item);
	delete item;
}


void GameScene::spawnNewPerson() {
	Person p;
	p.Init();
	p.setPosition(city.getRandomStreet());
	personList.push_back(p);
}

void GameScene::spawnNewPolice() {
	Police p;
	p.Init();
	p.setPosition(city.getRandomStreet());
	policeList.push_back(p);
}

vector<Person*> GameScene::getPeopleAround(sf::Vector2f pos, float r, SearchType st)
{
	sf::Vector2f min = pos - sf::Vector2f(r, r);
	sf::Vector2f max = pos + sf::Vector2f(r, r);
	int xmin = (int) (min.x / 64.0f);
	int ymin = (int) (min.y / 64.0f);
	int xmax = (int) (max.x / 64.0f);
	int ymax = (int) (max.y / 64.0f);
	if(xmin < 0) xmin = 0;
	if(ymin < 0) ymin = 0;
	if(xmax >= city.getTW()) xmax = city.getTW()-1;
	if(ymax >= city.getTH()) ymax = city.getTH()-1;

	vector<Person*> res;
	for(int x = xmin; x <= xmax; x++) {
		for(int y = ymin; y <= ymax; y++)
		{
			vector<Person*>& v = estructuraPepinoPeople[x][y];
			for(int i = 0; i < (int) v.size(); i++)
				if( (st == SEARCH_ANY ||
				     (st == SEARCH_DEAD && !v[i]->is_alive()) ||
				     (st == SEARCH_PANIC && v[i]->getState() == Person::STATE_PANIC))
				    && Utils::distance(pos, v[i]->m_position) <= r)
					res.push_back(v[i]);
		}
	}

	return res;
}

void GameScene::collide(Character* a)
{
	sf::Vector2f pos = a->m_position;
	float r = 10;

	sf::Vector2f min = pos - sf::Vector2f(r, r);
	sf::Vector2f max = pos + sf::Vector2f(r, r);
	int xmin = (int) (min.x / 64.0f);
	int ymin = (int) (min.y / 64.0f);
	int xmax = (int) (max.x / 64.0f);
	int ymax = (int) (max.y / 64.0f);
	if(xmin < 0) xmin = 0;
	if(ymin < 0) ymin = 0;
	if(xmax >= city.getTW()) xmax = city.getTW()-1;
	if(ymax >= city.getTH()) ymax = city.getTH()-1;

	for(int x = xmin; x <= xmax; x++) {
		for(int y = ymin; y <= ymax; y++)
		{
			{
				vector<Person*>& v = estructuraPepinoPeople[x][y];
				for(int i = 0; i < (int) v.size(); i++)
				{
					Character* b = v[i];

					if(Utils::distance(pos, b->m_position) < 10)
					{
						sf::Vector2f l = b->m_position - a->m_position;
						if(Utils::norm(l) == 0) continue;

						Utils::normalize(l);
						sf::Vector2f m = (b->m_position + a->m_position) / 2.0f;
						a->move(m-l*5.0f);
						b->move(m+l*5.0f);
					}
				}
			}

			{
				vector<Police*>& v = estructuraPepinoPolice[x][y];
				for(int i = 0; i < (int) v.size(); i++)
				{
					Character* b = v[i];

					if(Utils::distance(pos, b->m_position) < 10)
					{
						sf::Vector2f l = b->m_position - a->m_position;
						if(Utils::norm(l) == 0) continue;

						Utils::normalize(l);
						sf::Vector2f m = (b->m_position + a->m_position) / 2.0f;
						a->move(m-l*5.0f);
						b->move(m+l*5.0f);
					}
				}
			}
		}
	}
}

vector<Person*> GameScene::getPeopleSeen(Character* c, SearchType st)
{
	float r = 180;

	sf::Vector2f min = c->m_position - sf::Vector2f(r, r);
	sf::Vector2f max = c->m_position + sf::Vector2f(r, r);
	int xmin = (int) (min.x / 64.0f);
	int ymin = (int) (min.y / 64.0f);
	int xmax = (int) (max.x / 64.0f);
	int ymax = (int) (max.y / 64.0f);

	if(xmin < 0) xmin = 0;
	if(ymin < 0) ymin = 0;
	if(xmax >= city.getTW()) xmax = city.getTW()-1;
	if(ymax >= city.getTH()) ymax = city.getTH()-1;

	vector<Person*> res;
	for(int x = xmin; x <= xmax; x++) {
		for(int y = ymin; y <= ymax; y++)
		{
			vector<Person*>& v = estructuraPepinoPeople[x][y];
			for(int i = 0; i < (int) v.size(); i++)
				if( (st == SEARCH_ANY ||
				     (st == SEARCH_DEAD && !v[i]->is_alive()) ||
				     (st == SEARCH_PANIC && v[i]->getState() == Person::STATE_PANIC))
				    && c->canSee(v[i]->m_position))
					res.push_back(v[i]);
		}
	}

	return res;
}

void GameScene::Update() {

	InputEng* input = InputEng::getInstance();
        float delta = input->getFrameTime().asSeconds();
	if (input->getKeyDown(InputEng::NEW_SCENE))
		this->nextScene = new GameScene();

	input->setGlobalMousePos(
		App->convertCoords(sf::Vector2i(
			(int) input->getMousePos().x,
			(int) input->getMousePos().y), camera));


        //Camera Zoom
        if (input->getKeyDown(InputEng::CAM_ZOOM_IN)) {
            m_camZoomTrans.setPos(m_camZoom);
            m_camZoomTrans.goPos(0.4f);
            m_camZoomTrans.setTime(0.2f);
        }
        if (input->getKeyDown(InputEng::CAM_ZOOM_OUT)) {
            m_camZoomTrans.setPos(m_camZoom);
            m_camZoomTrans.goPos(0.5f);
            m_camZoomTrans.setTime(0.2f);
        }
        if (!m_camZoomTrans.reached()) {
            m_camZoomTrans.update(delta);
            m_camZoom = m_camZoomTrans.getPos();
        }


	estructuraPepinoPeople = vector<vector<vector<Person*> > > (city.getTW(), vector<vector<Person*> >(city.getTH()));
	estructuraPepinoPolice = vector<vector<vector<Police*> > > (city.getTW(), vector<vector<Police*> >(city.getTH()));

	for (std::list<Person>::iterator it = personList.begin(); it != personList.end(); ++it) {
		sf::Vector2i p = city.absoluteToTilePos(it->m_position);
		if(!city.validTile(p.x, p.y)) continue;
		estructuraPepinoPeople[p.x][p.y].push_back(&*it);
	}
	for (std::list<Police>::iterator it = policeList.begin(); it != policeList.end(); ++it) {
		sf::Vector2i p = city.absoluteToTilePos(it->m_position);
		if(!city.validTile(p.x, p.y)) continue;
		estructuraPepinoPolice[p.x][p.y].push_back(&*it);
	}

	//Player update
	player.Update();

	//Persons update
	for (std::list<Person>::iterator it = personList.begin(); it != personList.end(); ++it) {
		it->Update();
	}

	//Police update
	for (std::list<Police>::iterator it = policeList.begin(); it != policeList.end(); ++it) {
		it->Update();
	}

	//Items update
	for (std::list<Item>::iterator it = itemList.begin(); it != itemList.end(); ++it) {
		it->Update();
	}

	//COLLISIONS !!!!
	collide(&player);
	for (std::list<Person>::iterator it = personList.begin(); it != personList.end(); ++it)
		collide(&*it);
	for (std::list<Police>::iterator it = policeList.begin(); it != policeList.end(); ++it)
		collide(&*it);

	//Delete persons
	for (std::list<Person>::iterator it = personList.begin(); it != personList.end();) {
		if(it->isToBeDeleted())
		{
			std::list<Person>::iterator it2 = it;
			it2++;
			personList.erase(it);
			it = it2;
			spawnNewPerson();
		}
		else ++it;
	}

	//Delete items
	for (std::list<Item>::iterator it = itemList.begin(); it != itemList.end();) {
		if(it->isToBeDeleted())
		{
			std::list<Item	>::iterator it2 = it;
			it2++;
			itemList.erase(it);
			it = it2;
		}
		else ++it;
	}

	HandleCamInput();
	HandleEvents();
}

bool comp(Object* a, Object* b)
{
	if(a->m_prio == b->m_prio) {
		int ya = int(a->getPosition().y+0.1);
		int yb = int(b->getPosition().y+0.1);
		if(ya == yb) return a->getUniqueID() < b->getUniqueID();
		return ya < yb;
	}
	else return a->m_prio < b->m_prio;
}

void GameScene::Draw() {
	GraphEng* graphics = GraphEng::getInstance();

        camera.setSize(m_camViewportOrg.x*m_camZoom, m_camViewportOrg.y*m_camZoom);
        camera.setCenter(player.getPosition());

	App->setView(camera);

	//Map draw
	city.render();
	graphics->DrawAll();

	vector<Object*> v;
	v.push_back(&player);

	for (std::list<Person>::iterator it = personList.begin(); it != personList.end(); ++it)
		v.push_back(&*it);
	for (std::list<Police>::iterator it = policeList.begin(); it != policeList.end(); ++it)
		v.push_back(&*it);
	for (std::list<Item>::iterator it = itemList.begin(); it != itemList.end(); ++it)
		v.push_back(&*it);

	sort(v.begin(), v.end(), comp);
	for(int i = 0; i < (int) v.size(); i++)
	{
		v[i]->Draw();
		v[i]->DrawMark();
	}

	//Map draw
	city.renderTop();
	graphics->DrawAll();
	App->setView(App->getDefaultView());
	hud.Draw();
}

void GameScene::Destroy() {

}

void GameScene::HandleCamInput() {
	InputEng* input = InputEng::getInstance();

	if (input->getKeyState(InputEng::CAM_UP))
		camera.move(0, -input->getFrameTime().asSeconds()*2000);
	if (input->getKeyState(InputEng::CAM_DOWN))
		camera.move(0, input->getFrameTime().asSeconds()*2000);
	if (input->getKeyState(InputEng::CAM_LEFT))
		camera.move(-input->getFrameTime().asSeconds()*2000, 0);
	if (input->getKeyState(InputEng::CAM_RIGHT))
		camera.move(input->getFrameTime().asSeconds()*2000, 0);
}

void GameScene::HandleEvents() {
	while (!gameReg->eventQueue.empty()) {
		Event* e = gameReg->eventQueue.front();
		gameReg->eventQueue.pop();
		switch(e->type) {

		case EVENT_PLAYER_ACTION:
		{
			//EventPlayerAction* ev = (EventPlayerAction*)e;

			player.hitAction();
			std::vector<Person*> persons = getPeopleAround(player.getPosition(), 20, SEARCH_ANY);
			for (std::vector<Person*>::iterator it = persons.begin(); it != persons.end(); ++it) {
				if (!(*it)->is_alive()) continue;
				player.setKills(player.getKills()+1);
				(*it)->onHit();
				int n_moneys = Utils::randomInt(1, 3);
				for (int i = 0; i < n_moneys; ++i) spawnNewMoney((*it)->getPosition());
			}

			break;
		}

		/*
		case EVENT_DELETE_PERSON: {
			EventDeletePerson* ev = (EventDeletePerson*)e;

			for (std::list<Person>::iterator it = personList.begin(); it != personList.end(); ++it) {
				if (&(*it) == ev->person) {
					it = personList.erase(it);
					break;
				}
			}

			spawnNewPerson();
			break;
		}

		case EVENT_DELETE_ITEM: {
			EventDeleteItem* ev = (EventDeleteItem*)e;

			for (std::list<Item>::iterator it = itemList.begin(); it != itemList.end(); ++it) {
				if ((*it) == ev->item) {
					delete (*it);
					it = itemList.erase(it);
					break;
				}
			}

			break;
		}
		*/

		case EVENT_GAME_OVER: {
			//EventGameOver* ev = (EventGameOver*)e;
			MenuScene::setNewScore(player.getScore());
			nextScene = new MenuScene();
			break;
		}
		default:
			//Nothing
			break;
		}
		delete e;
	}
}
