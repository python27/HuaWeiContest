#include <iostream>
#include <string>
#include <sstream>
#include "message.h"
using namespace std;

void ProcessReceivedMsg(char buffer[512])
{
    string str(buffer);
    stringstream iss(str);
    string line;
    stringstream line_iss(line);
    while (getline(iss, line))
    {
        if (line == "seat/ ")
        {
            
            string first_token;
            string second_token;
            int pid;
            int jetton;
            int money;
            // read button line
            getline(iss, line);
            line_iss.str(line);
            line_iss >> first_token >> pid >> jetton >> money;

            cout << "I am button " << pid << " " << jetton << " " << money << endl;
            // read small blind line
            getline(iss, line);
            line_iss.str(line);
            line_iss >> first_token >> second_token >> pid >> jetton >> money;
            cout << "I am small blind " << pid << " " << jetton << " " << money << endl;
            // read next lines if any 
            while (1)
            {
                // end line break;
                getline(iss, line);
                if (line == "/seat ")
                {
                    cout << "i am end seat" << endl;
                    
                    break;
                }
                // big blind line
                else if (line.substr(0, 3) == "big")
                {
                     
                    line_iss.str(line);
                    line_iss >> first_token >> second_token >> pid >> jetton >> money;
                    cout << "i am big blind " << pid << " " << jetton << " " << money << endl;
                }
                else
                {
                    line_iss.str(line);
                    line_iss >> pid >> jetton >> money;
                    cout << "i am others " << pid << " " << jetton << " " << money << endl;
                }
            }


        }
        else
        {
            // do nothing 
        }

    }
    return;
}
