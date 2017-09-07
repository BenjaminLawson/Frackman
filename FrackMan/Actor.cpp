#include "Actor.h"
#include "StudentWorld.h"
#include "GameController.h"



/*
#############
GameObject
#############
*/


GameObject::GameObject(int imageID, int startX, int startY, StudentWorld *world, bool isVisible, Direction dir, double size, unsigned int depth, int state): GraphObject(imageID, startX, startY, dir, size, depth){
    setVisible(isVisible);
    m_state = state;
    m_world = world;
}

void GameObject::moveInFacingDirection(){
    switch (getDirection()) {
        case left:
            moveTo(getX() - 1, getY());
            break;
        case right:
            moveTo(getX() + 1, getY());
            break;
        case up:
            moveTo(getX(), getY() + 1);
            break;
        case down:
            moveTo(getX(), getY() - 1);
            break;
        default:
            break;
    }
}

/*
 #############
Person
 #############
 */

Person::Person(int imageID, int startX, int startY, int hitpoints, StudentWorld *world, Direction dir, double size, unsigned int depth): GameObject(imageID, startX, startY, world, true, dir, size, depth){
    m_hitpoints = m_maxHealth = hitpoints;
}

int Person::getHitpoints() const{return m_hitpoints;}

void Person::setHitpoints(int hp){m_hitpoints = hp;}

void Person::annoy(int amount, int cause){
    m_hitpoints -= amount;
    //TODO: should just be checked in other class?
    annoyResponse(m_hitpoints, cause);
}


/*
#############
FrackMan
#############
*/

FrackMan::FrackMan(StudentWorld *world): Person(IID_PLAYER, 30, 60, 10, world, right, 1.0, 0){
    m_nWater = 5;
    m_nCharge = 1;
    m_nGold = 0;
    
};

void FrackMan::doSomething(){
    //return immediately if frackman is dead
    if (getHitpoints() <= 0) {
        setState(STATE_DEAD);
        GameController::getInstance().playSound(SOUND_PLAYER_GIVE_UP);
        return;
    }
    
    //clear dirt in 4x4 area
    digDirt();
    
    //check for key press
    int ch;

    if (getWorld()->getKey(ch) == true) {
        
        switch (ch) {
            case KEY_PRESS_LEFT:
                if (getDirection() != left) {
                    setDirection(left);
                }
                else{
                    tryToMove(left);
                }
                break;
            case KEY_PRESS_RIGHT:
                if (getDirection() != right) {
                    setDirection(right);
                }
                else{
                    tryToMove(right);
                }
                break;
            case KEY_PRESS_UP:
                if (getDirection() != up) {
                    setDirection(up);
                }
                else{
                    tryToMove(up);
                }
                break;
            case KEY_PRESS_DOWN:
                if (getDirection() != down) {
                    setDirection(down);
                }
                else{
                    tryToMove(down);
                }
                break;
            case KEY_PRESS_TAB: //drop gold bribe
                dropGoldNugget();
                break;
            case KEY_PRESS_SPACE: //squirt
                useSquirtGun();
                break;
            case KEY_PRESS_ESCAPE: //die
                //TODO: should die on current or next tick?
                setHitpoints(0);
                break;
            case 'z':
            case 'Z':
                useSonar();
                break;
            default:
                break;
        }
    }
    
    //digDirt();
}

void FrackMan::useSquirtGun(){
    //check if out of water
    if (m_nWater > 0) {
        int x = getX();
        int y = getY();
        getWorld()->shiftPointInDirection(x, y, getDirection(), 4);
        //check that squirt spawn location is clear
        if (getWorld()->isClearOfDirt(x, y, 4) && !getWorld()->areObstaclesAt(x, y, 4)) {
            getWorld()->createSquirtWithDirectionAt(getDirection(), x, y);
        }
        m_nWater--;
        GameController::getInstance().playSound(SOUND_PLAYER_SQUIRT);
    }
}


void FrackMan::dropGoldNugget(){
    //check if out of gold
    if (m_nGold > 0) {
        m_nGold--;

        getWorld()->addBribeAt(getX(), getY());

    }
}

void FrackMan::useSonar(){
    //check if out of charges
    if (m_nCharge <= 0) {
        return;
    }
    
    m_nCharge--;
    
    getWorld()->setVisibleActorsWithinRadius(getX(), getY(), 12);
    
}

void FrackMan::digDirt(){
    int x = getX();
    int y = getY();
    bool didBreakDirt = false;
    
    //clear all dirt under frackman
    for (int i = x; i <= x + 3; i++) {
        for (int j = y; j <= y + 3; j++) {
            if (getWorld()->isDirtAt(i, j)) {
                getWorld()->clearDirtAt(i, j);
                didBreakDirt = true;
            }
        }
    }
    
    //if dirt was broken, play dig sound
    if (didBreakDirt) {
        GameController::getInstance().playSound(SOUND_DIG);
    }
}

void FrackMan::tryToMove(Direction d){
    int x = getX();
    int y = getY();
    switch (d) {
        case left:
            x--;
            break;
        case right:
            x++;
            break;
        case up:
            y++;
            break;
        case down:
            y--;
            break;
        default:
            break;
    }
    //check that location to move to is clear
    if (!getWorld()->areObstaclesAt(x, y, 4)) {
        moveTo(x, y);
    }
}

//PROTESTER

Protester::Protester(StudentWorld *world, int hitpoints, int imageID): Person(imageID, 60, 60, hitpoints, world, left){
    
    m_nSquaresToMoveInDirection = 0;
    
    //spec says min, should be max?
    m_ticksToWaitBetweenMoves = (0 > 3 - static_cast<int>(getWorld()->getLevel())/4) ? 0 : 3 - static_cast<int>(getWorld()->getLevel())/4;

    
    m_activeTicksSinceLastYell = 0;
    m_nTicksSinceLastTurn = 200;
    m_nRestingTicks = 0;
}





void Protester::doSomething(){
    if (getState() == STATE_DEAD) {
        return;
    }
    
    if (m_nRestingTicks > 0) { //RESTING
        m_nRestingTicks--;
        return;
    }
    
    m_nRestingTicks = m_ticksToWaitBetweenMoves;
    m_activeTicksSinceLastYell++;
    
    
    if (getState() == STATE_LEAVING_OIL_FIELD) {
        if (isAtExitPoint()) {
            setState(STATE_DEAD);
            getWorld()->decrProtesters();
        }
        else{
            setDirection(getWorld()->directionToMoveAt(getX(), getY()));
            moveInFacingDirection();
        }
        return;
    }
    
    if (getWorld()->frackManIsWithinRadius(getX(), getY(), 4) && isFacingFrackMan()) {
        if (activeTicksSinceLastYell() > 15) {
            yell();
            getWorld()->annoyFrackMan(2);
            return;
        }
    }

    //each type of protester has a unique part of their routine
    if(doSpecializedRoutine()){ //check if should return early
        //return immediately
        return;
    }
    
    
    if (isDirectClearPathToFrackMan() && !getWorld()->frackManIsWithinRadius(getX(), getY(), 4)) {
        //face frackman & move in direction 1
        int manX, manY;
        getWorld()->getFrackManPosition(manX, manY);
        if (getX() == manX) {
            if (getY() < manY) {
                setDirection(up);
                moveTo(getX(), getY() + 1);
            }
            else{
                setDirection(down);
                moveTo(getX(), getY() - 1);
            }
        }
        else if(getY() == manY){
            if (getX() < manX) {
                setDirection(right);
                moveTo(getX() + 1, getY());
            }
            else{
                setDirection(left);
                moveTo(getX() - 1, getY());
            }
        }
        resetNumSquaresToMoveInCurrentDirection();
        return;
    }
    
    //can't see frackman
    decrNumSquaresToMoveInDirection();
    if (getNumSquaresToMoveInDirection() <= 0) {
        //pick random direction to move
        Direction dir = none;
        
        do{
            int r = rand() % 4;
            switch (r) {
                case 0:
                    dir = up;
                    break;
                case 1:
                    dir = right;
                    break;
                case 2:
                    dir = down;
                    break;
                case 3:
                    dir = left;
                    break;
                default:
                    dir = right;
                    break;
            }
        } while (!getWorld()->directionIsClear(getX(), getY(), dir));
        
        setDirection(dir);
        
        pickNewNumSquaresToMove();
        
        
    }
    else if(isAtIntersection()){
        
        if (ticksSinceLastPerpendicularTurn() > 200) {
            Direction dir = none;
            if (getDirection() == left || getDirection() == right) {
                if (getWorld()->directionIsClear(getX(), getY(), up) && getWorld()->directionIsClear(getX(), getY(), down)) {
                    int r = rand() % 2;
                    if (r == 0) {
                        dir = up;
                    }
                    else{
                        dir = down;
                    }
                }
                else if(getWorld()->directionIsClear(getX(), getY(), up)){
                    dir = up;
                }
                else{
                    dir = down;
                }
            }
            else{
                if (getWorld()->directionIsClear(getX(), getY(), up) && getWorld()->directionIsClear(getX(), getY(), right)) {
                    int r = rand() % 2;
                    if (r == 0) {
                        dir = left;
                    }
                    else{
                        dir = right;
                    }
                }
                else if(getWorld()->directionIsClear(getX(), getY(), left)){
                    dir = left;
                }
                else{
                    dir = right;
                }
            }
            setDirection(dir);
            pickNewNumSquaresToMove();
            resetTicksSinceLastPerpendicularTurn();
        }
        
    }
    else{
        incrTicksSinceLastPerpendicularTurn();
    }
    
    //TODO: check num steps to move in direction
    
    if (getWorld()->directionIsClear(getX(), getY(), getDirection())) {
        moveInFacingDirection();
    }
    else{
        resetNumSquaresToMoveInCurrentDirection();
    }
    

}


bool Protester::isAtExitPoint(){
    return (getX() == 60 && getY() == 60);
}

void Protester::annoyResponse(int hp, int cause){
    if (hp > 0) {
        GameController::getInstance().playSound(SOUND_PROTESTER_ANNOYED);
        m_nRestingTicks = (50 < 100 - getWorld()->getLevel() * 10) ? 50 : 100 - getWorld()->getLevel() * 10;
        return;
    }
    
    if(getState() != STATE_LEAVING_OIL_FIELD){
        setState(STATE_LEAVING_OIL_FIELD);
        GameController::getInstance().playSound(SOUND_PROTESTER_GIVE_UP);
        m_nRestingTicks = 0;
        
        //if death by boulder, +500 points
        if (cause == CAUSE_BOULDER) {
            getWorld()->increaseScore(500);
        }
        //if death by squirt, +100 points (Regular)/ +250 (hardcore)
        else if (cause == CAUSE_SQUIRT) {
            getWorld()->increaseScore(getSquirtPoints());
        }
    }
    
}

bool Protester::isFacingFrackMan(){
    
    int manX, manY;
    getWorld()->getFrackManPosition(manX, manY);
    
    //if same position: technically facing frackman, so skip everything else
    if (getX() == manX && getY() == manY) {
        return true;
    }
    
    switch (getDirection()) {
        case up:
            if (getY() < manY && getX() >= manX - 4 && getX() <= manX + 4){ return true;}
            break;
        case down:
            if (getY() > manY && getX() >= manX - 4 && getX() <= manX + 4){ return true;}
            break;
        case right:
            if (getX() < manX && getY() >= manY - 4 && getY() <= manY + 4){ return true;}
            break;
        case left:
            if (getX() > manX && getY() >= manY - 4 && getY() <= manY + 4){ return true;}
        default:
            break;
    }
    
    return false;
}

void Protester::yell(){
    m_activeTicksSinceLastYell = 0;
    GameController::getInstance().playSound(SOUND_PROTESTER_YELL);
}


bool Protester::isDirectClearPathToFrackMan(){
    //same coordinate
    int manX, manY;
    getWorld()->getFrackManPosition(manX, manY);
    
    //y coord matches
    if (getY() == manY) {
        if (getX() < manX) {
            for (int i = getX(); i < manX; i++) {
                for (int j = 0; j <= 3; j++) {
                    if (getWorld()->isDirtAt(i, manY + j)) {
                        return false;
                    }
                }
            }
            return true;
        }
        else{ // >= manX
            for (int i = getX(); i > manX; i--) {
                for (int j = 0; j <= 3; j++) {
                    if (getWorld()->isDirtAt(i, manY + j)) {
                        return false;
                    }
                }
            }
            return true;
        }
    }
    
    //x coord matches
    if (getX() == manX) {
        if (getY() < manY) {
            for (int i = getY(); i < manY; i++) {
                for (int j = 0; j <= 3; j++) {
                    if (getWorld()->isDirtAt(manX + j, i)) {
                        return false;
                    }
                }
            }
            return true;
        }
        else{ // >= manY
            for (int i = getY(); i > manY; i--) {
                for (int j = 0; j <= 3; j++) {
                    if (getWorld()->isDirtAt(manX + j, i)) {
                        return false;
                    }
                }
            }
            return true;
        }
    }
    
    //don't have matching x/y coordinate, so there can't be line of sight
    return false;
}



void Protester::pickNewNumSquaresToMove(){
    int r = (rand() % 53) + 8;
    m_nSquaresToMoveInDirection = r;
}

bool Protester::isAtIntersection(){
    if (getDirection() == right || getDirection() == left) {
        if (getWorld()->directionIsClear(getX(), getY(), up)|| getWorld()->directionIsClear(getX(), getY(), down)) {
            return true;
        }
    }
    else if(getDirection() == up || getDirection() == down){
        if (getWorld()->directionIsClear(getX(), getY(), left) || getWorld()->directionIsClear(getX(), getY(), right)) {
            return true;
        }
    }
    return false;
}




/*
 #############
 REGULAR PROTESTER
 #############
 */

bool RegularProtester::doSpecializedRoutine(){
    //nothing to do
    return false;
}

void RegularProtester::takeBribe(){
    getWorld()->increaseScore(25);
    setState(STATE_LEAVING_OIL_FIELD);
}


/*
 #############
 HARDCORE PROTESTER
 #############
 */
void HardcoreProtester::takeBribe(){
    getWorld()->increaseScore(50);
    
    //protester becomes fixated
    int ticksToStare = (50 < 100 - getWorld()->getLevel()*10) ? 50 : 100 - getWorld()->getLevel()*10;
    restForTicks(ticksToStare);
    
}


bool HardcoreProtester::doSpecializedRoutine(){
    //search for frackman w/cell signal if distance > 4
    if (!getWorld()->frackManIsWithinRadius(getX(), getY(), 4)) {
        
        //TODO: number of steps includes radius?
        //int M = 12 + getWorld()->getLevel()*2;
        int M = 16 + getWorld()->getLevel()*2;
        

        
        if (getWorld()->numberOfMovesToFrackMan(getX(), getY()) <= M) {
            Direction dir = getWorld()->directionOfPathToFrackManAt(getX(), getY());
            setDirection(dir);
            moveInFacingDirection();
            return true;
        }
    }
    
    return false;
}



/*
 #############
Dirt
 #############
*/

//dirt doesn't do anything
void Dirt::doSomething(){}

/*
 #############
Boulder
 #############
*/



void Boulder::doSomething(){

    //check if dirt has been dug out under boulder
    if (getState() == STATE_ALIVE && !getWorld()->isDirtUnder(getX(), getY(), 4)) {
        setState(STATE_WAITING);
    }
    else if(getState() == STATE_WAITING){ //waiting to fall
        m_nTicks++;
        if (m_nTicks >= 30) {
            setState(STATE_FALLING);
            GameController::getInstance().playSound(SOUND_FALLING_ROCK);
        }
    }
    else if(getState() == STATE_FALLING){//falling
        //check if hits frackman
        if (getWorld()->frackManIsWithinRadius(getX(), getY(), 3)) {
            //kill frackman
            getWorld()->annoyFrackMan(100);
        }

        //kill protesters boulder lands on
        getWorld()->annoyProtestersWithinRadius(getX(), getY(), 3, 100, CAUSE_BOULDER);
        
        //boulder dies if collides with bottom, dirt, or other boulder
        if (getY() == 0 || getWorld()->isDirtUnder(getX(), getY(), 4) || getWorld()->isOtherBoulderAt(getX(), getY() - 1, this)) {
            setState(STATE_DEAD);
            
        }
        else {
            //move down 1
            moveTo(getX(), getY() - 1);
        }

    }
}

/*
 #############
 Goodie
 #############
 */
void Goodie::doSomething(){
    if (getState() == STATE_DEAD) {
        return;
    }
    
    if(!isDiscovered() && getWorld()->frackManIsWithinRadius(getX(), getY(), 4)){
        discoverGoodie();
        return;
    }
    
    if (isPickupable() && getWorld()->frackManIsWithinRadius(getX(), getY(), 3)) {
        //set state to dead
        setState(STATE_DEAD);
        hasBeenPickedUp();
        GameController::getInstance().playSound(m_sound);
        getWorld()->increaseScore(m_points);
    }
    
    doGoodieRoutine();
    
}

/*
 #############
 TEMPORARY GOODIE
 #############
 */


void TemporaryGoodie::doSomething(){
    Goodie::doSomething();
    
    if (shouldTick()) {
        m_nTicks--;
        if (m_nTicks <= 0) {
            setState(STATE_DEAD);
        }
    }
}

int TemporaryGoodie::calculateTicks(StudentWorld *world){
    return ((100 < 300 - 10*world->getLevel()) ? 100 : 300 - 10*world->getLevel());
}

/*
 #############
Barrel
 #############
 */
void Barrel::hasBeenPickedUp(){
    getWorld()->decrBarrels();
}


/*
 #############
GOLD NUGGET
 #############
 */
GoldNugget::GoldNugget(int startX, int startY, StudentWorld *world, int pointValue, bool isBribe): TemporaryGoodie(IID_GOLD, startX, startY, world, isBribe, pointValue){
    m_isBribe = isBribe;
    if (isBribe) {
        setState(STATE_TEMPORARY);
    }
    else{
        setState(STATE_PERMANENT);
    }
    
}



void GoldNugget::hasBeenPickedUp(){
    getWorld()->incrNuggets();
}

void GoldNugget::doGoodieRoutine(){
    if (m_isBribe && getWorld()->protesterWithinRadiusTakesBribe(getX(), getY(), 3)) {
        setState(STATE_DEAD);
        GameController::getInstance().playSound(SOUND_PROTESTER_FOUND_GOLD);
    }
}




/*
 #############
 SONAR KIT
 #############
 */
SonarKit::SonarKit(StudentWorld *world): TemporaryGoodie(IID_SONAR, 0, 60, world, true, 75){
    setState(STATE_TEMPORARY);
}

void SonarKit::hasBeenPickedUp(){
    getWorld()->incrSonar();
}

/*
 #############
 Water Pool
 #############
 */
WaterPool::WaterPool(int startX, int startY, StudentWorld *world): TemporaryGoodie(IID_WATER_POOL,startX, startY, world, true, 100){
    setState(STATE_TEMPORARY);
}

void WaterPool::hasBeenPickedUp(){
    getWorld()->incrWaterBy(5);
}

//SQUIRT
void Squirt::doSomething(){
    //check if squirt hits non-leaving protester, hits obstacle, or reaches max distance
    if(getWorld()->annoyProtestersWithinRadius(getX(), getY(), 3, 2, CAUSE_SQUIRT) || m_travelDistance <= 0 || !getWorld()->directionIsClear(getX(), getY(), getDirection())){
        setState(STATE_DEAD);
        return;
    }
    
    moveInFacingDirection();
    m_travelDistance--;
}
