#include <iostream>
#include <string>
#include <sstream>
#include <regex>
using namespace std;

void ProcessReceivedMsg(char buffer[SIZE])
{
    string str(buffer);
    stringstream iss(str);
    string line;
    while (getline(iss, line))
    {
        if (line == "seat/ ")
        {
            cout << "===============receive seat message" << endl; 
        }
        else
        {
        
        }


        string first_token;
        string second_token;
        int pid;
        int jetton;
        int money;
        cout << "line: " << line << endl;

        if (line.substr(0, 4) == "seat" || line.substr(0, 4) == "/sea")
        {
            // do nothing
        }
        else if (line.substr(0, 4) == "butt")
        {
            stringstream line_iss(line);
            // button
            line_iss >> first_token >> pid >> jetton >> money;

            cout << pid << " " << jetton << " " << money << endl;
        }
        else if (line.substr(0, 4) == "smal")
        {
            stringstream line_iss(line);
            // small blind
            line_iss >> first_token >> second_token >> pid >> jetton >> money;

            cout << pid << " " << jetton << " " << money << endl;
        }
        else if (line.substr(0, 4) == "big ")
        {

            stringstream line_iss(line);
            // big blind
            line_iss >> first_token >> second_token >>  pid >> jetton >> money;


            cout << pid << " " << jetton << " " << money << endl;
        }
        else
        {

            stringstream line_iss(line);
            // other players
            line_iss >> pid >> jetton >> money;

            cout << pid << " " << jetton << " " << money << endl;
        }

    }
    return 0;
}
