#ifndef GLOBAL_STRUCT_INCLUDE_H_
#define GLOBAL_STRUCT_INCLUDE_H_

enum MyColor
{
    SPADES = 0, 
    HEARTS, 
    CLUBS, 
    DIAMONDS,
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

#endif // GLOBAL_STRUCT_INCLUDE_H_
