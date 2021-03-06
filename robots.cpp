#include <libplayerc++/playerc++.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <math.h>
#include <vector>
#include <queue>

using namespace PlayerCc;
std::string  gHostname(PlayerCc::PLAYER_HOSTNAME);
uint32_t        gPort(6299);

#define PI_TWO 3.14159265358979323846 *2.
#define PI 3.14159265358979323846
#define PI_HALF 3.14159265358979323846 *.5
#define Square(x) ((x)*(x))
#define TOL .5 
#define EVADE_TURN 20.
#define STOP_RANGE .5 
#define STRAIGHT_MAX_SPEED .5
#define STRAIGHT_CRAWL_SPEED .1
#define STRAIGHT_TOLERANCE 0.1
#define TURN_MAX_SPEED 1. 
#define TURN_CRAWL_SPPED .01
#define TURN_TOLERANCE  .01
#define ROBOT_SPEED .3
#define DIST_TOL .1
#define MAX_SPEED .2
#define MAX_TURN 40.0

/*----------------------------------------------------------------------------------*/
class Collision
{
	public:
		Collision(Position2dProxy* pp, RangerProxy* lp); 
		//~Collision();
        virtual bool CheckMovement(void){};
        // Give a direction that has free space to move into
        virtual double DirectionOfFreeSpace(void){};
		double GetSpeed(){return(s);};
	protected:
        Position2dProxy* pos_proxy;
        RangerProxy* ranger;
		double s;
	private:
		Collision(){};
};
Collision::Collision(Position2dProxy* pp, RangerProxy* lp){
	pos_proxy=pp;
	ranger=lp;
}

/*----------------------------------------------------------------------------------*/
class Sonar: public Collision{
	public:
		Sonar(Position2dProxy* pos_proxy, RangerProxy* ranger);
		~Sonar();
		bool CheckMovement(double speed, double rotation);
		double DirectionOfFreeSpace(void);
};
Sonar::Sonar(Position2dProxy* pp, RangerProxy* lp):Collision(pp, lp){
	s=0.0;
}
Sonar::~Sonar(){
}
bool Sonar::CheckMovement(double speed, double rotation){
	double distL = ((*ranger)[1] + (*ranger)[0]);
 	double distR = ((*ranger)[2] + (*ranger)[0]);
	bool check = true;

	if((distL +TOL) < distR ) check = false;
    else

    if(distL > (distR+TOL)) check = false;
    else

    if((*ranger)[0] < STOP_RANGE) check = false;
    else check = true;
	return check;
}
double Sonar::DirectionOfFreeSpace(void){
	double turnrate=0.0;
	/*double distL = ((*ranger)[1] + (*ranger)[0]);
 	double distR = ((*ranger)[2] + (*ranger)[0]);

	if((distL +TOL) < distR ) turnrate = dtor(EVADE_TURN);
    else

    if(distL > (distR+TOL)) turnrate = dtor(-EVADE_TURN);
    else turnrate=0.0;

    if((*ranger)[0] < STOP_RANGE){
		if((*ranger)[3]>(*ranger)[4]) turnrate = dtor(-EVADE_TURN);
		else turnrate = dtor(EVADE_TURN);
    }
	return turnrate;*/
	if (((ranger->GetRange(0)+ranger->GetRange(3)+ranger->GetRange(4))/3)<1.0) double turnrate=2.0, s=0.0;
	else if (((ranger->GetRange(1)+ranger->GetRange(5))/2)<0.5) turnrate = 0.5, s=0.0;
	else if (((ranger->GetRange(2)+ranger->GetRange(5))/2)<0.5) turnrate = -0.5, s=0.0;
	else if (ranger->GetRange(1)<0.75&&ranger->GetRange(2)<0.75&&ranger->GetRange(5)<0.75) turnrate = 0.0, s=0.5;
	else if (((ranger->GetRange(0)+ranger->GetRange(1))/2)<0.5) turnrate = 0.5, s=0.0;
	else if (((ranger->GetRange(0)+ranger->GetRange(2))/2)<0.5) turnrate = -0.5, s=0.0;
	else if (((ranger->GetRange(1)+ranger->GetRange(3))/2)<0.25) turnrate = 0.5, s=0.0;
	else if (((ranger->GetRange(2)+ranger->GetRange(4))/2)<0.25) turnrate = -0.5, s=0.0;
	else if ((ranger->GetRange(3))<0.75) turnrate = 0.5, s=0.0;
	else if ((ranger->GetRange(4))<0.75) turnrate = -0.5, s=0.0;
	else if ((ranger->GetRange(0))<0.75) turnrate = 2.0, s=0.0;
	else if ((ranger->GetRange(0))>1.0) turnrate = 0.0, s=1.0;
	return turnrate;
}

/*----------------------------------------------------------------------------------*/
class Laser: public Collision{
	public:
		Laser(Position2dProxy* pos_proxy, RangerProxy* ranger);
		~Laser();
		bool CheckMovement(double speed, double rotation);
		double DirectionOfFreeSpace(void);
};
Laser::Laser(Position2dProxy* pp, RangerProxy* lp):Collision(pp, lp){
}
Laser::~Laser(){
}
bool Laser::CheckMovement(double speed, double rotation){
    if(ranger->GetMinRange()< STOP_RANGE) return false;
    else return true;
}
double Laser::DirectionOfFreeSpace(void){
	double turnrate=0.0;

    if(ranger->GetMinRange()< STOP_RANGE){
		if((*ranger)[3]>(*ranger)[4]) turnrate = dtor(-EVADE_TURN);
		else turnrate = dtor(EVADE_TURN);
    }
	return turnrate;
}

/*----------------------------------------------------------------------------------*/
class Behaviors
{
	public:
		Behaviors(Position2dProxy* pp); // Constructor
		//~Behaviors();
 		virtual void Init(void){}; // Overwritten by childeren to given them info
		virtual bool Tick(void) =0; //  Called every frame, true is Behavior complete
		virtual void Resume(void){}; // Resume after other behavior was done
 		double GetIntendedSpeed(){return(speed);};
		double GetIntendedRotation(){return(rotation);};
		void DoMove(void); //Perform the intended move (not overwriten by childen !!)
	protected:
    	double speed,rotation; // The intended speed in this frame
    	Position2dProxy* posProxy; // The position proxy to obain information !!
	private:
    	Behaviors(){}; // Private !!
};
Behaviors::Behaviors(Position2dProxy* pp){
	posProxy=pp;
}
//Behaviors::~Behaviors(){
//}
void Behaviors::DoMove(void){
	Tick();
}

class MoveStraight: public Behaviors {
	public:
		MoveStraight(Position2dProxy* pp);
		~MoveStraight();
		void Init(double d);
		bool Tick(void);
	private:
		double goal_x;
		double goal_y;
};
MoveStraight::MoveStraight(Position2dProxy* pp): Behaviors(pp){
	goal_x=0.;
	goal_y=0.;
}
MoveStraight::~MoveStraight(){
}
void MoveStraight::Init(double d){
	goal_x=posProxy->GetXPos() + d*cos(posProxy->GetYaw());
    goal_y=posProxy->GetYPos() + d*sin(posProxy->GetYaw());
    printf(" Straight Move to %f %f \n" ,goal_x,goal_y);
}
bool MoveStraight::Tick(void){
	double dist = sqrt(Square((goal_x - posProxy->GetXPos()))+
                        Square((goal_y - posProxy->GetYPos())));
	double speed;
    if(dist>1.) speed=STRAIGHT_MAX_SPEED;
    else speed= dist* STRAIGHT_MAX_SPEED +STRAIGHT_CRAWL_SPEED;
    posProxy->SetSpeed(speed,0.);
    if(dist<STRAIGHT_TOLERANCE) return(true);
    else return(false);
}

class Turn : public Behaviors{
	public:
		Turn(Position2dProxy* pp);
		~Turn();
		void Init(double a);
        bool Tick(void);
	private:
		double turn_to;
};
Turn::Turn(Position2dProxy* pp):Behaviors(pp){
	turn_to=0.;
}
Turn::~Turn(){
}
void Turn::Init(double a){
	turn_to=posProxy->GetYaw() + dtor(a);
    // NOTE GetYaw returns negative numbers for lower quadarants !!
    if(turn_to>PI)turn_to=turn_to-PI_TWO;
    printf(" Turn to %f \n",turn_to);
}
bool Turn::Tick(void){
	// Note, we do not  check for overshoot here!!
    double error=fabsf(turn_to-posProxy->GetYaw());
    double turn_rate;
    if(error>1.) turn_rate=TURN_MAX_SPEED;
    else turn_rate=TURN_MAX_SPEED*error+TURN_CRAWL_SPPED;
    posProxy->SetSpeed(0.,turn_rate);
    if(error<TURN_TOLERANCE) return(true);
    else return(false);
}

class Random: public Behaviors {
	public:
		Random(Position2dProxy* pp);
		~Random();
		void Init(void);
		bool Tick(void);
	private:
		double turn_dir_int;
		double d;
};
Random::Random(Position2dProxy* pp):Behaviors(pp){
	turn_dir_int=0.;
	d=0;
}
Random::~Random(){
}
void Random::Init(void){
	printf(" Move randomly for a bit\n");
}
bool Random::Tick(void){
	const int rand_num=rand();
	turn_dir_int = .9*turn_dir_int + 0.007*((double)(100-(rand_num%200)));
	posProxy->SetSpeed(ROBOT_SPEED,turn_dir_int);
	d=d+ROBOT_SPEED;
	if (d>50) return true;
	else return false;	
}

inline double angle_noral( double a ){
	while( a < -M_PI ) a += 2.0*M_PI;
	while( a >  M_PI ) a -= 2.0*M_PI;
	return a;
};
class MovePosition: public Behaviors {
	public:
		MovePosition(Position2dProxy* pp);
    	~MovePosition();
		void Init(player_pose2d_t move_to);
    	bool Tick(void);
	private:
		player_pose2d_t target;
};

MovePosition::MovePosition(Position2dProxy* pp):Behaviors(pp){
	target.px=0.;
	target.py=0.;
	target.pa=0.;
}
MovePosition::~MovePosition(){
}
void MovePosition::Init(player_pose2d_t move_to){
	target = move_to;
	printf(" Move to %f,%f \n",target.px,target.py);
}
bool MovePosition::Tick(void){
	double dist, angle;
	dist = sqrt(Square(target.px-posProxy->GetXPos())
				+Square(target.py-posProxy->GetYPos()));
	angle = atan2(target.py - posProxy->GetYPos(),target.px - posProxy->GetXPos());
	double yaw=posProxy->GetYaw(), ang_error=angle_noral(angle-yaw);
	double newspeed=0., newturnrate = limit(rtod(ang_error), -MAX_TURN, MAX_TURN);
	newturnrate = dtor(newturnrate);
	if (dist > DIST_TOL) newspeed = limit(dist * MAX_SPEED, -MAX_SPEED, MAX_SPEED);
	else newspeed = 0.0;
	if (fabs(newspeed) < 0.01){
		posProxy->SetSpeed(0.,0. );
		std::cout << "REACHED GOAL "<<std::endl;
		return(true);
	}
	posProxy->SetSpeed(newspeed, newturnrate);
	return(false);
}

class MoveNetwork: public Behaviors {
	public:
		MoveNetwork(Position2dProxy* pp);
    	~MoveNetwork();
		void Init(const char* name);
    	bool Tick(void);
	private:
		double distance, s_x, s_y;
		std::queue<double> position_queue; // implement a queue from the standard lib
		double position[2]; // create an array with x and y coordinates of destination
};
MoveNetwork::MoveNetwork(Position2dProxy* pp):Behaviors(pp){
	distance=0.;
	s_x=0.;
	s_y=0.;
}
MoveNetwork::~MoveNetwork(){
}
void MoveNetwork::Init(const char* name){
    std::ifstream readFile;
    readFile.open(name); // read text file
    std::string line;
    while(std::getline(readFile, line)){ // read lines in file
        std::istringstream numbers(line); // get numbers from text file
		double pos;
		while(numbers>>pos)	position_queue.push(pos); // put positions in queue
    }
	printf(" Move in a network of points for 2 times\n");
}
bool MoveNetwork::Tick(void){
    while(distance>=0){
		if(distance==0 && !position_queue.empty()){
			position[0] = position_queue.front(); // get first x position of queue
    		position_queue.pop(); // remove x position from top of queue
    		position[1] = position_queue.front(); // get first y position of queue
		    position_queue.pop(); // remove y position from top of queue
		    position_queue.push(position[0]); // put x position at end of the queue
    		position_queue.push(position[1]); // put y position at end of the queue	
		} // set destination
		s_x=position[0];
		s_y=position[1];
		double g_x = posProxy->GetXPos(), g_y = posProxy->GetYPos(), g_a = posProxy->GetYaw();
		distance = sqrt((s_x - g_x)*(s_x - g_x)+(s_y - g_y)*(s_y - g_y));
		double angle = atan2(s_y - g_y, s_x - g_x);
		double turn_rate = -1*(g_a-angle);
		if (turn_rate <= -1*M_PI) turn_rate = (turn_rate +(2*M_PI));
		else if (turn_rate >= M_PI) turn_rate = (turn_rate -(2*M_PI));
		if (distance>0.05)	posProxy->SetSpeed(distance/2,turn_rate);
		else distance=0;
		if (distance ==0) printf(" Reached %f,%f \n",position[0],position[1]);
		if(s_x==3.7 && s_y==-4.4) return true;
		else return false;
    }
}

class Wait: public Behaviors {
	public:
		Wait(Position2dProxy* pp);
		~Wait();
		void Init();
		bool Tick(void);
};
Wait::Wait(Position2dProxy* pp):Behaviors(pp){
	posProxy->SetSpeed(0,0);
}
Wait::~Wait(){
}
void Wait::Init(){
    posProxy->SetSpeed(0,0);
	printf(" Wait\n");
}
bool Wait::Tick(void){
    posProxy->SetSpeed(0,0);
	printf(" Wait\n");
}

/*----------------------------------------------------------------------------------*/
class Robot
{
	public:
        Robot(PlayerClient* client, int index); // Open the robot with a specific index
		void Tick(void); // Make the robot move
		player_pose2d_t GetPos(void); // Return current position of this robot
    protected:
        Position2dProxy* pos_proxy;
        RangerProxy* ranger;
		Behaviors* current_behavior;
	private:
		Robot(){};
};
Robot::Robot(PlayerClient* client, int index){
	Position2dProxy pp(client, index);
	pos_proxy = &pp;
	RangerProxy rp(client, index);
	ranger = &rp;
	pos_proxy->SetMotorEnable(true);
	ranger->RequestGeom();
	ranger->RequestConfigure();
}
void Robot::Tick(){
	if(!(current_behavior->Tick())) current_behavior->DoMove();
}
player_pose2d_t Robot::GetPos(){
	//pos_proxy->RequestGeom();
	//return pos_proxy->GetPose();
	player_pose2d_t pos;
	pos.px = pos_proxy->GetXPos();
	pos.py = pos_proxy->GetYPos();
	return pos;
}

class Robot_Move_Straight: public Robot{
	public:
		Robot_Move_Straight(PlayerClient* client, int index);
};
Robot_Move_Straight::Robot_Move_Straight(PlayerClient* client, int index):Robot(client, index){
	current_behavior = new MoveStraight(pos_proxy);
    ((MoveStraight*) current_behavior)->Init(10.0);
}

class Robot_Turn: public Robot{
	public:
		Robot_Turn(PlayerClient* client, int index);
};
Robot_Turn::Robot_Turn(PlayerClient* client, int index):Robot(client, index){
	current_behavior = new Turn(pos_proxy);
    ((Turn*) current_behavior)->Init(1.0);
}

class Robot_Random: public Robot{
	public:
		Robot_Random(PlayerClient* client, int index);
};
Robot_Random::Robot_Random(PlayerClient* client, int index):Robot(client, index){
	current_behavior = new Random(pos_proxy);
    ((Random*) current_behavior)->Init();
}

class Robot_Path: public Robot{
	public:
		Robot_Path(PlayerClient* client, int index);
};
Robot_Path::Robot_Path(PlayerClient* client, int index):Robot(client, index){
    current_behavior = new MoveStraight(pos_proxy);
    ((MoveStraight*) current_behavior)->Init(2.3);
	Collision* coll1;
	coll1 = new Sonar(pos_proxy, ranger);
}

class Robot_Points: public Robot{
	public:
		Robot_Points(PlayerClient* client, int index);
};
Robot_Points::Robot_Points(PlayerClient* client, int index):Robot(client, index){
	current_behavior = new MoveNetwork(pos_proxy);
    ((MoveNetwork*) current_behavior)->Init("position.txt");
}

class Robot_Wait: public Robot{
	public:
		Robot_Wait(PlayerClient* client, int index);
};
Robot_Wait::Robot_Wait(PlayerClient* client, int index):Robot(client, index){
	current_behavior = new Wait(pos_proxy);
    ((Wait*) current_behavior)->Init();
}

/*------------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	  // we throw exceptions on creation if we fail
	try{
		PlayerClient client(gHostname, gPort); // Conect to server
		client.Read();

		Robot_Points *my_robot;
		my_robot = new Robot_Points(&client,0);
		
		client.Read();

		for(;;){
			client.Read();
			my_robot->Tick();
		}
	}catch (PlayerCc::PlayerError & e){
		std::cerr << e << std::endl;
		return -1;
	}
}