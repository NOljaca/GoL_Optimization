#include "GameOfLife.h"
#include "CLI.cpp"

int main(){
    int t = 0;
    if (t==1){
        /*
        * Here is where I did all the debugging just leaving it here in case I need again
        */

        //GameOfLife gof("../resources/world.txt");
        //GameOfLife gof(10000,10000);
        //gof.toggle_display();
        //gof.toggle_debug();
        //gof.addGlider(7,7);
        //gof.addToad(10,7);
        //gof.addBeacon(13,7);
        //gof.addMethuselah(19,7);
        //gof.randomize();
        //gof.randomize1();
        //gof.set_delay(200);
        //gof.run_simulation(10, "CL");
        //gof.save_game("10000world");
    } else {
        CLI test;
        test.start();
    }

    return 0;
}