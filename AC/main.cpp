#include <iostream>
#include <list>
#include <vector>
#include <utility>
#include <cstdarg>
#include <map>

#include <fstream>

using namespace std;

typedef short PIXEL_VALUE;
typedef long long PIXEL_COUNT;
typedef long long PIXEL_INDEX;

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


// RLE压缩后的像素容器
typedef pair<PIXEL_VALUE, PIXEL_COUNT> RLE_PAIR;
class PIXEL_RLE_CONTAINER : public list < RLE_PAIR > {
public:
	typedef list < RLE_PAIR >::iterator PAIR_ITERATOR;
	struct PIXEL_RLE {	// RLE压缩后，像素容器中的像素
		PAIR_ITERATOR pair;
		PIXEL_COUNT offset;		// 在RLE_PAIR里的偏移
		PIXEL_INDEX index;		// 在整个RLE_CONTAINER里面的索引
		PIXEL_RLE_CONTAINER *base;

		bool isNonePixel() const { return index == -1; }
		PIXEL_VALUE getValue() const { return pair->first; }

		PIXEL_RLE operator+ (const PIXEL_COUNT &i) const
		{
			PIXEL_COUNT add_offset = i;
			PIXEL_RLE res = *this;

			while (add_offset != 0)
			{
				if (add_offset > 0)
				{
					if (res.pair->second > res.offset + add_offset)
					{
						res.index += add_offset;
						res.offset += add_offset;
						add_offset -= add_offset;
					}
					else
					{
						add_offset -= res.pair->second - res.offset;
						res.index += res.pair->second - res.offset;
						res.pair++;
						res.offset = 0;

						if (res.pair == res.base->end())
						{
							return res.base->none_pixel();
						}
					}
				}
				else
				{
					if (res.offset + add_offset >= 0)
					{
						res.index += add_offset;
						res.offset += add_offset;
						add_offset -= add_offset;
					}
					else
					{
						if (res.pair == res.base->begin())
						{
							return res.base->none_pixel();
						}

						add_offset -= -1 - res.offset;
						res.index += -1 - res.offset;
						res.pair--;
						res.offset = res.pair->second - 1;
					}
				}
			}

			return res;
		}

		// 获得下一个要处理的像素，只需处理RLE_PAIR的首尾两个像素即可
		PIXEL_RLE getNextProcPixel() const
		{
			if (offset == pair->second - 1)	// 如果是RLE_PAIR的尾部像素，就返回下一个RLE_PAIR的首部像素
			{
				return *this + 1;
			}
			else
			{	// 否则返回RLE_PAIR的尾部像素
				PIXEL_COUNT dis = pair->second - 1 - offset;
				return *this + dis;
			}
		}
	};	

public:
	PIXEL_RLE begin_pixel()
	{
		PIXEL_RLE res;
		res.pair = begin();
		res.offset = 0;
		res.index = 0;
		res.base = this;
		return res;
	}

	PIXEL_RLE none_pixel()
	{
		PIXEL_RLE res;
		res.pair = end();
		res.offset = 0;
		res.index = -1;
		res.base = this;
		return res;
	}
};

// 常规表示的像素容器
class PIXEL_CONTAINER : public map < PIXEL_INDEX, PIXEL_VALUE > {
private:
	PIXEL_COUNT width;
	PIXEL_RLE_CONTAINER cache;
	PIXEL_COUNT has_push;

public:
	PIXEL_CONTAINER(PIXEL_COUNT _width) : map < PIXEL_INDEX, PIXEL_VALUE >(), width(_width), has_push(0) { cout << _width << endl; }

	void output(PIXEL_COUNT all_count)
	{
		for (iterator it = begin(); it != end(); ++it)
		{
			iterator next = it;
			next++;

			if (next == end())
			{
				push_to_cache(it->second, all_count - it->first);
			}
			else
			{
				PIXEL_COUNT count = next->first - it->first;
				PIXEL_COUNT nextCount = 0;
				if (has_push / width < (has_push + count) / width)
				{
					nextCount = (has_push + count) % width;
				}
				push_to_cache(it->second, count - nextCount);
				push_to_cache(next->second, nextCount);
			}
		}
		out_from_cache();
	}

private:
	void push_to_cache(PIXEL_VALUE _value, PIXEL_COUNT _count)
	{
		PIXEL_RLE_CONTAINER::reverse_iterator pri = cache.rbegin();
		if (pri == cache.rend() || pri->first != _value)
		{
			cache.push_back(make_pair(_value, _count));
		}
		else
		{
			pri->second += _count;
		}
		has_push += _count;
	}

	void out_from_cache()
	{
		for (PIXEL_RLE_CONTAINER::iterator pi = cache.begin(); pi != cache.end(); ++pi)
		{
			cout << pi->first << ' ' << pi->second << endl;
		}
		cout << "0 0" << endl;
	}
};

class processor {
private:
	enum ADJACENT_TYPE{
		UPPER_LEFT = 0,
		UPPER,
		UPPER_RIGHT,
		LEFT,
		RIGHT,
		LOWER_LEFT,
		LOWER,
		LOWER_RIGHT,
		NOT_ADJACENT
	};

private:
	PIXEL_COUNT count;					// 未处理的像素个数
	PIXEL_COUNT width;
	bool inputFinish;

private:
	PIXEL_RLE_CONTAINER inputCache;			// RLE压缩后的像素输入缓存
	PIXEL_RLE_CONTAINER::PIXEL_RLE nextPixel;	// 待处理像素
	PIXEL_CONTAINER outputCache;

public:
	processor(PIXEL_COUNT _width) :count(0), width(_width), inputFinish(false), outputCache(_width) {}

	void push_back(PIXEL_VALUE _value, PIXEL_COUNT _count)
	{
		if (_value == 0 && _count == 0)
		{
			inputFinish = true;
		}
		else
		{
			inputCache.push_back(make_pair(_value, _count));
			if (count == 0)	// 第一次输入
			{
				nextPixel = inputCache.begin_pixel();
			}

			count += _count;
		}

		// 要处理一个RLE节点上的像素，必须要等待足够多的后续输入（需要待处理像素下一行的数据）
		// 所以每次放入节点时，都尝试处理尽可能多的像素点
		process();
	}

private:
	void process()
	{
		if (inputCache.begin() != inputCache.end())
		{
			while (canProcOnePixel(nextPixel))
			{
				processOnePixel(nextPixel);
				nextPixel = nextPixel.getNextProcPixel();
			}
		}

		if (inputFinish)
		{
			outputCache.output(count);
		}
	}

	bool canProcOnePixel(const PIXEL_RLE_CONTAINER::PIXEL_RLE &_pixel)
	{
		if (_pixel.isNonePixel()) return false;
		if (inputFinish) return true;

		// 对于一个RLE节点，我们只需要对其首尾两个像素，及这两个像素各自周围8个像素进行处理即可。
		// 这样，至少需要尾部像素的右下方的右下方的数据。
		return (count > _pixel.index + (width + 1) + (width + 1));
	}

	// 处理一个像素：对它和它周围8个像素进行边缘检测，并将结果输出
	void processOnePixel(const PIXEL_RLE_CONTAINER::PIXEL_RLE &_pixel)
	{
		if (outputCache.find(_pixel.index) == outputCache.end())
		{
			outputCache.insert(make_pair(_pixel.index, calcEdgeValue(_pixel)));
		}

		for (int i = UPPER_LEFT; i < NOT_ADJACENT; i++)
		{
			PIXEL_COUNT offset;
			if (getOffSet((ADJACENT_TYPE)i, _pixel.index, offset))
			{
				PIXEL_RLE_CONTAINER::PIXEL_RLE adjacentPixel = _pixel + offset;
				if (!adjacentPixel.isNonePixel()
					&& outputCache.find(adjacentPixel.index) == outputCache.end())
				{
					outputCache.insert(make_pair(adjacentPixel.index, calcEdgeValue(adjacentPixel)));
				}
			}
		}
	}

	// 计算像素与其所有相邻像素的灰度值绝对差值的最大值（边缘检测时对一个像素的处理）
	PIXEL_VALUE calcEdgeValue(const PIXEL_RLE_CONTAINER::PIXEL_RLE &_pixel)
	{
		PIXEL_VALUE result = 0;
		PIXEL_VALUE curValue = _pixel.getValue();
		for (int i = UPPER_LEFT; i < NOT_ADJACENT; i ++)
		{
			PIXEL_COUNT offset;
			if (getOffSet((ADJACENT_TYPE)i, _pixel.index, offset))
			{
				PIXEL_RLE_CONTAINER::PIXEL_RLE adjacentPixel = _pixel + offset;
				if (!adjacentPixel.isNonePixel())
				{
					PIXEL_VALUE adjValue = adjacentPixel.getValue();
					PIXEL_VALUE absDiffValue = abs_diff(curValue, adjValue);
					result = MAX_PIXEL_VALUE(result, absDiffValue);
				}
			}
		}
		return result;
	}

	// 根据相邻像素的类型计算偏移
	bool getOffSet(ADJACENT_TYPE _type, PIXEL_INDEX _index, PIXEL_COUNT &offset)
	{
		PIXEL_COUNT col = _index%width, row = _index/width;

		switch (_type)
		{
		case UPPER_LEFT:
			row--; col--;
			break;
		case UPPER:
			row--;
			break;
		case UPPER_RIGHT:
			row--; col++;
			break;
		case LEFT:
			col--;
			break;
		case RIGHT:
			col++;
			break;
		case LOWER_LEFT:
			row++; col--;
			break;
		case LOWER:
			row++;
			break;
		case LOWER_RIGHT:
			row++; col++;
			break;
		default: 
			return false;
		}

		if (col < 0 || col >= width || row < 0)
			return false;

		offset = row * width + col - _index;
		return true;
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
		processor proc(width);

		if (width > 0)
		{
			do
			{
				cin >> pixel >> count;
				proc.push_back(pixel, count);
			} while (pixel != 0 || count != 0);
		}
	} while (width != 0);

	return 0;
}
