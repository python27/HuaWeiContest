#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sstream>
#include "message.h"
#include "global_struct.h"
using namespace std;

#define BUF_SIZE 1024

/* global variable start */
extern char client_msg[BUF_SIZE];
extern int sockfd;
extern MyCard g_common_cards[5];
extern int g_current_common_cards_num;
extern MyCard g_player_cards[2];
/* global variable end */


int ProcessReceivedMsg(char buffer[BUF_SIZE])
{
    string str(buffer);
    stringstream iss(str);
    string line;
    stringstream line_iss(line);
    while (getline(iss, line))
    {
        /* seat info message */
        if (line == "seat/ ")
        {
            
            string first_token;
            string second_token;
            int pid;
            int jetton;
            int money;
            // read button line
            getline(iss, line);
            line_iss.clear();
            line_iss.str(line);
            line_iss >> first_token >> pid >> jetton >> money;

            //cout << "receive >>>>>>>>>>>>" << "button:" << pid << " " << jetton << " " << money << endl;

            // read small blind line
            getline(iss, line);
            line_iss.clear();
            line_iss.str(line);
            line_iss >> first_token >> second_token >> pid >> jetton >> money;
            //cout << "receive >>>>>>>>>>>>" << "small blind:" << pid << " " << jetton << " " << money<< endl;
            // read next lines if any 
            while (1)
            {
                // end line break;
                getline(iss, line);
                if (line == "/seat ")
                {
                    break;
                }
                // big blind line
                else if (line.substr(0, 3) == "big")
                {
                    line_iss.clear(); 
                    line_iss.str(line);
                    line_iss >> first_token >> second_token >> pid >> jetton >> money;
            //cout << "receive >>>>>>>>>>>>" << "big blind:" << pid << " " << jetton << " " << money << endl;
                }
                else
                {
                    line_iss.clear();
                    line_iss.str(line);
                    line_iss >> pid >> jetton >> money;
                    
                    //cout << "receive >>>>>>>>>>>>" << "other player:" << pid << " " << jetton << " "<<  money << endl;
                }
            }


        }
        /* game over message */
        else if (line == "game-over ")
        {
            close(sockfd);
            return 1;
        }
        /* blind message */
        else if (line == "blind/ ")
        {
            
        }
        /* hold cards message */
        else if (line == "hold/ ")
        {
            string color; string point;
            
            // hold first card
            getline(iss, line);
            line_iss.clear(); 
            line_iss.str(line);
            line_iss >> color >> point;
            //cout << "receive >>>>>>>>>>>>" <<"hold first card:" << color << " " << point << endl;

            // hold second card
            getline(iss, line);
            line_iss.clear(); 
            line_iss.str(line);
            line_iss >> color >> point;

            //cout << "receive >>>>>>>>>>>>" <<"hold second card:" << color << " " << point << endl;
            
            // hold message end
            getline(iss, line);
        }
        /* inquire message */
        else if (line == "inquire/ ")
        {
            while (1)
            {
                getline(iss, line);
                if (line == "/inquire ")
                {
                    break;
                }
                else if (line.substr(0, 5) == "total")
                {
                    string total_str, pot_str;
                    int num;
                    line_iss.clear();
                    line_iss.str(line);
                    line_iss >> total_str >> pot_str >> num;

                    //cout << "receive >>>>>>>>>>>>" <<"inquire total:" << pot_str << " " << num << endl;
                }
                else
                {
                    int pid, jetton, money, bet;
                    string action;
                    line_iss.clear();
                    line_iss.str(line);
                    line_iss >> pid >> jetton >> money >> bet >> action;
                    //cout << "receive >>>>>>>>>>>>" <<"inquire others:" << pid << " " << jetton << " " <<  money << " " << bet << " " << action << endl;
                }
            }

            // send action message
            memset(client_msg, 0, BUF_SIZE);
            sprintf(client_msg, "all_in \n");
            send(sockfd, client_msg, strlen(client_msg), 0); 
            
        }
        /* flop message */
        else if (line == "flop/ ")
        {
            string color;
            string point;
            // first card
            getline(iss, line);
            line_iss.clear(); 
            line_iss.str(line);
            line_iss >> color >> point;
            //cout << "common card 1:" << color << " " << point << endl;
            // second card
            getline(iss, line);
            line_iss.clear(); 
            line_iss.str(line);
            line_iss >> color >> point;
            //cout << "common card 2:" << color << " " << point << endl;
            // third card
            getline(iss, line);
            line_iss.clear();     
            line_iss.str(line);
            line_iss >> color >> point;
            //cout << "common card 3:" << color << " " << point << endl;
            // end messge
            getline(iss, line);

        }
        /* turn card message */
        else if (line == "turn/ ")
        {
            string color;
            string point;
            getline(iss, line);
            line_iss.clear(); 
            line_iss.str(line);
            line_iss >> color >> point;
            //cout << "common card 4:" << color << " " << point << endl;
            getline(iss, line);
        }
        else if (line == "river/ ")
        {
            string color;
            string point;
            getline(iss, line);
            line_iss.clear(); 
            line_iss.str(line);
            line_iss >> color >> point;
            cout << "common card 5:" << color << " " << point << endl;
            getline(iss, line);
        }
        else if (line == "showdown/ ")
        {
            // common start 
            getline(iss, line);
            for (int i = 0; i < 5; ++i)
            {
                // five common message
                string color; char point;
                getline(iss, line);
                line_iss.clear(); 
                line_iss.str(line);
                line_iss >> color >> point;

                //cout << "showdown common card "  << i + 1 << ":"<< color << " " << point << endl;
            }
            // common end
            getline(iss, line);

            // rank message
            while (1)
            {
                getline(iss, line);
                if (line == "/showdown ")
                {
                    break;
                }
                else
                {
                    string rank_str;
                    int pid;
                    string color1, color2;
                    string point1, point2;
                    string nut_hand;
                    line_iss.clear();
                    line_iss.str(line);
                    line_iss >> rank_str >> pid >> color1 >> point1 >> color2 >> point2 >> nut_hand;
                    //cout << "showdown rank:" << pid << " " << color1 << " " << color2 << " " << point2 << " " << nut_hand << endl;

                }
            }
        }
        /* pot win message */ 
        else if (line == "pot-win/ ")
        {
            while (1)
            {
                getline(iss, line);
                if (line == "/pot-win ")
                {
                    break;
                }
                else
                {
                    string comma_str;
                    int pid, num;
                    line_iss.clear();
                    line_iss.str(line);
                    line_iss >> pid >> comma_str >> num; 
                    //cout << "pot win:" << pid << " " << num << endl;
                }
            }
        }
        else
        {
            // do nothing
        }

    }
    return 0;
}
