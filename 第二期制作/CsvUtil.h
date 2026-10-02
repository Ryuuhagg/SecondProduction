#pragma once

#include <string>
#include <vector>

using namespace std;
// 9/24追加
// CSVの1行をカンマで分割する処理を変更しやすいように共通化
inline vector<string> SplitCSV(const string& line)
{
	vector<string> result;
	string item;

	for (char c : line)
	{
		if (c == ',')
		{
			result.push_back(item);
			item.clear();
		}
		else
		{
			item += c;
		}
	}

	result.push_back(item);

	return result;
}