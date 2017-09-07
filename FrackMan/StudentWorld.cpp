#include "StudentWorld.h"
#include <string>
#include <queue>
#include <sstream>




using namespace std;

GameWorld* createStudentWorld(string assetDir)
{
	return new StudentWorld(assetDir);
}

StudentWorld::~StudentWorld(){

}

int StudentWorld::init()
{
    //init frackman
    m_frackMan = new FrackMan(this);
    
    
    //fill map with dirt
    for (int i = 0; i < MAP_NROWS; i++) {
        for (int j = 0; j < MAP_NCOLS; j++) {
            m_dirt[i][j] = new Dirt(j,i, this);
        }
    }
    
    //set top 4 rows of dirt invisible
    for (int i = MAP_NROWS - 1; i >= MAP_NROWS - 4; i--) {
        for (int j = 0; j < MAP_NCOLS; j++) {
            clearDirtAt(j, i);
        }
    }
    
    //make vertical mineshaft
    for (int i = 4; i < MAP_NROWS - 4; i++) {
        for (int j = 30; j <= 33; j++) {
            clearDirtAt(j, i);
        }
    }
    
    
    
    m_nBarrels = 0;
    randomlyDistributeObjects();
    
    m_goodieChance = getLevel() * 25 + 300;
    m_ticksBetweenProtesters = (25 < 200 - getLevel()) ? 25 : 200 - getLevel();
    m_targetNumProtesters = (15 < 2 + getLevel()*1.5) ? 15 : 2 + getLevel()*1.5;
    m_nTicksSinceLastProtester = 0;
    m_nProtesters = 0;
    m_chanceOfHardcoreProtester = (90 < getLevel()*10 + 30) ? 90 : getLevel()*10 + 30;
    
    
    
    return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move()
{
    
    if (m_nProtesters > 0) {
        populateExitSearchStruct();
        populateFrackManSearchStruct();
    }
    
    
    setDisplayText();
    
    spawnGoodies();
    spawnProtesters();
    
    //call doSomething on all actors, frackman
    m_frackMan->doSomething();
    
    for (std::vector<GameObject*>::iterator it = m_actors.begin(); it != m_actors.end(); it++) {
        (*it)->doSomething();
    }
    
    
    
    
    removeDeadGameObjects();
    
    //TODO: add death response in frackman
    if (m_frackMan->getState() == STATE_DEAD) {
        decLives();
        return GWSTATUS_PLAYER_DIED;
    }
    
    if (m_nBarrels == 0) {
        GameController::getInstance().playSound(SOUND_FINISHED_LEVEL);
        return GWSTATUS_FINISHED_LEVEL;
    }
    
    return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp()
{
    //release frackman
    delete m_frackMan;
    
    //release dirt
    for (int i = 0; i < MAP_NROWS; i++) {
        for (int j = 0; j < MAP_NCOLS; j++) {
            delete m_dirt[i][j]; //ok even if nullptr
        }
    }
    
    //release actors
    std::vector<GameObject*>::iterator it = m_actors.begin();
    while (it != m_actors.end()) {
        delete (*it);
        it = m_actors.erase(it);
    }
}

void StudentWorld::setDisplayText(){
    int score = getScore();
    int level = getLevel();
    int lives = getLives();
    int hp = m_frackMan->getHitpoints();
    int health = m_frackMan->getMaxHealth();
    int water = m_frackMan->getWater();
    int gold = m_frackMan->getGold();
    int sonar = m_frackMan->getSonarCharge();
    int barrels = m_nBarrels;
    
    //calculate health from hp
    int percentHealth = (static_cast<double>(hp)/health) * 100;
    
    string stat = "";
    stat += "Scr: " + formattedPrefixFromNumber(score, '0', 6);
    stat += "  Lvl: " + formattedPrefixFromNumber(level, ' ', 2);
    stat += "  Lives: "  + formattedPrefixFromNumber(lives, ' ', 1);
    stat += "  Hlth: " + formattedPrefixFromNumber(percentHealth, ' ', 3) + "%";
    stat += "  Wtr: " + formattedPrefixFromNumber(water, ' ', 2);
    stat += "  Gld: " + formattedPrefixFromNumber(gold, ' ', 2);
    stat += "  Sonar: " + formattedPrefixFromNumber(sonar, ' ', 2);
    stat += "  Oil Left: " + formattedPrefixFromNumber(barrels, ' ', 2);
    setGameStatText(stat);
}

void StudentWorld::removeDeadGameObjects(){
    std::vector<GameObject*>::iterator it = m_actors.begin();
    while (it != m_actors.end()) {
        if((*it)->getState() == STATE_DEAD){ //object is dead
            delete *it;
            it = m_actors.erase(it);
        }
        else{
            it++;
        }
    }
}

string StudentWorld::formattedPrefixFromNumber(int n, char prefix, int length){
    string s = "";
    
    s += to_string(n);
    for (unsigned long i = s.size(); s.size() < length; i++) {
        s = prefix + s;
    }
    
    return s;
}

void StudentWorld::decrBarrels(){m_nBarrels--;}
void StudentWorld::incrNuggets(){m_frackMan->incrNuggets();}
void StudentWorld::incrSonar(){m_frackMan->incrSonar();}

//checks all points covered by boulder
bool StudentWorld::isBoulderAt(int x, int y){
    for (vector<GameObject *>::iterator it = m_actors.begin(); it != m_actors.end(); it++) {
        GameObject *c = *it;
        Boulder *r = dynamic_cast<Boulder*>(c);
        if (r != nullptr) { //GameObject is boulder
            
            if (isWithinRadius(x, y, c->getX(), c->getY(), 3)) {
                return true;
            }

        }
    }
    return false;
}

bool StudentWorld::isOtherBoulderAt(int x, int y, Boulder *t){
    for (vector<GameObject *>::iterator it = m_actors.begin(); it != m_actors.end(); it++) {
        GameObject *c = *it;
        Boulder *r = dynamic_cast<Boulder*>(c);
        if (r != nullptr && r!= t) { //GameObject is boulder, and isn't boulder passed as param
            
            if (isWithinRadius(x, y, c->getX(), c->getY(), 3)) {
                return true;
            }

        }
    }
    return false;
}


void StudentWorld::randomlyDistributeObjects(){
    //BOULDERS
    int nBoulders = ((getLevel() / 2 + 2) < 6) ? (getLevel() / 2 + 2) : 6;
    
    for (int i = 1; i <= nBoulders; i++) {
        int randX, randY;
        
        findValidObjectSpawnLocation(randX, randY);
        
        Boulder *b = new Boulder(randX, randY, this);
        addActor(b);
        
        //clear dirt behind boulder
        for (int i = randX; i <= randX + 3; i++) {
            for (int j = randY; j <= randY + 3; j++) {
                if (isDirtAt(i, j)) {
                    clearDirtAt(i, j);
                }
            }
        }
        
    }
    
    //BARRELS
    int nBarrels = (2 + getLevel() < 20) ? (2 + getLevel()) : 20;
    m_nBarrels = nBarrels;
    
    for (int i = 1; i <= nBarrels; i++) {
        int randX, randY;
        
        findValidObjectSpawnLocation(randX, randY);
        
        Barrel *b = new Barrel(randX, randY, this);
        addActor(b);
    }
    
    
    //GOLD
    int nNuggets = (5 - getLevel()/2 > 2) ? (5 - getLevel()/2) : 2;
    
    for (int i = 1; i <= nNuggets; i++) {
        int randX, randY;
        findValidObjectSpawnLocation(randX, randY);

        GoldNugget *g = new GoldNugget(randX, randY, this);
        addActor(g);
    }
}

void StudentWorld::findValidObjectSpawnLocation(int &x, int &y){
    int randX, randY;
    do{
        randX = rand() % 61;
        randY = rand() % 37 + 20;
    } while (isTooCloseToOtherObjects(randX, randY) || !isFilledWithDirt(randX, randY));
    x = randX;
    y = randY;
}


bool StudentWorld::isFilledWithDirt(int x, int y){
    for (int i = 0; i <= 3; i++) {
        for (int j = 0; j <= 3; j++) {
            if (!isDirtAt(x+i, y+j)) {
                return false;
            }
        }
    }
    
    return true;
}

void StudentWorld::setVisibleActorsWithinRadius(int x, int y, int radius){
    //for every actor
    for (std::vector<GameObject*>::iterator it = m_actors.begin(); it != m_actors.end(); it++) {
        //if actor is within radius, make it visible
        if(isWithinRadius(x, y, (*it)->getX(), (*it)->getY(), radius)){
            (*it)->setVisible(true);
        }
    }
}

void StudentWorld::shiftPointInDirection(int &x, int &y, GraphObject::Direction dir, int amt){
    switch (dir) {
        case GraphObject::left:
            x -= amt;
            break;
        case GraphObject::right:
            x += amt;
            break;
        case GraphObject::up:
            y += amt;
            break;
        case GraphObject::down:
            y -= amt;
            break;
        default:
            break;
    }
}

bool StudentWorld::directionIsClear(int x, int y, GraphObject::Direction dir){
    switch (dir) {
        case GraphObject::up:
            for (int i = 0; i <= 3; i++) {
                if (isDirtAt(x + i, y + 4)) {
                    return false;
                }
            }
            if (areObstaclesAt(x, y + 1, 4)) {
                return false;
            }
            break;
        case GraphObject::down:
            for (int i = 0; i <= 3; i++) {
                if (isDirtAt(x + i, y - 1)) {
                    return false;
                }
            }
            if (areObstaclesAt(x, y - 1, 4)) {
                return false;
            }
            break;
        case GraphObject::right:
            for (int i = 0; i <= 3; i++) {
                if (isDirtAt(x + 4, y + i)) {
                    return false;
                }
            }
            if (areObstaclesAt(x + 1, y, 4)) {
                return false;
            }
            break;
        case GraphObject::left:
            for (int i = 0; i <= 3; i++) {
                if (isDirtAt(x - 1, y + i)) {
                    return false;
                }
            }
            if (areObstaclesAt(x - 1, y, 4)) {
                return false;
            }
            break;
        default:
            break;
    }
    
    return true;
}

void StudentWorld::createSquirtWithDirectionAt(GraphObject::Direction dir, int x, int y){
    Squirt *s = new Squirt(x,y,this, dir);
    addActor(s);
}

bool StudentWorld::isTooCloseToOtherObjects(int x, int y){
    int minRadius = 6;
    
    //check distances from other actors
    for (vector<GameObject *>::iterator it = m_actors.begin(); it != m_actors.end(); it++) {
        GameObject *c = *it;
        if (isWithinRadius(x, y, c->getX(), c->getY(), minRadius)) {
            return true;
        }
    }
    
    return false;
}

bool StudentWorld::isDirtUnder(int x, int y, int width){
    for (int i = 0; i < width; i++) {
        if (isDirtAt(x + i, y - 1)) {
            return true;
        }
    }
    
    return false;
}

bool StudentWorld::areObstaclesAt(int x, int y, int size){
    //boundaries
    if (x < 0 || x > 64-size || y < 0 || y > 64-size) {
        return true;
    }
    
    //check for boulders
    if(isBoulderAt(x,y)){
        return true;
    }
    
    return false;
}

bool StudentWorld::frackManIsWithinRadius(int x, int y, int radius){
    if (isWithinRadius(x, y, m_frackMan->getX(), m_frackMan->getY(), radius)) {
        return true;
    }
    return false;
}

bool StudentWorld::protesterWithinRadiusTakesBribe(int x, int y, int radius){
    for (std::vector<GameObject*>::iterator it = m_actors.begin(); it != m_actors.end(); it++) {
        GameObject *c = *it;
        Protester *p = dynamic_cast<Protester*>(c);
        if (p != nullptr && isWithinRadius(x, y, p->getX(), p->getY(), radius)) {
            p->takeBribe();
            return true;
        }
    }
    return false;
}

bool StudentWorld::annoyProtestersWithinRadius(int x, int y, int radius, int annoyance, int cause){
    bool didAnnoy = false;
    for (std::vector<GameObject*>::iterator it = m_actors.begin(); it != m_actors.end(); it++) {
        GameObject *c = *it;
        Protester *p = dynamic_cast<Protester*>(c);
        if (p != nullptr && isWithinRadius(x, y, p->getX(), p->getY(), radius)) {
            if(p->getState() != STATE_LEAVING_OIL_FIELD){
                p->annoy(annoyance, cause);
                didAnnoy = true;
            }
        }
    }
    return didAnnoy;
}

//abstraction of distance
bool StudentWorld::isWithinRadius(int x1, int y1, int x2, int y2, int radius){
    if (getDistance(x1, y1, x2, y2) <= radius) {
        return true;
    }
    return false;
}

//calculate euclidean distance
double StudentWorld::getDistance(int x1, int y1, int x2, int y2){
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

void StudentWorld::spawnGoodies(){
    int r1 = rand() % m_goodieChance;
    if (r1 == 0) {
        int r2 = rand() % 5;
        if (r2 == 0) {
            //spawn sonar kit
            SonarKit *s = new SonarKit(this);
            addActor(s);
        }
        else{
            spawnWater();
        }
    }
}


void StudentWorld::spawnWater(){
    int randX, randY;
    
    do{
        randX = rand() % 61;
        randY = rand() % 61;
    } while (!isClearOfDirt(randX, randY, 4));
    
    WaterPool *w = new WaterPool(randX, randY, this);
    addActor(w);
}

bool StudentWorld::isClearOfDirt(int x, int y, int size){
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (isDirtAt(x+i, y+j)) {
                return false;
            }
        }
    }
    return true;
}


void StudentWorld::spawnProtesters(){
    if (m_nTicksSinceLastProtester >= m_ticksBetweenProtesters && m_nProtesters < m_targetNumProtesters) {
        
        int r = (rand() % 100) + 1;
        if (r <= m_chanceOfHardcoreProtester) {
            HardcoreProtester *hp = new HardcoreProtester(this);
            addActor(hp);
        }
        else{
            RegularProtester *rp = new RegularProtester(this);
            addActor(rp);
        }
        
        m_nProtesters++;
        m_nTicksSinceLastProtester = 0;
    }
    else{
        m_nTicksSinceLastProtester++;
    }
}



void StudentWorld::populateExitSearchStruct(){
    queue<Coord> coordQueue;
    char maze[66][66];
    //fill maze array from level
    char WALL = 'X', EMPTY = ' ', VISITED = 'V';
    
    for (int i = -1;  i < 65; i++) {
        for (int j = -1; j < 65; j++) {
            if (!isClearOfDirt(i, j, 4) || areObstaclesAt(i, j, 4)) {
                maze[i+1][j+1] = WALL;
            }
            else{
                maze[i+1][j+1] = EMPTY;
            }
        }
    }
    
    //TEST
    /*
    for (int i = 0;  i < 66; i++) {
        cout << endl;
        for (int j = 0; j < 66; j++) {
            cout << maze[i][j];
        }

    }
    cout << endl;
    */
    
    //note: array has different coord system than game
    Coord start(61,61);
    coordQueue.push(start);
    
    while (coordQueue.size() > 0) {
        Coord current = coordQueue.front();
        coordQueue.pop();

        
        
        //NORTH
        if(maze[current.r()-1][current.c()] == EMPTY){
            Coord north(current.r()-1, current.c());
            coordQueue.push(north);
            maze[current.r()-1][current.c()] = VISITED;
            
            directionMap[Coord(north.r()-1,north.c()-1)] = GraphObject::right;
        }
        

        
        //EAST
        if(maze[current.r()][current.c() + 1] == EMPTY){
            Coord east(current.r(), current.c() + 1);
            coordQueue.push(east);
            maze[current.r()][current.c() + 1] = VISITED;
            
            directionMap[Coord(east.r()-1, east.c()-1)] = GraphObject::down;
        }
        
        //SOUTH
        if(maze[current.r() + 1][current.c()] == EMPTY){
            Coord south(current.r() + 1, current.c());
            coordQueue.push(south);
            maze[current.r() + 1][current.c()] = VISITED;
            
            directionMap[Coord(south.r()-1, south.c()-1)] = GraphObject::left;
        }
        
        //WEST
        if(maze[current.r()][current.c() - 1] == EMPTY){
            Coord west(current.r(), current.c()-1);
            coordQueue.push(west);
            maze[current.r()][current.c() - 1] = VISITED;
            
            directionMap[Coord(west.r()-1, west.c()-1)] = GraphObject::up;
        }
         
         
        
 
        
        
        //TEST
        /*
        for (int i = 0;  i < 66; i++) {
            cout << endl;
            for (int j = 0; j < 66; j++) {
                cout << maze[i][j];
            }
            
        }
        cout << endl;
        */
        
    }
    
    //TEST
    /*
    for (int i = 0;  i < 66; i++) {
        cout << endl;
        for (int j = 0; j < 66; j++) {
            cout << maze[i][j];
        }
        
    }
    cout << endl;
     */
    
    //TEST
    /*
    char testMaze[64][64];
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {
            testMaze[i][j] = 'X';
        }
    }
    
    for (map<Coord, GraphObject::Direction>::iterator it = directionMap.begin(); it != directionMap.end(); it++) {
        Coord t = it->first;
        char p;
        switch (it->second) {
            case GraphObject::left :
                p = 'L';
                break;
            case GraphObject::right :
                p = 'R';
                break;
            case GraphObject::up :
                p = 'U';
                break;
            case GraphObject::down :
                p = 'D';
                break;
            default:
                break;
        }
        
        testMaze[t.r()][t.c()] = p;
    }
    
    for (int i = 0;  i < 64; i++) {
        cout << endl;
        for (int j = 0; j < 64; j++) {
            cout << testMaze[i][j];
        }
    
        
    }
    
    cout << endl;
    */
    
    
    
}



void StudentWorld::populateFrackManSearchStruct(){
    queue<CoordDistancePair> coordQueue;
    char maze[66][66];
    //fill maze array from level
    char WALL = 'X', EMPTY = ' ', VISITED = 'V';
    
    for (int i = -1;  i < 65; i++) {
        for (int j = -1; j < 65; j++) {
            if (!isClearOfDirt(i, j, 4) || areObstaclesAt(i, j, 4)) {
                maze[i+1][j+1] = WALL;
            }
            else{
                maze[i+1][j+1] = EMPTY;
            }
        }
    }

    
    //note: array has different coord system than game
    Coord start(m_frackMan->getX()+1,m_frackMan->getY()+1);
    
    //number of steps start at 5 away from FrackMan
    CoordDistancePair cdpStart(start, -5);
    coordQueue.push(cdpStart);
    

    while (coordQueue.size() > 0) {
        CoordDistancePair current = coordQueue.front();
        coordQueue.pop();
        

        
        //NORTH
        if(maze[current.coord.r()-1][current.coord.c()] == EMPTY){
            Coord north(current.coord.r()-1, current.coord.c());
            CoordDistancePair cdp(north, current.distance + 1);
            coordQueue.push(cdp);
            maze[current.coord.r()-1][current.coord.c()] = VISITED;
            
            DistanceDirectionPair dd(current.distance + 1, GraphObject::right);
            distanceDirectionMap[Coord(north.r()-1,north.c()-1)] = dd;
            

        }
        
        
        
        //EAST
        if(maze[current.coord.r()][current.coord.c() + 1] == EMPTY){
            Coord east(current.coord.r(), current.coord.c() + 1);
            CoordDistancePair cdp(east, current.distance + 1);
            coordQueue.push(cdp);
            maze[current.coord.r()][current.coord.c() + 1] = VISITED;
            
            DistanceDirectionPair dd(current.distance + 1, GraphObject::down);
            distanceDirectionMap[Coord(east.r()-1, east.c()-1)] = dd;

        }
        
        //SOUTH
        if(maze[current.coord.r() + 1][current.coord.c()] == EMPTY){
            Coord south(current.coord.r() + 1, current.coord.c());
            CoordDistancePair cdp(south, current.distance + 1);
            coordQueue.push(cdp);
            maze[current.coord.r() + 1][current.coord.c()] = VISITED;
            
            DistanceDirectionPair dd(current.distance + 1, GraphObject::left);
            distanceDirectionMap[Coord(south.r()-1, south.c()-1)] = dd;

        }
        
        //WEST
        if(maze[current.coord.r()][current.coord.c() - 1] == EMPTY){
            Coord west(current.coord.r(), current.coord.c()-1);
            CoordDistancePair cdp(west, current.distance + 1);
            coordQueue.push(cdp);
            maze[current.coord.r()][current.coord.c() - 1] = VISITED;
            
            DistanceDirectionPair dd(current.distance + 1, GraphObject::up);
            distanceDirectionMap[Coord(west.r()-1, west.c()-1)] = dd;

        }

        
    }
    

    
    
    
}


int StudentWorld::numberOfMovesToFrackMan(int x, int y){
    return distanceDirectionMap[Coord(x,y)].distance;
}

GraphObject::Direction StudentWorld::directionOfPathToFrackManAt(int x, int y){
    return distanceDirectionMap[Coord(x,y)].direction;
}

