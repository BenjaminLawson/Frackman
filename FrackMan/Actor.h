#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"

class StudentWorld;


//state constants for GameObjects
const int STATE_DEAD = 0;
const int STATE_ALIVE = 1;
const int STATE_FALLING = 2;
const int STATE_WAITING = 3;
const int STATE_PERMANENT = 4;
const int STATE_TEMPORARY = 5;
const int STATE_RESTING = 6;
const int STATE_LEAVING_OIL_FIELD = 7;

//GAMEOBJECT
//abstract
class GameObject: public GraphObject
{
public:
    GameObject(int imageID, int startX, int startY, StudentWorld *world, bool isVisible, Direction dir = right, double size = 1.0, unsigned int depth = 0, int state = STATE_ALIVE);
    virtual ~GameObject(){};

    virtual void doSomething() = 0;
    void setState(int state){m_state = state;}
    int getState() const{return m_state;}
    
    StudentWorld* getWorld(){return m_world;}
    
    void moveInFacingDirection();
private:
    int m_state;
    StudentWorld *m_world;
};


//PERSON
//abstract
class Person: public GameObject
{
public:
    Person(int imageID, int startX, int startY, int hitpoints, StudentWorld *world, Direction dir = right, double size = 1.0, unsigned int depth = 0);
    virtual ~Person(){};
    
    int getMaxHealth()const {return m_maxHealth;}
    int getHitpoints() const;
    void setHitpoints(int hp);
    void annoy(int amount, int cause);
    
private:
    int m_hitpoints;
    int m_maxHealth;
    
    virtual void annoyResponse(int hp, int cause){}
};

//FRACKMAN

class FrackMan: public Person
{
public:
    FrackMan(StudentWorld *world);
    virtual ~FrackMan(){};
    
    virtual void doSomething();
    
    int getWater() const{return m_nWater;}
    int getSonarCharge() const{return m_nCharge;}
    int getGold() const{return m_nGold;}
    
    void incrNuggets(){m_nGold++;}
    void decrNuggets(){m_nGold--;}
    
    void incrSonar(){m_nCharge++;}
    void incrWaterBy(int n){m_nWater += n;}
    
private:
    void tryToMove(Direction d);
    void digDirt();
    void useSonar();
    void useSquirtGun();
    void dropGoldNugget();
    
    int m_nWater;
    int m_nCharge;
    int m_nGold;
    
};

//PROTESTER
//abstract
class Protester: public Person
{
public:
    Protester(StudentWorld *world, int hitpoints = 5, int imageID = IID_PROTESTER);

    ~Protester(){}
    
    virtual void doSomething();
    
    virtual void takeBribe() = 0;
    
    void restForTicks(int nTicks){m_nRestingTicks = nTicks;}
    
private:
    virtual bool doSpecializedRoutine() = 0; //returns whether or not ended early
    virtual int getSquirtPoints() = 0;
    
    bool isAtExitPoint();
    bool isFacingFrackMan();
    
    virtual void annoyResponse(int hp, int cause);
    
    int activeTicksSinceLastYell(){return m_activeTicksSinceLastYell;}
    void yell();
    
    bool isDirectClearPathToFrackMan();
    
    void resetNumSquaresToMoveInCurrentDirection(){
        m_nSquaresToMoveInDirection = 0;
    }
    
    int getNumSquaresToMoveInDirection(){return m_nSquaresToMoveInDirection;}
    
    void decrNumSquaresToMoveInDirection(){m_nSquaresToMoveInDirection--;}
    
    bool canMoveInDirection(Direction dir);
    
    void pickNewNumSquaresToMove();
    
    bool isAtIntersection();
    
    int ticksSinceLastPerpendicularTurn(){return m_nTicksSinceLastTurn;}
    void incrTicksSinceLastPerpendicularTurn(){m_nTicksSinceLastTurn++;}
    void resetTicksSinceLastPerpendicularTurn(){m_nTicksSinceLastTurn = 0;}
    
    
    int m_nSquaresToMoveInDirection;
    int m_ticksToWaitBetweenMoves;
    int m_nRestingTicks;
    int m_activeTicksSinceLastYell;
    int m_nTicksSinceLastTurn;
    
};

//REGULARPROTESTER
class RegularProtester: public Protester
{
public:
    RegularProtester(StudentWorld *world): Protester(world){
        
    }
    virtual ~RegularProtester(){}
    
    
    
    virtual void takeBribe();
    
    
private:
    virtual bool doSpecializedRoutine();
    virtual int getSquirtPoints(){return 100;}
};

//HARDCOREPROTESTER
class HardcoreProtester: public Protester
{
public:
    HardcoreProtester(StudentWorld *world): Protester(world, 20, IID_HARD_CORE_PROTESTER){}
    virtual ~HardcoreProtester(){}
    
    virtual void takeBribe();
    
private:
    virtual bool doSpecializedRoutine();
    virtual int getSquirtPoints(){return 250;}
};

//DIRT
class Dirt: public GameObject
{
public:
    Dirt(int startX, int startY, StudentWorld *world): GameObject(IID_DIRT, startX, startY, world, true, right, 0.25, 3){};
    virtual ~Dirt(){};
    
    virtual void doSomething();
private:
    
};

//BOULDER
class Boulder: public GameObject
{
public:
    Boulder(int startX, int startY, StudentWorld *world): GameObject(IID_BOULDER, startX, startY, world, true, down, 1.0, 1){
        m_nTicks = 0;
        
    };
    virtual ~Boulder(){};
    
    virtual void doSomething();
private:
    int m_nTicks;
    
};

//GOODIE
//abstract
class Goodie: public GameObject{
public:
    Goodie(int imageID, int startX, int startY, StudentWorld *world, bool isVisible, int pointValue, int sound = SOUND_GOT_GOODIE): GameObject(imageID, startX, startY, world, isVisible, right, 1.0, 2){
        m_isDiscovered = isVisible;
        m_sound = sound;
        m_points = pointValue;
    }
    virtual ~Goodie(){}
    
    virtual void doSomething();
    bool isDiscovered(){return m_isDiscovered;}
    void discoverGoodie(){
        m_isDiscovered = true;
        setVisible(true);
    }
    virtual bool isPickupable(){return true;}
    int getPoints(){return m_points;}
private:
    bool m_isDiscovered;
    int m_sound;
    int m_points; //points when FRACKMAN picks up
    
    virtual void hasBeenPickedUp() = 0;
    virtual void doGoodieRoutine(){}
};

//TEMPORARYGOODIE
//abstract
//goodies that disappear after number of ticks
class TemporaryGoodie: public Goodie{
public:
    TemporaryGoodie(int imageID, int startX, int startY, StudentWorld *world, bool isVisible, int pointValue): Goodie(imageID, startX, startY, world, isVisible, pointValue){
        m_nTicks = calculateTicks(world);
    }
    virtual ~TemporaryGoodie(){}
    
    
    
    virtual void doSomething();
private:
    virtual bool shouldTick() = 0;
    virtual int calculateTicks(StudentWorld *world);
    int m_nTicks;
};


//BARREL
class Barrel: public Goodie
{
public:
    Barrel(int startX, int startY, StudentWorld *world): Goodie(IID_BARREL, startX, startY, world, false, 1000, SOUND_FOUND_OIL){}
    virtual ~Barrel(){}
    
private:
    virtual void hasBeenPickedUp();
};

//GOLDNUGGET
class GoldNugget: public TemporaryGoodie
{
public:
    GoldNugget(int startX, int startY, StudentWorld *world, int pointValue = 10, bool isBribe = false);
    virtual ~GoldNugget(){}
    
    virtual bool isPickupable(){return !m_isBribe;}
private:
    virtual int calculateTicks(StudentWorld *world){return 100;}
    virtual bool shouldTick(){return getState() == STATE_TEMPORARY;}
    bool m_isBribe;
    
    virtual void hasBeenPickedUp();
    virtual void doGoodieRoutine();
};


//SONARKIT
class SonarKit: public TemporaryGoodie
{
public:
    SonarKit(StudentWorld *world);
    virtual ~SonarKit(){}
    
private:
    virtual bool shouldTick(){return true;}
    virtual void hasBeenPickedUp();
};

//WATERPOOL
class WaterPool: public TemporaryGoodie
{
public:
    WaterPool(int startX, int startY, StudentWorld *world);
    
private:
    virtual bool shouldTick(){return true;}
    virtual void hasBeenPickedUp();
};

//SQUIRT
class Squirt: public GameObject
{
public:
    Squirt(int startX, int startY, StudentWorld *world, GraphObject::Direction dir): GameObject(IID_WATER_SPURT, startX, startY, world, true, dir, 1.0, 1), m_travelDistance(4){}
    virtual ~Squirt(){}
    
    virtual void doSomething();
private:
    int m_travelDistance;
};

#endif // ACTOR_H_
