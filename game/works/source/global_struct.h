#ifndef GLOBAL_STRUCT_INCLUDE_H_
#define GLOBAL_STRUCT_INCLUDE_H_
#include <vector>
#include <string>

enum MyColor
{
    SPADES = 0, 
    HEARTS, 
    CLUBS, 
    DIAMONDS,
};

enum NUT_HAND
{
    HIGH_CARD = 0,
    ONE_PAIR,
    TWO_PAIR,
    THREE_OF_A_KIND,
    STRAIGHT,
    FLUSH,
    FULL_HOUSE,
    FOUR_OF_A_KIND,
    STRAIGHT_FLUSH,
};

struct MyCard
{
public:
    int m_point;
    MyColor m_color;

public:
    // constructor
    MyCard() : m_point(0), m_color(SPADES) {}

    bool operator< (const MyCard& other) const
    {
        if (m_point != other.m_point)
        {
            return m_point < other.m_point;
        }
        else
        {
            return m_color < other.m_color;
        }
    }
};

struct InquireInfo
{
    int m_pid;
    int m_jetton;
    int m_money;
    int m_bet;
    std::string m_action;
};

struct SeatInfo
{
    int m_islive;
    int m_pid;
    int m_jetton;
    int m_money;
    std::vector<int> m_bets;

    ~SeatInfo()
    {
        m_bets.clear();
    }
};


struct PlayerType
{
    double m_handcardType;
    
};

#endif // GLOBAL_STRUCT_INCLUDE_H_
