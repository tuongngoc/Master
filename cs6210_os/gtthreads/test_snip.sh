!/bin/bash
#
# Author: Bhavin Thaker (bthaker3@gatech.edu)
#

make clean; make dining_main
OUTFILE=dining_main.OUT
rm -f $OUTFILE
./dining_main > $OUTFILE

NUM_PHIL0=`egrep eats $OUTFILE | sort  | egrep 0  | wc -l`
NUM_PHIL1=`egrep eats $OUTFILE | sort  | egrep 1  | wc -l`
NUM_PHIL2=`egrep eats $OUTFILE | sort  | egrep 2  | wc -l`
NUM_PHIL3=`egrep eats $OUTFILE | sort  | egrep 3  | wc -l`
NUM_PHIL4=`egrep eats $OUTFILE | sort  | egrep 4  | wc -l`

P0C0_PICKS=`egrep "Philosopher 0" dining_main.OUT  | egrep "picks up chopstick 0" | wc -l`
P0C0_PUTS=`egrep "Philosopher 0"  dining_main.OUT  | egrep "puts down chopstick 0" | wc -l`
P0C1_PICKS=`egrep "Philosopher 0" dining_main.OUT  | egrep "picks up chopstick 1" | wc -l`
P0C1_PUTS=`egrep "Philosopher 0"  dining_main.OUT  | egrep "puts down chopstick 1" | wc -l`

P1C1_PICKS=`egrep "Philosopher 1" dining_main.OUT  | egrep "picks up chopstick 1" | wc -l`
P1C1_PUTS=`egrep "Philosopher 1"  dining_main.OUT  | egrep "puts down chopstick 1" | wc -l`
P1C2_PICKS=`egrep "Philosopher 1" dining_main.OUT  | egrep "picks up chopstick 2" | wc -l`
P1C2_PUTS=`egrep "Philosopher 1"  dining_main.OUT  | egrep "puts down chopstick 2" | wc -l`

P2C2_PICKS=`egrep "Philosopher 2" dining_main.OUT  | egrep "picks up chopstick 2" | wc -l`
P2C2_PUTS=`egrep "Philosopher 2"  dining_main.OUT  | egrep "puts down chopstick 2" | wc -l`
P2C3_PICKS=`egrep "Philosopher 2" dining_main.OUT  | egrep "picks up chopstick 3" | wc -l`
P2C3_PUTS=`egrep "Philosopher 2"  dining_main.OUT  | egrep "puts down chopstick 3" | wc -l`

P3C3_PICKS=`egrep "Philosopher 3" dining_main.OUT  | egrep "picks up chopstick 3" | wc -l`
P3C3_PUTS=`egrep "Philosopher 3"  dining_main.OUT  | egrep "puts down chopstick 3" | wc -l`
P3C4_PICKS=`egrep "Philosopher 3" dining_main.OUT  | egrep "picks up chopstick 4" | wc -l`
P3C4_PUTS=`egrep "Philosopher 3"  dining_main.OUT  | egrep "puts down chopstick 4" | wc -l`

P4C4_PICKS=`egrep "Philosopher 4" dining_main.OUT  | egrep "picks up chopstick 4" | wc -l`
P4C4_PUTS=`egrep "Philosopher 4"  dining_main.OUT  | egrep "puts down chopstick 4" | wc -l`
P4C0_PICKS=`egrep "Philosopher 4" dining_main.OUT  | egrep "picks up chopstick 0" | wc -l`
P4C0_PUTS=`egrep "Philosopher 4"  dining_main.OUT  | egrep "puts down chopstick 0" | wc -l`

echo " "
echo "Philosopher0: #servings eaten=$NUM_PHIL0" 
echo "              #C0_picks=$P0C0_PICKS, #C0_puts=$P0C0_PUTS"
echo "              #C1_picks=$P0C1_PICKS, #C1_puts=$P0C1_PUTS"
echo "Philosopher1: #servings eaten=$NUM_PHIL1" 
echo "              #C1_picks=$P1C1_PICKS, #C1_puts=$P1C1_PUTS"
echo "              #C2_picks=$P1C2_PICKS, #C1_puts=$P1C2_PUTS"
echo "Philosopher2: #servings eaten=$NUM_PHIL2" 
echo "              #C2_picks=$P2C2_PICKS, #C2_puts=$P2C2_PUTS"
echo "              #C3_picks=$P2C3_PICKS, #C3_puts=$P2C3_PUTS"
echo "Philosopher3: #servings eaten=$NUM_PHIL3" 
echo "              #C3_picks=$P3C3_PICKS, #C3_puts=$P3C3_PUTS"
echo "              #C4_picks=$P3C4_PICKS, #C4_puts=$P3C4_PUTS"
echo "Philosopher4: #servings eaten=$NUM_PHIL4" 
echo "              #C4_picks=$P4C4_PICKS, #C4_puts=$P4C4_PUTS"
echo "              #C0_picks=$P4C0_PICKS, #C0_puts=$P4C0_PUTS"
echo " "