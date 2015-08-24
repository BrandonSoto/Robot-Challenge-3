#pragma config(Sensor, S1,     fear_light,     sensorLightInactive)
#pragma config(Sensor, S2,     gradient_light, sensorLightActive)
#pragma config(Sensor, S3,     sonar_sensor,   sensorSONAR)
#pragma config(Motor,  motorB,          left_motor,    tmotorNXT, PIDControl, driveLeft, encoder)
#pragma config(Motor,  motorC,          right_motor,   tmotorNXT, PIDControl, driveRight, encoder)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#include <linklist.h>

// behavior states
#define CALIBRATE_STATE 0
#define WANDER_STATE 1
#define DETECTION_STATE 2
#define AVOIDANCE_STATE 3
#define FOLLOWING_STATE 4
#define FEEDING_STATE 5
#define ESCAPE_STATE 6
#define DEATH_STATE 7

// hunger states
#define HUNGRY_FULL 0
#define DANGER_HUNGRY 1
#define DEAD_DANGER 2
#define DEAD 3
#define ABS_FULL 4

// constants for sonar detection
#define INCH 22// represents about 1 in. from front of robot (in cm)
#define MAX_DISTANCE 94 // represents about three feet from front of robot (in cm)

// globals
int state = CALIBRATE_STATE; // robot's current behavior state (see above for possible states)
int hunger_state = HUNGRY_FULL; // robot's current hunger state (see above for possible states)
int lowest_color = 0;		// used to determine color threshold
int highest_color = 80;	// used for determine color threshold
int energy_level = 240;	// robot's current energy level; NOTE: should be 0 - 240
int touch_msg = 0; // message from other brick; NOTE: 0 = not bumped, 1 = R bump, 2 = L bump, 3 = Both bump
int dir = 1;
int fear_level = 4; // robot's current fear level
int runTime = 0;
int lrcount = 1900;
int count = 2000; //count for dual actions3
int fed = 0;	// indicates whether the robot has fed
tList listIR; //Linked List for holding sonar data
float average; //holds the average distance the sonar sensor has found
bool last_turn = false;
int last_time = 0;
int elapsed_time = 0;
bool right_direction = false; // true if the robot is heading in the right direction on the gradient
bool seen_gradient = false; // true if the robot has seen the gradient already. Otherwise false. 

// function prototypes
float averageDist(float x); 
void backup(void); 
void leftAction(void);
void rightAction(void);
void dualAction(void);
void zZz(void);
void z(void);

/**
Gets the touch sensor values from the other brick
**/
void getComm() {
	touch_msg = message;
}
/*
 * Returns true if an object at the given distance within the robot's distance. Otherwise false.
 */
bool isWithinRange(int distance) {
	return distance <= MAX_DISTANCE;
}

/*
 * Returns true if the robot is currently in a gradient. 
 */
bool foundGradient() {
	return SensorValue[gradient_light] > ((highest_color + lowest_color) / 2);
}

/*
 * Returns true if the robot has seen a light flash and adjusts the robot's fear level accordingly. 
 */
bool isLightFlash() {
	if (time1[T1] >= 60000 && fear_level < 4)
	{
		fear_level = fear_level + 1;
		clearTimer(T1);
	}
	else if (time1[T1] >= 60000)
	{
		clearTimer(T1);
	}

	return SensorValue(fear_light) > 75; // TO DO
}

//calculates the average distance.
float averageDist(float x){
	tListNode dist;
	if (x <= 255){  //eliminates values beyond sensor range
		dist.value = x;
		abs(x);
	}
	if (listIR.size < 8){ //if the list size is less than max size add the value to the list
		insertNode(listIR,listIR.tail, dist); //adds new node
	} else if(abs(average - x) < 20){ //prevents the adding of values greater than 20 beyond our last calculated value
		deleteNode(listIR, listIR.head);  //deletes the first node
		insertNode(listIR,listIR.tail, dist); //adds the new node
	}
	tListNode *curr; // pointer to current node in link list
	curr = listIR.head; //start current node at head of link list
	float sum = 0;
	for(int i = 0; i < listIR.size; i++){
		sum = sum + curr -> value;
		curr = curr -> next;
	}
	average = sum / listIR.size; //determines average value
	wait1Msec(100); //wait 100 ms
	return average; //returns the average value.
}

/*
 * Updates the robot's current energy state according to its behavior hierarchy.
 */
void update_state() {
	getComm();
	displayCenteredTextLine(1, "state: %d", state);
	if (hunger_state == DEAD) {
		state = DEATH_STATE;
	} else if (touch_msg != 0) {
		state = AVOIDANCE_STATE;
	} else if (hunger_state != DANGER_HUNGRY && isLightFlash()) {
		state = ESCAPE_STATE;
	} else if (hunger_state != DANGER_HUNGRY && isWithinRange(SensorValue[sonar_sensor])) {
		state = DETECTION_STATE;
	} else if (hunger_state != HUNGRY_FULL && foundGradient() && fed == 0) {
		state = FOLLOWING_STATE;
	} else if (hunger_state != ABS_FULL && fed == 1) {
		state = FEEDING_STATE;
	} else if (touch_msg == 0) {
		state = WANDER_STATE;
	}
	ClearMessage();
	return;

}

// focuses on determining hunger state and decrements it. 
// NOTE: executes every once every second
task update_hunger() {
	while (true) {
		if (energy_level >= 240) {
			hunger_state = ABS_FULL;
				seen_gradient = false;
		} else if (energy_level > 120 && energy_level < 240) {
				hunger_state = HUNGRY_FULL;
		} else if (energy_level <= 120 && energy_level > 60) {
				hunger_state = DANGER_HUNGRY;
		} else if (energy_level <= 60 && energy_level > 0) {
				hunger_state = DEAD_DANGER;
		} else {
				hunger_state = DEAD;
		}

		if (state != FEEDING_STATE) {
			energy_level--; 
		}
		
		sleep(1000);
	}
}

/*
 * Calibrates the robot's light sensors to determine the light threshold. 
 */
void callibrate() {
	lowest_color = 100;
	highest_color = 0;

	while(true) {
		getComm();
		if (touch_msg != 0) {
			break;
		}
	}
	touch_msg = 0;
	wait1Msec(1000);
	ClearMessage();

	setMotorSpeed(left_motor, 30);
	setMotorSpeed(right_motor, 30);

	while(true) {
		getComm();
		if (touch_msg != 0) {
			break;
		}
		if (SensorValue(gradient_light) < lowest_color) {
			lowest_color = SensorValue(gradient_light);
		}

		if (SensorValue(gradient_light) > highest_color) {
			highest_color = SensorValue(gradient_light);
		}
	}
	playSound(soundBeepBeep);
	setMotorSpeed(left_motor, 0);
	setMotorSpeed(right_motor, 0);

	sleep(1000);
	state = WANDER_STATE;
	clearTimer(T1);
	clearTimer(T2);
	clearTimer(T3);
	clearTimer(T4);
	return;
}

/*
 * Has the robot wander like a drunken sailor. 
 */
void wander() {
	seen_gradient = false;
	int turnRand, timeRand, turnLeft, turnRight;
	turnLeft = 19;
	turnRight = 80;
	while(true) {
	  turnRand = random(100);
	  timeRand = random(300);
	  if(turnRand <= turnLeft) { //left logic
	    setMotorSpeed(left_motor, random(30) + 20);
	    setMotorSpeed(right_motor, 20);
	    turnRight = turnRight - 5;
	    turnLeft = turnLeft - 5;
	  } else if(turnRand >= turnRight) { //right logic
	    setMotorSpeed(left_motor, 30);
	    setMotorSpeed(right_motor, random(30) + 20);
	    turnRight = turnRight + 5;
	    turnLeft = turnLeft + 5;
	  } else { //straight logic
			setMotorSpeed(left_motor, 20);
			setMotorSpeed(right_motor, 20);
	  }
	  wait1Msec(timeRand); //random delay to change
	  update_state();
	  return;
  }
}

/*
 * Has the robot turn with the direction depending on its last turn. 
 */
void turn() {
	while (!foundGradient()) {
		if (last_turn) { //turn right
			setMotorSpeed(left_motor, 40);
			setMotorSpeed(right_motor, -20);

		} else { // turn right
			setMotorSpeed(left_motor, -20);
			setMotorSpeed(right_motor, 40);

		}

	}
	return;
}

/*
 * Has the robot feed while on a patch. 
 */
void feeding() {
	fed = 1;
	if (time100[T2] >= 10) {
		energy_level++;
		clearTimer(T2);
		if(energy_level >= 240) {
			fed = 0;
			playTone();
			return;
		}
	}
	if (!foundGradient()) {
		last_turn ^= last_turn;
		setMotorSpeed(left_motor, 0);
		setMotorSpeed(right_motor, 0);
		turn();
	} else {
		int turnRand, timeRand, turnLeft, turnRight;
		turnLeft = 19;
		turnRight = 80;
		turnRand = random(100);
	  timeRand = random(300);
	  if(turnRand <= turnLeft) { //left logic
	    setMotorSpeed(left_motor, random(30) + 20);
	    setMotorSpeed(right_motor, 20);
	    turnRight = turnRight - 5;
	    turnLeft = turnLeft - 5;
	  } else if(turnRand >= turnRight) { //right logic
	    setMotorSpeed(left_motor, 30);
	    setMotorSpeed(right_motor, random(30) + 20);
	    turnRight = turnRight + 5;
	    turnLeft = turnLeft + 5;
	  } else { //straight logic
			setMotorSpeed(left_motor, 20);
			setMotorSpeed(right_motor, 20);
	  }
	  wait1Msec(timeRand);
	}
	return;
}

/*
 *	Has the robot follow a gradient and determines the right direction of the gradient. 
 */
void following() {
	if (foundGradient()) {
		setMotorSpeed(left_motor, 30);
		setMotorSpeed(right_motor, 30);
		//if (!seen_gradient) seen_gradient = true;
		clearTimer(T3);
	} else {
		last_time = elapsed_time;
		elapsed_time = time1[T3];
		clearTimer(T3);
		if (elapsed_time > last_time && last_time !=0) {
			//setMotorSpeed(left_motor, 0);
			//setMotorSpeed(right_motor, 0);
			turn();
			right_direction = true;
			clearTimer(T2);
			fed = 1;
		} else {
			last_turn ^= last_turn;
			//setMotorSpeed(left_motor, 0);
			//setMotorSpeed(right_motor, 0);
			turn();
			right_direction = false;
		}

	}
	return;
}

//Receives the touch sensor values.
void objectAvoidance() {
	while(true) {
		touch_msg = message;
	  if (touch_msg == 1) {
	  		backup();
	  		rightAction();
	  } else if (touch_msg == 2) {
	      backup();
	      leftAction();
	  } else if (touch_msg == 3) {
	  	  backup();
	  	  motor[left_motor] = 0 * dir;
	  		motor[right_motor] = 0 * dir;
	  		wait1Msec(2000); //delay code 2 seconds
	  	  dualAction();
	  } else {
  		update_state();
  	}
    wait1Msec(500);
    update_state();
    return;
  }

}

void leftAction() {
	dir = 1; //drive motor
	motor[left_motor] = 40 * dir;
	motor[right_motor] = 0 * dir;
	//delay code for roughly 1 second offset added to create a minumum turn time.
	lrcount = 1900;
	while (touch_msg == 0) {
		z();
		if(lrcount < 9) {
		  	lrcount = 1900;
		  	break;
		}

	}
	return;
}

void rightAction() {
	dir = 1; //drive motor
	motor[left_motor] = 0 * dir;
	motor[right_motor] = 40 * dir;
	//delay code for roughly 1 second offset added to create a minumum turn time.
	lrcount = 1900;
	while (touch_msg == 0) {
		z();
		if(lrcount < 9) {
		  	lrcount = 1900;
		  	break;
		}

	}
	return;
}
/*
 * The logic that controls action on both sides at the same time.
 */
void dualAction() {
	dir = 1;
	//turn random direction
	motor[left_motor] = random(70) * dir;
	motor[right_motor] = random(70) * dir;
	while (touch_msg == 0) {
		zZz();
		if(count == 0) {
			count = 2000;
			break;
		}
	}
	return;
}

/* 
 * Has the robot move backwards for 1 second. 
 */
void backup(){
	dir = -1; //reverse motor
	motor[left_motor] = 30 * dir;
	motor[right_motor] = 30 * dir;
	wait1Msec(1000); 	//delay code for 1 second
}

/*
Sleep with a non random count and allows intermittent polling.
*/
void zZz() {
	wait1Msec(10); //wait 10  milliseconds
	count = count - 10; //subtract 10 from count.
}

/*
Sleep with a random count and allows intermittent polling.
*/
void z() {
	if (lrcount == 1900) { //checks to see if we have already been in the loop.
		lrcount = 300 + random(900); //set the lrcount between 300 and 1200 to allow turn and prevent infinite turn.
	}
	wait1Msec(10); //wait 10 milliseconds
	lrcount = lrcount - 10; //subtract 10 from lrcount.
}

/*
 * Responsible for sonar detection. 
 */
void objectDetection() {
	//102.00 3ft
	//22.00 1 inch
	fed = 0;
	float x = SensorValue[sonar_sensor];
	float ave = averageDist(x); //average of distance
	while(ave <= MAX_DISTANCE && ave > INCH) {
		x = SensorValue[sonar_sensor];
		ave = averageDist(x);
	  setMotorSpeed(left_motor, ave - 18);
	  setMotorSpeed(right_motor, ave - 18);
  }
  
  if (ave <= INCH) {
  	setMotorSpeed(left_motor, 0);
	setMotorSpeed(right_motor, 0);
	playTone(300,20);
	wait1Msec(100); //100 ms delay
	playTone(300,50); //found object
	wait1Msec(3000); //3 sec delay
	setMotorSpeed(left_motor, -30); //reverse
	setMotorSpeed(right_motor, -30);
	wait1Msec(1000); //1 sec delay
	 int y = random(1);
	 
	if(y == 0) { //turn right
		setMotorSpeed(left_motor, -30);
		  setMotorSpeed(right_motor, 30);
		wait1Msec(1000);
	} else {     //turn left
		setMotorSpeed(left_motor, 30);
	  setMotorSpeed(right_motor, -30);
		wait1Msec(1000);
	}
  }
  return;
}

//turns robot off
void death() {
	powerOff();
}

/*
 * Responsible for robot's fear response. Has the robot back up, turn, and run. 
 */
void fear()
{
	fed = 0;
	runTime = fear_level * 1000;

	if (fear_level != 0)
	{
		fear_level = fear_level - 1;
	}

	//whatever turn around function we have
	//turnAway();
	backup();
	dualAction();

	//run for runTime at max speed
	setMotor(motorB, 100);
	setMotor(motorC, 100);
	wait1Msec(runTime);
	stopAllMotors();
	clearTimer(T1);

  return;
}

/*
 * Main thread of the program. Acts as an arbiter for robot behavior. 
 */
task main() {
	callibrate();
	startTask(update_hunger);
	while(true) {
		switch (state) {
			case CALIBRATE_STATE:
				callibrate();
				break;
			case WANDER_STATE:
			  wander();
				break;
			case DETECTION_STATE:
				objectDetection();
				break;
			case AVOIDANCE_STATE:
				objectAvoidance();
				break;
			case FOLLOWING_STATE:
				following();
				break;
			case FEEDING_STATE:
				feeding();
				break;
			case ESCAPE_STATE:
				fear();
				break;
			case DEATH_STATE:
				playSound(soundDownwardTones);
				death();
				break;
			default:
				break;
		}

		displayCenteredTextLine(1, "state: %d", state);
		displayCenteredTextLine(2, "lowest: %d", highest_color);
		displayCenteredTextLine(3, "highest: %d", lowest_color);
		displayCenteredTextLine(4, "ultrasonic: %f", SensorValue[sonar_sensor]);
		displayCenteredTextLine(5, "energy: %d", energy_level);
		displayCenteredTextLine(6, "hunger state: %d", hunger_state);
		update_state();
		sleep(10);
	}
}