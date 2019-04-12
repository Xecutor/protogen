#pragma once

class Test {
public:
    int getFieldx()const
    {
        return m_fieldx;
    }
    int getAnother()const
    {
        return m_another;
    }
private:
    int m_fieldx = 0; //test1 x
    int m_another = 0; //test1 x
};
