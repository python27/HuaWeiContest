#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <map>
#include <set>
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
const double MAX_RR = 1.3;
const double MIN_RR = 1.0;

int g_initial_bet_cnt = 0;
int g_flop_bet_cnt = 0;
int g_turn_bet_cnt = 0;
int g_river_bet_cnt = 0;
const int g_InitialHandcardType = 3;
const double g_alpha = 0.8;
const double g_beta = 0.2;
int g_my_current_money = 0;                 // 我的当前资金
int g_my_current_jetton = 0;                // 我的当前筹码
const int g_MIN_PROTECT_JETTON = 1500;      // 最少保护筹码
bool g_protected_jetton_flag = false;
/* global variable end */



vector<InquireInfo> g_current_inquireinfo;
int g_current_inquire_totalpot;

vector<SeatInfo> g_seatinfo;
map<int, int> g_PID2Index;
map<int, PlayerType> g_PlayerBehavior;

/* function declaration */
NUT_HAND GetHandCardType(const vector<MyCard>& v);

/******** Utility functions *********/

int GetMyCurrentTotalMoney()
{
    int idx = g_PID2Index[g_PID];
    return g_seatinfo[idx].m_money + g_seatinfo[idx].m_jetton;
}


bool InKnownCardInFlop(const MyCard& curCard)
{
    if (curCard.m_point == g_player_cards[0].m_point && curCard.m_color == g_player_cards[0].m_color || 
        curCard.m_point == g_player_cards[1].m_point && curCard.m_color == g_player_cards[1].m_color || 
        curCard.m_point == g_common_cards[0].m_point && curCard.m_color == g_common_cards[0].m_color || 
        curCard.m_point == g_common_cards[1].m_point && curCard.m_color == g_common_cards[1].m_color || 
        curCard.m_point == g_common_cards[2].m_point && curCard.m_color == g_common_cards[2].m_color)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int CmpTwoPlayerCardsInFlop(vector<MyCard>& p1, vector<MyCard>& p2)
{
    NUT_HAND type1 = GetHandCardType(p1);
    NUT_HAND type2 = GetHandCardType(p2);
    if (type1 > type2) return 1;
    else if (type1 < type2) return -1;
    else
    {
        if      (p1[4].m_point > p2[4].m_point) return 1;
        else if (p1[4].m_point < p2[4].m_point) return -1;
        else if (p1[3].m_point > p2[3].m_point) return 1;
        else if (p1[3].m_point < p2[3].m_point) return -1;
        else if (p1[2].m_point > p2[2].m_point) return 1;
        else if (p1[2].m_point < p2[2].m_point) return -1;
        else if (p1[1].m_point > p2[1].m_point) return 1;
        else if (p1[1].m_point < p2[1].m_point) return -1;
        else if (p1[0].m_point > p2[0].m_point) return 1;
        else if (p1[0].m_point < p2[0].m_point) return -1;
        else return 0;
    }
}

// 从cards中选择所有的5张最好的牌
void ChooseKCardsFromNCards(vector<MyCard>& cards, int offset, int k, vector<MyCard>& cur_cards, vector<MyCard>& best_cards)
{
    int n = cards.size();
    if (k == 0)
    {
        sort(cur_cards.begin(), cur_cards.end());
        sort(best_cards.begin(), best_cards.end());
        if (CmpTwoPlayerCardsInFlop(cur_cards, best_cards) >= 0)
        {
            copy(cur_cards.begin(), cur_cards.end(), best_cards.begin());     
        }
        return;
    }

    for (int i = offset; i <= n-k; ++i)
    {
       // cout << "DEBUG: recursion; offset = " << i << endl;
       // cout << "DEBUG: recursion; n = " << n << endl;
       // cout << "DEBUG: recursion; k = " << k << endl;
       // cout << "DEBUG: recursion; n - k = " << n - k << endl;
        cur_cards.push_back(cards[i]);
        ChooseKCardsFromNCards(cards, i+1, k-1, cur_cards, best_cards);
        cur_cards.pop_back();
    }
}

// input 2 hand cards, return the best 5 cards in turn stage
vector<MyCard> ChooseBestNutHandCardsInTurn(const MyCard& card1, const MyCard& card2)
{
    vector<MyCard> v;
    v.push_back(card1);
    v.push_back(card2);
    v.push_back(g_common_cards[0]);
    v.push_back(g_common_cards[1]);
    v.push_back(g_common_cards[2]);
    v.push_back(g_common_cards[3]);

    //cout << "DEBUG: choose best nut hand given two cards" << endl;

    sort(v.begin(), v.end());
    // init
    vector<MyCard> best_cards(v.begin(), v.begin() + 5);
    vector<MyCard> cur_cards;
    ChooseKCardsFromNCards(v, 0, 5, cur_cards, best_cards);
    return best_cards;  
}


// input 2 hand cards, return the best 5 cards in river stage
vector<MyCard> ChooseBestNutHandCardsInRiver(const MyCard& card1, const MyCard& card2)
{
    vector<MyCard> v;
    v.push_back(card1);
    v.push_back(card2);
    v.push_back(g_common_cards[0]);
    v.push_back(g_common_cards[1]);
    v.push_back(g_common_cards[2]);
    v.push_back(g_common_cards[3]);
    v.push_back(g_common_cards[4]);

    sort(v.begin(), v.end());
    // init
    vector<MyCard> best_cards(v.begin(), v.begin() + 5);
    vector<MyCard> cur_cards;
    ChooseKCardsFromNCards(v, 0, 5, cur_cards, best_cards);
    return best_cards;
}

bool InKnownCardInTurn(const MyCard& card)
{
    if (card.m_point == g_player_cards[0].m_point && card.m_color == g_player_cards[0].m_color ||
        card.m_point == g_player_cards[1].m_point && card.m_color == g_player_cards[1].m_color ||
        card.m_point == g_common_cards[0].m_point && card.m_color == g_common_cards[0].m_color ||
        card.m_point == g_common_cards[1].m_point && card.m_color == g_common_cards[1].m_color ||
        card.m_point == g_common_cards[2].m_point && card.m_color == g_common_cards[2].m_color ||
        card.m_point == g_common_cards[3].m_point && card.m_color == g_common_cards[3].m_color)
    {
        return true;
    }
    else
    {
        return false;
    }
}

double WinProbabilityInTurn()
{
 
    // get my best nut hand in turn
    vector<MyCard> player_best_cards = ChooseBestNutHandCardsInTurn(g_player_cards[0], g_player_cards[1]);
    sort(player_best_cards.begin(), player_best_cards.end());
    
    // whole pokers
    vector<MyCard> v;
    for (int i = 2; i <= 14; ++i)
    {
        MyCard tmp;
        tmp.m_point = i;
        tmp.m_color = SPADES;
        v.push_back(tmp);
        tmp.m_point = i;
        tmp.m_color = HEARTS;
        v.push_back(tmp);
        tmp.m_point = i;
        tmp.m_color = CLUBS;
        v.push_back(tmp);
        tmp.m_point = i;
        tmp.m_color = DIAMONDS;
        v.push_back(tmp);
    }

    int win_cnt = 0;
    int lose_cnt = 0;
    int tot_cnt = 0;
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (!InKnownCardInTurn(v[i]))
        {
            for (size_t j = i + 1; j < v.size(); ++j)
            {
                if (!InKnownCardInTurn(v[j]))
                {
                    vector<MyCard> other_best_cards = ChooseBestNutHandCardsInTurn(v[i], v[j]);
                    
                    
                    sort(other_best_cards.begin(), other_best_cards.end()); 
                    if (CmpTwoPlayerCardsInFlop(player_best_cards, other_best_cards) >= 0)
                    {
                        win_cnt++;
                    }
                    else
                    {
                        lose_cnt++;
                    }
                    
                }
            }
        }
    }
    tot_cnt = win_cnt + lose_cnt;

    double oneplayerWinProb = double(win_cnt) / (double)(win_cnt + lose_cnt);

    int seat_num = g_seatinfo.size();
    int player_num = seat_num - 1; 

    double ans = 1.0;
    double w = (double)win_cnt;
    double t = (double)tot_cnt;
    for (int i = 0; i < player_num; ++i)
    {
        ans *= w / t;
        w -= 1.0;
        t -= 1.0;
    }

    return ans;
}


bool InKnownCardInRiver(const MyCard& card)
{
    if (card.m_point == g_player_cards[0].m_point && card.m_color == g_player_cards[0].m_color ||
        card.m_point == g_player_cards[1].m_point && card.m_color == g_player_cards[1].m_color ||
        card.m_point == g_common_cards[0].m_point && card.m_color == g_common_cards[0].m_color ||
        card.m_point == g_common_cards[1].m_point && card.m_color == g_common_cards[1].m_color ||
        card.m_point == g_common_cards[2].m_point && card.m_color == g_common_cards[2].m_color ||
        card.m_point == g_common_cards[3].m_point && card.m_color == g_common_cards[3].m_color ||
        card.m_point == g_common_cards[4].m_point && card.m_color == g_common_cards[4].m_color  )
    {
        return true;
    }
    else
    {
        return false;
    }
}
double WinProbabilityInRiver()
{

    // get my best nut hand in river
    vector<MyCard> player_best_cards = ChooseBestNutHandCardsInRiver(g_player_cards[0], g_player_cards[1]);
    
    // whole pokers
    vector<MyCard> v;
    for (int i = 2; i <= 14; ++i)
    {
        MyCard tmp;
        tmp.m_point = i;
        tmp.m_color = SPADES;
        v.push_back(tmp);
        tmp.m_point = i;
        tmp.m_color = HEARTS;
        v.push_back(tmp);
        tmp.m_point = i;
        tmp.m_color = CLUBS;
        v.push_back(tmp);
        tmp.m_point = i;
        tmp.m_color = DIAMONDS;
        v.push_back(tmp);
    }

    int win_cnt = 0;
    int lose_cnt = 0;
    int tot_cnt = 0;
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (!InKnownCardInRiver(v[i]))
        {
            for (size_t j = i + 1; j < v.size(); ++j)
            {
                if (!InKnownCardInRiver(v[j]))
                {
                    vector<MyCard> other_best_cards = ChooseBestNutHandCardsInRiver(v[i], v[j]);
                    
                    sort(player_best_cards.begin(), player_best_cards.end());
                    sort(other_best_cards.begin(), other_best_cards.end()); 
                    if (CmpTwoPlayerCardsInFlop(player_best_cards, other_best_cards) >= 0)
                    {
                        win_cnt++;
                    }
                    else
                    {
                        lose_cnt++;
                    }
                    
                }
            }
        }
    }
    tot_cnt = win_cnt + lose_cnt;

    double oneplayerWinProb = double(win_cnt) / (double)(win_cnt + lose_cnt);

    int seat_num = g_seatinfo.size();
    int player_num = seat_num - 1; 

    double ans = 1.0;
    double w = (double)win_cnt;
    double t = (double)tot_cnt;
    for (int i = 0; i < player_num; ++i)
    {
        ans *= w / t;
        w -= 1.0;
        t -= 1.0;
    }

    return ans;
    
}







double WinProbabilityInFlop()
{
    vector<MyCard> known_cards;
    known_cards.push_back(g_player_cards[0]);
    known_cards.push_back(g_player_cards[1]);
    known_cards.push_back(g_common_cards[0]);
    known_cards.push_back(g_common_cards[1]);
    known_cards.push_back(g_common_cards[2]);
    sort(known_cards.begin(), known_cards.end());
    NUT_HAND known_card_type = GetHandCardType(known_cards);

    vector<MyCard> v;
    for (int i = 2; i <= 14; ++i)
    {
        MyCard tmp;
        tmp.m_point = i;
        tmp.m_color = SPADES;
        v.push_back(tmp);
        tmp.m_point = i;
        tmp.m_color = HEARTS;
        v.push_back(tmp);
        tmp.m_point = i;
        tmp.m_color = CLUBS;
        v.push_back(tmp);
        tmp.m_point = i;
        tmp.m_color = DIAMONDS;
        v.push_back(tmp);
    }
    
    int win_cnt = 0;
    int lose_cnt = 0;
    int tot_cnt = 0;
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (!InKnownCardInFlop(v[i]))
        {
            for (size_t j = i + 1; j < v.size(); ++j)
            {
                if (!InKnownCardInFlop(v[j]))
                {
                    vector<MyCard> other_cards;
                    other_cards.push_back(v[i]);
                    other_cards.push_back(v[j]);
                    other_cards.push_back(g_common_cards[0]);
                    other_cards.push_back(g_common_cards[1]);
                    other_cards.push_back(g_common_cards[2]);
                    sort(other_cards.begin(), other_cards.end()); 
                    if (CmpTwoPlayerCardsInFlop(known_cards, other_cards) >= 0)
                    {
                        win_cnt++;
                    }
                    else
                    {
                        lose_cnt++;
                    }
                    
                }
            }
        }
    }
    tot_cnt = win_cnt + lose_cnt;

    double oneplayerWinProb = double(win_cnt) / (double)(win_cnt + lose_cnt);

    int seat_num = g_seatinfo.size();
    int player_num = seat_num - 1; 

    double ans = 1.0;
    double w = (double)win_cnt;
    double t = (double)tot_cnt;
    for (int i = 0; i < player_num; ++i)
    {
        ans *= w / t;
        w -= 1.0;
        t -= 1.0;
    }

    return ans;
    
}

NUT_HAND GetHandCardType(const vector<MyCard>& v)
{
    assert(v.size() == 5);
    if (v[0].m_point + 1 == v[1].m_point &&
        v[1].m_point + 1 == v[2].m_point &&
        v[2].m_point + 1 == v[3].m_point && 
        v[3].m_point + 1 == v[4].m_point &&
        v[0].m_color == v[1].m_color &&
        v[1].m_color == v[2].m_color &&
        v[2].m_color == v[3].m_color &&
        v[3].m_color == v[4].m_color)
    {
        return STRAIGHT_FLUSH;
    }
    else if (v[0].m_point == v[1].m_point &&
             v[1].m_point == v[2].m_point &&
             v[2].m_point == v[3].m_point 
             || v[1].m_point == v[2].m_point &&
                v[2].m_point == v[3].m_point &&
                v[3].m_point == v[4].m_point)
    {
        return FOUR_OF_A_KIND;
    }
    else if (v[0].m_point == v[1].m_point &&
             v[1].m_point == v[2].m_point &&
             v[3].m_point == v[4].m_point)
    {
        return FULL_HOUSE;
    }
    else if (v[0].m_color == v[1].m_color &&
             v[1].m_color == v[2].m_color &&
             v[2].m_color == v[3].m_color &&
             v[3].m_color == v[4].m_color)
    {
        return FLUSH;
    }
    else if (v[0].m_point + 1 == v[1].m_point &&
             v[1].m_point + 1 == v[2].m_point &&
             v[2].m_point + 1 == v[3].m_point &&
             v[3].m_point + 1 == v[4].m_point)
    {
        return STRAIGHT;
    }
    else if (v[0].m_point == v[1].m_point && v[1].m_point == v[2].m_point
            || v[1].m_point == v[2].m_point && v[2].m_point == v[3].m_point
            || v[2].m_point == v[3].m_point && v[3].m_point == v[4].m_point)
    {
        return THREE_OF_A_KIND;
    }
    else if (v[0].m_point == v[1].m_point && v[2].m_point == v[3].m_point
            || v[0].m_point == v[1].m_point && v[3].m_point == v[4].m_point
            || v[1].m_point == v[2].m_point && v[3].m_point == v[4].m_point)
    {
        return TWO_PAIR;
    }
    else if (v[0].m_point == v[1].m_point ||
             v[1].m_point == v[2].m_point ||
             v[2].m_point == v[3].m_point ||
             v[3].m_point == v[4].m_point)
    {
        return ONE_PAIR;
    }
    else
    {
        return HIGH_CARD;
    }

}

double GetRandomNumber()
{
    return (rand() + 0.0) / (RAND_MAX + 0.0);
}

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

int GetCurrentUnfoldPlayerNumber()
{
    int n = g_seatinfo.size();
    int cnt = 0;
    for (size_t i = 0; i < g_current_inquireinfo.size(); ++i)
    {
        if (g_current_inquireinfo[i].m_action == "fold")
        {
            cnt++;
        }
    }
    return n - cnt;
}



/**
 * return 2 - if has straight flush draw
 * return 1 - if has flush draw
 * return 0 - if has straight draw
 * return -1 - not draw
 */
int HasFlopDraw()
{
    int rets = -1;
    vector<MyCard> cards(g_common_cards, g_common_cards + 3);
    cards.push_back(g_player_cards[0]);
    cards.push_back(g_player_cards[1]);
    sort(cards.begin(), cards.end());

    MyCard tmp;
    for (size_t i = 0; i < 5; ++i)
    {
        // remove cards i
        tmp = cards[i];
        
        if (i == 0)
        {
            cards[i].m_point = cards[i+1].m_point - 1;
            cards[i].m_color = cards[i+1].m_color;
        }
        else
        {
            cards[i].m_point = cards[i-1].m_point + 1;
            cards[i].m_color = cards[i-1].m_color;
        }

        if (cards[0].m_point + 1 == cards[1].m_point && 
            cards[1].m_point + 1 == cards[2].m_point &&
            cards[2].m_point + 1 == cards[3].m_point &&
            cards[3].m_point + 1 == cards[4].m_point &&
            cards[0].m_color == cards[1].m_color &&
            cards[1].m_color == cards[2].m_color &&
            cards[2].m_color == cards[3].m_color &&
            cards[3].m_color == cards[4].m_color)
        {
            rets = 2; 
        }

        // restore cards i
        cards[i] = tmp;
    }

    if (rets == 2) return rets;

    for (size_t i = 0; i < 5; ++i)
    {
        // remove cards i
        tmp = cards[i];
        
        if (i == 0)
        {
            cards[i].m_point = cards[i+1].m_point - 1;
            cards[i].m_color = cards[i+1].m_color;
        }
        else
        {
            cards[i].m_point = cards[i-1].m_point + 1;
            cards[i].m_color = cards[i-1].m_color;
        }

        if (cards[0].m_color == cards[1].m_color &&
            cards[1].m_color == cards[2].m_color &&
            cards[2].m_color == cards[3].m_color &&
            cards[3].m_color == cards[4].m_color)
        {
            rets = 1; 
        }

        // restore cards i
        cards[i] = tmp;
    }

    if (rets == 1) return rets;


    for (size_t i = 0; i < 5; ++i)
    {
        // remove cards i
        tmp = cards[i];
        
        if (i == 0)
        {
            cards[i].m_point = cards[i+1].m_point - 1;
            cards[i].m_color = cards[i+1].m_color;
        }
        else
        {
            cards[i].m_point = cards[i-1].m_point + 1;
            cards[i].m_color = cards[i-1].m_color;
        }

        if (cards[0].m_point + 1 == cards[1].m_point && 
            cards[1].m_point + 1 == cards[2].m_point &&
            cards[2].m_point + 1 == cards[3].m_point &&
            cards[3].m_point + 1 == cards[4].m_point
           )
        {
            rets = 0; 
        }

        // restore cards i
        cards[i] = tmp;
    }

    return rets;
    
}


int GetMyPos()
{
    int n = g_seatinfo.size();
    int ans = 0;
    for (int i = 0; i < n; ++i)
    {
        if (g_seatinfo[i].m_pid == g_PID)
        {
            ans = i + 1;
            break;
        }
    }
    return ans;
}

int InitialHandCardsType(const MyCard& card1, const MyCard& card2)
{
    int n1 = card1.m_point;
    int n2 = card2.m_point;
    if (n1 < n2) swap(n1, n2);
    int c1 = card1.m_color;
    int c2 = card2.m_color;

    if (n1 == 14 && n2 == 14 || 
        n1 == 13 && n2 == 13 || 
        n1 == 12 && n2 == 12 ||
        n1 == 14 && n2 == 13 && c1 == c2 || 
        n1 == 14 && n2 == 12 && c1 == c2)
    {
        return 1;
    }
    else if (n1 == 11 && n2 == 11 || 
             n1 == 10 && n2 == 10 || 
             n1 == 14 && n2 == 11 && c1 == c2 ||
             n1 == 14 && n2 == 10 && c1 == c2 ||
             n1 == 14 && n2 == 13 ||
             n1 == 14 && n2 == 12 ||
             n1 == 13 && n2 == 12 && c1 == c2)
    {
        return 2;
    }
    else if (n1 == 9 && n2 == 9 ||
             n1 == 12 && n2 == 11 && c1 == c2 ||
             n1 == 13 && n2 == 11 && c1 == c2 ||
             n1 == 13 && n2 == 10 && c1 == c2 )
    {
        return 3;
    }
    else if (n1 == 14 && n2 == 8 && c1 == c2 ||
             n1 == 13 && n2 == 12 ||
             n1 == 8 && n2 == 8 ||
             n1 == 12 && n2 == 10 && c1 == c2 ||
             n1 == 14 && n2 == 9 && c1 == c2 ||
             n1 == 14 && n2 == 10 ||
             n1 == 14 && n2 == 11 ||
             n1 == 11 && n2 == 10 && c1 == c2)
    {
        return 4;
    }
    else if (n1 == 7 && n2 == 7 ||
             n1 == 12 && n2 == 9 && c1 == c2 ||
             n1 == 13 && n2 == 11 ||
             n1 == 12 && n2 == 11 ||
             n1 == 11 && n2 == 10 && c1 == c2 ||
             n1 == 14 && n2 == 7 && c1 == c2 ||
             n1 == 14 && n2 == 6 && c1 == c2 ||
             n1 == 14 && n2 == 5 && c1 == c2 ||
             n1 == 14 && n2 == 4 && c1 == c2 ||
             n1 == 14 && n2 == 3 && c1 == c2 ||
             n1 == 14 && n2 == 2 && c1 == c2 ||
             n1 == 11 && n2 == 9 && c1 == c2 ||
             n1 == 10 && n2 == 9 && c1 == c2 ||
             n1 == 13 && n2 == 9 && c1 == c2 ||
             n1 == 13 && n2 == 10 ||
             n1 == 12 && n2 == 10)
    {
        return 5;
    }
    else if (n1 == n2)
    {
        return 6;
    }
    else if (n1 == 14 || n2 == 14)
    {
        return 7;
    }
    else
    {
        return 8;
    }
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
        g_initial_bet_cnt++;
        
        int halfpot = g_current_inquire_totalpot;        

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
            if (g_initial_bet_cnt == 1)
            {
                int max_bet = GetMaxbetInCurrentInquireInfo();
                int raise_num = 5 * max_bet;
                raise_num = min(raise_num, halfpot);
                Raise(raise_num);
            }
            else
            {
                Call();
            }   
        }
        // group 2: strong
        else if (    (card1 == 11 && card2 == 11) // JJ
                  || (card1 == 10 && card2 == 10) // TT
                  || (card1 == 11 && card2 == 14 && color1 == color2) // AJs
                  || (card1 == 10 && card2 == 14 && color1 == color2) // ATs
                  || (card1 == 13 && card2 == 14) // AK
                  || (card1 == 12 && card2 == 14) // AQ
                  || (card1 == 12 && card2 == 13 && color1 == color2) // KQs
                )
        {

            if (g_initial_bet_cnt == 1)
            {
                int max_bet = GetMaxbetInCurrentInquireInfo();
                int raise_num = 3 * max_bet;
                raise_num = min(raise_num, halfpot);
                Raise(raise_num);
            }
            else
            {
                Call();
            }
        }
        // group 3: good
        else if (    (card1 == 9 && card2 == 9)     // 99
                  || (card1 == 11 && card2 == 12 && color1 == color2) // QJs
                  || (card1 == 11 && card2 == 13 && color1 == color2) // KJs
                  || (card1 == 10 && card2 == 13 && color1 == color2) // KTs
                )
        {

            if (g_initial_bet_cnt == 1)
            {
                int max_bet = GetMaxbetInCurrentInquireInfo();
                int raise_num = 1.2 * max_bet;
                raise_num = min(raise_num, halfpot);
                Raise(raise_num);
            }
            else
            {
                Call();
            }
        }
        // group 4:
        else if (   (card1 == 8  && card2 == 14 && color1 == color2) // A8s
                 || (card1 == 12 && card2 == 13)                    // KQ
                 || (card1 == 8  && card2 == 8)                      // 88
                 || (card1 == 10 && card2 == 12 && color1 == color2) // QTs
                 || (card1 == 9  && card2 == 14 && color1 == color2)  // A9s
                 || (card1 == 10 && card2 == 14) // AT
                 || (card1 == 11 && card2 == 14) // AJ
                 || (card1 == 10 && card2 == 11 && color1 == color2) // JTs
                )
        {
            Call();   
        }
        // group 5: <= 6 player can playing
        else if (   (card1 == 7 && card2 == 7) // 77
                 || (card1 == 9 && card2 == 12 && color1 == color2) // Q9s
                 || (card1 == 11 && card2 == 13) // KJ
                 || (card1 == 11 && card2 == 12) // QJ
                 || (card1 == 10 && card2 == 11 && color1 == color2) // JTs
                 || (card1 == 7 && card2 == 14 && color1 == color2)  // A7s
                 || (card1 == 6 && card2 == 14 && color1 == color2)  // A6s
                 || (card1 == 5 && card2 == 14 && color1 == color2)  // A5s
                 || (card1 == 4 && card2 == 14 && color1 == color2)  // A4s
                 || (card1 == 3 && card2 == 14 && color1 == color2)  // A3s
                 || (card1 == 2 && card2 == 14 && color1 == color2)  // A2s
                 || (card1 == 9 && card2 == 11 && color1 == color2)  // J9s
                 || (card1 == 9 && card2 == 10 && color1 == color2)  // T9s
                 || (card1 == 9 && card2 == 13 && color1 == color2)  // K9s
                 || (card1 == 10 && card2 == 13)    // KT
                 || (card1 == 10 && card2 == 12)    // QT
                )
        {
            int n = g_seatinfo.size();
            int fold_n = 0;
            for (size_t i = 0; i < g_current_inquireinfo.size(); i++)
            {
                if (g_current_inquireinfo[i].m_action == "fold") fold_n++;
            }
            int cur_n = n - fold_n;
            if (cur_n <= 6)
            {
                Call();
            }
            else
            {
                bool flag = false;
                for (size_t i = 0; i < g_current_inquireinfo.size(); ++i)
                {
                    if (g_current_inquireinfo[i].m_action != "fold")
                    {
                        int pid = g_current_inquireinfo[i].m_pid;
                        if (g_PlayerBehavior[pid].m_handcardType < 3.5)
                        {
                            flag = true;
                            break;
                        }
                    }
                    
                }

                if (flag)
                {
                    Fold();
                }
                else
                {
                    double r = GetRandomNumber();
                    if (r <= 0.6) Call();
                    else Fold();
                }
            }
            
        }
        // group 6: small pair
        else if (card1 == card2)
        {
            int seat_num = g_seatinfo.size();
            int pos = GetMyPos();
            if (pos >= 2 * seat_num / 3)
            {
                Call();
            }
            else
            {
                
                bool flag = false;
                for (size_t i = 0; i < g_seatinfo.size(); ++i)
                {
                    int pid = g_seatinfo[i].m_pid;
                    if (pid != g_PID)
                    {
                        if (g_PlayerBehavior[pid].m_handcardType < 4.0)
                        {
                            flag = true;
                        }
                    }
                }
                
                if (flag)
                {
                    Fold();
                }
                else
                {
                    double r = GetRandomNumber();
                    if (r < 0.15) Call();
                    else Fold();
                }
            }

        }
        // group 7: has at least an A
        else if (card1 == 14 || card2 == 14)
        {
            
            bool flag = false;
            for (size_t i = 0; i < g_current_inquireinfo.size(); ++i)
            {
                if (g_current_inquireinfo[i].m_action != "fold")
                {
                    int pid = g_current_inquireinfo[i].m_pid;
                    if (g_PlayerBehavior[pid].m_handcardType < 5.0)
                    {
                        flag = true;
                        break;
                    }
                }

            }
            if (flag)
            {
                Fold();
            }
            else
            {
                double r = GetRandomNumber();
                if (r < 0.10) Call();
                else Fold();
            }
        }
        // group 8: fold cards
        else
        {
            int n = g_seatinfo.size();
            int dead = 0;
            for (int i = 0; i < n; ++i)
            {
                if (g_seatinfo[i].m_islive == 0) dead++;
            }
            if (dead == n - 1) Call();
            else Fold();
        }
        return;
    }


    /********* 3 flop cards round ********/
    else if (g_current_common_cards_num == 3)
    {
        g_flop_bet_cnt++;

        vector<MyCard> v(g_common_cards, g_common_cards + 3);
        v.push_back(g_player_cards[0]);
        v.push_back(g_player_cards[1]);
        sort(v.begin(), v.end());

        double win_prob = WinProbabilityInFlop();

        // 筹码保护措施
        
        if (g_flop_bet_cnt == 1 && g_my_current_money + g_my_current_jetton <= g_MIN_PROTECT_JETTON)
        {
            if (win_prob >= 0.8)
            {
                Call();
            }
            else
            {
                Fold();
            }
            return;
        }
        


        double max_bet = GetMaxbetInCurrentInquireInfo();
        double pot = g_current_inquire_totalpot; 
        double return_prob = double(max_bet) / (double)(max_bet + pot);

        double cur_RR = win_prob / return_prob;
        double max_raise = pot / double((double)MAX_RR / win_prob - 1.0) - max_bet;
        max_raise = min(max_raise, pot / 2);
        max_raise = min(max_raise, 2 * max_bet);
        if (max_raise <= 0) max_raise = 0;

        /** first case: complete **/
        NUT_HAND cur_cardtype =  GetHandCardType(v);
        // straight flush
        if (STRAIGHT_FLUSH == cur_cardtype)
        {
            if (g_flop_bet_cnt == 1)
            {
                int max_bet = GetMaxbetInCurrentInquireInfo();
                Raise(5 * max_bet);
            }
            else
            {
                Call();
            }
        }
        // Quads
        else if (FOUR_OF_A_KIND == cur_cardtype)
        {
            if (g_flop_bet_cnt == 1)
            {
                 if (HasRaiseInCurrentInquireInfo())
                 {
                    int max_bet = GetMaxbetInCurrentInquireInfo();
                    Raise(4 * max_bet);
                 }
                 else
                 {
                    int max_bet = GetMaxbetInCurrentInquireInfo();
                    Raise(3 * max_bet);
                 }
            }
            else
            {
                Call();
            }
        }
        // Full-House
        else if (FULL_HOUSE == cur_cardtype)
        {
            int max_bet = GetMaxbetInCurrentInquireInfo();
            Raise(3 * max_bet);
        }
        // Flush
        else if (FLUSH == cur_cardtype)
        {
            if (v[4].m_point == 14)
            {
                if (g_flop_bet_cnt == 1)
                {
                    int max_bet = GetMaxbetInCurrentInquireInfo();
                    Raise(1.1 * max_bet);
                }
                else
                {
                    Call();
                }
            }
            else
            {
                if (cur_RR >= MAX_RR) Raise(max_raise);
                else if (cur_RR >= MIN_RR) Call();
                else
                {
                    if (win_prob >= 0.75)
                    {
                        Call();
                    }
                    else
                    {
                        bool positive = false;
                        for (size_t i = 0; i < g_current_inquireinfo.size();++i)
                        {
                            int pid = g_current_inquireinfo[i].m_pid;
                            string action = g_current_inquireinfo[i].m_action;
                            if (pid != g_PID && action != "fold" &&  g_PlayerBehavior[pid].m_handcardType <= g_InitialHandcardType)
                            {
                                positive = true;
                            }
                        }

                        if (positive)
                        {
                            Fold();
                        }
                        else
                        {
                            Call();
                        }
                    }
                }
            }
        }
        // straight
        else if (STRAIGHT == cur_cardtype)
        {
            if (cur_RR >= MAX_RR) Raise(max_raise);
            else if (cur_RR >= MIN_RR) Call();
            else
            {
                
                bool positive = false;
                for (size_t i = 0; i < g_current_inquireinfo.size();++i)
                {
                    int pid = g_current_inquireinfo[i].m_pid;
                    string action = g_current_inquireinfo[i].m_action;
                    if (pid != g_PID && action != "fold" &&  g_PlayerBehavior[pid].m_handcardType <= g_InitialHandcardType)
                    {
                        positive = true;
                    }
                }

                if (positive)
                {
                    Fold();
                }
                else
                {
                    Call();
                }


            }
        }

        /** Case 2: Has three of a kind **/
        else if (THREE_OF_A_KIND == cur_cardtype)
        {
            // has a pair in hand + 1 common card
            if (g_player_cards[0].m_point == g_player_cards[1].m_point)
            {
                if (g_flop_bet_cnt == 1)
                {
                    int max_bet = GetMaxbetInCurrentInquireInfo();
                    Raise(6 * max_bet / 5);
                }
                else
                {
                    Call();
                }
            }
            else
            {
                if (cur_RR >= MAX_RR) Raise(max_raise);
                else if (cur_RR >= MIN_RR) Call();
                else
                {
                        bool positive = false;
                        for (size_t i = 0; i < g_current_inquireinfo.size();++i)
                        {
                            int pid = g_current_inquireinfo[i].m_pid;
                            string action = g_current_inquireinfo[i].m_action;
                            if (pid != g_PID && action != "fold" &&  g_PlayerBehavior[pid].m_handcardType <= g_InitialHandcardType)
                            {
                                positive = true;
                            }
                        }

                        if (positive)
                        {
                            Fold();
                        }
                        else
                        {
                            Call();
                        }
                    
                }
            }
        }

        /** Case 3: Straight or Flush Draw **/
        // Flush Straight Draw
        else if ( 2 == HasFlopDraw())
        {
            if (cur_RR >= MAX_RR) Raise(max_raise);
            else if (cur_RR >= MIN_RR) Call();
            else
            {
                
                        bool positive = false;
                        for (size_t i = 0; i < g_current_inquireinfo.size();++i)
                        {
                            int pid = g_current_inquireinfo[i].m_pid;
                            string action = g_current_inquireinfo[i].m_action;
                            if (pid != g_PID && action != "fold" &&  g_PlayerBehavior[pid].m_handcardType <= g_InitialHandcardType)
                            {
                                positive = true;
                            }
                        }

                        if (positive)
                        {
                            Fold();
                        }
                        else
                        {
                            Call();
                        }
            }
        
        }
        // Flush Draw 
        else if ( 1 == HasFlopDraw())
        {
            // common cards already has flush
            if (g_common_cards[0].m_color == g_common_cards[1].m_color &&
                g_common_cards[1].m_color == g_common_cards[2].m_color)
            {
                double r = GetRandomNumber();
                if (r <= 0.8) Call();
                else
                {
                    
                        bool positive = false;
                        for (size_t i = 0; i < g_current_inquireinfo.size();++i)
                        {
                            int pid = g_current_inquireinfo[i].m_pid;
                            string action = g_current_inquireinfo[i].m_action;
                            if (pid != g_PID && action != "fold" &&  g_PlayerBehavior[pid].m_handcardType <= g_InitialHandcardType)
                            {
                                positive = true;
                            }
                        }

                        if (positive)
                        {
                            Fold();
                        }
                        else
                        {
                            Call();
                        }
                }
            }
            else
            {
                if (cur_RR >= MAX_RR) Raise(max_raise);
                else if (cur_RR >= MIN_RR) Call();
                else
                {
                    
                        bool positive = false;
                        for (size_t i = 0; i < g_current_inquireinfo.size();++i)
                        {
                            int pid = g_current_inquireinfo[i].m_pid;
                            string action = g_current_inquireinfo[i].m_action;
                            if (pid != g_PID && action != "fold" &&  g_PlayerBehavior[pid].m_handcardType <= g_InitialHandcardType)
                            {
                                positive = true;
                            }
                        }

                        if (positive)
                        {
                            Fold();
                        }
                        else
                        {
                            Call();
                        }
                }
            }
        }
        // Straight Draw
        else if ( 0 == HasFlopDraw())
        {
            if (cur_RR >= MAX_RR) Raise(max_raise);
            else if (cur_RR >= MIN_RR) Call();
            else
            {
                
                        bool positive = false;
                        for (size_t i = 0; i < g_current_inquireinfo.size();++i)
                        {
                            int pid = g_current_inquireinfo[i].m_pid;
                            string action = g_current_inquireinfo[i].m_action;
                            if (pid != g_PID && action != "fold" &&  g_PlayerBehavior[pid].m_handcardType <= g_InitialHandcardType)
                            {
                                positive = true;
                            }
                        }

                        if (positive)
                        {
                            Fold();
                        }
                        else
                        {
                            Call();
                        }
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
                if (cur_RR >= MAX_RR) Raise(max_raise);
                else if (cur_RR >= MIN_RR) Call();
                else
                {
                    
                        bool positive = false;
                        for (size_t i = 0; i < g_current_inquireinfo.size();++i)
                        {
                            int pid = g_current_inquireinfo[i].m_pid;
                            string action = g_current_inquireinfo[i].m_action;
                            if (pid != g_PID && action != "fold" &&  g_PlayerBehavior[pid].m_handcardType <= g_InitialHandcardType)
                            {
                                positive = true;
                            }
                        }

                        if (positive)
                        {
                            Fold();
                        }
                        else
                        {
                            Call();
                        }
                }
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
                if (cur_RR >= MAX_RR) Raise(max_raise);
                else if (cur_RR >= MIN_RR) Call();
                else
                {
                
                        bool positive = false;
                        for (size_t i = 0; i < g_current_inquireinfo.size();++i)
                        {
                            int pid = g_current_inquireinfo[i].m_pid;
                            string action = g_current_inquireinfo[i].m_action;
                            if (pid != g_PID && action != "fold" &&  g_PlayerBehavior[pid].m_handcardType <= g_InitialHandcardType)
                            {
                                positive = true;
                            }
                        }

                        if (positive)
                        {
                            Fold();
                        }
                        else
                        {
                            Call();
                        }
                }
            }
        }
        else
        {
            if (cur_RR >= MAX_RR) Raise(max_raise);
            else if (cur_RR >= MIN_RR) Call();
            else
            {
                
                        bool positive = false;
                        for (size_t i = 0; i < g_current_inquireinfo.size();++i)
                        {
                            int pid = g_current_inquireinfo[i].m_pid;
                            string action = g_current_inquireinfo[i].m_action;
                            if (pid != g_PID && action != "fold" &&  g_PlayerBehavior[pid].m_handcardType <= g_InitialHandcardType)
                            {
                                positive = true;
                            }
                        }

                        if (positive)
                        {
                            Fold();
                        }
                        else
                        {
                            Call();
                        }
            }
        }


        return;
    }
    /********* turn card round *******/
    else if (g_current_common_cards_num == 4)
    {
        g_turn_bet_cnt++;        
        
        double win_prob = WinProbabilityInTurn();
        
        // 筹码保护措施
        
        if (g_my_current_money + g_my_current_jetton <= g_MIN_PROTECT_JETTON)
        {
            if (win_prob >= 0.8)
            {
                Call();
            }
            else
            {
                Fold();
            }
            return;
        }
        



        bool hasPositive = false;
        for(size_t i = 0; i < g_current_inquireinfo.size(); ++i)
        {
            int pid = g_current_inquireinfo[i].m_pid;
            string action = g_current_inquireinfo[i].m_action;
            double playertype = g_PlayerBehavior[pid].m_handcardType;
            double mytype = g_PlayerBehavior[g_PID].m_handcardType;
            if (pid != g_PID && action != "fold" && playertype < mytype)
            {
                hasPositive = true;
            } 
        }

        if (hasPositive == true)
        {
            win_prob -= 0.1;
        }
        else
        {
            win_prob += 0.2;
        }

        double max_bet = GetMaxbetInCurrentInquireInfo();
        double pot = g_current_inquire_totalpot; 
        double return_prob = double(max_bet) / (double)(max_bet + pot);

        double cur_RR = win_prob / return_prob;
        double max_raise = pot / double((double)MAX_RR / win_prob - 1.0) - max_bet;
        max_raise = min(max_raise, pot / 2);
        if (max_raise <= 0) max_raise = 0;

        // max_raise multiply a coefficient 
        double alpha = 1.0 / (double)pow(2.0, (double)(g_turn_bet_cnt));
        max_raise = alpha * max_raise;

        if (cur_RR >= MAX_RR)
        {
            Raise(max_raise);
        }
        else if (cur_RR >= MIN_RR)
        {
            Call();
        }
        else
        {
            Fold();
        }
        
    }
    /********* river card round *********/
    else if (g_current_common_cards_num == 5)
    {
        g_river_bet_cnt++;        
        
        double win_prob = WinProbabilityInRiver();

        // 筹码保护措施
        
        if (g_my_current_money + g_my_current_jetton <= g_MIN_PROTECT_JETTON)
        {
            if (win_prob >= 0.8)
            {
                Call();
            }
            else
            {
                Fold();
            }
            return;
        }
        



        bool hasPositive = false;
        for(size_t i = 0; i < g_current_inquireinfo.size(); ++i)
        {
            int pid = g_current_inquireinfo[i].m_pid;
            string action = g_current_inquireinfo[i].m_action;
            double playertype = g_PlayerBehavior[pid].m_handcardType;
            double mytype = g_PlayerBehavior[g_PID].m_handcardType;
            if (pid != g_PID && action != "fold" && playertype < mytype)
            {
                hasPositive = true;
            } 
        }

        if (hasPositive == true)
        {
            win_prob -= 0.1;
        }
        else
        {
            win_prob += 0.2;
        }
        
        
            

        double max_bet = GetMaxbetInCurrentInquireInfo();
        double pot = g_current_inquire_totalpot; 
        double return_prob = double(max_bet) / (double)(max_bet + pot);

        double cur_RR = win_prob / return_prob;
        double max_raise = pot / double((double)MAX_RR / win_prob - 1.0) - max_bet;
        
        max_raise = min(max_raise, pot / 2);
        if (max_raise <= 0) max_raise = 0;
        double alpha = 1.0 / (double)pow(2.0, double(g_river_bet_cnt-1));
        max_raise = alpha * max_raise;
        
        if (cur_RR >= MAX_RR)
        {
            Raise(max_raise);
        }
        else if (cur_RR >= MIN_RR)
        {
            Call();
        }
        else
        {
            Fold();
        }
        
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

            // clear all bet round num info
            g_initial_bet_cnt = 0;
            g_flop_bet_cnt = 0;
            g_turn_bet_cnt = 0;
            g_river_bet_cnt = 0;
            g_my_current_money = 0;
            g_my_current_jetton = 0;

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

            if (g_PlayerBehavior.find(pid) == g_PlayerBehavior.end())
            {
                g_PlayerBehavior[pid].m_handcardType = g_InitialHandcardType;
            }

            if (pid == g_PID)
            {
                g_my_current_money = money;
                g_my_current_jetton = jetton;
            }


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


            if (g_PlayerBehavior.find(pid) == g_PlayerBehavior.end())
            {
                g_PlayerBehavior[pid].m_handcardType = g_InitialHandcardType;
            }

            if (pid == g_PID)
            {
                g_my_current_money = money;
                g_my_current_jetton = jetton;
            }


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

                    if (g_PlayerBehavior.find(pid) == g_PlayerBehavior.end())
                    {
                        g_PlayerBehavior[pid].m_handcardType = g_InitialHandcardType;
                    }

                    if (pid == g_PID)
                    {
                        g_my_current_money = money;
                        g_my_current_jetton = jetton;
                    }

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

                    if (g_PlayerBehavior.find(pid) == g_PlayerBehavior.end())
                    {
                        g_PlayerBehavior[pid].m_handcardType = g_InitialHandcardType;
                    }

                    if (pid == g_PID)
                    {
                        g_my_current_money = money;
                        g_my_current_jetton = jetton;
                    }

                }
            }


        }

        /** game over message, DONE **/
        else if (line == "game-over ")
        {
            
            g_PlayerBehavior.clear();            
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

                    if (pid == g_PID)
                    {
                        g_my_current_money = money;
                        g_my_current_jetton = jetton;
                    }

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

                    MyCard card1, card2;
                    card1.m_point = toPoint(point1);
                    card1.m_color = toMyColor(color1);
                    card2.m_point = toPoint(point2);
                    card2.m_color = toMyColor(color2);
                    
                    int handcardtype = InitialHandCardsType(card1, card2);
                    if (g_PlayerBehavior.find(pid) == g_PlayerBehavior.end())
                    {
                        g_PlayerBehavior[pid].m_handcardType = g_InitialHandcardType * g_alpha + (double)handcardtype * g_beta;
                    }
                    else
                    {
                        g_PlayerBehavior[pid].m_handcardType = g_PlayerBehavior[pid].m_handcardType * g_alpha + (double)handcardtype * g_beta;
                    }
                    
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
