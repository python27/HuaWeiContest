#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <algorithm>
#include <utility>
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
extern int g_PID;
/* global variable end */


vector<InquireInfo> g_current_inquireinfo;
int g_current_inquire_totalpot;

vector<SeatInfo> g_seatinfo;
map<int, int> g_PID2Index;


/******** Utility functions *********/
MyColor toMyColor(const string& color)
{
    MyColor ret;
    if (color == "SPADES")
        ret = SPADES;
    else if (color == "HEARTS")
        ret = HEARTS;
    else if (color == "CLUBS")
        ret = CLUBS;
    else if (color == "DIAMONDS")
        ret = DIAMONDS;
    else
        ;
    return ret;
}

int toPoint(const string& point)
{
    int ret = -1;
    if (point == "10") return 10;
    char ch = point[0];
    switch(ch)
    {
        case 'J': ret = 11; break;
        case 'Q': ret = 12; break;
        case 'K': ret = 13; break;
        case 'A': ret = 14; break;
        default:  ret = ch - '0'; break;
    }
    return ret;
}

int GetMaxbetInCurrentInquireInfo()
{
    vector<int> bets;
    for (size_t i = 0; i < g_current_inquireinfo.size(); ++i)
    {
        bets.push_back(g_current_inquireinfo[i].m_bet);
    }
    return *max_element(bets.begin(), bets.end());
}


bool HasRaiseInCurrentInquireInfo()
{
    for (size_t i = 0; i < g_current_inquireinfo.size(); ++i)
    {
        if (g_current_inquireinfo[i].m_action == "raise")
            return true;
    }

    return false;
}
/******** Utility functions *********/




/******************* send actions ********************/
void Check()
{
    memset(client_msg, 0, BUF_SIZE);
    sprintf(client_msg, "check \n");
    send(sockfd, client_msg, strlen(client_msg), 0); 
}

void Call()
{
    memset(client_msg, 0, BUF_SIZE);
    sprintf(client_msg, "call \n");
    send(sockfd, client_msg, strlen(client_msg), 0); 
}

void Raise(int num)
{
    memset(client_msg, 0, BUF_SIZE);
    sprintf(client_msg, "raise %d \n", num);
    send(sockfd, client_msg, strlen(client_msg), 0); 
}

void AllIn()
{
    memset(client_msg, 0, BUF_SIZE);
    sprintf(client_msg, "all_in \n");
    send(sockfd, client_msg, strlen(client_msg), 0); 
}

void Fold()
{
    memset(client_msg, 0, BUF_SIZE);
    sprintf(client_msg, "fold \n");
    send(sockfd, client_msg, strlen(client_msg), 0); 
}
/******************* send actions ********************/





/****************** My Action Strategy *****************/
void ActionStrategy()
{

    /******** preflop, hand cards *********/
    if (g_current_common_cards_num == 0)
    {
        int card1 = g_player_cards[0].m_point;
        int card2 = g_player_cards[1].m_point;
        MyColor color1 = g_player_cards[0].m_color;
        MyColor color2 = g_player_cards[1].m_color;
        if (card1 > card2) swap(card1, card2);
        // group 1: premium
        if (    (card1 == 14 && card2 == 14)   // AA
             || (card1 == 13 && card2 == 13)   // KK
             || (card1 == 12 && card2 == 12)   // QQ
             || (card1 == 13 && card2 == 14 && color1 == color2) // AKs
             || (card1 == 12 && card2 == 14 && color1 == color2) // AQs 
           )
        {
            int max_bet = GetMaxbetInCurrentInquireInfo();
            int raise_num = 5 * max_bet;
            Raise(raise_num);
        }
        // group 2: strong
        else if (    (card1 == 11 && card2 == 11) // JJ
                  || (card1 == 10 && card2 == 10) // TT
                  || (card1 == 11 && card2 == 14) // AJs
                  || (card1 == 10 && card2 == 14 && color1 == color2) // ATs
                  || (card1 == 13 && card2 == 14) // AK
                  || (card1 == 12 && card2 == 14) // AQ
                  || (card1 == 12 && card2 == 13 && color1 == color2) // KQs
                )
        {
            int max_bet = GetMaxbetInCurrentInquireInfo();
            int raise_num = 3 * max_bet;
            Raise(raise_num);
        }
        // group 3: good
        else if (    (card1 == 9 && card2 == 9)     // 99
                  || (card1 == 11 && card2 == 12 && color1 == color2) // QJs
                  || (card1 == 11 && card2 == 13 && color1 == color2) // KJs
                  || (card1 == 10 && card2 == 13 && color1 == color2) // KTs
                )
        {
            Call();
        }
        // others
        else
        {
            Fold();
        }
        return;
    }


    /********* 3 flop cards round ********/
    else if (g_current_common_cards_num == 3)
    {
        vector<MyCard> v(g_common_cards, g_common_cards + 3);
        v.push_back(g_player_cards[0]);
        v.push_back(g_player_cards[1]);
        sort(v.begin(), v.end());
        /** first case: complete **/
        // straight flush
        if ( v[0].m_point + 1 == v[1].m_point && v[1].m_point + 1 == v[2].m_point
             && v[2].m_point + 1 == v[3].m_point && v[3].m_point + 1 == v[4].m_point
             && v[0].m_color == v[1].m_color && v[0].m_color == v[2].m_color 
             && v[0].m_color == v[3].m_color && v[0].m_color == v[4].m_color)
        {
            int max_bet = GetMaxbetInCurrentInquireInfo();
            Raise(5 * max_bet);
        }
        // Quads
        else if (v[1].m_point == v[4].m_point || v[0].m_point == v[3].m_point)
        {
             if (HasRaiseInCurrentInquireInfo())
             {
                int max_bet = GetMaxbetInCurrentInquireInfo();
                Raise(4 * max_bet);
             }
             else
             {
                int max_bet = GetMaxbetInCurrentInquireInfo();
                Raise(2 * max_bet);
             }
        }
        // Full-House
        else if (v[0].m_point == v[1].m_point && v[2].m_point == v[4].m_point ||
                 v[0].m_point == v[2].m_point && v[3].m_point == v[4].m_point)
        {
            int max_bet = GetMaxbetInCurrentInquireInfo();
            Raise(3 * max_bet);
        }
        // Flush
        else if (v[0].m_color == v[1].m_color && v[1].m_color == v[2].m_color &&
                 v[2].m_color == v[3].m_color && v[3].m_color == v[4].m_color)
        {
            if (v[4].m_point == 14)
            {
                int max_bet = GetMaxbetInCurrentInquireInfo();
                Raise(1.1 * max_bet);
            }
            else
            {
                Call();
            }
        }
        // straight
        else if (v[0].m_point + 1 == v[1].m_point && v[1].m_point + 1 == v[2].m_point
                 && v[2].m_point + 1 == v[3].m_point && v[3].m_point + 1 == v[4].m_point)
        {
            Call(); 
        }

        /** Case 2: Has three of a kind **/
        else if (v[0].m_point == v[2].m_point || v[1].m_point == v[3].m_point
                 || v[2].m_point == v[4].m_point)
        {
            // has a pair in hand + 1 common card
            if (g_player_cards[0].m_point == g_player_cards[1].m_point)
            {
                int max_bet = GetMaxbetInCurrentInquireInfo();
                Raise(6 * max_bet / 5);
            }
            else
            {
                Call();
            }
        }

        /** Case 3: Straight or Flush Draw **/
        // Flush Draw
        else if (   v[1].m_color == v[2].m_color && v[2].m_color == v[3].m_color &&
                    v[3].m_color == v[4].m_color 
                ||  v[0].m_color == v[2].m_color && v[2].m_color == v[3].m_color &&
                    v[3].m_color == v[4].m_color
                ||  v[0].m_color == v[1].m_color && v[1].m_color == v[3].m_color &&
                    v[3].m_color == v[4].m_color
                ||  v[0].m_color == v[1].m_color && v[1].m_color == v[2].m_color &&
                    v[2].m_color == v[4].m_color
                ||  v[0].m_color == v[1].m_color && v[1].m_color == v[2].m_color &&
                    v[2].m_color == v[3].m_color
                )
        {
            // common cards already has flush
            if (g_common_cards[0].m_color == g_common_cards[1].m_color &&
                g_common_cards[1].m_color == g_common_cards[2].m_color)
            {
                Fold();
            }
            else
            {
                Call();
            }
        }
        /** Case 4: Has a Pair **/
        else if (v[3].m_point == v[4].m_point)
        {
            if (g_player_cards[0].m_point == g_player_cards[1].m_point &&
                g_player_cards[0].m_point == v[3].m_point)
            {
                Call();
            }
            else if (g_player_cards[0].m_point == v[2].m_point)
            {
                Call();
            }
            else
            {
                Fold();
            }
        }
        /** Case 5: nothing in common cards, high card in hand **/
        else if (g_player_cards[0].m_point == 13 && g_player_cards[1].m_point == 14)
        {
            if (g_common_cards[0].m_point == 13 || g_common_cards[0].m_point == 14 ||
                g_common_cards[1].m_point == 13 || g_common_cards[1].m_point == 14 ||
                g_common_cards[2].m_point == 13 || g_common_cards[2].m_point == 14)
            {
                Call();
            }
            else
            {
                Fold();
            }
        }
        else
        {
            Fold();
        }
    }
    /********* turn card round *******/
    else if (g_current_common_cards_num == 4)
    {
        Call();
    }
    /********* river card round *********/
    else if (g_current_common_cards_num == 5)
    {
        Call();
    }
    return;
}

int ProcessReceivedMsg(char buffer[BUF_SIZE])
{
    string str(buffer);
    stringstream iss(str);
    string line;
    stringstream line_iss(line);
    while (getline(iss, line))
    {
        /* seat info message, DONE */
        if (line == "seat/ ")
        {
            string first_token;
            string second_token;
            int pid;
            int jetton;
            int money;

            // clear previous seat info
            g_seatinfo.clear();
            g_PID2Index.clear();

            SeatInfo tmpseat;
            int tmpIndex = 0;

            // read button line
            getline(iss, line);
            line_iss.clear();
            line_iss.str(line);
            line_iss >> first_token >> pid >> jetton >> money;
            // push back
            tmpseat.m_islive = 1;
            tmpseat.m_pid = pid;
            tmpseat.m_jetton = jetton;
            tmpseat.m_money = money;
            g_seatinfo.push_back(tmpseat);
            g_PID2Index[pid] = tmpIndex++;


            // read small blind line
            getline(iss, line);
            line_iss.clear();
            line_iss.str(line);
            line_iss >> first_token >> second_token >> pid >> jetton >> money;
            // push back
            tmpseat.m_islive = 1;
            tmpseat.m_pid = pid;
            tmpseat.m_jetton = jetton;
            tmpseat.m_money = money;
            g_seatinfo.push_back(tmpseat);
            g_PID2Index[pid] = tmpIndex++;

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
                    // push back
                    tmpseat.m_islive = 1;
                    tmpseat.m_pid = pid;
                    tmpseat.m_jetton = jetton;
                    tmpseat.m_money = money;
                    g_seatinfo.push_back(tmpseat);
                    g_PID2Index[pid] = tmpIndex++;

                }
                else
                {
                    line_iss.clear();
                    line_iss.str(line);
                    line_iss >> pid >> jetton >> money;
                    // push back
                    tmpseat.m_islive = 1;
                    tmpseat.m_pid = pid;
                    tmpseat.m_jetton = jetton;
                    tmpseat.m_money = money;
                    g_seatinfo.push_back(tmpseat);
                    g_PID2Index[pid] = tmpIndex++;
                }
            }


        }

        /** game over message, DONE **/
        else if (line == "game-over ")
        {
            close(sockfd);
            return 1;
        }


        /** blind message **/
        else if (line == "blind/ ")
        {
            
        }

        /** hold cards message, DONE **/
        else if (line == "hold/ ")
        {
            string color; string point;
            
            // hold first card
            getline(iss, line);
            line_iss.clear(); 
            line_iss.str(line);
            line_iss >> color >> point;
            
            g_player_cards[0].m_color = toMyColor(color);
            g_player_cards[0].m_point = toPoint(point);

            // hold second card
            getline(iss, line);
            line_iss.clear(); 
            line_iss.str(line);
            line_iss >> color >> point;

            g_player_cards[1].m_color = toMyColor(color);
            g_player_cards[1].m_point = toPoint(point);
            
            sort(g_player_cards, g_player_cards + 2);
            g_current_common_cards_num = 0;

            // hold message end
            getline(iss, line);

        }

        /** inquire message **/
        else if (line == "inquire/ ")
        {
            // clear previous inquire info
            g_current_inquireinfo.clear();
            g_current_inquire_totalpot = 0;

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
                    // store current inquire total pot
                    g_current_inquire_totalpot = num;

                }
                else
                {
                    int pid, jetton, money, bet;
                    string action;
                    line_iss.clear();
                    line_iss.str(line);
                    line_iss >> pid >> jetton >> money >> bet >> action;

                    // store current inquire info
                    InquireInfo tmp;
                    tmp.m_pid = pid;
                    tmp.m_jetton = jetton;
                    tmp.m_money = money;
                    tmp.m_bet = bet;
                    tmp.m_action = action;

                    // update global seat info
                    int cur_index = g_PID2Index[pid];
                    assert(g_seatinfo[cur_index].m_pid == pid);
                    g_seatinfo[cur_index].m_jetton = jetton;
                    g_seatinfo[cur_index].m_money = money;
                    g_seatinfo[cur_index].m_bets.push_back(bet);
                    if (action == "fold")
                        g_seatinfo[cur_index].m_islive = 0;

                    // store current info
                    g_current_inquireinfo.push_back(tmp);
                    
                }
            }


            // send action message
            cout << "start action " << endl;
            ActionStrategy();
            cout << "end action "  << endl;
            
        }


        /** flop message **/
        else if (line == "flop/ ")
        {
            string color;
            string point;

            // first card
            getline(iss, line);
            line_iss.clear(); 
            line_iss.str(line);
            line_iss >> color >> point;

            g_common_cards[0].m_color = toMyColor(color);
            g_common_cards[0].m_point = toPoint(point);

            // second card
            getline(iss, line);
            line_iss.clear(); 
            line_iss.str(line);
            line_iss >> color >> point;

            g_common_cards[1].m_color = toMyColor(color);
            g_common_cards[1].m_point = toPoint(point);

            // third card
            getline(iss, line);
            line_iss.clear();     
            line_iss.str(line);
            line_iss >> color >> point;

            g_common_cards[2].m_color = toMyColor(color);
            g_common_cards[2].m_point = toPoint(point);

            sort(g_common_cards, g_common_cards + 3);
            g_current_common_cards_num = 3;

            // end messge
            getline(iss, line);

        }


        /** turn card message **/
        else if (line == "turn/ ")
        {
            string color;
            string point;
            getline(iss, line);
            line_iss.clear(); 
            line_iss.str(line);
            line_iss >> color >> point;
            
            g_common_cards[3].m_color = toMyColor(color);
            g_common_cards[3].m_point = toPoint(point);

            sort(g_common_cards, g_common_cards + 4);
            g_current_common_cards_num = 4;

            getline(iss, line);
        }

        /** river card message **/
        else if (line == "river/ ")
        {
            string color;
            string point;
            getline(iss, line);
            line_iss.clear(); 
            line_iss.str(line);
            line_iss >> color >> point;

            g_common_cards[4].m_color = toMyColor(color);
            g_common_cards[4].m_point = toPoint(point);
            
            sort(g_common_cards, g_common_cards + 5);
            g_current_common_cards_num = 5;

            getline(iss, line);
        }


        /** shut down message **/
        else if (line == "showdown/ ")
        {
            // common start 
            getline(iss, line);
            for (int i = 0; i < 5; ++i)
            {
                // five common message
                string color; string point;
                getline(iss, line);
                line_iss.clear(); 
                line_iss.str(line);
                line_iss >> color >> point;
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

                }
            }
        }


        /** pot win message **/ 
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
