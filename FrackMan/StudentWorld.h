#ifndef STUDENTWORLD_H_
#define STUDENTWORLD_H_


#include "GameWorld.h"
#include "GameConstants.h"
#include "GameController.h"
#include "Actor.h"

#include "GraphObject.h"

#include <random>
#include <string>
#include <vector>
#include <map>



const int MAP_NCOLS = 64;
const int MAP_NROWS = 64;

const int CAUSE_NEUTRAL = 0;
const int CAUSE_BOULDER = 1;
const int CAUSE_SQUIRT = 3;


class StudentWorld : public GameWorld
{
public:
	StudentWorld(std::string assetDir): GameWorld(assetDir){}
    virtual ~StudentWorld();

    virtual int init();

    virtual int move();

    virtual void cleanUp();
    
    bool isDirtAt(int x, int y){
        return m_dirt[y][x] != nullptr;
    }
    void clearDirtAt(int x, int y){
        delete m_dirt[y][x];
        m_dirt[y][x] = nullptr;
    }
    bool isBoulderAt(int x, int y);
    bool isOtherBoulderAt(int x, int y, Boulder *t /* pass self in */);
    
    bool frackManIsWithinRadius(int x, int y, int radius);
    bool protesterWithinRadiusTakesBribe(int x, int y, int radius);
    bool annoyProtestersWithinRadius(int x, int y, int radius, int annoyance, int cause);
    
    
    void annoyFrackMan(int amount){
        m_frackMan->annoy(amount, CAUSE_NEUTRAL);
    }
    
    //used by sonar
    void setVisibleActorsWithinRadius(int x, int y, int radius);
    
    void decrBarrels();
    
    //frackman mutators
    void incrNuggets();
    void incrSonar();
    void incrWaterBy(int n){m_frackMan->incrWaterBy(n);}
    
    
    void decrProtesters(){m_nProtesters--;}
    
    void createSquirtWithDirectionAt(GraphObject::Direction dir, int x, int y);
    bool isDirtUnder(int x, int y, int width);
    bool isClearOfDirt(int x, int y, int size);
    
    bool areObstaclesAt(int x, int y, int size);
    
    void addBribeAt(int x, int y){
        GoldNugget *g = new GoldNugget(x, y, this, 0, true);
        addActor(g);
    }
    

    void getFrackManPosition(int &x, int &y){
        x = m_frackMan->getX();
        y = m_frackMan->getY();
    }
    
    //for protesters
    GraphObject::Direction directionToMoveAt(int x, int y){
        return directionMap[Coord(x,y)];
    }
    //for hardcore protesters
    int numberOfMovesToFrackMan(int x, int y);
    GraphObject::Direction directionOfPathToFrackManAt(int x, int y);
    
    
    //Utilities
    void shiftPointInDirection(int &x, int &y, GraphObject::Direction dir, int amt);
    bool directionIsClear(int x, int y, GraphObject::Direction);
    
    
    
private:
    void setDisplayText();
    void randomlyDistributeObjects();
    void findValidObjectSpawnLocation(int &x, int &y);
    bool isTooCloseToOtherObjects(int x, int y);
    bool isWithinRadius(int x1, int y1, int x2, int y2, int radius);
    bool isFilledWithDirt(int x, int y);
    void removeDeadGameObjects();
    
    void spawnGoodies();
    void spawnWater();
    void spawnProtesters();
    
    
    void addActor(GameObject *a){
        m_actors.push_back(a);
    }
    
    std::string formattedPrefixFromNumber(int n, char prefix, int length);
    
    double getDistance(int x1, int y1, int x2, int y2);
    
    
    //maze search
    void populateExitSearchStruct();
    
    class Coord
    {
    public:
        Coord(int rr, int cc) : m_r(rr), m_c(cc) {}
        int r() const { return m_r; }
        int c() const { return m_c; }
        
        bool operator<(const Coord &other) const{
            
            if(m_r < other.m_r){
                return true;
            }
            
            if(m_r == other.m_r){
                if(m_c < other.m_c){
                    return true;
                }
            }
            return false;
        }
        
    private:
        int m_r;
        int m_c;
    };
    std::map<Coord, GraphObject::Direction> directionMap; //to exit
    

    //note: need default constructor for use with map
    struct DistanceDirectionPair{
        DistanceDirectionPair(): distance(0), direction(GraphObject::none){};
        DistanceDirectionPair(int dis, GraphObject::Direction dir): distance(dis), direction(dir){}
        int distance;
        GraphObject::Direction direction;
    };
    
    struct CoordDistancePair{
        CoordDistancePair(Coord c, int d): coord(c), distance(d){}
        Coord coord;
        int distance;
    };
    
    void populateFrackManSearchStruct();
    std::map<Coord, DistanceDirectionPair> distanceDirectionMap; //to frackman
    
    
    
    std::vector<GameObject*> m_actors;
    Dirt *m_dirt[MAP_NROWS][MAP_NCOLS];
    FrackMan *m_frackMan;
    
    
    
    int m_nBarrels;
    int m_goodieChance;
    
    int m_ticksBetweenProtesters;
    int m_nTicksSinceLastProtester;
    
    int m_nProtesters;
    int m_targetNumProtesters;
    int m_chanceOfHardcoreProtester;
};

#endif // STUDENTWORLD_H_
