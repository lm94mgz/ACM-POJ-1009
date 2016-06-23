#include <iostream>
#include <list>
#include <vector>
#include <utility>
#include <cstdarg>

using namespace std;

typedef short PIXEL_VALUE;
typedef unsigned long long PIXEL_COUNT;
typedef pair<PIXEL_VALUE, PIXEL_COUNT> RLE_PAIR;
typedef list<RLE_PAIR> PIXEL_CONTAINER;

#define PIXEL_VALUE_NONE -1
#define MAX_PIXEL_VALUE(a, b) ((a) > (b) ? (a) : (b))

inline PIXEL_VALUE abs_diff(PIXEL_VALUE lhs, PIXEL_VALUE rhs)
{
	if (lhs == PIXEL_VALUE_NONE || rhs == PIXEL_VALUE_NONE)
	{
		return PIXEL_VALUE_NONE;
	}
	return (lhs > rhs ? lhs - rhs : rhs - lhs);
}

class outList
{
private:
	enum { MAX_CACHE_SIZE = 100 };

private:
	PIXEL_CONTAINER data;
	int size;

public:
	outList(int _width) :size(0) { cout << _width << endl; }
	~outList() 
	{ 
		out_all(); 
		cout << "0 0" << endl; 
	}

	void push_back(PIXEL_VALUE _value, PIXEL_COUNT _count)
	{
		PIXEL_CONTAINER::reverse_iterator pi = data.rbegin();
		if (pi == data.rend() || pi->first != _value)
		{
			if (size > MAX_CACHE_SIZE)
			{
				out_all();
			}

			data.push_back(make_pair(_value, _count));
			size++;
		}
		else
		{
			pi->second += _count;
		}
	}

private:
	void out_all()
	{
		for (PIXEL_CONTAINER::iterator out_pi = data.begin(); out_pi != data.end(); ++out_pi)
		{
			cout << out_pi->first << ' ' << out_pi->second << endl;
		}
		data.clear();
		size = 0;
	}
};

struct PIXEL_PROC_VALUE {
	enum VALUE_TYPE{
		VALUE = 0,				// 像素的值
		RIGHT_DIFF,				// 像素与其右边像素的绝对差值，以下同理
		LOWER_LEFT_DIFF,
		LOWER_DIFF,
		LOWER_RIGHT_DIFF,
		MAX_PROC_VALUE
	};
	PIXEL_VALUE values[MAX_PROC_VALUE];
};

class process_line {
private:
	typedef vector<PIXEL_PROC_VALUE> PIXEL_LINE;

private:
	PIXEL_LINE line;			// 对于某个属性，如果该列中所有像素的值不都一样，分别记在这里
	PIXEL_PROC_VALUE common;	// 对于某个属性，如果该列中所有像素的值都一样，记在这里
	bool flag[PIXEL_PROC_VALUE::MAX_PROC_VALUE];	// 标记某个属性是否全列有一样的值

private:
	int count;					// 已经记录的像素个数
	int width;					// 列宽度

public:
	process_line(int _width) :count(0), width(_width) 
	{ 
		line.resize(_width); 
		memset(flag, 0, sizeof(flag));
	}

	bool isFull() { return (count >= width); }
	int getCount() { return count; }

	void clear()
	{
		count = 0; 
		memset(flag, 0, sizeof(flag));
	}

	// 返回值：成功放入后剩余的像素个数
	PIXEL_COUNT push_back(PIXEL_VALUE _value, PIXEL_COUNT _count)
	{
		if (count == 0 && _count >= width)
		{
			flag[PIXEL_PROC_VALUE::VALUE] = true;
			common.values[PIXEL_PROC_VALUE::VALUE] = _value;
			count = width;
			_count -= count;
		}
		else
		{
			for (; count < width && _count > 0; ++count)
			{
				line[count].values[PIXEL_PROC_VALUE::VALUE] = _value;
				_count--;
			}
		}
		return _count;
	}

	void setValue(PIXEL_PROC_VALUE::VALUE_TYPE _type, int pos, PIXEL_VALUE _value)
	{
		line[pos].values[_type] = _value;
	}

	void setValue(PIXEL_PROC_VALUE::VALUE_TYPE _type, PIXEL_VALUE _value)
	{
		common.values[_type] = _value;
		flag[_type] = true;
	}

	PIXEL_VALUE getValue(PIXEL_PROC_VALUE::VALUE_TYPE _type, int pos)
	{
		if (flag[_type])
		{
			return common.values[_type];
		}
		else if (pos < count && pos >= 0)
		{
			return line[pos].values[_type];
		}
		else
		{
			return PIXEL_VALUE_NONE;
		}
	}

	bool getFlag(PIXEL_PROC_VALUE::VALUE_TYPE _type) { return flag[_type]; }
};

class inList
{
private:
	process_line *preLine;
	process_line *curLine;
	process_line *nextLine;

private:
	int width;
	outList *out;

public:
	inList(int _width) :width(_width)
	{
		preLine = new process_line(_width);
		curLine = new process_line(_width);
		nextLine = new process_line(_width);
		out = new outList(_width);
	}

	~inList()
	{
		delete preLine;
		delete curLine;
		delete nextLine;
		delete out; 
	}

	void push_back(PIXEL_VALUE _value, PIXEL_COUNT _count)
	{
		if (_value == 0 && _count == 0)
		{
			_process();
		}
		else
		{
			while (_count > 0)
			{
				if (preLine->getFlag(PIXEL_PROC_VALUE::VALUE) && preLine->getValue(PIXEL_PROC_VALUE::VALUE, 0) == _value
					&& curLine->getFlag(PIXEL_PROC_VALUE::VALUE) && curLine->getValue(PIXEL_PROC_VALUE::VALUE, 0) == _value
					&& nextLine->getCount() == 0 && _count > width)
				{
					_count = fast_process(_value, _count);
				}
				else
				{
					_count = common_process(_value, _count);
				}
			}

		}
	}

private:
	PIXEL_COUNT fast_process(PIXEL_VALUE _pixel, PIXEL_COUNT _count)
	{
		int zero_count = curLine->getCount() + nextLine->getCount() + (_count / width - 1) * width;
		out->push_back(0, zero_count);
		_count -= (_count / width - 1) * width;

		curLine->clear();
		nextLine->clear();
		return _count;
	}

	PIXEL_COUNT common_process(PIXEL_VALUE _pixel, PIXEL_COUNT _count)
	{
		if (!curLine->isFull() && _count > 0)
		{
			_count = curLine->push_back(_pixel, _count);
		}
		if (!nextLine->isFull() && _count > 0)
		{
			_count = nextLine->push_back(_pixel, _count);
		}

		if (nextLine->isFull())
		{
			_process();
		}
		return _count;
	}

	void _process()
	{
		PIXEL_VALUE lhs, rhs;
		for (int pos = 0; pos < curLine->getCount(); ++pos)
		{
			lhs = curLine->getValue(PIXEL_PROC_VALUE::VALUE, pos);

			if (!curLine->getFlag(PIXEL_PROC_VALUE::VALUE))
			{
				rhs = curLine->getValue(PIXEL_PROC_VALUE::VALUE, pos + 1);
				curLine->setValue(PIXEL_PROC_VALUE::RIGHT_DIFF, pos, abs_diff(lhs, rhs));
			}
			else
			{
				curLine->setValue(PIXEL_PROC_VALUE::RIGHT_DIFF, 0);
			}

			rhs = nextLine->getValue(PIXEL_PROC_VALUE::VALUE, pos);
			curLine->setValue(PIXEL_PROC_VALUE::LOWER_DIFF, pos, abs_diff(lhs, rhs));

			rhs = nextLine->getValue(PIXEL_PROC_VALUE::VALUE, pos - 1);
			curLine->setValue(PIXEL_PROC_VALUE::LOWER_LEFT_DIFF, pos, abs_diff(lhs, rhs));

			rhs = nextLine->getValue(PIXEL_PROC_VALUE::VALUE, pos + 1);
			curLine->setValue(PIXEL_PROC_VALUE::LOWER_RIGHT_DIFF, pos, abs_diff(lhs, rhs));

			PIXEL_VALUE value = 0;
			PIXEL_VALUE value1 = curLine->getValue(PIXEL_PROC_VALUE::RIGHT_DIFF, pos);
			PIXEL_VALUE value2 = curLine->getValue(PIXEL_PROC_VALUE::LOWER_DIFF, pos);
			PIXEL_VALUE value3 = curLine->getValue(PIXEL_PROC_VALUE::LOWER_LEFT_DIFF, pos);
			PIXEL_VALUE value4 = curLine->getValue(PIXEL_PROC_VALUE::LOWER_RIGHT_DIFF, pos);
			PIXEL_VALUE value5 = curLine->getValue(PIXEL_PROC_VALUE::RIGHT_DIFF, pos - 1);
			PIXEL_VALUE value6 = preLine->getValue(PIXEL_PROC_VALUE::LOWER_DIFF, pos);
			PIXEL_VALUE value7 = preLine->getValue(PIXEL_PROC_VALUE::LOWER_LEFT_DIFF, pos + 1);
			PIXEL_VALUE value8 = preLine->getValue(PIXEL_PROC_VALUE::LOWER_RIGHT_DIFF, pos - 1);

			value = MAX_PIXEL_VALUE(value, value1);
			value = MAX_PIXEL_VALUE(value, value2);
			value = MAX_PIXEL_VALUE(value, value3);
			value = MAX_PIXEL_VALUE(value, value4);
			value = MAX_PIXEL_VALUE(value, value5);
			value = MAX_PIXEL_VALUE(value, value6);
			value = MAX_PIXEL_VALUE(value, value7);
			value = MAX_PIXEL_VALUE(value, value8);

			out->push_back(value, 1);
		}

		moveLine();
	}

	void moveLine()
	{
		process_line *temp = preLine;
		preLine = curLine;
		curLine = nextLine;
		nextLine = temp;
		nextLine->clear();
	}
};

int main()
{
	int width;
	PIXEL_VALUE pixel;
	PIXEL_COUNT count;

	do
	{
		cin >> width;
		inList input(width);

		do
		{
			cin >> pixel >> count;
			input.push_back(pixel, count);
		} while (pixel != 0 && count != 0);

	} while (width != 0);

	return 0;
}
