#include <iostream>
#include <string>
#include <vector>
using namespace std;
int cnt = 1;
void ChooseKFromN(vector<int>& people,int offset, int k, vector<int>& ans)
{
    int n = people.size();

    if (k == 0)
    {
        cout << "combination No. " << cnt++ << endl;
        for (size_t i = 0; i < ans.size(); ++i)
        {
            cout << " " << ans[i];
        }
        cout << endl;
        return;
    }

    for (int i = offset; i <= n - k; ++i)
    {
        ans.push_back(people[i]);
        ChooseKFromN(people, i + 1, k - 1, ans);
        ans.pop_back();
    }
}

int main()
{
    vector<int> people;
    int n = 7; int k = 5;
    for (int i = 0; i < n; ++i) people.push_back(i+1);
    vector<int> ans;
    ChooseKFromN(people, 0, k, ans);
    return 0;
}
